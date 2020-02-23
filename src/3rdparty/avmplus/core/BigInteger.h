/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*- */
/* vi: set ts=4 sw=4 expandtab: (add to ~/.vimrc: set modeline modelines=5) */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef __avmplus_BigInteger__
#define __avmplus_BigInteger__

#include <cstdint>
#include <cassert>
#include <cstring>

//
//  BigInteger is an implementation of arbitrary precision integers.  This class is used by the
//   D2A class in mathutils.h to print doubles to greater than 15 digits of precision.  It has
//   been implemented with that use in mind, so if you want to use it for some other purpose
//   be aware that certain basic features have not been implemented: divide() assumes the result
//   is a single digit, there is no support for negative values, and subtract() assumes that the
//   term being subtracted from is larger than the operand.
//
//  By far, memory allocation is the gating speed factor.  D2A never uses more than 8 or so values
//   during a given run, so a simple memoryCache is used to keep 8 temporaries around for quick reuse.
//   This would likely have to be adjusted for more general use.
namespace avmplus
{
    class BigInteger
    {
        public:

            // Memory management
            // --------------------------------------------------------
            BigInteger()
            {
                numWords = 0;
            }

            ~BigInteger()
            {
            }
            inline void setFromInteger(int32_t initVal)
            {
                wordBuffer[0] = initVal;
                numWords = 1;
            }
            void setFromDouble(double value);
            void setFromBigInteger(const BigInteger* from,  int32_t offset,  int32_t amount);

            double doubleValueOf() const; // returns double approximation of this integer


            // operations
            // --------------------------------------------------------

            // Compare (sum = this+offset)  vs other.  if sum > other, return 1.  if sum < other,
            //  return -1, else return 0.  Note that all terms are assumed to be positive.
            int32_t compare(const BigInteger *other) const;

            // same as above, but compare this+offset with other.
            int32_t compareOffset(const BigInteger *other, const BigInteger *offset)
            {
                BigInteger tempInt;
                tempInt.setFromInteger(0);
                add(offset,&tempInt);
                return tempInt.compare(other);
            }


            // Add or subtract one BigInteger from another.  isAdd determines if + or - is performed.
            //  If result is specified, write result into it (allows for reuse of temporaries), else
            //  allocate a new BigInteger for the result.
            BigInteger* addOrSubtract(const BigInteger* other, bool isAdd, BigInteger* result) const;

            // syntactic sugar for simple case of adding two BigIntegers
            inline BigInteger* add(const BigInteger* other, BigInteger* result) const
            {
                return addOrSubtract(other, true, result);
            }
            // syntactic sugar for simple case of subtracting two BigIntegers
            inline BigInteger* subtract(const BigInteger* other, BigInteger* result) const
            {
                return addOrSubtract(other, false, result);
            }

            // Increment this by other.  other is assumed to be positive
            inline void incrementBy(const BigInteger* other)
            {
                BigInteger tempInt;
                tempInt.setFromInteger(0);
                addOrSubtract(other,true,&tempInt);
                copyFrom(&tempInt);
            }

            // Decrement this by other. other is assumed to be positive and smaller than this.
            inline void decrementBy(const BigInteger* other)
            {
                BigInteger tempInt;
                tempInt.setFromInteger(0);
                addOrSubtract(other,false,&tempInt);
                copyFrom(&tempInt);
            }

            // Shift a BigInteger by <shiftBy> bits to right or left.  If
            //  result is not NULL, write result into it, else allocate a new BigInteger.
            BigInteger* rshift(uint32_t shiftBy, BigInteger* result) const;
            BigInteger* lshift(uint32_t shiftBy, BigInteger* result) const;

            // Same as above but store result back into "this"
            void lshiftBy(uint32_t shiftBy)
            {
                BigInteger tempInt;
                tempInt.setFromInteger(0);
                lshift(shiftBy,&tempInt);
                copyFrom(&tempInt);
            }

            void rshiftBy(uint32_t shiftBy)
            {
                BigInteger tempInt;
                tempInt.setFromInteger(0);
                rshift(shiftBy,&tempInt);
                copyFrom(&tempInt);
            }

            // Multiply this by an integer factor and add an integer increment.  Store result back in this.
            //  Note, the addition is essentially free
            void multAndIncrementBy( int32_t factor,  int32_t addition);
            inline void multBy( int32_t factor)
            {
                multAndIncrementBy(factor,0);
            }

            // Shorthand for multiplying this by a double valued factor (and storing result back in this).
            inline bool multBy(double factor)
            {
                BigInteger bigFactor;
                bigFactor.setFromDouble(factor);
				// https://watsonexp.corp.adobe.com/#bug=3841671
				// The problem is with the fact that the buffer to hold the result of parseFloat - a double
				// precision floating point number - is stack allocated and hence if the user supplied
				// string which is the input argument to parseFloat(String) is large enough, there is a 
				// possibility that buffer might overflow if the range estimated for the output does not fit
				// in the stack allocated buffer's range. Investigations to make the buffer heap allocated 
				// and expand based on the size required for the output didn't bear fruit. So we are now 
				// returning early when we detect a potential overflow which eventually will have parseFloat
				// return 'NaN' (Not a Number).
				if (this->numWords + bigFactor.numWords > kMaxBigIntegerBufferSize + 2)
					return false;
                multBy(&bigFactor);
				return true;
            }

            // Multiply by another BigInteger.  If optional arg result is not null, reuse it for
            //  result, else allocate a new result.
            //  (note, despite the name "smallerNum", argument does not need to be smaller than this).
            BigInteger* mult(const BigInteger* other, BigInteger* result) const;
            void multBy(const BigInteger *other)
            {
                BigInteger tempInt;
                tempInt.setFromInteger(0);
                mult(other,&tempInt);
                copyFrom(&tempInt);
            }

            // divide this by divisor, put the remainder (i.e. this % divisor) into residual.  If optional third argument result
            //  is not null, use it to store the result of the div, else allocate a new BigInteger for the result.
            //   Note: this has been hard to get right.  If bugs do show up, use divideByReciprocalMethod instead
            //   Note2: this is optimized for the types of numerator/denominators generated by D2A.  It will not work when
            //    the result would be a value bigger than 9.  For general purpose BigInteger division, use divideByReciprocalMethod.
            BigInteger* quickDivMod(const BigInteger* divisor, BigInteger* modResult, BigInteger* divResult);

            /* Thought this would be faster than the above, but its not */
            BigInteger* divideByReciprocalMethod(const BigInteger* divisor, BigInteger* modResult, BigInteger* divResult);
            uint32_t lg2() const;



            //  q = this->divBy(divisor)  is equilvalent to:  q = this->divMod(divisor,remainderResult); this = remainderResult;
            BigInteger* divBy(const BigInteger* divisor, BigInteger* divResult);

            //  copy words from another BigInteger.  By default, copy all words into this.  If copyOffsetWords
            //   is not -1, then copy numCopyWords starting at word copyOffsetWords.
            void copyFrom(const BigInteger *other,  int32_t copyOffsetWords=-1,  int32_t numCopyWords=-1)
            {
                 int32_t numCopy = (numCopyWords == -1) ? other->numWords : numCopyWords;
                 int32_t copyOffset = (copyOffsetWords == -1) ? 0 : copyOffsetWords;

                copyBuffer(other->wordBuffer+copyOffset, numCopy);
            }

            //  Data members
            // --------------------------------------------------------
            static const int kMaxBigIntegerBufferSize=128;
            uint32_t wordBuffer[kMaxBigIntegerBufferSize+2];
            int32_t  numWords;

        private:
            //  sign;  Simplifications are made assuming all numbers are positive

            // Utility methods
            // --------------------------------------------------------

            // set value to a simple integer
            inline void setValue(uint32_t val)
            {
                this->numWords = 1;
                this->wordBuffer[0] = val;
            }


            // copy wordBuffer from another BigInteger
            inline void copyBuffer(const uint32_t *newBuff,  int32_t size)
            {
                numWords = size;
                assert(newBuff != wordBuffer);
                assert(numWords <= kMaxBigIntegerBufferSize);
                memcpy(wordBuffer, newBuff, numWords*sizeof(uint32_t));
            }

            inline void setNumWords( int32_t newNumWords,bool initToZero=false)
            {
                int32_t oldNumWords = numWords;
                numWords = newNumWords;
                assert(numWords <= kMaxBigIntegerBufferSize);
                if (initToZero && oldNumWords < numWords)
                {
                    for( int32_t x = oldNumWords-1; x < numWords; x++)
                        wordBuffer[x] = 0;
                }
            }

            // remove leading zero words from number.
            inline void trimLeadingZeros()
            {
                int32_t x;
                for( x = numWords-1; x >= 0 && wordBuffer[x] == 0; x--)
                    ;
                this->numWords = (x == -1) ? 1 : x+1; // make sure a zero value has numWords = 1
            }
    };

}
#endif // __avmplus_BigInteger__

