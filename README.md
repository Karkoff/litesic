# LiteSic

Player de áudio estilo Winamp em C++ nativo com Win32/GDI, sem runtimes extras.
Rodando no Windows 7, 8, 10 e 11. Três telas: player principal, equalizador
de 10 bandas e lista de reprodução.

## Compilar

```bash
make                  # Release 64 bits
make DEBUG=1          # Debug
make ARCH=32          # Release 32 bits
make clean            # Limpar build/
```

No VS Code: `Ctrl+Shift+B` para compilar, `F5` para depurar.

## Depurar

Abra o projeto no VS Code e pressione `F5`. A task usa gdb do MSYS2 MINGW64.

## Documentação

- [AGENT.md](AGENT.md) — regras e convenções do projeto
- [PLANO-VSCODE.md](PLANO-VSCODE.md) — plano completo de arquitetura
- [docs/ROADMAP.md](docs/ROADMAP.md) — milestones e tarefas
