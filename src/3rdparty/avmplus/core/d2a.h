/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*- */
/* vi: set ts=4 sw=4 expandtab: (add to ~/.vimrc: set modeline modelines=5) */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef __avmplus_d2a__
#define __avmplus_d2a__
#include "BigInteger.h"

namespace avmplus
{
    // Double-to-string conversion utilities.
    //
    // See d2a.cpp for most documentation.

    class D2A
    {
    public:
        D2A(double value, bool fixedPrecision, int32_t minPrecision=0);

        int32_t nextDigit();
        int32_t expBase10() { return base10Exp; }

        double   value;             // Double value for quick work when e and mantissa are small.
        int32_t  e;
        uint64_t mantissa;          // On input, value = mantissa*2^e;  Only last 53 bits are used.
        int32_t  mantissaPrec;      // The number of bits of precision that are present in the mantissa.
        int32_t  base10Exp;         // The (derived) base 10 exponent of value.
        bool     finished;          // Set to true when we've output all relevant digits.
        bool     bFastEstimateOk;   // Use double, rather than BigInteger math.

    private:
        bool lowOk;             // For IEEE unbiased rounding, this is true when mantissa is even.  When true, use >= in mMinus test instead of >
        bool highOk;            // Ditto, but for mPlus test.

        // If !bFastEstimateOk, use these.
        BigInteger r;           // On initialization, input double <value> = r / s.  After each nextDigit() call, r = r % s.
        BigInteger s;
        BigInteger mPlus;       // When (r+mPlus) > s, we have generated all relevant digits.  Just return 0 for remaining nextDigit requests.
        BigInteger mMinus;      // When (r < mMins), we have generated all relevant digits.  Just return 0 form remaining nextDigit requests.

        // If bFastEstimateOk, use these - same as above, but integer value stored in double.
        double      dr;
        double      ds;
        double      dMPlus;
        double      dMMinus;

        // Estimate base 10 exponent of number, scale r,s,mPlus,mMinus appropriately.
        // Returns result of fixup_ExponentEstimate(est).
        int32_t scale();
        
        // Used by scale to adjust for possible off-by-one error in the base 10 exponent estimate.
        // Returns exact base10 exponent of number.
        int32_t fixup_ExponentEstimate(int32_t expEst);
    };
}

#endif /* __avmplus_d2a__ */
