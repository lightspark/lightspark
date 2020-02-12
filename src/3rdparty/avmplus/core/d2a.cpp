/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*- */
/* vi: set ts=4 sw=4 expandtab: (add to ~/.vimrc: set modeline modelines=5) */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Double-to-string conversion
//
// The salient literature is:
//
// Guy L Steele Jr and Jon L White, "How to print floating-point numbers accurately",
// ACM PLDI 1990.
//
// Robert G Burger and R Kent Dybvig, "Printing floating-point numbers quickly and 
// accurately", ACM PLDI 1996.
//
// The implementation in this file follows the latter paper, which builds on the former.
#include "d2a.h"
#include <cstdint>
#include <cmath>

namespace avmplus
{
    // Support values and functions

    const double kLog2_10 = 0.30102999566398119521373889472449;     // log2(10)

    const int32_t maxBase2Precision = 53;                                                   // Max number of bits of precision needed to encode this mantissa
    const uint64_t two_pow_maxBase2PrecisionMinus1 = uint64_t(1) << (maxBase2Precision-1);  // 2^(maxBase2Precision-1)

    // optimize pow(10,x) for commonly sized numbers;
    static const double kPowersOfTen[23] =
    {
         1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,
        1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
        1e20, 1e21, 1e22
    };

    static inline double quickPowTen(int32_t exp)
    {
        if (0 <= exp && exp < 23)
            return kPowersOfTen[exp];
        else
            return pow(10,exp);
    }

    // For results larger than 10^22, error creeps into the double estimate.
    // Use BigIntegers to avoid error.
    static inline void quickBigPowTen(int32_t exp, BigInteger* result)
    {
        if (0 < exp && exp < 22) {
            result->setFromDouble(kPowersOfTen[exp]);
        } 
        else if (exp > 0) {
            // IEEE double has roundoff error after 1e22, need to compute exactly
            result->setFromDouble(kPowersOfTen[21]);
            exp -= 21;
            while (exp-- > 0)
                result->multBy((int32_t)10);
        }
        else {
            // We won't get here because we only deal in positive exponents
            result->setFromDouble(pow(10,exp)); // but just in case
        }
    }

    // Use left shifts to compute powers of 2 < 64
    static inline double quickPowTwo(int32_t exp)
    {
        if (0 < exp && exp < 64)
            return (double)(uint64_t(1) << exp);
        else
            return pow(2,exp);
    }

    D2A::D2A(double avalue, bool fixedPrecision, int32_t minPrecision)
        : value(avalue)
        , finished(false)
        , bFastEstimateOk(false)
    {
        // Break double down into integer mantissa (max size unsigned 53 bits)
        // and integer base 2 expondent (max size 11 signed bits):
        //
        //   value = mantissa*2^e, mantissa and e are integers.
		double fracMantissa = ::frexp(value, &e);
		// correct mantissa and eptr to get integer values
		//  for both
		e -= 53; // 52 mantissa bits + the hidden bit
		mantissa = (uint64_t)(fracMantissa * (double)(1LL << 53));

        if (fixedPrecision)
        {
            lowOk = highOk = true;
        }
        else
        {
            bool round = (mantissa & 1) == 0;   // If mantissa is even, need to round
            lowOk = highOk = round;             // IEEE standard unbiased rounding
        }

        // Compute mantissaPrec, number of bits required to represent the mantissa.
        // The loop subtracts the number of zeroes at the most significant end from 
        // the max precision.
        mantissaPrec = maxBase2Precision;
        while ( mantissaPrec != 0 && (((mantissa >> --mantissaPrec) & 1) == 0) )
            ;
        mantissaPrec++;

        // We get error prone when there is more than 15 base 10 digits of precision.
        // (15 / kLog2_10) == 49.828921423310435218054791442341.
        int32_t absE = (e > 0 ? e : -e);
        if (absE + mantissaPrec - 1 < 50)
            bFastEstimateOk = true;

        // Represent this double as a ratio of two integers.  Use infinitely precise integers
        // if bFastEstimateOk is false.  mPlus and mMinus represent the range of doubles around 
        // this value which would round to this same value when converted from string back to
        // number via atod().

        if (bFastEstimateOk)
        {
            if (e >= 0)
            {
                if (mantissa != two_pow_maxBase2PrecisionMinus1)
                {
                    double be = quickPowTwo(e); // 2^e
                    
                    dr = (double)mantissa*be*2;
                    ds = 2;
                    dMPlus = be;
                    dMMinus = be;
                }
                else
                {
                    double be = quickPowTwo(e);; // 2^e
                    double be1 = be*2; // 2^(e+1)
                    
                    dr = (double)mantissa*be1*2;
                    ds = 4; // 2*2;
                    dMPlus  = be1;
                    dMMinus = be;
                }
            }
            else if (mantissa != two_pow_maxBase2PrecisionMinus1)
            {
                dr = (double)mantissa*2.0;
                ds = quickPowTwo(1-e); // pow(2,-e)*2;
                dMPlus  = 1;
                dMMinus = 1;
            }
            else
            {
                dr = (double)mantissa*4.0;
                ds = quickPowTwo(2-e); // pow(2, 1-e)*2;
                dMPlus  = 2;
                dMMinus = 1;
            }
            
            // Adjust stopping conditions for fixed-precision formatting, Burger & Dybvig section 4.
            //
            // Compatibility note:
            // This is incorrect in several ways, for both 'absolute' digit positions
            // and 'relative' digit positions.

            if (fixedPrecision)
            {
                double fixedPrecisionPowTen = quickPowTen( minPrecision );
                ds *= fixedPrecisionPowTen;
                dr *= fixedPrecisionPowTen;
            }
        }
        else
        {
            if (e >= 0)
            {
                if (mantissa != two_pow_maxBase2PrecisionMinus1)
                {
                    BigInteger be;
                    be.setFromInteger(1);
                    be.lshiftBy(e); // == pow(2,e);
                    
                    r.setFromDouble(value);
                    r.lshiftBy(1); // r = mantissa*be*2
                    
                    s.setFromInteger(2);
                    
                    mPlus.setFromBigInteger(&be,0,be.numWords);  // == be;
                    mMinus.setFromBigInteger(&be,0,be.numWords); // == be;
                }
                else
                {
                    BigInteger be;
                    be.setFromInteger(1);
                    be.lshiftBy(e); // be = pow(2,e);
                    
                    BigInteger be1;
                    be1.setFromInteger(0);
                    be.lshift(1,&be1); // be1 == be*2;
                    
                    r.setFromDouble(value*4.0); // r == mantissa*be1*2;
                    s.setFromInteger(4); // 2*2
                    mPlus.setFromBigInteger(&be1,0,be1.numWords); // == be1;
                    mMinus.setFromBigInteger(&be,0,be.numWords); // == be;
                }
            }
            else if (mantissa != two_pow_maxBase2PrecisionMinus1)
            {
                r.setFromDouble( (double)(mantissa*2) );
                s.setFromInteger(2);
                s.lshiftBy(-e); // s = pow(2,-e)*2;
                mPlus.setFromInteger(1);
                mMinus.setFromInteger(1);
            }
            else
            {
                r.setFromDouble( (double)(mantissa*4) );
                s.setFromInteger(2);
                s.lshiftBy(1-e);  // s == pow(2, 1-e)*2;
                mPlus.setFromInteger(2);
                mMinus.setFromInteger(1);
            }
            
            // Adjust stopping conditions for fixed-precision formatting, Burger & Dybvig section 4.
            //
            //  Note: this is a cheap approximation of the correct adjustment.  It will likely
            //   lead to occasional off by one errors in the final digit generated for toPrecision() when
            //   a truncation might be required.  This mode is actually less accurate than normal mode anyway.
            //   Only normal mode is gauranteed to generate a string which will result in the same value when converted back
            //   from string (base 10) to double (base 2).  Extra digits generated by toPrecision will often be junk
            //   (but will display a value more similar to what you would see in a code debugger, where 0.012 can
            //    appear as 0.012999999999999999999 or such apparent nonsense)
            
            if (fixedPrecision)
            {
                BigInteger bFixedPrecisionPowTen;
                bFixedPrecisionPowTen.setFromInteger(0);
                quickBigPowTen( minPrecision, &bFixedPrecisionPowTen );
                s.multBy(&bFixedPrecisionPowTen);
                r.multBy(&bFixedPrecisionPowTen);
            }
        }
        base10Exp = scale();
    }

    // nextDigit() returns the next relevant digit in the number being converted, else -1 if
    // there are no more relevant digits.

    int32_t D2A::nextDigit()
    {
        if (finished)
            return -1;

        bool withinLowEndRoundRange;
        bool withinHighEndRoundRange;
        int32_t quotient;

        if ( bFastEstimateOk )
        {
            quotient = (int32_t)(dr / ds);
            double mod = fmod(dr,ds);
            dr = mod;

            // check if remaining ratio r/s is within the range of floats which would round to the value we have output
            //  so far when read in from a string.
            withinLowEndRoundRange  = (lowOk  ? (dr <= dMMinus)   : (dr < dMMinus));
            withinHighEndRoundRange = (highOk ? (dr+dMPlus >= ds) : (dr+dMPlus > ds));
        }
        else
        {
            BigInteger bigQuotient;
            bigQuotient.setFromInteger(0);
            r.divBy(&s, &bigQuotient); // r = r %s,  bigQuotient = r / s.
            quotient = (int32_t)(bigQuotient.wordBuffer[0]); // todo: optimize away need for BigInteger result?  We know it should always be a single digit
                                              // r <= mMinus               :  r < rMinus
            withinLowEndRoundRange  = (lowOk  ? (r.compare(&mMinus) != 1)  : (r.compare(&mMinus) == -1));
                                              // r+mPlus >= s                     :  r+mPlus > s
            withinHighEndRoundRange = (highOk ? (r.compareOffset(&s,&mPlus) != -1) : (r.compareOffset(&s,&mPlus) == 1));
        }

        if (quotient < 0 || quotient > 9)
        {
            assert(quotient >= 0);
            assert(quotient < 10);
            quotient = 0;
        }

        if (!withinLowEndRoundRange)
        {
            if (!withinHighEndRoundRange) // if not within either error range, set up to generate the next digit.
            {
                if ( bFastEstimateOk )
                {
                    dr *= 10;
                    dMPlus *= 10;
                    dMMinus *= 10;
                }
                else
                {
                    r.multBy((int32_t)10);
                    mPlus.multBy((int32_t)10);
                    mMinus.multBy((int32_t)10);
                }
            }
            else
            {
                quotient++;
                finished = true;
            }
        }
        else if (!withinHighEndRoundRange)
        {
            finished = true;
        }
        else
        {
            if ( (bFastEstimateOk && (dr*2 < ds)) ||
                 (!bFastEstimateOk && (r.compareOffset(&s,&r) == -1)) ) // if (r*2 < s)  todo: faster to do lshift and compare?
            {
                finished = true;
            }
            else
            {
                quotient++;
                finished = true;
            }
        }
        return quotient;
    }

    int32_t D2A::fixup_ExponentEstimate(int32_t exponentEstimate)
    {
        int32_t correctedEstimate;
        if (bFastEstimateOk)
        {
            if (highOk ? (dr+dMPlus) >= ds : dr+dMPlus > ds)
            {
                correctedEstimate = exponentEstimate+1;
            }
            else
            {
                dr *= 10;
                dMPlus *= 10;
                dMMinus *= 10;
                correctedEstimate = exponentEstimate;
            }
        }
        else
        {
            if (highOk ? (r.compareOffset(&s,&mPlus) != -1) : (r.compareOffset(&s,&mPlus) == 1))
            {
                correctedEstimate = exponentEstimate+1;
            }
            else
            {
                r.multBy((int32_t)10);
                mPlus.multBy((int32_t)10);
                mMinus.multBy((int32_t)10);
                correctedEstimate = exponentEstimate;
            }
        }
        return correctedEstimate;
    }

    int32_t D2A::scale()
    {
        // estimate base10 exponent:
        int32_t base2Exponent = e + mantissaPrec-1;
        int32_t exponentEstimate = (int32_t)ceil( (base2Exponent * kLog2_10) - 0.0000000001);

        if (bFastEstimateOk)
        {
            double scale = quickPowTen( (exponentEstimate > 0) ? exponentEstimate : -exponentEstimate);

            if (exponentEstimate >= 0)
            {
                ds *= scale;
                return fixup_ExponentEstimate(exponentEstimate);
            }
            else
            {
                dr *= scale;
                dMPlus *= scale;
                dMMinus *= scale;
                return fixup_ExponentEstimate(exponentEstimate);
            }
        }
        else
        {
            BigInteger scale;
            scale.setFromInteger(0);
            quickBigPowTen( (exponentEstimate > 0) ? exponentEstimate : -exponentEstimate, &scale);

            if (exponentEstimate >= 0)
            {
                s.multBy(&scale);
                return fixup_ExponentEstimate(exponentEstimate);
            }
            else
            {
                r.multBy(&scale);
                mPlus.multBy(&scale);
                mMinus.multBy(&scale);

                return fixup_ExponentEstimate(exponentEstimate);
            }
        }
    }
}
