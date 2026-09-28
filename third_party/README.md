# Third Party Libraries

Bibliotecas de terceiros vendorizadas no projeto LiteSic.

## miniaudio

- **Versão**: v0.11.25 (2026-03-04)
- **Fonte**: https://miniaud.io | https://github.com/mackron/miniaudio
- **Licença**: Public Domain ou MIT-0 (à escolha)
- **Data do vendoring**: 2026-01-01

## stb_vorbis

- **Versão**: v1.22
- **Fonte**: https://github.com/nothings/stb
- **Licença**: MIT License ou Public Domain (Unlicense) — à escolha
- **Data do vendoring**: 2026-01-01

## Integração

O miniaudio usa o stb_vorbis para decodificação de arquivos OGG Vorbis.
Para que a integração funcione, o header `stb_vorbis.h` deve ser incluído
antes de `miniaudio.h` (o que acontece automaticamente quando o arquivo
`.h` do stb é encontrado). A implementação é ativada definindo
`MINIAUDIO_IMPLEMENTATION` em apenas uma translation unit (`src/miniaudio_impl.c`).
