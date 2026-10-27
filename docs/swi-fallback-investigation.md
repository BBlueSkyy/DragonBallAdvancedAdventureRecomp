# Continuação: interpretação ampliada após SWI

Base do jogo: `perf/runtime-memory-hotpaths`, `03321d0222d13b07b9bf38dc513b01ee1a338d42` (PR Draft #1).
Engine anterior: `3dc84528e741157a14bc1e901f347fb3f9dcf3d0`.
Engine proposta: `3886062fad7574a1aea03ed60e5f6b63dfac79cb`, [PR Draft #2](https://github.com/BBlueSkyy/gbarecomp/pull/2), baseada na branch da #1.

## Gargalo comprovado e alteração

O retorno de uma SWI interpretada podia ampliar a interpretação para a função guest chamadora, mesmo quando essa função já tinha uma entrada nativa. A condição reproduzida exige um miss no vetor 8 e uma chamada BL guest aberta. A SWI não cria um novo frame BL; o fallback usava o retorno do frame que já estava aberto, interpretando o restante do chamador antes de devolver o controle.

A correção limita esse caso ao retorno de exceção SVC da própria SWI e preserva o frame do chamador para o protocolo existente de cancelamento/redispatch do código gerado. Ela ocorre depois da restauração de SPSR/bancos, validação do ledger e tick da instrução. Não remove verificações de segurança nem muda valores de ciclos, scheduler, IRQ, DMA, FPS ou sincronização.

É uma remoção comprovada de trabalho interpretado desnecessário. **Ainda não foi demonstrado que esse caminho seja a causa predominante do slowdown de Dragon Ball no Moto G100.**

## Evidência sem ROM/BIOS privada

Foram usadas instruções escritas para os testes e as bibliotecas reais da engine. O teste de regressão falha na #1 e passa após a correção. A comparação inclui retorno ARM/Thumb, profundidades BL 0/1/3, unwind/redispatch, SWIs aninhadas, forced-stop, subárvores comuns e proteção de frames durante uma IRQ realmente entregue e reconhecida.

No cenário de amplificação, uma SWI é seguida de 10 mil iterações nativas já disponíveis (`ADD`, `SUBS`, `BNE`, `BX LR`):

| Cenário | Instruções interpretadas antes → depois | Ciclos guest em ambos | Estado serializado igual |
| --- | --- | ---: | --- |
| SWI sem IRQ pendente | 30.002 → 1 | 50.007 | 545.066 bytes; FNV `05a1ba3b57a5fce1` |
| SWI com IRQ pendente | 30.006 → 5 | 50.016 | 545.066 bytes; FNV `9f547cc01820372e` |

As quatro instruções interpretadas do handler de IRQ continuam presentes. O chamador passa a executar uma vez pelo caminho nativo.

Em 12 pares de execuções alternadas por cenário no host Linux x86_64/GCC 13.3, as medianas foram 2,723 → 0,553 ms sem IRQ e 2,490 → 0,903 ms com IRQ. Incluem o custo inicial de registrar o miss, comum aos dois lados. São medidas sintéticas desse caminho, **não ganhos de FPS ou de gameplay no Android**. Nenhuma opção O2/O3 foi adicionada ao jogo.

Passaram 10 CTests essenciais: novo retorno SWI, retorno IRQ, snapshot SVC, tail dispatch, bus, timers, DMA, elegibilidade de IRQ, PPU e áudio. Uma revisão independente repetiu os dois cenários e confirmou os resultados. Evidência completa, fonte e amostras: `docs/BRIDGE_SWI_BOUNDARY.md` e `tests/performance/results/2026-10-26-swi-boundary-host.json` na engine fixada.

## Cobertura nativa, RAM e incertezas

Os workflows atuais não geram os corpos AOT da BIOS; a engine usa tabela BIOS vazia quando esses corpos não existem. A configuração não habilita BIOS HLE, e o processo Android desabilita recompilação dinâmica por padrão. Portanto, o caminho de fallback investigado é relevante à configuração e um miss não se cura automaticamente por compilação no aparelho. Overlays previamente disponíveis são um caso separado.

Isso não estabelece a quantidade de misses, SWIs com frames abertos ou instruções interpretadas por frame da primeira fase. O catálogo efetivo dos templates e os PCs de código RAM executados também continuam desconhecidos. A interface de arquivos do repositório não forneceu o conteúdo dos arquivos gerados muito grandes; os shards não foram regenerados.

A análise da engine confirmou que o catálogo RAM é registrado pelo construtor emitido no dispatch gerado. `lookup_native_ram()` verifica modo e bytes completos, inclusive em hits de cache; bytes alterados são rejeitados antes de AOT. Esses mecanismos e o fallback foram preservados. Não foram inventados endereços/templates de RAM ou importados os de outro jogo. Não há evidência adicional para atribuir a maior parte do tempo à PPU, áudio ou scheduler, por isso não foram modificados.

Callbacks de mods que definem seu próprio retorno mantêm sua ABI anterior. O teste com IRQ também revelou uma diferença preexistente entre BIOS nativa e interpretada no instante de restaurar SPSR em relação ao tick de retorno SVC. A correção não move esse tick; o estado final da versão interpretada antes/depois é idêntico no cenário com IRQ. Não se reivindica resolver essa diferença ou validar todas as combinações de IRQ aninhadas.

## Build e próximo passo

A única mudança de build é o pin da engine para `3886062fad7574a1aea03ed60e5f6b63dfac79cb` no Codemagic e no workflow manual do Actions. Android ARM64, shards O1, gráficos/áudio/controles/saves e metadados do jogo permanecem configurados como na #1. Nenhum shard foi alterado, nenhum APK foi gerado nesta investigação e nenhuma build remota foi disparada.

As PRs #1 e main ficam inalteradas; esta é uma Draft separada, sem merge. A próxima validação útil dentro das restrições é um teste normal de gameplay de um APK dessa nova branch, compilado manualmente quando o proprietário decidir usar seus minutos. Não é necessário qualquer novo procedimento de log, CSV, ADB ou instrumentação no aparelho. Até esse teste, a melhoria global da gameplay permanece não comprovada.
