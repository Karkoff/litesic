/*
 * build_third_party_smoke.c — Teste de smoke para third_party/
 *
 * Usa a mesma configuração do aplicativo e vincula miniaudio_impl.o.
 * Decodifica um WAV em memória; aceita arquivos opcionais para verificar
 * MP3/FLAC/OGG com a implementação real, sem depender de dispositivo.
 */

#include "ma_config.h"
#include "stb_vorbis.h"
#include "miniaudio.h"
#include <stdio.h>

#if defined(MA_NO_WAV) || defined(MA_NO_MP3) || defined(MA_NO_FLAC)
#error Os decoders WAV, MP3 e FLAC devem estar habilitados.
#endif

int main(int argc, char** argv)
{
    /* WAV PCM mono, 16 bits, 8 kHz, duas amostras. Sem dispositivo de audio. */
    static const unsigned char wav[] = {
        'R','I','F','F',40,0,0,0,'W','A','V','E',
        'f','m','t',' ',16,0,0,0,1,0,1,0,64,31,0,0,
        128,62,0,0,2,0,16,0,'d','a','t','a',4,0,0,0,0,0,0,0
    };
    ma_decoder decoder;
    float samples[256];
    ma_uint64 frames = 0;
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 1, 8000);
    if (ma_decoder_init_memory(wav, sizeof(wav), &config, &decoder) != MA_SUCCESS) {
        fprintf(stderr, "WAV decoder initialization failed\n");
        return 1;
    }
    ma_decoder_read_pcm_frames(&decoder, samples, 2, &frames);
    ma_decoder_uninit(&decoder);
    if (frames != 2) return 1;
    puts("Dependency smoke: real miniaudio WAV decoding PASS");

    /* Arquivos opcionais para validar os outros formatos, sem downloads. */
    for (int i = 1; i < argc; ++i) {
        if (ma_decoder_init_file(argv[i], &config, &decoder) != MA_SUCCESS) {
            fprintf(stderr, "Decoder initialization failed: %s\n", argv[i]);
            return 1;
        }
        frames = 0;
        ma_decoder_read_pcm_frames(&decoder, samples, 256, &frames);
        ma_decoder_uninit(&decoder);
        if (frames == 0) return 1;
        printf("File decode PASS: %s (%llu frames)\n", argv[i], (unsigned long long)frames);
    }
    return 0;
}
