
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
#pragma comment (user,"$Id: flags.c,v 1.5 2008/03/06 02:05:48 steve Exp $")
#else
#ident "$Id: flags.c,v 1.5 2008/03/06 02:05:48 steve Exp $"
#endif

# include "jxr_priv.h"
# include <assert.h>

unsigned jxr_get_IMAGE_WIDTH(jxr_image_t image)
{
    return image->width1 + 1;
}

unsigned jxr_get_IMAGE_HEIGHT(jxr_image_t image)
{
    return image->height1 + 1;
}

unsigned jxr_get_EXTENDED_IMAGE_WIDTH(jxr_image_t image)
{
    return image->extended_width;
}

unsigned jxr_get_EXTENDED_IMAGE_HEIGHT(jxr_image_t image)
{
    return image->extended_height;
}

int jxr_get_TILING_FLAG(jxr_image_t image)
{
    if (TILING_FLAG(image))
        return 1;
    else
        return 0;
}

unsigned jxr_get_TILE_COLUMNS(jxr_image_t image)
{
    return image->tile_columns + 1;
}

unsigned jxr_get_TILE_ROWS(jxr_image_t image)
{
    return image->tile_rows + 1;
}

int jxr_get_TILE_WIDTH(jxr_image_t image, unsigned column)
{
    if (column > image->tile_columns) {
        return 0;
    } else if (column == image->tile_columns) {
        if (column == 0)
            return EXTENDED_WIDTH_BLOCKS(image);
        else
            return EXTENDED_WIDTH_BLOCKS(image) - image->tile_column_position[column-1];
    } else {
        return image->tile_column_width[column];
    }
}

int jxr_get_TILE_HEIGHT(jxr_image_t image, unsigned row)
{
    if (row > image->tile_rows) {
        return 0;
    } else if (row == image->tile_rows) {
        if (row == 0)
            return EXTENDED_HEIGHT_BLOCKS(image);
        else
            return EXTENDED_HEIGHT_BLOCKS(image) - image->tile_row_position[row-1];
    } else {
        return image->tile_row_height[row];
    }
}

int jxr_get_ALPHACHANNEL_FLAG(jxr_image_t image)
{
    if (ALPHACHANNEL_FLAG(image))
        return 1;
    else
        return 0;
}

jxrc_t_pixelFormat jxr_get_pixel_format(jxr_image_t image)
{
    return image->ePixelFormat;
}

/*
* $Log: flags.c,v $
*
* Revision 1.7 2009/05/29 12:00:00 microsoft
* Reference Software v1.6 updates.
*
* Revision 1.6 2009/04/13 12:00:00 microsoft
* Reference Software v1.5 updates.
*
* Revision 1.5 2008/03/06 02:05:48 steve
* Distributed quantization
*
* Revision 1.4 2008/02/26 23:52:44 steve
* Remove ident for MS compilers.
*
* Revision 1.3 2008/02/26 23:28:53 steve
* Remove C99 requirements from the API.
*
* Revision 1.2 2007/11/26 01:47:15 steve
* Add copyright notices per MS request.
*
* Revision 1.1 2007/06/06 17:19:12 steve
* Introduce to CVS.
*
*/

