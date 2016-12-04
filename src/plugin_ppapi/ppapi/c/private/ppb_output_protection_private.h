/* Copyright 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_output_protection_private.idl,
 *   modified Tue Oct  8 13:22:13 2013.
 */

#ifndef PPAPI_C_PRIVATE_PPB_OUTPUT_PROTECTION_PRIVATE_H_
#define PPAPI_C_PRIVATE_PPB_OUTPUT_PROTECTION_PRIVATE_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"

#define PPB_OUTPUTPROTECTION_PRIVATE_INTERFACE_0_1 \
    "PPB_OutputProtection_Private;0.1"
#define PPB_OUTPUTPROTECTION_PRIVATE_INTERFACE \
    PPB_OUTPUTPROTECTION_PRIVATE_INTERFACE_0_1

/**
 * @file
 * This file defines the API for output protection. Currently, it only supports
 * Chrome OS.
 */


/**
 * @addtogroup Enums
 * @{
 */
/**
 * Content protection methods applied on video output link.
 */
typedef enum {
  PP_OUTPUT_PROTECTION_METHOD_PRIVATE_NONE = 0,
  PP_OUTPUT_PROTECTION_METHOD_PRIVATE_HDCP = 1 << 0
} PP_OutputProtectionMethod_Private;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_OutputProtectionMethod_Private, 4);

/**
 * Video output link types.
 */
typedef enum {
  PP_OUTPUT_PROTECTION_LINK_TYPE_PRIVATE_NONE = 0,
  PP_OUTPUT_PROTECTION_LINK_TYPE_PRIVATE_UNKNOWN = 1 << 0,
  PP_OUTPUT_PROTECTION_LINK_TYPE_PRIVATE_INTERNAL = 1 << 1,
  PP_OUTPUT_PROTECTION_LINK_TYPE_PRIVATE_VGA = 1 << 2,
  PP_OUTPUT_PROTECTION_LINK_TYPE_PRIVATE_HDMI = 1 << 3,
  PP_OUTPUT_PROTECTION_LINK_TYPE_PRIVATE_DVI = 1 << 4,
  PP_OUTPUT_PROTECTION_LINK_TYPE_PRIVATE_DISPLAYPORT = 1 << 5,
  PP_OUTPUT_PROTECTION_LINK_TYPE_PRIVATE_NETWORK = 1 << 6
} PP_OutputProtectionLinkType_Private;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_OutputProtectionLinkType_Private, 4);
/**
 * @}
 */

/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * The <code>PPB_OutputProtection_Private</code> interface allows controlling
 * output protection.
 *
 * <strong>Example:</strong>
 *
 * @code
 * op = output_protection->Create(instance);
 * output_protection->QueryStatus(op, &link_mask, &protection_mask,
 *                                done_callback);
 * @endcode
 *
 * In this example, the plugin wants to enforce HDCP for HDMI link.
 * @code
 * if (link_mask & PP_OUTPUT_PROTECTION_LINK_TYPE_PRIVATE_HDMI) {
 *   output_protection->EnableProtection(
 *       op, PP_OUTPUT_PROTECTION_METHOD_PRIVATE_HDCP, done_callback);
 * }
 * @endcode
 *
 * After EnableProtection() completes, the plugin has to query protection
 * status periodically to make sure the protection is enabled and remains
 * enabled.
 */
struct PPB_OutputProtection_Private_0_1 {
  /**
   * Create() creates a new <code>PPB_OutputProtection_Private</code> object.
   *
   * @pram[in] instance A <code>PP_Instance</code> identifying one instance of
   * a module.
   *
   * @return A <code>PP_Resource</code> corresponding to a
   * <code>PPB_OutputProtection_Private</code> if successful, 0 if creation
   * failed.
   */
  PP_Resource (*Create)(PP_Instance instance);
  /**
   * IsOutputProtection() determines if the provided resource is a
   * <code>PPB_OutputProtection_Private</code>.
   *
   * @param[in] resource A <code>PP_Resource</code> corresponding to a
   * <code>PPB_OutputProtection_Private</code>.
   *
   * @return <code>PP_TRUE</code> if the resource is a
   * <code>PPB_OutputProtection_Private</code>, <code>PP_FALSE</code> if the
   * resource is invalid or some type other than
   * <code>PPB_OutputProtection_Private</code>.
   */
  PP_Bool (*IsOutputProtection)(PP_Resource resource);
  /**
   * Query link status and protection status.
   * Clients have to query status periodically in order to detect changes.
   *
   * @param[in] resource A <code>PP_Resource</code> corresponding to a
   * <code>PPB_OutputProtection_Private</code>.
   * @param[out] link_mask The type of connected output links, which is a
   * bit-mask of the <code>PP_OutputProtectionLinkType_Private</code> values.
   * @param[out] protection_mask Enabled protection methods, which is a
   * bit-mask of the <code>PP_OutputProtectionMethod_Private</code> values.
   * @param[in] callback A <code>PP_CompletionCallback</code> to run on
   * asynchronous completion of QueryStatus(). This callback will only run if
   * QueryStatus() returns <code>PP_OK_COMPLETIONPENDING</code>.
   *
   * @return An int32_t containing an error code from <code>pp_errors.h</code>.
   */
  int32_t (*QueryStatus)(PP_Resource resource,
                         uint32_t* link_mask,
                         uint32_t* protection_mask,
                         struct PP_CompletionCallback callback);
  /**
   * Set desired protection methods.
   *
   * When the desired protection method(s) have been applied to all applicable
   * output links, the relevant bit(s) of the protection_mask returned by
   * QueryStatus() will be set. Otherwise, the relevant bit(s) of
   * protection_mask will not be set; there is no separate error code or
   * callback.
   *
   * Protections will be disabled if no longer desired by all instances.
   *
   * @param[in] resource A <code>PP_Resource</code> corresponding to a
   * <code>PPB_OutputProtection_Private</code>.
   * @param[in] desired_protection_mask The desired protection methods, which
   * is a bit-mask of the <code>PP_OutputProtectionMethod_Private</code>
   * values.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called with
   * <code>PP_OK</code> when the protection request has been made. This may be
   * before the protection have actually been applied. Call QueryStatus to get
   * protection status. If it failed to make the protection request, the
   * callback is called with <code>PP_ERROR_FAILED</code> and there is no need
   * to call QueryStatus().
   *
   * @return An int32_t containing an error code from <code>pp_errors.h</code>.
   */
  int32_t (*EnableProtection)(PP_Resource resource,
                              uint32_t desired_protection_mask,
                              struct PP_CompletionCallback callback);
};

typedef struct PPB_OutputProtection_Private_0_1 PPB_OutputProtection_Private;
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_OUTPUT_PROTECTION_PRIVATE_H_ */

