
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
#pragma comment (user,"$Id: cw_emit.c,v 1.2 2008/03/02 18:35:27 steve Exp $")
#else
#ident "$Id: cw_emit.c,v 1.2 2008/03/02 18:35:27 steve Exp $"
#endif

# include  "jxr_priv.h"
# include  <stdlib.h>
# include  <string.h>
# include  <assert.h>

int jxrc_start_file(jxr_container_t cp
#ifndef JPEGXR_ADOBE_EXT
	, FILE*fd
#endif //#ifndef JPEGXR_ADOBE_EXT
)
{
#ifndef JPEGXR_ADOBE_EXT
      assert(cp->fd == 0);
#endif //#ifndef JPEGXR_ADOBE_EXT

      /* initializations */
      cp->separate_alpha_image_plane = 0;
      cp->image_count_mark = 0;
      cp->alpha_count_mark = 0;
      cp->alpha_offset_mark = 0;
      cp->alpha_band = 0;

#ifdef JPEGXR_ADOBE_EXT
      cp->file_mark = cp->wb.tell();
#else //#ifdef JPEGXR_ADOBE_EXT
      cp->fd = fd;
      cp->file_mark = ftell(cp->fd);
#endif // #ifdef JPEGXR_ADOBE_EXT

      const unsigned char head_bytes[8] = {0x49, 0x49, 0xbc, 0x01,
					   0x08, 0x00, 0x00, 0x00 };

#ifdef JPEGXR_ADOBE_EXT
	  cp->wb.write(head_bytes, 8);
#else //#ifdef JPEGXR_ADOBE_EXT
      fwrite(head_bytes, 1, 8, cp->fd);
#endif //#ifdef JPEGXR_ADOBE_EXT

      return 0;
}

int jxrc_begin_ifd_entry(jxr_container_t cp)
{
      assert(cp->image_count == 0);
      cp->image_count = 1;
      return 0;
}


int jxrc_set_pixel_format(jxr_container_t cp, jxrc_t_pixelFormat fmt)
{
      memcpy(cp->pixel_format, jxr_guids[fmt], 16);

      return 0;
}

jxrc_t_pixelFormat jxrc_get_pixel_format(jxr_container_t cp)
{
#ifndef JPEGXR_ADOBE_EXT
    unsigned char guid[16];
#endif //#ifndef JPEGXR_ADOBE_EXT
    int i;
   
    for(i=0; i< NUM_GUIDS; i++)
    {
        if(isEqualGUID(cp->pixel_format, jxr_guids[i]))
        {
            break;
        }
    }
    if(i==NUM_GUIDS)
        assert(0);
    return (jxrc_t_pixelFormat)i;

}


int jxrc_set_image_shape(jxr_container_t cp, unsigned wid, unsigned hei)
{
      cp->wid = wid;
      cp->hei = hei;
      return 0;
}

int jxrc_set_image_band_presence(jxr_container_t cp, unsigned bands)
{
    cp->image_band = (uint8_t) bands;
    return 0;
}

/*
This will induce the encoder to write as many of the 
IFD tags of table A.4 (the listing of IFD tags) as it can 
Off by default.
#define WRITE_OPTIONAL_IFD_TAGS
*/

int jxrc_set_separate_alpha_image_plane(jxr_container_t cp, unsigned int alpha_present)
{
      cp->separate_alpha_image_plane = alpha_present;      
      return 0;
}

static void emit_ifd(jxr_container_t cp)
{
    unsigned long ifd_mark;
    int idx;

    assert(cp->image_count > 0);
    if (cp->image_count > 1){
        assert(0);
        ifd_mark = 0;
    } else {
        ifd_mark = 8;
    }

    struct ifd_table ifd[64];
    unsigned char buf[1024];

    int num_ifd = 0;
    int num_buf = 0;

#ifdef WRITE_OPTIONAL_IFD_TAGS
    int ifd_cnt;
    char * input_string;

    /* various sample strings used to test storage */
    char document_name[] = {"JPEG XR image created by JPEG XR sample encoder."};
    char image_description[] = {"The core codestream begins at WMPHOTO"};
    char equipment_make[] = {"???"};
    char equipment_model[] = {"JXR 0X1"};
    char page_name[]={"Test page name"};
    unsigned short page_number[] = {1,2};
    char software_name_version[]={"JPEG XR reference software v1.6"};
    char date_time[]={"2009:04:01 12:34:56"};
    char artist_name[]={"JPEG Committee"};
    char host_computer[]={"JXR"};
    char copyright_notice[]={"©JPEG Committee"};
    unsigned long spatial_xfrm = 0;
    unsigned char profile_idc = 111;
    unsigned char level_idc = 255;
    float width_res = 96.0;
    float height_res = 96.0;

    input_string = document_name;
    ifd_cnt = strlen(input_string) + 1;
    ifd[num_ifd].tag = 0x010d; /* DOCUMENT_NAME */
    ifd[num_ifd].type = 2; /* UTF8 */
    ifd[num_ifd].cnt = ifd_cnt;
    if (ifd_cnt > 4)
        ifd[num_ifd].value_.v_long = ifd_mark + num_buf;
    else
        memcpy(ifd[num_ifd].value_.v_sbyte, input_string, ifd_cnt);

    for (idx = 0 ; idx < ifd_cnt ; idx += 1)
        buf[num_buf++] = input_string[idx];
    if (num_buf & 0x1)
        buf[num_buf++] = (unsigned char) 0;
    num_ifd += 1;

    input_string = image_description;
    ifd_cnt = strlen(input_string) + 1;
    ifd[num_ifd].tag = 0x010e; /* IMAGE_DESCRIPTION */
    ifd[num_ifd].type = 2; /* UTF8 */
    ifd[num_ifd].cnt = ifd_cnt;
    if (ifd_cnt > 4)
        ifd[num_ifd].value_.v_long = ifd_mark + num_buf;
    else
        memcpy(ifd[num_ifd].value_.v_sbyte, input_string, ifd_cnt);

    for (idx = 0 ; idx < ifd_cnt ; idx += 1)
        buf[num_buf++] = input_string[idx];
    if (num_buf & 0x1)
        buf[num_buf++] = (unsigned char) 0;
    num_ifd += 1;

    input_string = equipment_make;
    ifd_cnt = strlen(input_string) + 1;
    ifd[num_ifd].tag = 0x010f; /* EQUIPMENT_MAKE */
    ifd[num_ifd].type = 2; /* UTF8 */
    ifd[num_ifd].cnt = ifd_cnt;
    if (ifd_cnt > 4)
        ifd[num_ifd].value_.v_long = ifd_mark + num_buf;
    else
        memcpy(ifd[num_ifd].value_.v_sbyte, input_string, ifd_cnt);

    for (idx = 0 ; idx < ifd_cnt ; idx += 1)
        buf[num_buf++] = input_string[idx];
    if (num_buf & 0x1)
        buf[num_buf++] = (unsigned char) 0;
    num_ifd += 1;

    input_string = equipment_model;
    ifd_cnt = strlen(input_string) + 1;
    ifd[num_ifd].tag = 0x0110; /* EQUIPMENT_MODEL */
    ifd[num_ifd].type = 2; /* UTF8 */
    ifd[num_ifd].cnt = ifd_cnt;
    if (ifd_cnt > 4)
        ifd[num_ifd].value_.v_long = ifd_mark + num_buf;
    else
        memcpy(ifd[num_ifd].value_.v_sbyte, input_string, ifd_cnt);

    for (idx = 0 ; idx < ifd_cnt ; idx += 1)
        buf[num_buf++] = input_string[idx];
    if (num_buf & 0x1)
        buf[num_buf++] = (unsigned char) 0;
    num_ifd += 1;

    input_string = page_name;
    ifd_cnt = strlen(input_string) + 1;
    ifd[num_ifd].tag = 0x011d; /* PAGE_NAME */
    ifd[num_ifd].type = 2; /* UTF8 */
    ifd[num_ifd].cnt = ifd_cnt;
    if (ifd_cnt > 4)
        ifd[num_ifd].value_.v_long = ifd_mark + num_buf;
    else
        memcpy(ifd[num_ifd].value_.v_sbyte, input_string, ifd_cnt);

    for (idx = 0 ; idx < ifd_cnt ; idx += 1)
        buf[num_buf++] = input_string[idx];
    if (num_buf & 0x1)
        buf[num_buf++] = (unsigned char) 0;
    num_ifd += 1;

    ifd[num_ifd].tag = 0x0129; /* PAGE_NUMBER */
    ifd[num_ifd].type = 3; /* USHORT */
    ifd[num_ifd].cnt = 2;
    ifd[num_ifd].value_.v_short[0] = page_number[0];
    ifd[num_ifd].value_.v_short[1] = page_number[1]; 
    num_ifd += 1;

    input_string = software_name_version;
    ifd_cnt = strlen(input_string) + 1;
    ifd[num_ifd].tag = 0x0131; /* SOFTWARE_NAME_VERSION */
    ifd[num_ifd].type = 2; /* UTF8 */
    ifd[num_ifd].cnt = ifd_cnt;
    if (ifd_cnt > 4)
        ifd[num_ifd].value_.v_long = ifd_mark + num_buf;
    else
        memcpy(ifd[num_ifd].value_.v_sbyte, input_string, ifd_cnt);

    for (idx = 0 ; idx < ifd_cnt ; idx += 1)
        buf[num_buf++] = input_string[idx];
    if (num_buf & 0x1)
        buf[num_buf++] = (unsigned char) 0;
    num_ifd += 1;

    input_string = date_time;
    ifd_cnt = strlen(input_string) + 1;
    ifd[num_ifd].tag = 0x0132; /* DATE_TIME */
    ifd[num_ifd].type = 2; /* UTF8 */
    ifd[num_ifd].cnt = ifd_cnt;
    if (ifd_cnt > 4)
        ifd[num_ifd].value_.v_long = ifd_mark + num_buf;
    else
        memcpy(ifd[num_ifd].value_.v_sbyte, input_string, ifd_cnt);

    for (idx = 0 ; idx < ifd_cnt ; idx += 1)
        buf[num_buf++] = input_string[idx];
    if (num_buf & 0x1)
        buf[num_buf++] = (unsigned char) 0;
    num_ifd += 1;

    input_string = artist_name;
    ifd_cnt = strlen(input_string) + 1;
    ifd[num_ifd].tag = 0x013b; /* ARTIST_NAME */
    ifd[num_ifd].type = 2; /* UTF8 */
    ifd[num_ifd].cnt = ifd_cnt;
    if (ifd_cnt > 4)
        ifd[num_ifd].value_.v_long = ifd_mark + num_buf;
    else
        memcpy(ifd[num_ifd].value_.v_sbyte, input_string, ifd_cnt);

    for (idx = 0 ; idx < ifd_cnt ; idx += 1)
        buf[num_buf++] = input_string[idx];
    if (num_buf & 0x1)
        buf[num_buf++] = (unsigned char) 0;
    num_ifd += 1;

    input_string = host_computer;
    ifd_cnt = strlen(input_string) + 1;
    ifd[num_ifd].tag = 0x013c; /* HOST_COMPUTER */
    ifd[num_ifd].type = 2; /* UTF8 */
    ifd[num_ifd].cnt = ifd_cnt;
    if (ifd_cnt > 4)
        ifd[num_ifd].value_.v_long = ifd_mark + num_buf;
    else
        memcpy(ifd[num_ifd].value_.v_sbyte, input_string, ifd_cnt);

    for (idx = 0 ; idx < ifd_cnt ; idx += 1)
        buf[num_buf++] = input_string[idx];
    if (num_buf & 0x1)
        buf[num_buf++] = (unsigned char) 0;
    num_ifd += 1;

    input_string = copyright_notice;
    ifd_cnt = strlen(input_string) + 1;
    ifd[num_ifd].tag = 0x8298; /* COPYRIGHT_NOTICE */
    ifd[num_ifd].type = 2; /* UTF8 */
    ifd[num_ifd].cnt = ifd_cnt;
    if (ifd_cnt > 4)
        ifd[num_ifd].value_.v_long = ifd_mark + num_buf;
    else
        memcpy(ifd[num_ifd].value_.v_sbyte, input_string, ifd_cnt);

    for (idx = 0 ; idx < ifd_cnt ; idx += 1)
        buf[num_buf++] = input_string[idx];
    if (num_buf & 0x1)
        buf[num_buf++] = (unsigned char) 0;
    num_ifd += 1;

    ifd[num_ifd].tag = 0xa001; /* COLOR_SPACE */
    ifd[num_ifd].type = 3; /* USHORT */
    ifd[num_ifd].cnt = 1;
    ifd[num_ifd].value_.v_short[0] = 0xffff; /* Just use this value for testing */
    ifd[num_ifd].value_.v_short[1] = 0; 
    num_ifd += 1;
#endif

    ifd[num_ifd].tag  = 0xbc01; /* PIXEL_FORMAT */
    ifd[num_ifd].type = 1;      /* BYTE */
    ifd[num_ifd].cnt  = 16;
    ifd[num_ifd].value_.v_long = ifd_mark + num_buf;

    for (idx = 0 ; idx < 16 ; idx += 1)
        buf[num_buf++] = cp->pixel_format[idx];
    num_ifd += 1;

#ifdef WRITE_OPTIONAL_IFD_TAGS
    ifd[num_ifd].tag  = 0xbc02; /* SPATIAL_XFRM_PRIMARY */
    ifd[num_ifd].type = 4;      /* ULONG */
    ifd[num_ifd].cnt  = 1;
    ifd[num_ifd].value_.v_long = spatial_xfrm; 
    num_ifd += 1;

    /* IMAGE_TYPE should only be present when multiple images are in the image */
    /* Fix this condition if encoder can handle multiple images */
    if (0){
        ifd[num_ifd].tag  = 0xbc04; /* IMAGE_TYPE */
        ifd[num_ifd].type = 4;      /* ULONG */
        ifd[num_ifd].cnt  = 1;
        ifd[num_ifd].value_.v_long = 0;
        num_ifd += 1;
    }

    ifd[num_ifd].tag  = 0xbc05; /* PTM_COLOR_INFO() */
    ifd[num_ifd].type = 1;      /* BYTE */
    ifd[num_ifd].cnt  = 4;
    ifd[num_ifd].value_.v_byte[0] = 2; /* default to unspecified */
    ifd[num_ifd].value_.v_byte[1] = 2; /* default to unspecified */
    ifd[num_ifd].value_.v_byte[2] = 2; /* default to no rotation */
    ifd[num_ifd].value_.v_byte[3] = 0; /* default to no rotation */
    num_ifd += 1;

    ifd[num_ifd].tag  = 0xbc06; /* PROFILE_LEVEL_CONTAINER() */
    ifd[num_ifd].type = 1;      /* BYTE */
    ifd[num_ifd].cnt  = 4;
    ifd[num_ifd].value_.v_byte[0] = profile_idc; 
    ifd[num_ifd].value_.v_byte[1] = level_idc; 
    ifd[num_ifd].value_.v_byte[2] = 0; 
    ifd[num_ifd].value_.v_byte[3] = 1; 
    num_ifd += 1;

#endif

    ifd[num_ifd].tag  = 0xbc80; /* IMAGE_WIDTH */
    ifd[num_ifd].type = 4;      /* ULONG */
    ifd[num_ifd].cnt  = 1;
    ifd[num_ifd].value_.v_long = cp->wid;

    num_ifd += 1;

    ifd[num_ifd].tag  = 0xbc81; /* IMAGE_HEIGHT */
    ifd[num_ifd].type = 4;      /* ULONG */
    ifd[num_ifd].cnt  = 1;
    ifd[num_ifd].value_.v_long = cp->hei;

    num_ifd += 1;

#ifdef WRITE_OPTIONAL_IFD_TAGS
    ifd[num_ifd].tag  = 0xbc82; /* WIDTH_RESOLUTION */
    ifd[num_ifd].type = 11;      /* FLOAT */
    ifd[num_ifd].cnt  = 1;
    ifd[num_ifd].value_.v_float = width_res; 
    num_ifd += 1;

    ifd[num_ifd].tag  = 0xbc83; /* HEIGHT_RESOLUTION */
    ifd[num_ifd].type = 11;      /* FLOAT */
    ifd[num_ifd].cnt  = 1;
    ifd[num_ifd].value_.v_float = height_res; 
    num_ifd += 1;
#endif

    ifd[num_ifd].tag  = 0xbcc0; /* IMAGE_OFFSET */
    ifd[num_ifd].type = 4;      /* ULONG */
    ifd[num_ifd].cnt  = 1;
    ifd[num_ifd].value_.v_long = 0;

    num_ifd += 1;

    ifd[num_ifd].tag  = 0xbcc1; /* IMAGE_COUNT */
    ifd[num_ifd].type = 4;      /* ULONG */
    ifd[num_ifd].cnt  = 1;
    ifd[num_ifd].value_.v_long = 0;

    num_ifd += 1;

    if (cp->separate_alpha_image_plane) {
        ifd[num_ifd].tag  = 0xbcc2; /* ALPHA_OFFSET */
        ifd[num_ifd].type = 4;      /* ULONG */
        ifd[num_ifd].cnt  = 1;
        ifd[num_ifd].value_.v_long = 0;

        num_ifd += 1;

        ifd[num_ifd].tag  = 0xbcc3; /* ALPHA_COUNT */
        ifd[num_ifd].type = 4;      /* ULONG */
        ifd[num_ifd].cnt  = 1;
        ifd[num_ifd].value_.v_long = 0; 

        num_ifd += 1;
    }

#ifdef WRITE_OPTIONAL_IFD_TAGS
    ifd[num_ifd].tag  = 0xbcc4; /* IMAGE_BAND_PRESENCE */
    ifd[num_ifd].type = 1;      /* BYTE */
    ifd[num_ifd].cnt  = 1;
    ifd[num_ifd].value_.v_byte[0] = cp->image_band;
    num_ifd += 1;

    if (cp->separate_alpha_image_plane) {
        ifd[num_ifd].tag  = 0xbcc5; /* ALPHA_BAND_PRESENCE */
        ifd[num_ifd].type = 1;      /* BYTE */
        ifd[num_ifd].cnt  = 1;
        ifd[num_ifd].value_.v_byte[0] = cp->alpha_band;
        num_ifd += 1;
    }

    ifd[num_ifd].tag  = 0xea1c; /* PADDING_DATA */
    ifd[num_ifd].type = 7;      /* UNDEFINED */
    ifd[num_ifd].cnt  = 21;     /* Value set arbitrarily to tests this IFD entry */
    ifd[num_ifd].value_.v_long = ifd_mark + num_buf;

    assert(ifd[num_ifd].cnt > 1);

    buf[num_buf++] = 0x1c;
    buf[num_buf++] = 0xea;
    for (idx = 2 ; idx < ifd[num_ifd].cnt ; idx += 1)
        buf[num_buf++] = 0;
    num_ifd += 1;
#endif

    unsigned long ifd_len = 2 + 12*num_ifd + 4; /* NUM_ENTRIES + 12 bytes per entry + ZERO_OR_NEXT_IFD_OFFSET */

    cp->image_offset_mark = ifd_mark + ifd_len + num_buf;

    for (idx = 0 ; idx < num_ifd ; idx += 1) {
        switch (ifd[idx].type) {
              case 1: /* BYTE */
              case 2: /* UTF8 */
              case 6: /* SBYTE */
              case 7: /* UNDEFINED */
                  if (ifd[idx].cnt > 4)
                      ifd[idx].value_.v_long += ifd_len;
                  break;
              case 3: /* USHORT */
              case 8: /* SSHORT */
                  if (ifd[idx].cnt > 2)
                      ifd[idx].value_.v_long += ifd_len;
                  break;
              case 4: /* ULONG */
              case 9: /* SLONG */
              case 11: /* FLOAT */
                  if (ifd[idx].cnt > 1)
                      ifd[idx].value_.v_long += ifd_len;
                  /* If we find the IMAGE_OFFSET, we know its value
                  now. Write it into v_long */
                  if (ifd[idx].tag == 0xbcc0) /* IMAGE_OFFSET */
                      ifd[idx].value_.v_long = cp->image_offset_mark;
                  /* For the following markers, record the locations to be filled in later */
                  if (ifd[idx].tag == 0xbcc1) /* IMAGE_COUNT */
                      cp->image_count_mark = ifd_mark + 2 + 12*idx + 8;
                  if (ifd[idx].tag == 0xbcc2) /* ALPHA_OFFSET */
                      cp->alpha_offset_mark = ifd_mark + 2 + 12*idx + 8;
                  if (ifd[idx].tag == 0xbcc3) /* ALPHA_COUNT */
                      cp->alpha_count_mark = ifd_mark + 2 + 12*idx + 8;

                  break;
              case 5: /* URATIONAL */
              case 10: /* SRATIONAL */
              case 12: /* DOUBLE */
                  ifd[idx].value_.v_long += ifd_len;
                  break;
              default: /* RESERVED */
                  assert(0);
        }
    }

    unsigned char scr[12];
    scr[0] = (num_ifd>>0) & 0xff;
    scr[1] = (num_ifd>>8) & 0xff;
#ifdef JPEGXR_ADOBE_EXT
	cp->wb.write(scr,2);
#else //#ifdef JPEGXR_ADOBE_EXT
    fwrite(scr, 1, 2, cp->fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
    for (idx = 0 ; idx < num_ifd ; idx += 1) {
        scr[0] = (ifd[idx].tag >> 0) & 0xff;
        scr[1] = (ifd[idx].tag >> 8) & 0xff;
        scr[2] = (ifd[idx].type>> 0) & 0xff;
        scr[3] = (ifd[idx].type>> 8) & 0xff;
        scr[4] = (ifd[idx].cnt >> 0) & 0xff;
        scr[5] = (ifd[idx].cnt >> 8) & 0xff;
        scr[6] = (ifd[idx].cnt >>16) & 0xff;
        scr[7] = (ifd[idx].cnt >>24) & 0xff;

        switch (ifd[idx].type) {
            case 1: /* BYTE */
            case 2: /* UTF8 */
            case 6: /* SBYTE */
            case 7: /* UNDEFINED */
                if (ifd[idx].cnt <= 4) {
                    scr[ 8] = ifd[idx].value_.v_byte[0];
                    scr[ 9] = ifd[idx].value_.v_byte[1];
                    scr[10] = ifd[idx].value_.v_byte[2];
                    scr[11] = ifd[idx].value_.v_byte[3];
                } else {
                    scr[ 8] = (ifd[idx].value_.v_long >> 0) & 0xff;
                    scr[ 9] = (ifd[idx].value_.v_long >> 8) & 0xff;
                    scr[10] = (ifd[idx].value_.v_long >>16) & 0xff;
                    scr[11] = (ifd[idx].value_.v_long >>24) & 0xff;
                }
                break;
            case 3: /* USHORT */
            case 8: /* SSHORT */
                if (ifd[idx].cnt <= 2) {
                    scr[ 8] = (ifd[idx].value_.v_short[0] >> 0) & 0xff;
                    scr[ 9] = (ifd[idx].value_.v_short[0] >> 8) & 0xff;
                    scr[10] = (ifd[idx].value_.v_short[1] >> 0) & 0xff;
                    scr[11] = (ifd[idx].value_.v_short[1] >> 8) & 0xff;
                } else {
                    scr[ 8] = (ifd[idx].value_.v_long >> 0) & 0xff;
                    scr[ 9] = (ifd[idx].value_.v_long >> 8) & 0xff;
                    scr[10] = (ifd[idx].value_.v_long >>16) & 0xff;
                    scr[11] = (ifd[idx].value_.v_long >>24) & 0xff;
                }
                break;
            case 4: /* ULONG */
            case 9: /* SLONG */
            case 11: /* FLOAT */
            case 5: /* URATIONAL */
            case 10: /* SRATIONAL */
            case 12: /* DOUBLE */
                scr[ 8] = (ifd[idx].value_.v_long >> 0) & 0xff;
                scr[ 9] = (ifd[idx].value_.v_long >> 8) & 0xff;
                scr[10] = (ifd[idx].value_.v_long >>16) & 0xff;
                scr[11] = (ifd[idx].value_.v_long >>24) & 0xff;
                break;
            default:
                assert(0);
                break;
        }
#ifdef JPEGXR_ADOBE_EXT
		cp->wb.write(scr,12);
#else //#ifdef JPEGXR_ADOBE_EXT
        fwrite(scr, 1, 12, cp->fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
    }
#ifdef JPEGXR_ADOBE_EXT
	cp->next_ifd_mark = cp->wb.tell();
#else //#ifdef JPEGXR_ADOBE_EXT
    cp->next_ifd_mark = ftell(cp->fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
    scr[0] = 0;
    scr[1] = 0;
    scr[2] = 0;
    scr[3] = 0;

#ifdef JPEGXR_ADOBE_EXT
	cp->wb.write(scr,4);

	cp->wb.write(buf,num_buf);
#else //#ifdef JPEGXR_ADOBE_EXT
    fwrite(scr, 1, 4, cp->fd);

    fwrite(buf, 1, num_buf, cp->fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
}

int jxrc_begin_image_data(jxr_container_t cp)
{
      emit_ifd(cp);
      return 0;
}

int jxrc_write_container_post(jxr_container_t cp)
{
#ifdef JPEGXR_ADOBE_EXT
	  uint32_t mark = cp->wb.tell();
#else //#ifdef JPEGXR_ADOBE_EXT
      uint32_t mark = ftell(cp->fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
      mark = (mark+1)&~1;

      assert(mark > cp->image_offset_mark);
      uint32_t count = mark - cp->image_offset_mark;

      DEBUG("CONTAINER: measured bitstream count=%u\n", count);

#ifdef JPEGXR_ADOBE_EXT
	  cp->wb.seek(cp->image_count_mark, SEEK_SET);
#else //#ifdef JPEGXR_ADOBE_EXT
      fseek(cp->fd, cp->image_count_mark, SEEK_SET);
#endif //#ifdef JPEGXR_ADOBE_EXT
      unsigned char scr[4];
      scr[0] = (count >>  0) & 0xff;
      scr[1] = (count >>  8) & 0xff;
      scr[2] = (count >> 16) & 0xff;
      scr[3] = (count >> 24) & 0xff;
#ifdef JPEGXR_ADOBE_EXT
	  cp->wb.write(scr,4);
#else //#ifdef JPEGXR_ADOBE_EXT
      fwrite(scr, 1, 4, cp->fd);
#endif //#ifdef JPEGXR_ADOBE_EXT

      if(cp->separate_alpha_image_plane)
      {
#ifdef JPEGXR_ADOBE_EXT
		  cp->wb.seek(cp->alpha_offset_mark, SEEK_SET);
#else //#ifdef JPEGXR_ADOBE_EXT
          fseek(cp->fd, cp->alpha_offset_mark, SEEK_SET);          
#endif //#ifdef JPEGXR_ADOBE_EXT
          count = mark;
          scr[0] = (count >>  0) & 0xff;
          scr[1] = (count >>  8) & 0xff;
          scr[2] = (count >> 16) & 0xff;
          scr[3] = (count >> 24) & 0xff;
#ifdef JPEGXR_ADOBE_EXT
		  cp->wb.write(scr, 4);
#else //#ifdef JPEGXR_ADOBE_EXT
          fwrite(scr, 1, 4, cp->fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
      }      
#ifdef JPEGXR_ADOBE_EXT
	  cp->wb.seek(mark, SEEK_SET);
#else //#ifdef JPEGXR_ADOBE_EXT
      fseek(cp->fd, mark, SEEK_SET);
#endif //#ifdef JPEGXR_ADOBE_EXT
      cp->alpha_begin_mark = mark;
      return 0;
}

int jxrc_write_container_post_alpha(jxr_container_t cp)
{
#ifdef JPEGXR_ADOBE_EXT
	  uint32_t mark = cp->wb.tell();
#else //#ifdef JPEGXR_ADOBE_EXT
      uint32_t mark = ftell(cp->fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
      mark = (mark+1)&~1;
      
      uint32_t count = mark - cp->alpha_begin_mark;
      DEBUG("CONTAINER: measured alpha count=%u\n", count);
      
      if(cp->separate_alpha_image_plane)
      {
          unsigned char scr[4];
#ifdef JPEGXR_ADOBE_EXT
		  cp->wb.seek(cp->alpha_count_mark, SEEK_SET);
#else //#ifdef JPEGXR_ADOBE_EXT
          fseek(cp->fd, cp->alpha_count_mark, SEEK_SET);          
#endif //#ifdef JPEGXR_ADOBE_EXT
          count = mark;
          scr[0] = (count >>  0) & 0xff;
          scr[1] = (count >>  8) & 0xff;
          scr[2] = (count >> 16) & 0xff;
          scr[3] = (count >> 24) & 0xff;
#ifdef JPEGXR_ADOBE_EXT
		  cp->wb.write(scr,4);
#else //#ifdef JPEGXR_ADOBE_EXT
          fwrite(scr, 1, 4, cp->fd);
#endif //#ifdef JPEGXR_ADOBE_EXT
      }      
#ifdef JPEGXR_ADOBE_EXT
	  cp->wb.seek(mark, SEEK_SET);
#else //#ifdef JPEGXR_ADOBE_EXT
      fseek(cp->fd, mark, SEEK_SET);
#endif //#ifdef JPEGXR_ADOBE_EXT
      return 0;
}
/*
* $Log: cw_emit.c,v $
* Revision 1.2 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.1 2009/04/13 12:00:00 microsoft
* Reference Software v1.5 updates.
*
*/

