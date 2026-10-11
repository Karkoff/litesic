# Fixtures de áudio da M5

Arquivos originais, sintéticos e silenciosos, gerados por `generate.py`.
Não contêm gravações ou músicas de terceiros. Podem ser usados sob a licença
MIT deste projeto. FFmpeg é necessário somente para regenerar os arquivos;
os testes e o executável não dependem de FFmpeg nem de Python.

```powershell
py -3.11 tests/fixtures/generate.py C:/ffmpeg/bin/ffmpeg.exe
```

- `native-44100.wav`, `native-48000.wav`: PCM estéreo, 16 bits, dois segundos.
- `tagged.mp3`, `tagged-v4.mp3`: 0,25 s, ID3v2.3/2.4, título `Canção M5`,
  artista `LiteSic`.
- `tagged-v1.mp3`: mesmos frames MP3, removido o ID3v2 e acrescentado ID3v1
  com título `Faixa antiga`, artista `LiteSic`.
- `tagged.flac`, `tagged.ogg`: 0,25 s e os mesmos título/artista. O OGG tem
  um comentário de 70.000 caracteres para forçar continuação entre páginas.

Os testes truncam cópias em memória, verificam metadados conhecidos e usam
os decodificadores reais. Fixtures maiores da GUI ficam em `build/gui` e são
geradas separadamente por `scripts/gui.ps1 -Action m5-fixtures`.
