/*
 * test_format_time.c — Testes unitários para FormatTime (M3-T1)
 *
 * Testa a função FormatTime() isolada, sem dependência de GDI/janelas.
 * Compila com: gcc -o test_format_time.exe test_format_time.c
 * Roda com: ./test_format_time.exe
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>

/* Incluir a implementação de FormatTime inline (para teste unitário puro).
 * Em produção, FormatTime está em util.cpp e é linkada normalmente. */
#include "../src/util.h"

/* Contador de testes. */
static int g_testsRun = 0;
static int g_testsFail = 0;

#define TEST(name) \
    do { \
        g_testsRun++; \
        printf("  test %d: %-50s ", g_testsRun, name); \
    } while (0)

#define ASSERT_WSTR_EQ(expected, actual, msg) \
    do { \
        if (wcscmp((expected), (actual)) != 0) { \
            g_testsFail++; \
            printf("FAIL (%s: expected [%ls], got [%ls])\n", \
                   (msg), (expected), (actual)); \
            return; \
        } \
    } while (0)

#define ASSERT_FLOAT_EQ(expected, actual, epsilon, msg) \
    do { \
        double _exp = (expected), _act = (actual); \
        if (_act < _exp - (epsilon) || _act > _exp + (epsilon)) { \
            g_testsFail++; \
            printf("FAIL (%s: expected ~%f, got %f)\n", \
                   (msg), _exp, _act); \
            return; \
        } \
    } while (0)

/* ------------------------------------------------------------------ */
/* Testes                                                              */
/* ------------------------------------------------------------------ */

static void test_format_time_zero_seconds(void)
{
    TEST("FormatTime(0s, elapsed) = \"00:00\"");
    wchar_t buf[16] = { 0 };
    FormatTime(0.0, FALSE, 10.0, buf, ARRAYSIZE(buf));
    ASSERT_WSTR_EQ(L"00:00", buf, "zero seconds elapsed");
}

static void test_format_time_normal(void)
{
    TEST("FormatTime(45s, elapsed) = \"00:45\"");
    wchar_t buf[16] = { 0 };
    FormatTime(45.0, FALSE, 120.0, buf, ARRAYSIZE(buf));
    ASSERT_WSTR_EQ(L"00:45", buf, "45 seconds elapsed");
}

static void test_format_time_minutes_seconds(void)
{
    TEST("FormatTime(125s, elapsed) = \"02:05\"");
    wchar_t buf[16] = { 0 };
    FormatTime(125.0, FALSE, 300.0, buf, ARRAYSIZE(buf));
    ASSERT_WSTR_EQ(L"02:05", buf, "125 seconds = 02:05");
}

static void test_format_time_remaining(void)
{
    TEST("FormatTime(45s, remaining, dur=120s) = \"-01:15\"");
    wchar_t buf[16] = { 0 };
    FormatTime(45.0, TRUE, 120.0, buf, ARRAYSIZE(buf));
    ASSERT_WSTR_EQ(L"-01:15", buf, "remaining time");
}

static void test_format_time_remaining_zero_position(void)
{
    TEST("FormatTime(0s, remaining) = \"02:00\" (no minus at start)");
    wchar_t buf[16] = { 0 };
    FormatTime(0.0, TRUE, 120.0, buf, ARRAYSIZE(buf));
    /* Quando position é ~0, mostra decorrido mesmo com showRemaining=TRUE. */
    ASSERT_WSTR_EQ(L"02:00", buf, "remaining at start shows elapsed");
}

static void test_format_time_max_99_59(void)
{
    TEST("FormatTime(7200s, elapsed) = \"99:59\" (capped)");
    wchar_t buf[16] = { 0 };
    FormatTime(7200.0, FALSE, 7200.0, buf, ARRAYSIZE(buf));
    ASSERT_WSTR_EQ(L"99:59", buf, "capped at 99:59");
}

static void test_format_time_rounding(void)
{
    TEST("FormatTime(59.6s, elapsed) = \"01:00\" (rounded)");
    wchar_t buf[16] = { 0 };
    FormatTime(59.6, FALSE, 120.0, buf, ARRAYSIZE(buf));
    ASSERT_WSTR_EQ(L"01:00", buf, "59.6 rounds to 60s = 01:00");
}

static void test_format_time_small_buffer(void)
{
    TEST("FormatTime with tiny buffer (cchMax < 6) — no crash");
    wchar_t buf[2] = { 0 };
    FormatTime(45.0, FALSE, 120.0, buf, 2);
    /* Deve retornar sem escrever nada (buffer muito pequeno). */
    ASSERT_WSTR_EQ(L"", buf, "tiny buffer");
}

static void test_format_time_just_under_minute(void)
{
    TEST("FormatTime(59s, elapsed) = \"00:59\"");
    wchar_t buf[16] = { 0 };
    FormatTime(59.0, FALSE, 120.0, buf, ARRAYSIZE(buf));
    ASSERT_WSTR_EQ(L"00:59", buf, "just under a minute");
}

static void test_format_time_exact_hour(void)
{
    TEST("FormatTime(3600s, elapsed) = \"60:00\"");
    wchar_t buf[16] = { 0 };
    FormatTime(3600.0, FALSE, 3600.0, buf, ARRAYSIZE(buf));
    ASSERT_WSTR_EQ(L"60:00", buf, "exactly one hour");
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    printf("\n=== M3-T1: FormatTime unit tests ===\n\n");

    test_format_time_zero_seconds();
    test_format_time_normal();
    test_format_time_minutes_seconds();
    test_format_time_remaining();
    test_format_time_remaining_zero_position();
    test_format_time_max_99_59();
    test_format_time_rounding();
    test_format_time_small_buffer();
    test_format_time_just_under_minute();
    test_format_time_exact_hour();

    printf("\n--- Results: %d tests, %d failures ---\n\n",
           g_testsRun, g_testsFail);

    return (g_testsFail > 0) ? 1 : 0;
}
