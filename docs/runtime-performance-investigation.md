# Investigação de slowdown: custos reais no runtime

Base do jogo: `perf/guest-shards-o1`, commit `0b015b64104f6e7ecfbd84c725d078021504508e`.
Engine anterior: `e353848ca8e82e7860469a235784a000102e7781`.
Engine proposta: `3dc84528e741157a14bc1e901f347fb3f9dcf3d0`, [PR Draft da engine #1](https://github.com/BBlueSkyy/gbarecomp/pull/1).

## Resultado e limite

Foram eliminadas chamadas intermediárias pequenas, mas muito frequentes, na classificação/espelhamento dos acessos à memória, na consulta de IRQ por instrução e na implementação concreta da consulta de ciclos. Os testes locais comprovam redução desse custo de CPU e preservação dos resultados do teste sintético. **Não comprovam que o slowdown severo da primeira fase foi resolvido no Moto G100.** Não houve execução do jogo, ROM/BIOS, APK, benchmark no aparelho ou build remota nesta investigação.

O workflow Codemagic Android ARM64 continua manual. Sua única alteração funcional é fixar a nova revisão da engine; o workflow manual do Actions usa a mesma revisão. Os 16 arquivos recompilados continuam em O1. Dispatch/symbol maps continuam em O0. Nenhum arquivo C++ derivado do jogo foi alterado ou regenerado. `main` e `perf/guest-shards-o1` permanecem preservadas.

## Auditoria dos cinco caminhos

| Caminho | Evidência | Decisão |
| --- | --- | --- |
| `runtime_tick()` | O scheduler já acumula ciclos e materializa dispositivos somente no horizonte de eventos. A consulta minúscula de IE/IF/IME ainda atravessava uma função separada a cada instrução. | Tornar essa consulta inline, sem reduzir a frequência dos checks ou mudar os ciclos. |
| Dispatch/chamadas | `lookup_in()` já usa cache de 4096 entradas, validando tabela, PC e modo. Resolução inclui hooks, código mutável, resume e fallbacks necessários. | Manter o dispatcher e a ABI gerada. A presença/frequência dos misses na gameplay permanece desconhecida. |
| Memória guest/MMIO | Cada read/write chamava `classify()` e `resolve_offset()` em outra unidade de compilação; a consulta concreta de ciclos ainda chamava outro wrapper. | Mover os corpos exatos de classificação/offset para o header e compartilhar o cálculo puro de ciclos. Manter o caminho completo de MMIO, observers, mirrors, mapeador ROM, GPIO, EEPROM/flash e open bus. |
| Scheduler/PPU/frames | PPU percorre dots, mas o teste isolado de intervalos de 64–1232 ciclos custou aproximadamente 59–68 microssegundos por frame GBA no host; intervalos menores de 1/4 ciclos custam mais. O scheduler agrupa trabalho; o frame pacer não foi alterado. | Não há evidência de que o compositor, a PPU ou a sincronização sejam a causa principal no aparelho. Nenhuma alteração nesses mecanismos. |
| C++ gerado/ARM64 | O gerador emite prologue/yield e tick por instrução, além de helpers de memória/flags. A engine já recebe opções de otimização na configuração Android. Assembly ARM64 confirmou chamadas intermediárias removidas sem regeneração. | Não presumir que O2 nos shards resolve; não aumentar agressivamente inlining nos 9 milhões de linhas nem alterar codegen sem medida específica. |

A consulta de ciclos continua usando o método **virtual** do barramento. O primeiro experimento de acesso direto à tabela foi rejeitado por um novo teste com barramento derivado. A versão publicada preserva overrides, ponteiro ativo, largura truncada, WAITCNT/MEMCTL e fallback sem barramento. A tabela de prefetch não é modificada por uma consulta de custo.

## Medida local final

Linux x86_64, GCC 13.3.0; bibliotecas runtime em Release/O3, chamador em O1/g0. O benchmark liga as bibliotecas reais e não precisa de ROM. Foram oito pares A/B em ordem alternada, sete amostras por execução, afinidade em uma CPU permitida. Medianas de 56 amostras por caminho:

| Operação | Antes (ns) | Depois (ns) | Redução de custo |
| --- | ---: | ---: | ---: |
| Tick de um ciclo | 7,8685 | 6,7825 | 13,8% |
| Leitura EWRAM de 32 bits | 11,6625 | 8,5035 | 27,1% |
| Consulta de ciclos | 5,2615 | 4,9935 | 5,1% |
| Instrução aritmética com yield/tick | 14,4005 | 12,6030 | 12,5% |
| Instrução de load com yield/ciclos/tick | 32,3085 | 27,0045 | 16,4% |
| Lote: 3 aritméticas + load + store | 100,9830 | 85,5610 | 15,3% |

São custos sintéticos no computador, **não FPS nem previsão de ganho no Snapdragon 870**. O ganho de 5,1% na consulta de ciclos é pequeno. Layout de código e variação do host afetam medições de nanossegundos: experimentos anteriores oscilaram, inclusive no yield isolado. Não há reivindicação de melhora independente no dispatcher/yield, que não foram otimizados deliberadamente.

As 16 execuções finais produziram os mesmos 86.800.000 ciclos guest, 309 voltas de frame da PPU e resultado de CPU. O estado serializado conjunto de bus/IO/áudio/PPU/CPU, com 545.042 bytes, teve checksum FNV-1a `d998a3524f656302` em todas. O RTC foi fixado exclusivamente no fixture para evitar variação do relógio do host; a sincronização do aplicativo não mudou.

## Verificações

- 15 CTests essenciais passaram: memória/ciclos, IRQ, bus, DMA normal/bulk, timers, PPU, stack guard, tail dispatch, captura de diagnóstico on/off, áudio, saves/config, frame pacer e interpreter smoke.
- Novo teste: 47.775.744 comparações de custos, configurações WAITCNT, controle de EWRAM, regiões, larguras truncadas, argumentos sequential não booleanos, troca de barramento e prefetch preservado. Casos adicionais: barramento nulo e override virtual derivado.
- IRQ: 14 fontes, máscaras cruzadas, IME, consultas repetidas sem efeito colateral e acknowledge W1C.
- Revisão independente identificou uma limitação de portabilidade no benchmark, corrigida com `_putenv_s` no Windows. A revisão inicial não identificou o contrato virtual; a inspeção ARM64 o revelou e o novo teste o protege.
- Cross-compilação O2/PIC para ARM64 Linux/musl com Clang do Zig 0.17.0: fontes reais de bus e bridge compilaram. Saíram seis chamadas de classify, seis de resolve_offset e uma de waits_access_cycles do objeto do bus. A consulta de IRQ foi incorporada nos caminhos por instrução. A chamada virtual de runtime_mem_cycles continua presente.
- Isso não é uma compilação/link do APK com NDK/Bionic, nem valida TLS ou desempenho no Android. A compilação completa da engine com GCC 13 tem um bloqueio preexistente na expressão lambda/nullptr de runtime.cpp:1921; ambos os lados do benchmark usaram a mesma conversão local com `+lambda`, não incluída na proposta.

## O que ainda pode explicar slowdown adicional

Os workflows não geram os corpos recompilados da BIOS e a configuração padrão não habilita BIOS HLE. A configuração local da engine confirmou BIOS AOT ausente; misses podem entrar na interpretação. A frequência desses caminhos durante a primeira fase não foi comprovada. Código copiado para RAM e cobertura incompleta também podem impor interpretação no Android, onde a recompilação dinâmica é desabilitada. Não foi ligada HLE, fabricado sucesso, alterada metadata de funções ou presumida cobertura nativa completa.

Foi comparada a integração de [WarioWareTwistedRecomp](https://github.com/mstan/WarioWareTwistedRecomp/blob/e959da47db510917971e19adf0e6e9d20adbbcf0/CMakeLists.txt): shards em O2 e engine comum; sua configuração contém metadados revisados de código RAM para manter caminhos de áudio nativos. Isso é referência de organização/cobertura, não evidência de que copiar suas flags ou endereços corrigiria Dragon Ball.

O que foi comprovado nesta entrega é o custo redundante do runtime e sua redução local. Quanto desse custo responde pela lentidão do jogo e quanto depende de fallback/interpretação continua em aberto. A PR permanece Draft e sem merge; não é apresentada como gameplay corrigida.

## Evidência reproduzível

Na revisão fixada da engine:

- `docs/RUNTIME_MEMORY_HOTPATHS.md`: comandos, ambiente, limitações e comparação ARM64.
- `tests/performance/runtime_bench.cpp`: fixture sem ROM, opt-in, fora do build normal.
- `tests/performance/compare_runtime.py`: compara dois executáveis e verifica estabilidade/igualdade dos resultados.
- `tests/performance/results/2026-10-26-host.json`: amostras brutas e resultados completos.

Nenhum procedimento adicional de log/ADB/CSV no celular é exigido. Para avaliar o ganho real será necessária uma futura build e teste normal do APK, iniciado manualmente pelo proprietário quando decidir usar seus minutos.
