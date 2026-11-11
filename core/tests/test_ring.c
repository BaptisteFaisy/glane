/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * test_ring.c — vérifie le tampon circulaire (GLA-03, GLA-16).
 */
#include "glane/glane_ring.h"
#include "test_util.h"

static glane_sample mk(uint32_t seq)
{
    glane_sample s;
    glane_sample_init(&s, 1, GLANE_UNIT_C);
    s.seq = seq;
    return s;
}

static void test_init_and_push(void)
{
    glane_sample buf[4];
    glane_ring r;
    glane_ring_init(&r, buf, 4);
    CHECK_EQ_I(glane_ring_count(&r), 0u);

    glane_sample s = mk(10);
    CHECK_EQ_I(glane_ring_push(&r, &s), 0);  /* pas d'écrasement */
    CHECK_EQ_I(glane_ring_count(&r), 1u);

    glane_sample out;
    CHECK_EQ_I(glane_ring_peek(&r, 0, &out), 0);
    CHECK_EQ_I(out.seq, 10u);
    /* peek hors bornes */
    CHECK_EQ_I(glane_ring_peek(&r, 1, &out), -1);
    CHECK_EQ_I(glane_ring_peek(&r, 99, &out), -1);
}

static void test_fill_and_overwrite(void)
{
    glane_sample buf[3];
    glane_ring r;
    glane_ring_init(&r, buf, 3);
    /* Remplit 3 éléments : seq 1,2,3. */
    for (uint32_t i = 1; i <= 3; i++) {
        glane_sample s = mk(i);
        CHECK_EQ_I(glane_ring_push(&r, &s), 0);
    }
    CHECK_EQ_I(glane_ring_count(&r), 3u);
    glane_sample out;
    CHECK_EQ_I(glane_ring_peek(&r, 0, &out), 0);
    CHECK_EQ_I(out.seq, 1u);  /* plus ancien */
    CHECK_EQ_I(glane_ring_peek(&r, 2, &out), 0);
    CHECK_EQ_I(out.seq, 3u);  /* plus récent */

    /* 4e push écrase le plus ancien (seq 1) et retourne 1. */
    glane_sample s4 = mk(4);
    CHECK_EQ_I(glane_ring_push(&r, &s4), 1);
    CHECK_EQ_I(glane_ring_count(&r), 3u);  /* capacité inchangée */
    CHECK_EQ_I(glane_ring_peek(&r, 0, &out), 0);
    CHECK_EQ_I(out.seq, 2u);  /* seq 1 écrasé, 2 est le plus ancien */
    CHECK_EQ_I(glane_ring_peek(&r, 2, &out), 0);
    CHECK_EQ_I(out.seq, 4u);
}

static void test_find_gap_none(void)
{
    glane_sample buf[5];
    glane_ring r;
    glane_ring_init(&r, buf, 5);
    for (uint32_t i = 0; i < 5; i++) {
        glane_sample s = mk(100 + i);
        glane_ring_push(&r, &s);
    }
    size_t gap = 999;
    CHECK_EQ_I(glane_ring_find_gap(&r, &gap), -1);  /* aucune perte */
    CHECK_EQ_I(gap, 999u);  /* inchangé */
}

static void test_find_gap_present(void)
{
    glane_sample buf[5];
    glane_ring r;
    glane_ring_init(&r, buf, 5);
    /* seq 10,11,13,14 : trou entre 11 et 13 (position 1). */
    uint32_t seqs[5] = {10, 11, 13, 14, 15};
    for (int i = 0; i < 5; i++) {
        glane_sample s = mk(seqs[i]);
        glane_ring_push(&r, &s);
    }
    size_t gap = 999;
    int p = glane_ring_find_gap(&r, &gap);
    CHECK_EQ_I(p, 1);       /* gap entre index 1 (seq 11) et 2 (seq 13) */
    CHECK_EQ_I(gap, 1u);
}

static void test_find_gap_wrap(void)
{
    glane_sample buf[3];
    glane_ring r;
    glane_ring_init(&r, buf, 3);
    /* UINT32_MAX -> 0 est un wrap legitime, pas un trou. */
    uint32_t seqs[3] = {UINT32_MAX - 1, UINT32_MAX, 0};
    for (int i = 0; i < 3; i++) {
        glane_sample s = mk(seqs[i]);
        glane_ring_push(&r, &s);
    }
    size_t gap = 999;
    CHECK_EQ_I(glane_ring_find_gap(&r, &gap), -1);
}

static void test_single_and_empty(void)
{
    glane_sample buf[2];
    glane_ring r;
    glane_ring_init(&r, buf, 2);
    /* Anneau vide : find_gap rend -1 (count < 2). */
    CHECK_EQ_I(glane_ring_find_gap(&r, NULL), -1);
    glane_sample s = mk(5);
    glane_ring_push(&r, &s);
    /* Un seul élément : pas de comparaison possible. */
    CHECK_EQ_I(glane_ring_find_gap(&r, NULL), -1);
}

int main(void)
{
    test_init_and_push();
    test_fill_and_overwrite();
    test_find_gap_none();
    test_find_gap_present();
    test_find_gap_wrap();
    test_single_and_empty();
    TEST_DONE();
}