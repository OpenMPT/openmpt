/*
**  Fast Hartley transform, the part shared by its C and SSE2 forms
**  Copyright 1988, 1993; Ron Mayer
**      Copyright (c) 1999-2000 Takehiro Tominaga
**
**  The notice at the top of fft.c applies to this file.
*/

/**
 * \file
 * \internal
 * \brief The rotation table and the first butterflies of each pass of the fast
 *        Hartley transform, for fht() in fft.c and fht_SSE2() in
 *        vector/xmm_quantize_sub.c.
 *
 * Only the inner loop with the rotations has an SSE2 form. The rest of each
 * pass is the same in both, and is here.
 */

#ifndef LAME_FFT_PVT_H
#define LAME_FFT_PVT_H

#include "machine.h"
#include "util.h"

/** \internal \brief The number of passes after the first: 1024 = 4^5. */
#define TRI_SIZE (5-1)

/**
 * \internal
 * \brief The cosine and the sine of the rotation step of each pass after the
 *        first.
 */
static const FLOAT costab[TRI_SIZE * 2] = {
    9.238795325112867e-01, 3.826834323650898e-01,
    9.951847266721969e-01, 9.801714032956060e-02,
    9.996988186962042e-01, 2.454122852291229e-02,
    9.999811752826011e-01, 6.135884649154475e-03
};

/**
 * \internal
 * \brief The radix-4 butterflies at offset 0 and at offset kx of every block of
 *        one pass, the part of the pass that needs no rotation.
 * \param fz  the transform data.
 * \param fn  the end of the data.
 * \param k1  the distance of the second input of a butterfly.
 * \param k2  the distance of the third input.
 * \param k3  the distance of the fourth input.
 * \param k4  the block size of the pass.
 * \param kx  the offset of the second butterfly in a block.
 */
static inline void
fht_pass_head(FLOAT * fz, FLOAT const *fn, int k1, int k2, int k3, int k4, int kx)
{
    FLOAT  *fi = fz;
    FLOAT  *gi = fi + kx;

    do {
        FLOAT   f0, f1, f2, f3;
        f1 = fi[0] - fi[k1];
        f0 = fi[0] + fi[k1];
        f3 = fi[k2] - fi[k3];
        f2 = fi[k2] + fi[k3];
        fi[k2] = f0 - f2;
        fi[0] = f0 + f2;
        fi[k3] = f1 - f3;
        fi[k1] = f1 + f3;
        f1 = gi[0] - gi[k1];
        f0 = gi[0] + gi[k1];
        f3 = SQRT2 * gi[k3];
        f2 = SQRT2 * gi[k2];
        gi[k2] = f0 - f2;
        gi[0] = f0 + f2;
        gi[k3] = f1 - f3;
        gi[k1] = f1 + f3;
        gi += k4;
        fi += k4;
    } while (fi < fn);
}

#endif /* LAME_FFT_PVT_H */
