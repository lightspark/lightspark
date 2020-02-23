/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*- */
/* vi: set ts=4 sw=4 expandtab: (add to ~/.vimrc: set modeline modelines=5) */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "BigInteger.h"
#include <cmath>

#define X86_MATH
namespace avmplus
{



void BigInteger::setFromDouble(double value)
{
    int e;

	double fracMantissa = ::frexp(value, &e);
	// correct mantissa and eptr to get integer values
	//  for both
	e -= 53; // 52 mantissa bits + the hidden bit
	uint64_t mantissa = (uint64_t)(fracMantissa * (double)(1LL << 53));

    numWords = (2 + ((e > 0 ? e : -e) /32));

    assert(numWords <= 128);
    wordBuffer[1] = (uint32_t)(mantissa >> 32);
    wordBuffer[0] = (uint32_t)(mantissa & 0xffffffff);
    numWords = (wordBuffer[1] == 0 ? 1 : 2);

    if(e < 0)
        rshiftBy(-e);
    else
        lshiftBy(e);
}

void BigInteger::setFromBigInteger(const BigInteger* from, int32_t offset, int32_t amount)
{
    numWords = amount;
    assert(numWords <= kMaxBigIntegerBufferSize);
    memcpy( (uint8_t*)wordBuffer,
            (uint8_t*)&(from->wordBuffer[offset]),
            amount*sizeof(uint32_t));
}

double BigInteger::doubleValueOf() const
{
    // todo:  there's got to be a quicker/smaller code alg for this.
    if (numWords == 1)
        return (double)wordBuffer[0];

    // determine how many of bits are used by the top word
    int bitsInTopWord=1;
    for(uint32_t topWord = wordBuffer[numWords-1]; topWord > 1; topWord >>= 1)
        bitsInTopWord++;

    // start result with top word.  We will accumulate the most significant 53 bits of data into it.
    int nextWord = numWords-1;

    // used for rounding:
    bool bit53 = false;
    bool bit54 = false;
    bool rest = false;

    const uint64_t ONEL = 1UL;
    uint64_t resultMantissa = 0;
    uint64_t w = 0;
    int pos = 53;
    int bits = bitsInTopWord;
    int wshift = 0;
    while(pos > 0)
    {
        // extract word and | it in
        w = wordBuffer[nextWord--];
        resultMantissa |= (w >> wshift);
        pos -= bits;

        if (pos > 0)
        {
            if (nextWord > -1)
            {
                // ready for next word
                bits =   (pos > 31) ? 32 : pos;
                wshift = (pos > 31) ? 0  : 32-bits;
                resultMantissa <<= bits;
            }
            else
            {
                break; // not enough data for full 53 bits
            }
        }
    }

    // rounding
    if (pos > 0)
    {
        ;  // insufficient data for rounding
    }
    else
    {
        bit53 = ( resultMantissa & 0x1 ) ? true : false;
        if (bits == 32)
        {
            // last extract was exactly 32 bits, so rest of bits are in next word if there is one
            if (nextWord > -1)
            {
                w = wordBuffer[nextWord--];
                bit54 = ( w & (ONEL<<31) ) ? true : false;
                rest = ( w & ((ONEL<<31)-1) ) ? true : false;
            }
        }
        else
        {
            // we know bit54 is in this word but the rest of the data may be in the next
            assert(bits < 32 && wshift > 0);
            bit54 = ( w & (ONEL<<(wshift-1)) ) ? true : false;

            if (wshift > 1)
                rest = ( w & ((ONEL<<(wshift-1))-1) ) ? true : false;

            // pick up any residual bits
            if (nextWord > -1)
                rest = rest || (wordBuffer[nextWord--] != 0);
        }
    }

    /**
     * ieee rounding is to nearest even value (bit54 is 2^-1)
     *   x...1 1..     -> round up since odd (but53 set)
     *   x...0 1...1.. -> round up since value > 1/2
     */
    if (bit54 && (bit53 || rest))
        resultMantissa += 1;

    double result=0;
    int32_t  expBase2 = lg2() + 1 - 53; // if negative, then we've already captured all the data in the mantissaResult and we can ignore.
    result = (double)resultMantissa;
    if (expBase2 > 0)
    {
        if (expBase2 < 64)
        {
            uint64_t uint64_1 = 1;
            result *= (double)(uint64_1 << expBase2);
        }
        else
        {
            result *= pow(2,expBase2);
        }
    }

    return result;
}

// Compare this vs other, return -1 if this < other, 0 if this == other, or 1 if this > other.
//  (todo: optimization for D2A would be to check if comparison was larger than an argument value
//  currently, we add the argument to this and compare the result with other).
int32_t BigInteger::compare(const BigInteger *other) const
{
    // if a is bigger than other, subtract has to be positive (assuming unsigned value)
    int result = 0;
    if (numWords > other->numWords)
        result = 1;
    else if (numWords < other->numWords)
        result = -1;
    else
    {
        // otherwise, they are they same number of uint32_t words.  need to check numWords by numWords
        for(int x = numWords-1; x > -1 ; x--)
        {
            if (wordBuffer[x] != other->wordBuffer[x])
            {
                result = (wordBuffer[x] < other->wordBuffer[x]) ? -1 : 1;
                break;
            }
        }
    }
    return result;
}

// Multiply by an integer factor and add an integer increment.  The addition is essentially free,
//  use this with a zero second argument for basic multiplication.
void BigInteger::multAndIncrementBy(int32_t factor, int32_t addition)
{
    uint64_t opResult;

    // perform mult op, moving carry forward.
    uint64_t carry = addition; // init cary with first addition
    int x;
    for(x=0; x < numWords; x++)
    {
        opResult = (uint64_t)wordBuffer[x] * (uint64_t)factor + carry;
        carry = opResult >> 32;
        wordBuffer[x] = (uint32_t)(opResult & 0xffffffff);
    }

    // if carry goes over the existing top, may need to expand wordBuffer
    if (carry)
    {
        setNumWords(numWords+1);
        wordBuffer[x] = (uint32_t)carry;
    }

}

// Multiply by another BigInteger.  If optional arg result is not null, reuse it for
//  result, else allocate a new result.
//  (note, despite the name "smallerNum", argument does not need to be smaller than this).
BigInteger* BigInteger::mult(const BigInteger* smallerNum, BigInteger* result) const
{
    // Need to know which is the bigger number in terms of number of words.
    const BigInteger *biggerNum = this;
    if (biggerNum->numWords < smallerNum->numWords)
    {
        const BigInteger *temp = biggerNum;
        biggerNum = smallerNum;
        smallerNum = temp;
    }

    // Make sure result is big enough, initialize with zeros
    int maxNewNumWords = biggerNum->numWords + smallerNum->numWords;
    result->setNumWords(maxNewNumWords); // we'll trim the excess at the end.

    // wipe entire buffer (initToZero flag in setNumWords only sets new elements to zero)
    for(int x = 0; x < maxNewNumWords; x++)
        result->wordBuffer[x] = 0;

    // do the math like gradeschool.  To optimize, use an FFT (http://numbers.computation.free.fr/Constants/Algorithms/fft.html)
    for(int x = 0; x < smallerNum->numWords; x++)
    {
        uint64_t factor = (uint64_t) smallerNum->wordBuffer[x];
        if (factor) // if 0, nothing to do.
        {
            uint32_t* pResult = result->wordBuffer+x; // each pass we rewrite elements of result offset by the pass iteration
            uint64_t  product;
            uint64_t  carry = 0;

            for(int y = 0; y < biggerNum->numWords; y++)
            {
                product = (biggerNum->wordBuffer[y] * factor) + *pResult + carry;
                carry = product >> 32;
                *pResult++ = (uint32_t)(product & 0xffffffff);
            }
            *pResult = (uint32_t)carry;
        }
    }

    // remove leading zeros
    result->trimLeadingZeros();

    return result;
}

// Divide this by divisor, put the remainder (i.e. this % divisor) into residual.  If optional third argument result
//  is not null, use it to store the result of the div, else allocate a new BigInteger for the result.
//   Note: this has been hard to get right.  If bugs do show up, use divideByReciprocalMethod instead
//   Note2: this is optimized for the types of numerator/denominators generated by D2A.  It will not work when
//    the result would be a value bigger than 9.  For general purpose BigInteger division, use divideByReciprocalMethod.
BigInteger* BigInteger::quickDivMod(const BigInteger* divisor, BigInteger* residual, BigInteger* result)
{
    // handle easy cases where divisor is >= this
    int compareTo = this->compare(divisor);
    if (compareTo == -1)
    {
        residual->copyFrom(this);
        result->setValue(0);
        return result;
    }
    else if (compareTo == 0)
    {
        residual->setValue(0);
        result->setValue(1);
        return result;
    }

    int dWords = divisor->numWords;
    /*  this section only necessary for true division instead of special case division needed by D2A.  We are
         assuming the result is a single digit value < 10 and > -1
    int next   = this->numWords - dWords;
    residual->copyFrom(this, next, dWords); // residual holds a divisor->numWords sized chunk of this.
    */
    residual->copyFrom(this,0,numWords);
    BigInteger decrement;
    decrement.setFromInteger(0);
    result->setNumWords(divisor->numWords, true);
    uint64_t factor;

    //do // need to loop over dword chunks of residual to make this handle division of any arbitrary bigIntegers
    {
        // guess largest factor that * divisor will fit into residual
        const uint64_t n = (uint64_t)(residual->wordBuffer[residual->numWords-1]);
        factor = n / divisor->wordBuffer[dWords-1];
        if ( ((factor <= 0) || (factor > 10))   // over estimate of 9 could be 10
             && residual->numWords > 1 && dWords > 1)
        {
            uint64_t bigR = ( ((uint64_t)residual->wordBuffer[residual->numWords-1]) << 32)
                             + (residual->wordBuffer[residual->numWords-2]);
            factor =  bigR / divisor->wordBuffer[dWords-1];
            if (factor > 9)
            {                 //  Note:  This only works because of the relative size of the two operands
                              //   which the D2A class produces.  todo: try generating a numerator out of the
                              //   the top 32 significant bits of residual (may have to get bits from two seperate words)
                              //   and a denominator out of the top 24 significant bits of divisor and use them for
                              //   the factor guess.  Same principal as above applied to 8 bit units.
                factor = 9;
                /*
                BigInteger::free(decrement);
                return divideByReciprocalMethod(divisor, residual, result);
                */
            }
        }
        if (factor)
        {
            decrement.copyFrom(divisor);
            decrement.multAndIncrementBy( (uint32_t)factor,0);
            // check for overestimate
            // fix bug 121952: must check for larger overestimate, which
            // can occur despite the checks above in some rare cases.
            // To see this case, use:
            //      this=4398046146304000200000000
            //      divisor=439804651110400000000000
            while (decrement.compare(residual) == 1 && factor > 0)
            {
                decrement.decrementBy(divisor);
                factor--;
            }

            // reduce dividend (aka residual) by factor*divisor, leave remainder in residual
            residual->decrementBy(&decrement);
        }

        // check for factor 0 underestimate
        int comparedTo = residual->compare(divisor);
        if (comparedTo == 1) // correct guess if its an off by one estimate
        {
            residual->decrementBy(divisor);
            factor++;
        }

        result->wordBuffer[0] = (uint32_t)factor;

    /* The above works for the division requirements of D2A, where divisor is always around 10 larger
        than the dividend and the result is always a digit 1-9.
        To make this routine work for general division, the following would need to be fleshed out /debugged.
        residual->trimLeadingZeros();

        // While we have more words to divide by and the residual has less words than the
        //  divisor,
        if (--next >= 0)
        {
            do
            {
                result->lshiftBy(32);  // shift current result over by a word
                residual->lshiftBy(32);// shift remainder over by a word
                residual->wordBuffer[0] = this->wordBuffer[next]; // drop next word of "this" into bottom word of remainder
            }
            while(residual->numWords < dWords);
        }

    } while(next >= 0);
    */
    }

    // trim zeros off top of residual
    result->trimLeadingZeros();
    return result;
}

/*  Was hoping dividing via a Newton's approximation of the reciprocal would be faster, but
     its not (makes D2A about twice as slow!).  Its not the 32bit divide above that's slow,
     its how many times you have to iterate over entire BigIntegers.  Below uses a shift,
     and three multiplies:
*/
BigInteger* BigInteger::divideByReciprocalMethod(const BigInteger* divisor, BigInteger* residual, BigInteger* result)
{
    // handle easy cases where divisor is >= this
    int compareTo = this->compare(divisor);
    if (compareTo == -1)
    {
        residual->copyFrom(this);
        result->setValue(0);
        return result;
    }
    else if (compareTo == 0)
    {
        residual->setValue(0);
        result->setValue(1);
        return result;
    }

    uint32_t d2Prec = divisor->lg2();
    uint32_t e = 1 + d2Prec;
    uint32_t ag = 1;
    uint32_t ar = 31 + this->lg2() - d2Prec;
    BigInteger u;
    u.setFromInteger(1);

    BigInteger ush;
    ush.setFromInteger(1);

    BigInteger usq;
    usq.setFromInteger(0);

    while (1)
    {
        u.lshift(e + 1,&ush);
        divisor->mult(&u,&usq); // usq = divisor*u^2
        usq.multBy(&u);
        ush.subtract(&usq, &u); // u = ush - usq;

        int32_t ush2 = u.lg2(); // ilg2(u);
        e *= 2;
        ag *= 2;
        int32_t usq2 = 4 + ag;
        ush2 -= usq2; // BigInteger* diff = ush->subtract(usq); // ush -= usq;  ush > 0
        if (ush2 > 0) // (ush->compare(usq) == 1)
        {
            u.rshiftBy(ush2); // u >>= ush;
            e -= ush2;
        }
        if (ag > ar)
            break;
    }
    result = this->mult(&u,result);         // mult by reciprocal (scaled by e)
    result->rshiftBy(e);                    // remove scaling

    BigInteger temp;
    temp.setFromInteger(0);
    divisor->mult(result, &temp);  // compute residual as this - divisor*result
    this->subtract(&temp,residual);  //  todo: should be a more optimal way of doing this

    // ... doesn't work, e is the wrong scale for this (too large)....
    // residual = this->lshift(e,residual); // residual = (this*2^e - result) / 2^e
    // residual->decrementBy(result);
    // residual->rshiftBy(e);                   // remove scaling
    return(result);
}

//  q = this->divBy(divisor)  is equilvalent to:  q = this->quickDivMod(divisor,remainderResult); this = remainderResult;
BigInteger* BigInteger::divBy(const BigInteger* divisor, BigInteger* divResult)
{
    BigInteger tempInt;
    tempInt.setFromInteger(0);

    quickDivMod(divisor, &tempInt, divResult);  // this has been hard to get right.  If bugs do show up,
    //divResult = divideByReciprocalMethod(divisor,tempInt,divResult); //  try this version instead  Its slower, but less likely to be buggy
    this->copyFrom(&tempInt);
    return divResult;
}

uint32_t BigInteger::lg2() const
{
    uint32_t powersOf2 = (numWords-1)*32;
    for(uint32_t topWord = wordBuffer[numWords-1]; topWord > 1; topWord >>= 1)
        powersOf2++;
    return powersOf2;
}


// Shift a BigInteger by <shiftBy> bits to left.  If
//  result is not NULL, write result into it, else allocate a new BigInteger.
BigInteger* BigInteger::lshift(uint32_t shiftBy, BigInteger* result) const
{
    // calculate how much larger the result will be
    int numNewWords = shiftBy >> 5; // i.e. div by 32
    int totalWords  = numNewWords + numWords + 1; // overallocate by 1, trim if necessary at end.
    result->setNumWords(totalWords,true);


    // make sure we don't create more words for 0
    if ( numWords == 1 && wordBuffer[0] == 0 )
    {
        result->setValue(0); // 0 << num is still 0
        return result;
    }
    const uint32_t* pSourceBuff = wordBuffer;
    uint32_t* pResultBuff = result->wordBuffer;
    for(int x = 0; x < numNewWords; x++)
            *pResultBuff++ = 0;
    // move bits from wordBuffer into result's wordBuffer shifted by (shiftBy % 32)
    shiftBy &= 0x1f;
    if (shiftBy) {
        uint32_t carry = 0;
        int    shiftCarry = 32 - shiftBy;
        for(int x=0; x < numWords; x++)
        {
            *pResultBuff++ = *pSourceBuff << shiftBy | carry;
            carry =  *pSourceBuff++ >> shiftCarry;
        }
        *pResultBuff = carry; // final carry may add a word
        if (*pResultBuff)
            ++totalWords;     // prevent trim of overallocated extra word
        }
    else for(int x=0; x < numWords; x++)  // shift was by a clean multiple of 32, just move words
    {
        *pResultBuff++ = *pSourceBuff++;
    }
    result->numWords = totalWords - 1; // trim over allocated extra word
    return result;
}

// Shift a BigInteger by <shiftBy> bits to right.  If
//  result is not NULL, write result into it, else allocate a new BigInteger.
//  (todo: might be possible to combine rshift and lshift into a single function
//  with argument specifying which direction to shift, but result might be messy/harder to get right)
BigInteger* BigInteger::rshift(uint32_t shiftBy, BigInteger* result) const
{
    int numRemovedWords = shiftBy >> 5; // i.e. div by 32

    // calculate how much larger the result will be
    int totalWords = numWords - numRemovedWords;
    result->setNumWords(totalWords,true);

    // check for underflow
    if (numRemovedWords > numWords)
    {
        result->setValue(0);
        return result;
    }

    // move bits from wordBuffer into result's wordBuffer shifted by (shiftBy % 32)
    uint32_t* pResultBuff = &(result->wordBuffer[totalWords-1]);
    const uint32_t* pSourceBuff = &(wordBuffer[numWords-1]);
    shiftBy &= 0x1f;
    if (shiftBy)
    {
        int shiftCarry = 32 - shiftBy;
        uint32_t carry = 0;

        for(int x=totalWords-1; x > -1; x--)
        {
            *pResultBuff-- = *pSourceBuff >> shiftBy | carry;
            carry =  *pSourceBuff-- << shiftCarry;
        }
    }
    else for(int x=totalWords-1; x > -1; x--)   // shift was by a clean multiple of 32, just move words
    {
        *pResultBuff-- = *pSourceBuff--;
    }
    result->numWords = totalWords;
    result->trimLeadingZeros(); // shrink numWords down to correct value
    return result;
}

//  Add or subtract two BigIntegers.  If subtract, we assume the argument is smaller than this since we
//   don't support negative numbers.  If optional third argument result is not null, reuse it for result
//   else allocate a new BigInteger for the result.
BigInteger *BigInteger::addOrSubtract(const BigInteger* smallerNum, bool isAdd, BigInteger* result) const
{
    // figure out which operand is the biggest in terms of number of words.
    int comparedTo = compare(smallerNum);
    const BigInteger* biggerNum = this;
    if (comparedTo < 0)
    {
        const BigInteger* temp = biggerNum;
        biggerNum = smallerNum;
        smallerNum = temp;
        assert(isAdd || comparedTo >= 0); // the d2a/a2d code should never subtract a larger from a smaller.
                                             //  all of the BigInteger code assumes positive numbers.  Proper
                                             //  handling would be to create a negative result here (and check for
                                             //  negative results everywhere).
    }

    result->setNumWords(biggerNum->numWords+1, true);

    // Handle a result of zero specially
    if (!comparedTo)
    {
        if (!isAdd || (numWords == 1 && wordBuffer[0] == 0))
        {
            result->setValue(0);
            return result;
        }
    }

    // do the math: loop over common words of both numbers, performing - or + and carrying the
    //  overflow/borrow forward to next word.
    uint64_t borrow = 0;
    uint64_t x;
    int index;
    for( index = 0; index < smallerNum->numWords; index++)
    {
        if (isAdd)
            x = ((uint64_t)biggerNum->wordBuffer[index]) + smallerNum->wordBuffer[index] + borrow;
        else
            x = ((uint64_t)biggerNum->wordBuffer[index]) - smallerNum->wordBuffer[index] - borrow; // note x is unsigned.  Ok even if result would be negative however
        borrow = x >> 32 & (uint32_t)1;
        result->wordBuffer[index] = (uint32_t)(x & 0xffffffff);
    }

    // loop over remaining words of the larger number, carying borrow/overflow forward
    for( ; index < biggerNum->numWords; index++)
    {
        if (isAdd)
            x = ((uint64_t)biggerNum->wordBuffer[index]) + borrow;
        else
            x = ((uint64_t)biggerNum->wordBuffer[index]) - borrow;
        borrow = x >> 32 & (uint32_t)1;
        result->wordBuffer[index] = (uint32_t)(x & 0xffffffff);
    }

    // handle final overflow if this is add
    if (isAdd && borrow)
    {
        result->wordBuffer[index] = (uint32_t)(borrow & 0xffffffff);
        index++;
    }
    // loop backwards over result, removing leading zeros from our numWords count
    while( !(result->wordBuffer[--index]) )
    {
    }
    assert(index >= 0);  // If this fires then there's something wrong with the zero check above
    result->numWords = index+1;

    return result;

}

}
