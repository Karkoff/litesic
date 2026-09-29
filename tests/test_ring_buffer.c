/*
 * test_ring_buffer.c — Testes unitários do ring buffer de visualização (M4-T1)
 *
 * Testa a lógica do ring buffer isoladamente, sem depender do miniaudio ou
 * de janelas. O ring buffer é uma estrutura de dados pura com:
 *   - Buffer circular de float[VISUALIZATION_BUFFER_SIZE]
 *   - Posição de escrita (writePos)
 *   - Contador de amostras disponíveis (count)
 *   - CRITICAL_SECTION para proteção concorrente
 *
 * Como a implementação real está em audio.cpp e usa CRITICAL_SECTION do
 * Windows, este teste reimplementa uma versão simplificada da mesma lógica
 * em C puro para validar o algoritmo de wraparound e ordem das amostras.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* TRUE/FALSE para ambiente Windows/MinGW. */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* ------------------------------------------------------------------ */
/* Configurações                                                      */
/* ------------------------------------------------------------------ */

#define VISUALIZATION_BUFFER_SIZE  512

/* ------------------------------------------------------------------ */
/* Estrutura do ring buffer (espelho da implementação em audio.cpp)   */
/* ------------------------------------------------------------------ */

typedef struct
{
    float        buffer[VISUALIZATION_BUFFER_SIZE];
    int          writePos;
    int          count;
} VizRingBuffer;

/* ------------------------------------------------------------------ */
/* Operações básicas do ring buffer                                   */
/* Versão simplificada sem CRITICAL_SECTION (testes unitários).       */
/* ------------------------------------------------------------------ */

static void ring_buffer_init(VizRingBuffer* rb)
{
    memset(rb->buffer, 0, sizeof(rb->buffer));
    rb->writePos = 0;
    rb->count    = 0;
}

/* Escreve uma única amostra. Sempre succeed — sobrescreve se cheio. */
static int ring_buffer_write(VizRingBuffer* rb, float sample)
{
    if (rb->count >= VISUALIZATION_BUFFER_SIZE)
    {
        /* Buffer cheio — sobrescrever a amostra mais antiga e avançar writePos. */
        rb->buffer[rb->writePos] = sample;
        rb->writePos = (rb->writePos + 1) % VISUALIZATION_BUFFER_SIZE;
        /* count permanece VISUALIZATION_BUFFER_SIZE. */
    }
    else
    {
        rb->buffer[rb->writePos] = sample;
        rb->writePos = (rb->writePos + 1) % VISUALIZATION_BUFFER_SIZE;
        rb->count++;
    }

    return TRUE;
}

/* Lê N amostras mais recentes. Retorna o número lido. */
static int ring_buffer_read_recent(VizRingBuffer* rb, float* out, int count)
{
    if (rb->count == 0 || out == NULL || count <= 0)
        return 0;

    /* Limitar ao tamanho do buffer. */
    if (count > VISUALIZATION_BUFFER_SIZE)
        count = VISUALIZATION_BUFFER_SIZE;

    int available = rb->count;
    int toCopy = count < available ? count : available;

    /* Copiar amostras mais recentes: writePos-1, writePos-2, ... */
    for (int i = 0; i < toCopy; i++)
    {
        int idx = (rb->writePos - 1 - i + VISUALIZATION_BUFFER_SIZE) % VISUALIZATION_BUFFER_SIZE;
        out[i] = rb->buffer[idx];
    }

    return toCopy;
}

/* Lê N amostras mais antigas (para teste de ordem). */
static int ring_buffer_read_oldest(VizRingBuffer* rb, float* out, int count)
{
    if (rb->count == 0 || out == NULL || count <= 0)
        return 0;

    if (count > VISUALIZATION_BUFFER_SIZE)
        count = VISUALIZATION_BUFFER_SIZE;

    int available = rb->count;
    int toCopy = count < available ? count : available;

    /* Copiar amostras mais antigas: writePos-count, writePos-count+1, ... */
    int startIdx = (rb->writePos - available + VISUALIZATION_BUFFER_SIZE) % VISUALIZATION_BUFFER_SIZE;
    for (int i = 0; i < toCopy; i++)
    {
        int idx = (startIdx + i + VISUALIZATION_BUFFER_SIZE) % VISUALIZATION_BUFFER_SIZE;
        out[i] = rb->buffer[idx];
    }

    return toCopy;
}

/* ------------------------------------------------------------------ */
/* Testes                                                             */
/* ------------------------------------------------------------------ */

static int test_write_read_order(void)
{
    VizRingBuffer rb;
    float out[16];
    int i;

    printf("  test_write_read_order... ");

    ring_buffer_init(&rb);

    /* Escrever 16 amostras sequenciais. */
    for (i = 0; i < 16; i++)
        ring_buffer_write(&rb, (float)i);

    assert(rb.count == 16);

    /* Ler as 16 amostras mais antigas — devem estar em ordem. */
    int n = ring_buffer_read_oldest(&rb, out, 16);
    assert(n == 16);

    for (i = 0; i < 16; i++)
    {
        if (out[i] != (float)i)
        {
            printf("FAIL (expected %.1f at index %d, got %.1f)\n",
                   (float)i, i, out[i]);
            return FALSE;
        }
    }

    printf("PASS\n");
    return TRUE;
}

static int test_wraparound(void)
{
    VizRingBuffer rb;
    float out[8];
    int i;

    printf("  test_wraparound... ");

    ring_buffer_init(&rb);

    /* Preencher o buffer até a capacidade. */
    for (i = 0; i < VISUALIZATION_BUFFER_SIZE; i++)
        ring_buffer_write(&rb, (float)i);

    assert(rb.count == VISUALIZATION_BUFFER_SIZE);
    printf("(writePos=%d count=%d) ", rb.writePos, rb.count);

    /* Escrever mais 8 amostras — devem sobrescrever as primeiras. */
    for (i = 0; i < 8; i++)
        ring_buffer_write(&rb, (float)(VISUALIZATION_BUFFER_SIZE + i));

    assert(rb.count == VISUALIZATION_BUFFER_SIZE); /* Não excede. */
    printf("(writePos=%d count=%d) ", rb.writePos, rb.count);

    /* Ler as 8 amostras mais recentes — read_recent retorna em ordem
     * inversa: out[0] = mais recente (519), out[7] = menos recente (512). */
    int n = ring_buffer_read_recent(&rb, out, 8);
    assert(n == 8);

    /* Debug: imprimir o que foi lido. */
    printf("[ ");
    for (i = 0; i < 8; i++)
        printf("%.0f ", out[i]);
    printf("]\n");

    for (i = 0; i < 8; i++)
    {
        float expected = (float)(VISUALIZATION_BUFFER_SIZE + 7 - i); /* 519, 518, ..., 512 */
        if (out[i] != expected)
        {
            printf("FAIL (expected %.1f at index %d, got %.1f)\n",
                   expected, i, out[i]);
            return FALSE;
        }
    }

    printf("PASS\n");
    return TRUE;
}

static int test_empty_read(void)
{
    VizRingBuffer rb;
    float out[8];

    printf("  test_empty_read... ");

    ring_buffer_init(&rb);

    /* Ler do buffer vazio deve retornar 0. */
    int n = ring_buffer_read_recent(&rb, out, 8);
    assert(n == 0);

    printf("PASS\n");
    return TRUE;
}

static int test_partial_read(void)
{
    VizRingBuffer rb;
    float out[16];
    int i;

    printf("  test_partial_read... ");

    ring_buffer_init(&rb);

    /* Escrever apenas 4 amostras. */
    for (i = 0; i < 4; i++)
        ring_buffer_write(&rb, (float)(i * 10));

    assert(rb.count == 4);

    /* Tentar ler 16 — só deve retornar 4. */
    int n = ring_buffer_read_recent(&rb, out, 16);
    assert(n == 4);

    /* As amostras mais recentes são: 30, 20, 10, 0 (ordem inversa). */
    assert(out[0] == 30.0f);
    assert(out[1] == 20.0f);
    assert(out[2] == 10.0f);
    assert(out[3] == 0.0f);

    printf("PASS\n");
    return TRUE;
}

static int test_write_full_overwrite(void)
{
    VizRingBuffer rb;

    printf("  test_write_full_overwrite... ");

    ring_buffer_init(&rb);

    /* Preencher completamente. */
    for (int i = 0; i < VISUALIZATION_BUFFER_SIZE; i++)
        ring_buffer_write(&rb, (float)i);

    assert(rb.count == VISUALIZATION_BUFFER_SIZE);

    /* Escrever mais — deve sobrescrever e retornar TRUE. */
    int result = ring_buffer_write(&rb, 999.0f);
    assert(result == TRUE);
    assert(rb.count == VISUALIZATION_BUFFER_SIZE); /* Count não muda. */

    printf("PASS\n");
    return TRUE;
}

static int test_large_read(void)
{
    VizRingBuffer rb;
    float out[VISUALIZATION_BUFFER_SIZE];
    int i;

    printf("  test_large_read... ");

    ring_buffer_init(&rb);

    /* Escrever N amostras. */
    for (i = 0; i < 256; i++)
        ring_buffer_write(&rb, (float)i);

    assert(rb.count == 256);

    /* Tentar ler mais do que existe — deve limitar ao disponível. */
    int n = ring_buffer_read_recent(&rb, out, VISUALIZATION_BUFFER_SIZE);
    assert(n == 256); /* Limitado ao count. */

    printf("PASS\n");
    return TRUE;
}

/* ------------------------------------------------------------------ */
/* Main                                                               */
/* ------------------------------------------------------------------ */

int main(void)
{
    int passed = 0;
    int failed = 0;
    int total;

    printf("\n=== Ring Buffer Tests (M4-T1) ===\n\n");

    if (test_write_read_order())     passed++; else failed++;
    if (test_wraparound())           passed++; else failed++;
    if (test_empty_read())           passed++; else failed++;
    if (test_partial_read())         passed++; else failed++;
    if (test_write_full_overwrite()) passed++; else failed++;
    if (test_large_read())           passed++; else failed++;

    total = passed + failed;

    printf("\nResults: %d/%d tests passed", passed, total);
    if (failed > 0)
        printf(" (%d FAILED)", failed);
    printf("\n\n");

    return failed > 0 ? 1 : 0;
}
