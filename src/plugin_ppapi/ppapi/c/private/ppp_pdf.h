// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_C_PRIVATE_PPP_PDF_H_
#define PPAPI_C_PRIVATE_PPP_PDF_H_

#include <stdint.h>

#include "ppapi/c/dev/pp_print_settings_dev.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_point.h"
#include "ppapi/c/pp_var.h"

#define PPP_PDF_INTERFACE_1 "PPP_Pdf;1"
#define PPP_PDF_INTERFACE PPP_PDF_INTERFACE_1

typedef enum {
  // Rotates the page 90 degrees clockwise from its current orientation.
  PP_PRIVATEPAGETRANSFORMTYPE_ROTATE_90_CW,
  // Rotates the page 90 degrees counterclockwise from its current orientation.
  PP_PRIVATEPAGETRANSFORMTYPE_ROTATE_90_CCW
} PP_PrivatePageTransformType;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_PrivatePageTransformType, 4);

typedef enum {
  PP_PRIVATEDUPLEXMODE_NONE = 0,
  PP_PRIVATEDUPLEXMODE_SIMPLEX = 1,
  PP_PRIVATEDUPLEXMODE_SHORT_EDGE = 2,
  PP_PRIVATEDUPLEXMODE_LONG_EDGE = 3,
  PP_PRIVATEDUPLEXMODE_LAST = PP_PRIVATEDUPLEXMODE_LONG_EDGE
} PP_PrivateDuplexMode_Dev;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_PrivateDuplexMode_Dev, 4);

struct PP_PdfPrintPresetOptions_Dev {
  // Returns whether scaling is disabled. Returns same information as the
  // PPP_Printing_Dev's method IsScalingDiabled().
  PP_Bool is_scaling_disabled;

  // Number of copies to be printed.
  int32_t copies;

  // DuplexMode to be used for printing.
  PP_PrivateDuplexMode_Dev duplex;

  // True if all the pages in the PDF are the same size.
  PP_Bool is_page_size_uniform;

  // Only valid if |is_page_size_uniform| is true. The page size.
  PP_Size uniform_page_size;
};
PP_COMPILE_ASSERT_STRUCT_SIZE_IN_BYTES(PP_PdfPrintPresetOptions_Dev, 24);

struct PPP_Pdf_1_1 {
  // Returns an absolute URL if the position is over a link.
  PP_Var (*GetLinkAtPosition)(PP_Instance instance,
                              PP_Point point);

  // Requests that the plugin apply the given transform to its view.
  void (*Transform)(PP_Instance instance, PP_PrivatePageTransformType type);

  // Return true if print preset options are updated from document.
  PP_Bool (*GetPrintPresetOptionsFromDocument)(
      PP_Instance instance,
      PP_PdfPrintPresetOptions_Dev* options);

  void (*EnableAccessibility)(PP_Instance instance);
};

typedef PPP_Pdf_1_1 PPP_Pdf;

#endif  // PPAPI_C_PRIVATE_PPP_PDF_H_
