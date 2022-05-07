

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
#pragma comment (user,"$Id: cr_parse.c,v 1.9 2008/03/21 21:23:14 steve Exp $")
#else
#ident "$Id: cr_parse.c,v 1.9 2008/03/21 21:23:14 steve Exp $"
#endif

# include "jxr_priv.h"
# include <string.h>
# include <stdlib.h>
# include <assert.h>

static int read_ifd(jxr_container_t container
#ifdef JPEGXR_ADOBE_EXT
	, rbitstream &rb
#else //#ifdef JPEGXR_ADOBE_EXT
	, FILE*fd
#endif //#ifdef JPEGXR_ADOBE_EXT
	, int image_number, uint32_t*ifd_next);

jxr_container_t jxr_create_container(void)
{
    jxr_container_t res = (jxr_container_t)
        jpegxr_calloc(1, sizeof (struct jxr_container));
    return res;
}

void jxr_destroy_container(jxr_container_t container)
{
    if(container == NULL)
        return;
#ifdef JPEGXR_ADOBE_EXT
#define JPEGXR_ADOBE_MAX_IMAGES 64
	if ( container->table ) {
		for ( int i=0; i<JPEGXR_ADOBE_MAX_IMAGES; i++) {
			if ( container->table[i] ) {
				jpegxr_free(container->table[i]);
				container->table[i] = 0;
			}
		}
	}
	jpegxr_free(container->table_cnt);
	container->table_cnt = 0;
	jpegxr_free(container->table);
	container->table = 0;
#endif //#ifdef JPEGXR_ADOBE_EXT
    jpegxr_free(container);
}

int jxr_read_image_container(jxr_container_t container
#ifdef JPEGXR_ADOBE_EXT
	, const uint8_t *data, int32_t len
#else //#ifdef JPEGXR_ADOBE_EXT
	, FILE*fd
#endif //#ifdef JPEGXR_ADOBE_EXT
)
{
    unsigned char buf[4];
    size_t rc;

#ifdef JPEGXR_ADOBE_EXT
	rbitstream rb(data, len);
	rc = rb.read(buf,4);
#else //#ifndef JPEGXR_ADOBE_EXT
    rc = fread(buf, 1, 4, fd);
#endif //#ifndef JPEGXR_ADOBE_EXT
    if (rc < 4)
        return JXR_EC_BADMAGIC;

    if (buf[0] != 0x49) return JXR_EC_BADMAGIC;
    if (buf[1] != 0x49) return JXR_EC_BADMAGIC;
    if (buf[2] != 0xbc) return JXR_EC_BADMAGIC;
    if (buf[3] != 0x01) return JXR_EC_BADMAGIC; /* Version. */

#ifdef JPEGXR_ADOBE_EXT
	rb.read(buf,4);
#else //#ifndef JPEGXR_ADOBE_EXT
    rc = fread(buf, 1, 4, fd);
#endif //#ifndef JPEGXR_ADOBE_EXT
    if (rc != 4) return JXR_EC_IO;

    uint32_t ifd_off = (buf[3] << 24) + (buf[2]<<16) + (buf[1]<<8) + (buf[0]<<0);
    container->image_count = 0;

#ifdef JPEGXR_ADOBE_EXT
	container->table_cnt = (unsigned*)jpegxr_calloc(1,JPEGXR_ADOBE_MAX_IMAGES*sizeof(unsigned));
	container->table = (struct ifd_table**)jpegxr_calloc(1,JPEGXR_ADOBE_MAX_IMAGES*sizeof(struct ifd_table*));
#endif //#ifdef JPEGXR_ADOBE_EXT

    while (ifd_off != 0) {
        container->image_count += 1;
        
#ifdef JPEGXR_ADOBE_EXT
		if ( container->image_count >= JPEGXR_ADOBE_MAX_IMAGES ) {
			return 0;
		}
#else //#ifdef JPEGXR_ADOBE_EXT
        container->table_cnt = (unsigned*)jpegxr_realloc(container->table_cnt,
            container->image_count * sizeof(unsigned));
        container->table = (struct ifd_table**) jpegxr_realloc(container->table,
            container->image_count * sizeof(struct ifd_table*));
#endif //#ifndef JPEGXR_ADOBE_EXT

        uint32_t ifd_next;
        if (ifd_off & 0x1) return JXR_EC_IO;
#ifdef JPEGXR_ADOBE_EXT
        rc = rb.seek(ifd_off, SEEK_SET);
        rc = read_ifd(container, rb, container->image_count-1, &ifd_next);
#else //#ifndef JPEGXR_ADOBE_EXT
        rc = fseek(fd, ifd_off, SEEK_SET);
        rc = read_ifd(container, fd, container->image_count-1, &ifd_next);
#endif //#ifndef JPEGXR_ADOBE_EXT

#ifndef JPEGXR_ADOBE_EXT
        if (rc < 0) return (int) rc;
#endif //#ifndef JPEGXR_ADOBE_EXT

        ifd_off = ifd_next;
    }

    return 0;
}

int jxrc_image_count(jxr_container_t container)
{
    return container->image_count;
}

int jxrc_document_name(jxr_container_t container, int image, char ** string)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0x010d)
            break;
    }
    
    if (idx >= ifd_cnt) return -1; 
    if (ifd[idx].tag != 0x010d) return -1;
    assert(ifd[idx].type == 2);

    assert(string[0] == 0);
    string[0] = (char *) jpegxr_malloc(ifd[idx].cnt * sizeof(char));
    assert(string[0] != 0);

    if (ifd[idx].cnt <= 4) {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.v_sbyte[i];
    }
    else {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.p_sbyte[i];
    }

    return 0;
}

int jxrc_image_description(jxr_container_t container, int image, char ** string)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0x010e)
            break;
    }
    
    if (idx >= ifd_cnt) return -1; 
    if (ifd[idx].tag != 0x010e) return -1;
    assert(ifd[idx].type == 2);

    assert(string[0] == 0);
    string[0] = (char *) jpegxr_malloc(ifd[idx].cnt * sizeof(char));
    assert(string[0] != 0);

    if (ifd[idx].cnt <= 4) {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.v_sbyte[i];
    }
    else {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.p_sbyte[i];
    }

    return 0;
}

int jxrc_equipment_make(jxr_container_t container, int image, char ** string)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0x010f)
            break;
    }
    
    if (idx >= ifd_cnt) return -1; 
    if (ifd[idx].tag != 0x010f) return -1;
    assert(ifd[idx].type == 2);

    assert(string[0] == 0);
    string[0] = (char *) jpegxr_malloc(ifd[idx].cnt * sizeof(char));
    assert(string[0] != 0);

    if (ifd[idx].cnt <= 4) {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.v_sbyte[i];
    }
    else {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.p_sbyte[i];
    }

    return 0;
}

int jxrc_equipment_model(jxr_container_t container, int image, char ** string)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0x0110)
            break;
    }
    
    if (idx >= ifd_cnt) return -1; 
    if (ifd[idx].tag != 0x0110) return -1;
    assert(ifd[idx].type == 2);

    assert(string[0] == 0);
    string[0] = (char *) jpegxr_malloc(ifd[idx].cnt * sizeof(char));
    assert(string[0] != 0);

    if (ifd[idx].cnt <= 4) {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.v_sbyte[i];
    }
    else {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.p_sbyte[i];
    }

    return 0;
}

int jxrc_page_name(jxr_container_t container, int image, char ** string)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0x011d)
            break;
    }
    
    if (idx >= ifd_cnt) return -1; 
    if (ifd[idx].tag != 0x011d) return -1;
    assert(ifd[idx].type == 2);

    assert(string[0] == 0);
    string[0] = (char *) jpegxr_malloc(ifd[idx].cnt * sizeof(char));
    assert(string[0] != 0);

    if (ifd[idx].cnt <= 4) {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.v_sbyte[i];
    }
    else {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.p_sbyte[i];
    }

    return 0;
}

int jxrc_page_number(jxr_container_t container, int image, unsigned short * value)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0x0129)
            break;
    }
    
    if (idx >= ifd_cnt) return -1; 
    if (ifd[idx].tag != 0x0129) return -1;
    assert(ifd[idx].cnt == 2);
    assert(ifd[idx].type == 3);

    value[0] = ifd[idx].value_.v_short[0];
    value[1] = ifd[idx].value_.v_short[1];

    return 0;
}

int jxrc_software_name_version(jxr_container_t container, int image, char ** string)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0x0131)
            break;
    }
    
    if (idx >= ifd_cnt) return -1; 
    if (ifd[idx].tag != 0x0131) return -1;
    assert(ifd[idx].type == 2);

    assert(string[0] == 0);
    string[0] = (char *) jpegxr_malloc(ifd[idx].cnt * sizeof(char));
    assert(string[0] != 0);

    if (ifd[idx].cnt <= 4) {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.v_sbyte[i];
    }
    else {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.p_sbyte[i];
    }

    return 0;
}

int jxrc_date_time(jxr_container_t container, int image, char ** string)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0x0132)
            break;
    }
    
    if (idx >= ifd_cnt) return -1; 
    if (ifd[idx].tag != 0x0132) return -1;
    assert(ifd[idx].type == 2);

    assert(string[0] == 0);
    string[0] = (char *) jpegxr_malloc(ifd[idx].cnt * sizeof(char));
    assert(string[0] != 0);
    assert(ifd[idx].cnt == 20);

    unsigned i;
    for (i = 0 ; i < ifd[idx].cnt ; i++)
        string[0][i] = ifd[idx].value_.p_sbyte[i];

    assert(string[0][4] == 0x3a && string[0][7] == 0x3a && string[0][13] == 0x3a && string[0][16] == 0x3a);
    assert(string[0][10] == 0x20);
    assert((string[0][5] == '0' && (string[0][6] >= '1' && string[0][6] <= '9')) || (string[0][5] == '1' && (string[0][6] >= '0' && string[0][6] <= '2')));
    assert((string[0][8] == '0' && (string[0][9] >= '1' && string[0][9] <= '9')) || ((string[0][8] == '1' || string[0][8] == '2') && (string[0][9] >= '0' && string[0][9] <= '9')) || (string[0][8] == '3' && (string[0][9] >= '0' && string[0][9] <= '1')));
    assert(((string[0][11] == '0' || string[0][11] == '1') && (string[0][12] >= '0' && string[0][12] <= '9')) || (string[0][11] == '2' && (string[0][12] >= '0' && string[0][12] <= '3')));
    assert(string[0][14] >= '0' && string[0][14] < '6');
    assert(string[0][15] >= '0' && string[0][15] <= '9');
    assert(string[0][17] >= '0' && string[0][17] < '6');
    assert(string[0][18] >= '0' && string[0][18] <= '9');

    return 0;
}

int jxrc_artist_name(jxr_container_t container, int image, char ** string)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0x013b)
            break;
    }
    
    if (idx >= ifd_cnt) return -1; 
    if (ifd[idx].tag != 0x013b) return -1;
    assert(ifd[idx].type == 2);

    assert(string[0] == 0);
    string[0] = (char *) jpegxr_malloc(ifd[idx].cnt * sizeof(char));
    assert(string[0] != 0);

    if (ifd[idx].cnt <= 4) {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.v_sbyte[i];
    }
    else {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.p_sbyte[i];
    }

    return 0;
}

int jxrc_host_computer(jxr_container_t container, int image, char ** string)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0x013c)
            break;
    }
    
    if (idx >= ifd_cnt) return -1; 
    if (ifd[idx].tag != 0x013c) return -1;
    assert(ifd[idx].type == 2);

    assert(string[0] == 0);
    string[0] = (char *) jpegxr_malloc(ifd[idx].cnt * sizeof(char));
    assert(string[0] != 0);

    if (ifd[idx].cnt <= 4) {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.v_sbyte[i];
    }
    else {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.p_sbyte[i];
    }

    return 0;
}

int jxrc_copyright_notice(jxr_container_t container, int image, char ** string)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0x8298)
            break;
    }
    
    if (idx >= ifd_cnt) return -1; 
    if (ifd[idx].tag != 0x8298) return -1;
    assert(ifd[idx].type == 2);

    assert(string[0] == 0);
    string[0] = (char *) jpegxr_malloc(ifd[idx].cnt * sizeof(char));
    assert(string[0] != 0);

    if (ifd[idx].cnt <= 4) {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.v_sbyte[i];
    }
    else {
        unsigned i;
        for (i = 0 ; i < ifd[idx].cnt ; i++)
            string[0][i] = ifd[idx].value_.p_sbyte[i];
    }

    return 0;
}

unsigned short jxrc_color_space(jxr_container_t container, int image)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xa001)
            break;
    }
    
    if (idx >= ifd_cnt) return 0; 
    if (ifd[idx].tag != 0xa001) return 0;
    assert(ifd[idx].cnt == 1);
    assert(ifd[idx].type == 3);

    unsigned short value = ifd[idx].value_.v_short[0];
    if (value != 0x0001)
        value = 0xffff;
    if (value == 0x0001) {
        switch(jxrc_image_pixelformat(container, image)) {
            case JXRC_FMT_24bppRGB:
            case JXRC_FMT_24bppBGR:
            case JXRC_FMT_32bppBGR:
            case JXRC_FMT_48bppRGB:
            case JXRC_FMT_32bppBGRA:
            case JXRC_FMT_64bppRGBA:
            case JXRC_FMT_32bppPBGRA:
            case JXRC_FMT_64bppPRGBA:
            case JXRC_FMT_32bppCMYK:
            case JXRC_FMT_40bppCMYKAlpha:
            case JXRC_FMT_64bppCMYK:
            case JXRC_FMT_80bppCMYKAlpha:
            case JXRC_FMT_24bpp3Channels:
            case JXRC_FMT_32bpp4Channels:
            case JXRC_FMT_40bpp5Channels:
            case JXRC_FMT_48bpp6Channels:
            case JXRC_FMT_56bpp7Channels:
            case JXRC_FMT_64bpp8Channels:
            case JXRC_FMT_32bpp3ChannelsAlpha:
            case JXRC_FMT_40bpp4ChannelsAlpha:
            case JXRC_FMT_48bpp5ChannelsAlpha:
            case JXRC_FMT_56bpp6ChannelsAlpha:
            case JXRC_FMT_64bpp7ChannelsAlpha:
            case JXRC_FMT_72bpp8ChannelsAlpha:
            case JXRC_FMT_48bpp3Channels:
            case JXRC_FMT_64bpp4Channels:
            case JXRC_FMT_80bpp5Channels:
            case JXRC_FMT_96bpp6Channels:
            case JXRC_FMT_112bpp7Channels:
            case JXRC_FMT_128bpp8Channels:
            case JXRC_FMT_64bpp3ChannelsAlpha:
            case JXRC_FMT_80bpp4ChannelsAlpha:
            case JXRC_FMT_96bpp5ChannelsAlpha:
            case JXRC_FMT_112bpp6ChannelsAlpha:
            case JXRC_FMT_128bpp7ChannelsAlpha:
            case JXRC_FMT_144bpp8ChannelsAlpha:
            case JXRC_FMT_8bppGray:
            case JXRC_FMT_16bppGray:
            case JXRC_FMT_BlackWhite:
            case JXRC_FMT_16bppBGR555:
            case JXRC_FMT_16bppBGR565:
            case JXRC_FMT_32bppBGR101010:
            case JXRC_FMT_32bppCMYKDIRECT:
            case JXRC_FMT_64bppCMYKDIRECT:
            case JXRC_FMT_40bppCMYKDIRECTAlpha:
            case JXRC_FMT_80bppCMYKDIRECTAlpha:
            case JXRC_FMT_12bppYCC420:
            case JXRC_FMT_16bppYCC422:
            case JXRC_FMT_20bppYCC422:
            case JXRC_FMT_32bppYCC422:
            case JXRC_FMT_24bppYCC444:
            case JXRC_FMT_30bppYCC444:
            case JXRC_FMT_48bppYCC444:
            case JXRC_FMT_20bppYCC420Alpha:
            case JXRC_FMT_24bppYCC422Alpha:
            case JXRC_FMT_30bppYCC422Alpha:
            case JXRC_FMT_48bppYCC422Alpha:
            case JXRC_FMT_32bppYCC444Alpha:
            case JXRC_FMT_40bppYCC444Alpha:
            case JXRC_FMT_64bppYCC444Alpha:
                break;
            default:
                /* Color space tag can equal 1 only if format is UINT  */
                assert(0);
                break;
        }
    }
    return value;
}

jxrc_t_pixelFormat jxrc_image_pixelformat(jxr_container_t container, int imagenum)
{
    unsigned char guid[16];
    int i;
    assert(imagenum < container->image_count);

    unsigned ifd_cnt = container->table_cnt[imagenum];
    struct ifd_table*ifd = container->table[imagenum];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xbc01)
            break;
    }

    assert(idx < ifd_cnt);
    assert(ifd[idx].tag == 0xbc01);
    assert(ifd[idx].cnt == 16);
    memcpy(guid, ifd[idx].value_.p_byte, 16);
    for(i=0; i< NUM_GUIDS; i++)
    {
        if(isEqualGUID(guid, jxr_guids[i]))
        {
            break;
        }
    }
    if(i==NUM_GUIDS)
        assert(0);
    return (jxrc_t_pixelFormat)i;
    
}

unsigned long jxrc_spatial_xfrm_primary(jxr_container_t container, int image)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xbc02)
            break;
    }

    if (idx >= ifd_cnt) return 0;
    if (ifd[idx].tag != 0xbc02) return 0;
    assert(ifd[idx].cnt == 1);

    unsigned long spatial_xfrm;
    switch (ifd[idx].type) {
        case 4: /* ULONG */
            spatial_xfrm = (unsigned long) ifd[idx].value_.v_long;
            break;
        case 3: /* USHORT */
            spatial_xfrm = (unsigned long) ifd[idx].value_.v_short[0];
            break;
        case 1: /* BYTE */
            spatial_xfrm = (unsigned long) ifd[idx].value_.v_byte[0];
            break;
        default:
			spatial_xfrm=0;
            assert(0);
            break;
    }

    if (spatial_xfrm > 7 
#ifndef JPEGXR_ADOBE_EXT
		|| spatial_xfrm < 0
#endif //#ifndef JPEGXR_ADOBE_EXT
		)
        spatial_xfrm = 0;
    return spatial_xfrm;
}

unsigned long jxrc_image_type(jxr_container_t container, int image)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xbc04)
            break;
    }
    
    if (idx >= ifd_cnt) return 0; 
    if (ifd[idx].tag != 0xbc04) return 0;
    assert(ifd[idx].cnt == 1);
    assert(ifd[idx].type == 4);

    unsigned long image_type = ifd[idx].value_.v_long;
    image_type &= 0x00000003;
    return image_type;
}

int jxrc_ptm_color_info(jxr_container_t container, int image, unsigned char * buf)
{
    assert(image < container->image_count);
    assert(buf);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xbc05)
            break;
    }
    
    if (idx >= ifd_cnt) return -1; 
    if (ifd[idx].tag != 0xbc05) return -1;
    assert(ifd[idx].cnt == 4);
    assert(ifd[idx].type == 1);

    uint32_t i;
    for (i = 0 ; i < 4 ; i += 1)
        buf[i] = ifd[idx].value_.v_byte[i];

    return 0;
}

int jxrc_profile_level_container(jxr_container_t container, int image, unsigned char * profile, unsigned char * level) {
    assert(image < container->image_count);
    assert(profile);
    assert(level);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned char * data = 0;

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xbc06)
            break;
    }
    
    if (idx >= ifd_cnt) return -1; 
    if (ifd[idx].tag != 0xbc06) return -1;
    assert(ifd[idx].type == 1);
    assert(ifd[idx].cnt > 3);

    if (ifd[idx].cnt <= 4) {
        data = (unsigned char *) (ifd[idx].value_.v_sbyte);
    }
    else {
        data = (unsigned char *) (ifd[idx].value_.p_sbyte);
    }

    uint32_t count_remaining = ifd[idx].cnt;
    uint8_t last, last_flag;
    for (last = 0 ; last == 0 ; last = last_flag) {
        profile[0] = data[0];
        level[0] = data[1];
        last_flag = (data[3] & 0x1);
        data += 4;
        count_remaining -= 4;
        assert(count_remaining == 0 || count_remaining > 3);
    }

    return 0;
}

unsigned long jxrc_image_width(jxr_container_t container, int image)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xbc80)
            break;
    }

    assert(idx < ifd_cnt);
    assert(ifd[idx].tag == 0xbc80);
    assert(ifd[idx].cnt == 1);

    unsigned long width;
    switch (ifd[idx].type) {
        case 4: /* ULONG */
            width = (unsigned long) ifd[idx].value_.v_long;
            break;
        case 3: /* USHORT */
            width = (unsigned long) ifd[idx].value_.v_short[0];
            break;
        case 1: /* BYTE */
            width = (unsigned long) ifd[idx].value_.v_byte[0];
            break;
        default:
			width=0;
            assert(0);
            break;
    }

    return width;
}

unsigned long jxrc_image_height(jxr_container_t container, int image)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xbc81)
            break;
    }

    assert(idx < ifd_cnt);
    assert(ifd[idx].tag == 0xbc81);
    assert(ifd[idx].cnt == 1);

    unsigned long height;
    switch (ifd[idx].type) {
        case 4: /* ULONG */
            height = (unsigned long) ifd[idx].value_.v_long;
            break;
        case 3: /* USHORT */
            height = (unsigned long) ifd[idx].value_.v_short[0];
            break;
        case 1: /* BYTE */
            height = (unsigned long) ifd[idx].value_.v_byte[0];
            break;
        default:
			height=0;
            assert(0);
            break;
    }

    return height;
}

float jxrc_width_resolution(jxr_container_t container, int image)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xbc82)
            break;
    }

    if (idx >= ifd_cnt) return 96.0;
    if (ifd[idx].tag != 0xbc82) return 96.0;
    assert(ifd[idx].cnt == 1);
    assert(ifd[idx].type == 11);

    float width_resolution = ifd[idx].value_.v_float;
    if (width_resolution == 0.0)
        width_resolution = 96.0;
    return width_resolution;
}

float jxrc_height_resolution(jxr_container_t container, int image)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xbc83)
            break;
    }

    if (idx >= ifd_cnt) return 96.0;
    if (ifd[idx].tag != 0xbc83) return 96.0;
    assert(ifd[idx].cnt == 1);
    assert(ifd[idx].type == 11);

    float height_resolution = ifd[idx].value_.v_float;
    if (height_resolution == 0.0)
        height_resolution = 96.0;
    return height_resolution;
}

unsigned long jxrc_image_offset(jxr_container_t container, int image)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xbcc0)
            break;
    }

    assert(idx < ifd_cnt);
    assert(ifd[idx].tag == 0xbcc0);
    assert(ifd[idx].cnt == 1);

    unsigned long pos;
    switch (ifd[idx].type) {
        case 4: /* ULONG */
            pos = (unsigned long) ifd[idx].value_.v_long;
            break;
        case 3: /* USHORT */
            pos = (unsigned long) ifd[idx].value_.v_short[0];
            break;
        case 1: /* BYTE */
            pos = (unsigned long) ifd[idx].value_.v_byte[0];
            break;
        default:
			pos=0;
            assert(0);
            break;
    }

    return pos;
}

unsigned long jxrc_image_bytecount(jxr_container_t container, int image)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xbcc1)
            break;
    }

    assert(idx < ifd_cnt);
    assert(ifd[idx].tag == 0xbcc1);
    assert(ifd[idx].cnt == 1);

    unsigned long pos;
    switch (ifd[idx].type) {
        case 4: /* ULONG */
            pos = (unsigned long) ifd[idx].value_.v_long;
            break;
        case 3: /* USHORT */
            pos = (unsigned long) ifd[idx].value_.v_short[0];
            break;
        case 1: /* BYTE */
            pos = (unsigned long) ifd[idx].value_.v_byte[0];
            break;
        default:
			pos=0;
            assert(0);
            break;
    }

    return pos;
}

unsigned long jxrc_alpha_offset(jxr_container_t container, int image)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xBCC2)
            break;
    }

    if (idx >= ifd_cnt) return 0; /* No alpha coded image */
    if (ifd[idx].tag != 0xbcc2) return 0;
    assert(ifd[idx].cnt == 1);

    unsigned long pos;
    switch (ifd[idx].type) {
        case 4: /* ULONG */
            pos = (unsigned long) ifd[idx].value_.v_long;
            break;
        case 3: /* USHORT */
            pos = (unsigned long) ifd[idx].value_.v_short[0];
            break;
        case 1: /* BYTE */
            pos = (unsigned long) ifd[idx].value_.v_byte[0];
            break;
        default:
			pos=0;
            assert(0);
            break;
    }

    return pos;
}

unsigned long jxrc_alpha_bytecount(jxr_container_t container, int image)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xbcc3)
            break;
    }

    
    if (idx >= ifd_cnt) return 0; /* No alpha coded image */
    if (ifd[idx].tag != 0xbcc3) return 0;
    assert(ifd[idx].cnt == 1);

    unsigned long pos;
    switch (ifd[idx].type) {
        case 4: /* ULONG */
            pos = (unsigned long) ifd[idx].value_.v_long;
            break;
        case 3: /* USHORT */
            pos = (unsigned long) ifd[idx].value_.v_short[0];
            break;
        case 1: /* BYTE */
            pos = (unsigned long) ifd[idx].value_.v_byte[0];
            break;
        default:
			pos=0;
            assert(0);
            break;
    }

    return pos;
}

unsigned char jxrc_image_band_presence(jxr_container_t container, int image)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xbcc4)
            break;
    }

    
    if (idx >= ifd_cnt) return (unsigned char)-1; 
    if (ifd[idx].tag != 0xbcc4) return (unsigned char)-1;
    assert(ifd[idx].cnt == 1);
    assert(ifd[idx].type == 1);

    return ifd[idx].value_.v_byte[0];
}

unsigned char jxrc_alpha_band_presence(jxr_container_t container, int image)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xbcc5)
            break;
    }

    
    if (idx >= ifd_cnt) return (unsigned char)-1; /* tag is not present */
    if (ifd[idx].tag != 0xbcc5) return (unsigned char)-1;
    assert(ifd[idx].cnt == 1);
    assert(ifd[idx].type == 1);

    return ifd[idx].value_.v_byte[0];
}

int jxrc_padding_data(jxr_container_t container, int image)
{
    assert(image < container->image_count);

    unsigned ifd_cnt = container->table_cnt[image];
    struct ifd_table*ifd = container->table[image];

    unsigned idx;
    for (idx = 0 ; idx < ifd_cnt ; idx += 1) {
        if (ifd[idx].tag == 0xea1c)
            break;
    }
    
    if (idx >= ifd_cnt) return -1; 
    if (ifd[idx].tag != 0xea1c) return -1;
    assert(ifd[idx].type == 7);
    assert(ifd[idx].cnt > 1);

#ifndef NDEBUG
    unsigned char * data;
    if (ifd[idx].cnt <= 4)
        data = (unsigned char *) ifd[idx].value_.v_sbyte;
    else
        data = (unsigned char *) ifd[idx].value_.p_sbyte;

    assert(data[0] == 0x1c);
    assert(data[1] == 0xea);
#endif
#ifndef JPEGXR_ADOBE_EXT
    unsigned i;
#endif //#ifndef JPEGXR_ADOBE_EXT

    return 0;
}

uint16_t bytes2_to_off(uint8_t*bp)
{
    return (bp[1]<<8) + (bp[0]);
}

uint32_t bytes4_to_off(uint8_t*bp)
{
    return (bp[3]<<24) + (bp[2]<<16) + (bp[1]<<8) + (bp[0]);
}

static int read_ifd(jxr_container_t container
#ifdef JPEGXR_ADOBE_EXT
	, rbitstream &rb
#else //#ifdef JPEGXR_ADOBE_EXT
	, FILE*fd
#endif //#ifdef JPEGXR_ADOBE_EXT
	, int image_number, uint32_t*ifd_next)
{
    *ifd_next = 0;

    unsigned char buf[16];
    size_t rc;
    int idx;
    int ifd_tag_prev = 0;
    int alpha_tag_check = 0;
    uint32_t ifd_off;

	(void)ifd_tag_prev;

#ifdef JPEGXR_ADOBE_EXT
	rc = rb.read(buf,2);
#else //#ifdef JPEGXR_ADOBE_EXT
    rc = fread(buf, 1, 2, fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
    if (rc != 2) return -1;

    uint16_t entry_count = (buf[1]<<8) + (buf[0]<<0);
    container->table_cnt[image_number] = entry_count;

    struct ifd_table*cur = (struct ifd_table*)jpegxr_calloc(entry_count, sizeof(struct ifd_table));
    assert(cur != 0);

    container->table[image_number] = cur;

    /* First read in the entire directory. Don't interpret the
    types yet, just save the values as v_bytes. We will go
    through the types later and interpret the values more
    precisely. */
    for (idx = 0 ; idx < entry_count ; idx += 1) {
#ifdef JPEGXR_ADOBE_EXT
		rc = rb.read(buf,12);
#else //#ifdef JPEGXR_ADOBE_EXT
        rc = fread(buf, 1, 12, fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
        assert(rc == 12);

        uint16_t ifd_tag = (buf[1]<<8) + (buf[0]<<0);
        if (ifd_tag == 0xbcc2)
            alpha_tag_check += 1;
        if (ifd_tag == 0xbcc3)
            alpha_tag_check += 2;
        if (ifd_tag == 0xbcc5)
            alpha_tag_check += 4;
        uint16_t ifd_type = (buf[3]<<8) + (buf[2]<<0);
        if (ifd_type == 7)
            assert (ifd_tag == 0x8773 || ifd_tag == 0xea1c);
        uint32_t ifd_cnt = (buf[7]<<24) + (buf[6]<<16) + (buf[5]<<8) + buf[4];

        assert(ifd_tag > ifd_tag_prev);
        ifd_tag_prev = ifd_tag;

        cur[idx].tag = ifd_tag;
        cur[idx].type = ifd_type;
        cur[idx].cnt = ifd_cnt;
        
        cur[idx].value_.v_byte[0] = buf[8];
        cur[idx].value_.v_byte[1] = buf[9];
        cur[idx].value_.v_byte[2] = buf[10];
        cur[idx].value_.v_byte[3] = buf[11];
    }
    /* verify alpha ifd tags appear only in allowed combinations */
    assert(alpha_tag_check == 0 || alpha_tag_check == 3 || alpha_tag_check == 7);

#ifdef JPEGXR_ADOBE_EXT
	rc = rb.read(buf,4);
#else //#ifdef JPEGXR_ADOBE_EXT
    rc = fread(buf, 1, 4, fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
    assert(rc == 4);

    /* Now interpret the tag types/values for easy access later. */
    for (idx = 0 ; idx < entry_count ; idx += 1) {
        switch (cur[idx].type) {

        case 1: /* BYTE */
        case 2: /* UTF8 */
        case 6: /* SBYTE */
        case 7: /* UNDEFINED */
            DEBUG("Container %d: tag 0x%04x BYTE:", image_number, cur[idx].tag);
            if (cur[idx].cnt > 4) {
                ifd_off = bytes4_to_off(cur[idx].value_.v_byte);
                assert((ifd_off & 1) == 0);
#ifdef JPEGXR_ADOBE_EXT
				rb.seek(ifd_off, SEEK_SET);
#else //#ifdef JPEGXR_ADOBE_EXT
                fseek(fd, ifd_off, SEEK_SET);
#endif //#ifdef JPEGXR_ADOBE_EXT
                cur[idx].value_.p_byte = (uint8_t*)jpegxr_malloc(cur[idx].cnt);
#ifdef JPEGXR_ADOBE_EXT
                rb.read(cur[idx].value_.p_byte, cur[idx].cnt);
#else //#ifdef JPEGXR_ADOBE_EXT
                fread(cur[idx].value_.p_byte, 1, cur[idx].cnt, fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
#if defined(DETAILED_DEBUG)
                int bb;
                for (bb = 0 ; bb < cur[idx].cnt ; bb += 1)
                    DEBUG("%02x", cur[idx].value_.p_byte[bb]);
#endif
                if (cur[idx].type == 2) {
                    uint32_t cc;
                    for (cc = 1 ; cc < cur[idx].cnt ; cc += 1)
                        assert((cur[idx].value_.p_byte[cc - 1] != 0) || (cur[idx].value_.p_byte[cc] != 0));
                }
            }
            else {
                if (cur[idx].type == 2) {
                    uint32_t cc;
                    for (cc = 1 ; cc < cur[idx].cnt ; cc += 1)
                        assert((cur[idx].value_.v_byte[cc - 1] != 0) || (cur[idx].value_.v_byte[cc] != 0));
                }
            }
            /* No action required to access individual bytes */
            DEBUG("\n");
            break;

        case 3: /* USHORT */
        case 8: /* SSHORT */
            if (cur[idx].cnt <= 2) {
                cur[idx].value_.v_short[0] = bytes2_to_off(cur[idx].value_.v_byte + 0);
                cur[idx].value_.v_short[1] = bytes2_to_off(cur[idx].value_.v_byte + 2);
                DEBUG("Container %d: tag 0x%04x SHORT %u SHORT %u\n", image_number,
                    cur[idx].tag, cur[idx].value_.v_short[0], cur[idx].value_.v_short[1]);
            } else {
                ifd_off = bytes4_to_off(cur[idx].value_.v_byte);
                assert((ifd_off & 1) == 0);
#ifdef JPEGXR_ADOBE_EXT
				rb.seek(ifd_off, SEEK_SET);
#else //#ifdef JPEGXR_ADOBE_EXT
                fseek(fd, ifd_off, SEEK_SET);
#endif //#ifdef JPEGXR_ADOBE_EXT
                cur[idx].value_.p_short = (uint16_t*) jpegxr_calloc(cur[idx].cnt, sizeof(uint16_t));

                DEBUG("Container %d: tag 0x%04x SHORT\n", image_number,
                    cur[idx].tag);
                uint16_t cdx;
                for (cdx = 0 ; cdx < cur[idx].cnt ; cdx += 1) {
                    uint8_t buf[2];
#ifdef JPEGXR_ADOBE_EXT
					rb.read(buf,2);
#else //#ifdef JPEGXR_ADOBE_EXT
                    fread(buf, 1, 2, fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
                    cur[idx].value_.p_short[cdx] = bytes2_to_off(buf);
                    DEBUG(" %u", cur[idx].value_.p_short[cdx]);
                }
                DEBUG("\n");
            }
            break;

        case 4: /* ULONG */
        case 9: /* SLONG */
        case 11: /* FLOAT */
            if (cur[idx].cnt == 1) {
                cur[idx].value_.v_long = bytes4_to_off(cur[idx].value_.v_byte);
                DEBUG("Container %d: tag 0x%04x LONG %u\n", image_number,
                    cur[idx].tag, cur[idx].value_.v_long);
            } else {
                ifd_off = bytes4_to_off(cur[idx].value_.v_byte);
                assert((ifd_off & 1) == 0);
#ifdef JPEGXR_ADOBE_EXT
				rb.seek(ifd_off, SEEK_SET);
#else //#ifdef JPEGXR_ADOBE_EXT
                fseek(fd, ifd_off, SEEK_SET);
#endif //#ifdef JPEGXR_ADOBE_EXT
                cur[idx].value_.p_long = (uint32_t*) jpegxr_calloc(cur[idx].cnt, sizeof(uint32_t));

                DEBUG("Container %d: tag 0x%04x LONG\n", image_number,
                    cur[idx].tag);
                uint32_t cdx;
                for (cdx = 0 ; cdx < cur[idx].cnt ; cdx += 1) {
                    uint8_t buf[4];
#ifdef JPEGXR_ADOBE_EXT
					rb.read(buf,4);
#else //#ifdef JPEGXR_ADOBE_EXT
                    fread(buf, 1, 4, fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
                    cur[idx].value_.p_long[cdx] = bytes4_to_off(buf);
                    DEBUG(" %u", cur[idx].value_.p_long[cdx]);
                }
                DEBUG("\n");
            }
            break;

        case 5: /* URATIONAL */
        case 10: /* SRATIONAL */
        case 12: /* DOUBLE */
            /* Always offset */
            ifd_off = bytes4_to_off(cur[idx].value_.v_byte);
            assert((ifd_off & 1) == 0);
#ifdef JPEGXR_ADOBE_EXT
			rb.seek(ifd_off, SEEK_SET);
#else //#ifdef JPEGXR_ADOBE_EXT
            fseek(fd, ifd_off, SEEK_SET);
#endif //#ifdef JPEGXR_ADOBE_EXT
            cur[idx].value_.p_rational = (uint64_t*) jpegxr_calloc(cur[idx].cnt, sizeof(uint64_t));

            DEBUG("Container %d: tag 0x%04x LONG\n", image_number,
                cur[idx].tag);
            uint64_t cdx;
            for (cdx = 0 ; cdx < cur[idx].cnt ; cdx += 1) {
                uint8_t buf[4];
#ifdef JPEGXR_ADOBE_EXT
				rb.read(buf,4);
#else //#ifdef JPEGXR_ADOBE_EXT
                fread(buf, 1, 4, fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
                cur[idx].value_.p_long[2 * cdx + 0] = bytes4_to_off(buf);
#ifdef JPEGXR_ADOBE_EXT
				rb.read(buf,4);
#else //#ifdef JPEGXR_ADOBE_EXT
                fread(buf, 1, 4, fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
                cur[idx].value_.p_long[2 * cdx + 1] = bytes4_to_off(buf);
                DEBUG(" %u", cur[idx].value_.p_rational[cdx]);
            }
            DEBUG("\n");
            break;

        default:
            DEBUG("Container %d: tag 0x%04x type=%u\n", image_number, cur[idx].tag, cur[idx].type);
            break;
        }
    }

    /* Tell the caller about the next ifd. */
    *ifd_next = (buf[3] << 24) + (buf[2]<<16) + (buf[1]<<8) + (buf[0]<<0);

    return 0;
}


/*
* $Log: cr_parse.c,v $
* Revision 1.11 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.10 2009/04/13 12:00:00 microsoft
* Reference Software v1.5 updates.
*
* Revision 1.9 2008/03/21 21:23:14 steve
* Some debug dump of the container.
*
* Revision 1.8 2008/03/05 19:32:02 gus
* *** empty log message ***
*
* Revision 1.7 2008/03/01 02:46:08 steve
* Add support for JXR container.
*
* Revision 1.6 2008/02/28 18:50:31 steve
* Portability fixes.
*
* Revision 1.5 2008/02/26 23:52:44 steve
* Remove ident for MS compilers.
*
* Revision 1.4 2007/11/26 01:47:15 steve
* Add copyright notices per MS request.
*
* Revision 1.3 2007/11/07 18:11:35 steve
* Accept magic number version 0
*
* Revision 1.2 2007/07/30 23:09:57 steve
* Interleave FLEXBITS within HP block.
*
* Revision 1.1 2007/06/06 17:19:12 steve
* Introduce to CVS.
*
*/

