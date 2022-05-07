

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
***********************************************************************/

#ifdef _MSC_VER
#pragma comment (user,"$Id: io.c,v 1.14 2008/03/05 06:58:10 gus Exp $")
#else
#ident "$Id: io.c,v 1.14 2008/03/05 06:58:10 gus Exp $"
#endif

# include "jxr_priv.h"
# include <assert.h>

void _jxr_rbitstream_initialize(struct rbitstream*str
#ifndef JPEGXR_ADOBE_EXT
	, FILE*fd
#endif //#ifndef JPEGXR_ADOBE_EXT
)
{
    str->bits_avail = 0;
#ifndef JPEGXR_ADOBE_EXT
    str->fd = fd;
#endif //#ifndef JPEGXR_ADOBE_EXT
    str->read_count = 0;
}

size_t _jxr_rbitstream_bitpos(struct rbitstream*str)
{
    return str->read_count*8 - str->bits_avail;
}

void _jxr_rbitstream_mark(struct rbitstream*str)
{
    assert(str->bits_avail == 0);
#ifdef JPEGXR_ADOBE_EXT
    str->mark_stream_position = str->tell();
#else //#ifdef JPEGXR_ADOBE_EXT
    str->mark_stream_position = ftell(str->fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
    assert(str->mark_stream_position >= 0);
    str->read_count = 0;
}

void _jxr_rbitstream_seek(struct rbitstream*str, uint64_t off)
{
    assert(str->bits_avail == 0);
    /* NOTE: Should be using fseek64? */
#ifdef JPEGXR_ADOBE_EXT
	str->seek(str->mark_stream_position + (long)off, SEEK_SET);
#else //#ifdef JPEGXR_ADOBE_EXT
    int rc = fseek(str->fd, str->mark_stream_position + (long)off, SEEK_SET);
#endif //#ifdef JPEGXR_ADOBE_EXT
    str->read_count = (size_t) off;
#ifndef JPEGXR_ADOBE_EXT
    assert(rc >= 0);
#endif //#ifndef JPEGXR_ADOBE_EXT
}

/*
* Cause the bitstream to consume enough bits that the next bit is on
* a byte boundary. This is for getting alignment, which is sometimes
* needed.
*/
void _jxr_rbitstream_syncbyte(struct rbitstream*str)
{
    str->bits_avail = 0;
}

/*
* Get the next 8 bits from the input file and adjust the bitstream
* pointers so that the bits can be read out.
*/
static int get_byte(struct rbitstream*str)
{
    int tmp;
    assert(str->bits_avail == 0);
#ifdef JPEGXR_ADOBE_EXT
    tmp = str->getc();
#else //#ifdef JPEGXR_ADOBE_EXT
    tmp = fgetc(str->fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
    if (tmp == EOF)
        return EOF;

    str->byte = tmp;
    str->bits_avail = 8;
    str->read_count += 1;
#if 0
    DEBUG(" in byte: 0x%02x (bitpos=%zd)\n",
        str->byte, str->read_count*8-8);
#endif
    return 0;
}

/*
* The following read integers of various width from the data input
* stream. This handles the bit ordering and packing of the bits.
*/
int _jxr_rbitstream_uint1(struct rbitstream*str)
{
    if (str->bits_avail == 0) {
        get_byte(str);
    }

    assert(str->bits_avail > 0);

    str->bits_avail -= 1;
    return (str->byte & (1 << str->bits_avail))? 1 : 0;
}

uint8_t _jxr_rbitstream_uint2(struct rbitstream*str)
{
    uint8_t tmp = 0;

    tmp |= _jxr_rbitstream_uint1(str);
    tmp <<= 1;
    tmp |= _jxr_rbitstream_uint1(str);
    return tmp;
}

uint8_t _jxr_rbitstream_uint3(struct rbitstream*str)
{
    uint8_t tmp = 0;

    tmp |= _jxr_rbitstream_uint1(str);
    tmp <<= 1;
    tmp |= _jxr_rbitstream_uint1(str);
    tmp <<= 1;
    tmp |= _jxr_rbitstream_uint1(str);
    return tmp;
}

uint8_t _jxr_rbitstream_uint4(struct rbitstream*str)
{
    uint8_t tmp;
    int idx;

    if (str->bits_avail == 0)
        get_byte(str);

    if (str->bits_avail == 4) {
        str->bits_avail = 0;
        return str->byte & 0x0f;
    }

    tmp = 0;
    for (idx = 0 ; idx < 4 ; idx += 1) {
        tmp <<= 1;
        tmp |= _jxr_rbitstream_uint1(str);
    }

    return tmp;
}

uint8_t _jxr_rbitstream_uint6(struct rbitstream*str)
{
    uint8_t tmp;
    int idx;

    tmp = _jxr_rbitstream_uint4(str);
    for (idx = 4 ; idx < 6 ; idx += 1) {
        tmp <<= 1;
        tmp |= _jxr_rbitstream_uint1(str);
    }

    return tmp;
}

uint8_t _jxr_rbitstream_uint8(struct rbitstream*str)
{
    uint8_t tmp;
    int idx;

    if (str->bits_avail == 0)
        get_byte(str);

    if (str->bits_avail == 8) {
        str->bits_avail = 0;
        return str->byte;
    }

    tmp = 0;
    for (idx = 0 ; idx < 8 ; idx += 1) {
        tmp <<= 1;
        tmp |= _jxr_rbitstream_uint1(str);
    }

    return tmp;
}

uint16_t _jxr_rbitstream_uint12(struct rbitstream*str)
{
    uint16_t tmp = 0;

    tmp = _jxr_rbitstream_uint8(str);
    tmp <<= 4;
    tmp |= _jxr_rbitstream_uint4(str);
    return tmp;
}

uint16_t _jxr_rbitstream_uint15(struct rbitstream*str)
{
    uint16_t tmp = 0;

    tmp = _jxr_rbitstream_uint8(str);
    tmp <<= 4;
    tmp |= _jxr_rbitstream_uint4(str);
    tmp <<= 3;
    tmp |= _jxr_rbitstream_uint3(str);
    return tmp;
}

uint16_t _jxr_rbitstream_uint16(struct rbitstream*str)
{
    uint16_t tmp = 0;

    tmp = _jxr_rbitstream_uint8(str);
    tmp <<= 8;
    tmp |= _jxr_rbitstream_uint8(str);
    return tmp;
}

uint32_t _jxr_rbitstream_uint32(struct rbitstream*str)
{
    uint32_t tmp = 0;

    tmp = _jxr_rbitstream_uint16(str);
    tmp <<= 16;
    tmp |= _jxr_rbitstream_uint16(str);
    return tmp;
}

uint32_t _jxr_rbitstream_uintN(struct rbitstream*str, int N)
{
    uint32_t tmp = 0;
    assert(N <= 32);

    while (N > 0) {
        tmp <<= 1;
        tmp |= _jxr_rbitstream_uint1(str);
        N -= 1;
    }
    return tmp;
}

int _jxr_rbitstream_intE(struct rbitstream*str, int code_size,
                         const unsigned char*codeb, const signed char*codev)
{
    int bits = 0;
    unsigned val = 0;

    while (codeb[val << (code_size-bits)] != bits) {
        val <<= 1;
        val |= _jxr_rbitstream_uint1(str);
        bits += 1;
        assert(bits <= code_size);
    }

    return codev[val << (code_size-bits)];
}

int64_t _jxr_rbitstream_intVLW(struct rbitstream*str)
{
    uint64_t val = _jxr_rbitstream_uint8(str);
    if (val < 0xfb) {
        uint64_t tmp = _jxr_rbitstream_uint8(str);
        val = val*256 + tmp;

    } else if (val == 0xfb) {
        val = _jxr_rbitstream_uint32(str);

    } else if (val == 0xfc) {
        val = _jxr_rbitstream_uint32(str);
        uint64_t tmp = _jxr_rbitstream_uint32(str);
        val = (val << (uint64_t)32) + tmp;

    } else {
        return 0;
    }

    return val;
}

void _jxr_wbitstream_initialize(struct wbitstream*str
#ifndef JPEGXR_ADOBE_EXT
	, FILE*fd
#endif //#ifndef JPEGXR_ADOBE_EXT
)
{
    str->byte = 0;
    str->bits_ready = 0;
#ifndef JPEGXR_ADOBE_EXT
    str->fd = fd;
#endif //#ifndef JPEGXR_ADOBE_EXT
    str->write_count = 0;
}

size_t _jxr_wbitstream_bitpos(struct wbitstream*str)
{
    return str->write_count*8 + str->bits_ready;
}

static void put_byte(struct wbitstream*str)
{
    assert(str->bits_ready == 8);
#ifdef JPEGXR_ADOBE_EXT
    str->putbyte(str->byte);
#else //#ifdef JPEGXR_ADOBE_EXT
    fputc(str->byte, str->fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
    str->byte = 0;
    str->bits_ready = 0;
    str->write_count += 1;
}

void _jxr_wbitstream_syncbyte(struct wbitstream*str)
{
    if (str->bits_ready > 0)
        str->bits_ready = 8;
}

void _jxr_wbitstream_flush(struct wbitstream*str)
{
    _jxr_wbitstream_syncbyte(str);
    if (str->bits_ready > 0)
        put_byte(str);
}

void _jxr_wbitstream_uint1(struct wbitstream*str, int val)
{
    if (str->bits_ready == 8)
        put_byte(str);

    if (val)
        str->byte |= 0x80 >> str->bits_ready;
    str->bits_ready += 1;
}

void _jxr_wbitstream_uint2(struct wbitstream*str, uint8_t val)
{
    int idx;
    for (idx = 0 ; idx < 2 ; idx += 1) {
        _jxr_wbitstream_uint1(str, val & (0x02 >> idx));
    }
}

void _jxr_wbitstream_uint3(struct wbitstream*str, uint8_t val)
{
    int idx;
    for (idx = 0 ; idx < 3 ; idx += 1) {
        _jxr_wbitstream_uint1(str, val & (0x04 >> idx));
    }
}

void _jxr_wbitstream_uint4(struct wbitstream*str, uint8_t val)
{
    int idx;
    for (idx = 0 ; idx < 4 ; idx += 1) {
        _jxr_wbitstream_uint1(str, val & (0x08 >> idx));
    }
}

void _jxr_wbitstream_uint6(struct wbitstream*str, uint8_t val)
{
    int idx;
    for (idx = 0 ; idx < 6 ; idx += 1) {
        _jxr_wbitstream_uint1(str, val & (0x20 >> idx));
    }
}

void _jxr_wbitstream_uint8(struct wbitstream*str, uint8_t val)
{
    if (str->bits_ready == 8)
        put_byte(str);

    if (str->bits_ready == 0) {
        str->bits_ready = 8;
        str->byte = val;
        return;
    }

    int idx;
    for (idx = 0 ; idx < 8 ; idx += 1) {
        _jxr_wbitstream_uint1(str, val & (0x80 >> idx));
    }
}

void _jxr_wbitstream_uint12(struct wbitstream*str, uint16_t val)
{
    int idx;
    for (idx = 0 ; idx < 12 ; idx += 1) {
        _jxr_wbitstream_uint1(str, val & (0x800 >> idx));
    }
}

void _jxr_wbitstream_uint15(struct wbitstream*str, uint16_t val)
{
    int idx;
    for (idx = 0 ; idx < 15 ; idx += 1) {
        _jxr_wbitstream_uint1(str, val & (0x4000 >> idx));
    }
}

void _jxr_wbitstream_uint16(struct wbitstream*str, uint16_t val)
{
    _jxr_wbitstream_uint8(str, val >> 8);
    _jxr_wbitstream_uint8(str, val >> 0);
}

void _jxr_wbitstream_uint32(struct wbitstream*str, uint32_t val)
{
    _jxr_wbitstream_uint8(str, val >> 24);
    _jxr_wbitstream_uint8(str, val >> 16);
    _jxr_wbitstream_uint8(str, val >> 8);
    _jxr_wbitstream_uint8(str, val >> 0);
}

void _jxr_wbitstream_uintN(struct wbitstream*str, uint32_t val, int N)
{
    assert(N <= 32);
    while (N > 0) {
        _jxr_wbitstream_uint1(str, 1 & (val >> (N-1)));
        N -= 1;
    }
}

void _jxr_wbitstream_intVLW(struct wbitstream*str, uint64_t val)
{
    if (val == 0) {
        _jxr_wbitstream_uint8(str, 0xfe);
    } else if (val < 0xfb00) {
        _jxr_wbitstream_uint16(str, (uint16_t)val);
    } else if (val < 0x100000000ULL) {
        _jxr_wbitstream_uint8(str, 0xfb);
        _jxr_wbitstream_uint32(str, (uint32_t)val);
    } else {
        _jxr_wbitstream_uint8(str, 0xfc);
        _jxr_wbitstream_uint32(str, val >> 32ULL);
        _jxr_wbitstream_uint32(str, val >> 0ULL);
    }
}

void _jxr_wbitstream_mark(struct wbitstream*str)
{
    if (str->bits_ready == 8)
        _jxr_wbitstream_flush(str);

    assert(str->bits_ready == 0);
    /* str->mark_stream_position = ftell(str->fd); */
    str->write_count = 0;
}

const char* _jxr_vlc_index_name(int vlc)
{
    switch (vlc) {
        case AbsLevelIndDCLum: return "AbsLevelIndDCLum";
        case AbsLevelIndDCChr: return "AbsLevelIndDCChr";
        case DecFirstIndLPLum: return "DecFirstIndLPLum";
        case AbsLevelIndLP0: return "AbsLevelIndLP0";
        case AbsLevelIndLP1: return "AbsLevelIndLP1";
        case AbsLevelIndHP0: return "AbsLevelIndHP0";
        case AbsLevelIndHP1: return "AbsLevelIndHP1";
        case DecIndLPLum0: return "DecIndLPLum0";
        case DecIndLPLum1: return "DecIndLPLum1";
        case DecFirstIndLPChr: return "DecFirstIndLPChr";
        case DecIndLPChr0: return "DecIndLPChr0";
        case DecIndLPChr1: return "DecIndLPChr1";
        case DecNumCBP: return "DecNumCBP";
        case DecNumBlkCBP: return "DecNumBlkCBP";
        case DecIndHPLum0: return "DecIndHPLum0";
        case DecIndHPLum1: return "DecIndHPLum1";
        case DecFirstIndHPLum: return "DecFirstIndHPLum";
        case DecFirstIndHPChr: return "DecFirstIndHPChr";
        case DecIndHPChr0: return "DecIndHPChr0";
        case DecIndHPChr1: return "DecIndHPChr1";
        default: return "?????";
    }
}


/*
* $Log: io.c,v $
* Revision 1.16 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.15 2009/04/13 12:00:00 microsoft
* Reference Software v1.5 updates.
*
* Revision 1.14 2008/03/05 06:58:10 gus
* *** empty log message ***
*
* Revision 1.13 2008/02/28 18:50:31 steve
* Portability fixes.
*
* Revision 1.12 2008/02/26 23:52:44 steve
* Remove ident for MS compilers.
*
* Revision 1.11 2007/12/07 01:20:34 steve
* Fix adapt not adapting on line ends.
*
* Revision 1.10 2007/12/06 17:53:35 steve
* No longer need bitval dump.
*
* Revision 1.9 2007/11/30 01:50:58 steve
* Compression of DCONLY GRAY.
*
* Revision 1.8 2007/11/26 01:47:15 steve
* Add copyright notices per MS request.
*
* Revision 1.7 2007/11/19 18:22:34 steve
* Skip ESCaped FLEXBITS tiles.
*
* Revision 1.6 2007/11/14 23:56:17 steve
* Fix TILE ordering, using seeks, for FREQUENCY mode.
*
* Revision 1.5 2007/11/08 19:38:38 steve
* Get stub DCONLY compression to work.
*
* Revision 1.4 2007/11/08 02:52:32 steve
* Some progress in some encoding infrastructure.
*
* Revision 1.3 2007/08/15 01:54:11 steve
* Add level2 filter to decoder.
*
* Revision 1.2 2007/06/21 17:31:22 steve
* Successfully parse LP components.
*
* Revision 1.1 2007/06/06 17:19:12 steve
* Introduce to CVS.
*
*/

