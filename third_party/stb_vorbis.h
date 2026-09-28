/*
 * stb_vorbis.h — Declarações mínimas para miniaudio Vorbis backend
 *
 * Este header fornece apenas as declarações que o miniaudio precisa
 * para configurar seu decoder customizado de OGG Vorbis. A implementação
 * real está em stb_vorbis.c (incluída com STB_VORBIS_IMPLEMENTATION).
 *
 * Adaptado do stb_vorbis.h original (nothings/stb) — domínio público.
 */

#ifndef STB_VORBIS_INCLUDE_STB_VORBIS_H
#define STB_VORBIS_INCLUDE_STB_VORBIS_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- types ---------- */

typedef struct stb_vorbis stb_vorbis;

typedef struct
{
    char *alloc_buffer;
    int   alloc_buffer_length_in_bytes;
} stb_vorbis_alloc;

typedef struct
{
    unsigned int sample_rate;
    int channels;

    unsigned int setup_memory_required;
    unsigned int setup_temp_memory_required;
    unsigned int temp_memory_required;

    int max_frame_size;
} stb_vorbis_info;

typedef struct
{
    char *vendor;

    int comment_list_length;
    char **comment_list;
} stb_vorbis_comment;

/* ---------- general info ---------- */

extern stb_vorbis_info stb_vorbis_get_info(stb_vorbis *f);
extern stb_vorbis_comment stb_vorbis_get_comment(stb_vorbis *f);
extern int stb_vorbis_get_error(stb_vorbis *f);
extern void stb_vorbis_close(stb_vorbis *f);
extern int stb_vorbis_get_sample_offset(stb_vorbis *f);
extern unsigned int stb_vorbis_get_file_offset(stb_vorbis *f);

/* ---------- pushdata API ---------- */

#ifndef STB_VORBIS_NO_PUSHDATA_API
extern stb_vorbis *stb_vorbis_open_pushdata(
         const unsigned char *datablock, int datablock_length_in_bytes,
         int *datablock_memory_consumed_in_bytes,
         int *error,
         const stb_vorbis_alloc *alloc_buffer);

extern int stb_vorbis_decode_frame_pushdata(
         stb_vorbis *f,
         const unsigned char *datablock, int datablock_length_in_bytes,
         int *channels,
         float ***output,
         int *samples);

extern void stb_vorbis_flush_pushdata(stb_vorbis *f);
#endif

/* ---------- pulling input API ---------- */

#ifndef STB_VORBIS_NO_PULLDATA_API

#if !defined(STB_VORBIS_NO_STDIO) && !defined(STB_VORBIS_NO_INTEGER_CONVERSION)
extern int stb_vorbis_decode_filename(const char *filename, int *channels, int *sample_rate, short **output);
#endif
#if !defined(STB_VORBIS_NO_INTEGER_CONVERSION)
extern int stb_vorbis_decode_memory(const unsigned char *mem, int len, int *channels, int *sample_rate, short **output);
#endif

extern stb_vorbis *stb_vorbis_open_memory(const unsigned char *data, int len,
                                  int *error, const stb_vorbis_alloc *alloc_buffer);

#ifndef STB_VORBIS_NO_STDIO
extern stb_vorbis *stb_vorbis_open_filename(const char *filename,
                                  int *error, const stb_vorbis_alloc *alloc_buffer);
extern stb_vorbis *stb_vorbis_open_file(FILE *f, int close_handle_on_close,
                                  int *error, const stb_vorbis_alloc *alloc_buffer);
extern stb_vorbis *stb_vorbis_open_file_section(FILE *f, int close_handle_on_close,
                int *error, const stb_vorbis_alloc *alloc_buffer, unsigned int len);
#endif

extern int stb_vorbis_seek_frame(stb_vorbis *f, unsigned int sample_number);
extern int stb_vorbis_seek(stb_vorbis *f, unsigned int sample_number);
extern int stb_vorbis_seek_start(stb_vorbis *f);

extern unsigned int stb_vorbis_stream_length_in_samples(stb_vorbis *f);
extern float        stb_vorbis_stream_length_in_seconds(stb_vorbis *f);

extern int stb_vorbis_get_frame_float(stb_vorbis *f, int *channels, float ***output);

#ifndef STB_VORBIS_NO_INTEGER_CONVERSION
extern int stb_vorbis_get_frame_short_interleaved(stb_vorbis *f, int num_c, short *buffer, int num_shorts);
extern int stb_vorbis_get_frame_short(stb_vorbis *f, int num_c, short **buffer, int num_samples);
#endif

extern int stb_vorbis_get_samples_float_interleaved(stb_vorbis *f, int channels, float *buffer, int num_floats);
extern int stb_vorbis_get_samples_float(stb_vorbis *f, int channels, float **buffer, int num_samples);

#ifndef STB_VORBIS_NO_INTEGER_CONVERSION
extern int stb_vorbis_get_samples_short_interleaved(stb_vorbis *f, int channels, short *buffer, int num_shorts);
extern int stb_vorbis_get_samples_short(stb_vorbis *f, int channels, short **buffer, int num_samples);
#endif

#endif /* STB_VORBIS_NO_PULLDATA_API */

/* ---------- error codes ---------- */

enum STBVorbisError
{
    VORBIS__no_error,

    VORBIS_need_more_data = 1,

    VORBIS_invalid_api_mixing,
    VORBIS_outofmem,
    VORBIS_feature_not_supported,
    VORBIS_too_many_channels,
    VORBIS_file_open_failure,
    VORBIS_seek_without_length,

    VORBIS_unexpected_eof = 10,
    VORBIS_seek_invalid,

    VORBIS_invalid_setup = 20,
    VORBIS_invalid_stream,

    VORBIS_missing_capture_pattern = 30,
    VORBIS_invalid_stream_structure_version,
    VORBIS_continued_packet_flag_invalid,
    VORBIS_incorrect_stream_serial_number,
    VORBIS_invalid_first_page,
    VORBIS_bad_packet_type,
    VORBIS_cant_find_last_page,
    VORBIS_seek_failed,
    VORBIS_ogg_skeleton_not_supported

};

#ifdef __cplusplus
}
#endif

#endif /* STB_VORBIS_INCLUDE_STB_VORBIS_H */
