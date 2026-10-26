# Continuação: Android ARM64 no GitHub Actions (somente repositório PRIVADO)

Este complemento não gera APK sozinho. Ele prepara o runner GitHub Actions para compilar as 16 unidades C++ e `dispatch_table.cpp` que você gerou no Termux. Não envia ROM ou BIOS.

## Aplicar o complemento

No Termux, dentro de `~/DragonBallAdvancedAdventure_USA_Android`, extraia o ZIP de complemento. Ele acrescenta `.github/workflows/android-arm64-debug.yml`, `scripts/generate_ci_metadata.py` e `scripts/prepare_private_repo.sh`, e substitui `CMakeLists.txt` pela versão de primeiro boot com otimização -O0.

## Preparar os arquivos para o repositório privado

```bash
bash scripts/prepare_private_repo.sh
# Revise os arquivos antes de enviar:
git status --short
```

O script valida que 16 C++ shards existem e que cada arquivo gerado tem menos de 95 MB. Adiciona os arquivos do host e, forçadamente, somente o código C++/headers gerado (que de outro modo fica ignorado). Não adiciona `.gba`, BIOS, saves, bibliotecas `.o`, configurações locais nem os clones `gbarecomp` e `recomp-ui`.

## Criar e enviar o repositório privado (ação manual explícita)

```bash
pkg install gh git -y
gh auth login
# Confirme que o usuário GitHub autenticado é BBlueSkyy:
gh api user --jq .login
# Revise o que será adicionado:
git diff --cached --stat
# Configure seu Git user.name e user.email caso necessário:
git commit -m "Initial private Android ARM64 recompilation"
gh repo create BBlueSkyy/DragonBallAdvancedAdventureRecomp --private --source . --remote origin --push
```

**Crie como privado.** Isso é importante porque os C++ gerados são derivados de uma ROM comercial. Nunca anexe esses fontes, ROM ou BIOS a uma release pública. `gh auth login` é interativo: não envie tokens ao chat.

Se o GitHub recusar algum arquivo maior que 100 MB, ajuste `codegen_shards` para 32 no `game.toml`, gere novamente no Termux e altere a validação da contagem no script `generate_ci_metadata.py`.

## Rodar a primeira build

GitHub → repo privado → Actions → `Dragon Ball Advanced Adventure - Android ARM64` → Run workflow.

A build utiliza o GBARecomp `e353848` e recomp-ui `3f1d6f1`, obtém submódulos e instala NDK `27.1.12297006`, SDK API 35 e Gradle 8.11.1. O APK aparece nos artifacts do job `build` se a compilação e linkedição terminarem sem erros. É apenas um APK de depuração, não um port jogável validado.

## Caso falhe

Compartilhe apenas as últimas 60 linhas de saída da etapa que falhou, removendo quaisquer dados privados. O primeiro erro real é mais útil do que dezenas de outros erros de compilação em cascata.
