#ifndef __stdint_minimal_H
#define __stdint_minimal_H

/*************************************************************************
*
* This software module was originally contributed by Microsoft
* Corporation in the course of development of the
* ITU-T T.832 | ISO/IEC 29199-2 ("JPEG XR") format standard for
* reference purposes and its performance may not have been optimized.
*
* This software module is an implementation of one or more
* tools as specified by the JPEG XR standard.
*
* ITU/ISO/IEC give You a royalty-free, worldwide, non-exclusive
* copyright license to copy, distribute, and make derivative works
* of this software module or modifications thereof for use in
* products claiming conformance to the JPEG XR standard as
* specified by ITU-T T.832 | ISO/IEC 29199-2.
*
* ITU/ISO/IEC give users the same free license to this software
* module or modifications thereof for research purposes and further
* ITU/ISO/IEC standardization.
*
* Those intending to use this software module in products are advised
* that its use may infringe existing patents. ITU/ISO/IEC have no
* liability for use of this software module or modifications thereof.
*
* Copyright is not released for products that do not conform to
* to the JPEG XR standard as specified by ITU-T T.832 |
* ISO/IEC 29199-2.
*
* Microsoft Corporation retains full right to modify and use the code
* for its own purpose, to assign or donate the code to a third party,
* and to inhibit third parties from using the code for products that
* do not conform to the JPEG XR standard as specified by ITU-T T.832 |
* ISO/IEC 29199-2.
*
* This copyright notice must be included in all copies or derivative
* works.
*
* Copyright (c) ITU-T/ISO/IEC 2008, 2009.
**********************************************************************/

#ifdef _MSC_VER
#pragma comment (user,"$Id: stdint_minimal.h,v 1.2 2008/02/28 18:50:31 steve Exp $")
#else
#ident "$Id: stdint_minimal.h,v 1.2 2008/02/28 18:50:31 steve Exp $"
#endif

/*
* Microsoft VC compilers do not support C99, so we provide here a few
* of the basic typedefs that we need to get the code to
* compile. Since C99 stdint.h is not supported, the typedefs cannot
* be done portably, so we just do the most likely types.
*
* Note that this is specific to Microsoft compilers and not the
* Microsoft system. Mingw32, for example, does support C99 and this
* header file is not used in that case.
*/

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

#endif
