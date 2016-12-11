/* Copyright 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_platform_verification_private.idl,
 *   modified Fri Oct 18 15:02:09 2013.
 */

#ifndef PPAPI_C_PRIVATE_PPB_PLATFORM_VERIFICATION_PRIVATE_H_
#define PPAPI_C_PRIVATE_PPB_PLATFORM_VERIFICATION_PRIVATE_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_var.h"

#define PPB_PLATFORMVERIFICATION_PRIVATE_INTERFACE_0_2 \
    "PPB_PlatformVerification_Private;0.2"
#define PPB_PLATFORMVERIFICATION_PRIVATE_INTERFACE \
    PPB_PLATFORMVERIFICATION_PRIVATE_INTERFACE_0_2

/**
 * @file
 * This file defines the API for platform verification. Currently, it only
 * supports Chrome OS.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * The <code>PPB_PlatformVerification_Private</code> interface allows authorized
 * services to verify that the underlying platform is trusted. An example of a
 * trusted platform is a Chrome OS device in verified boot mode.
 */
struct PPB_PlatformVerification_Private_0_2 {
  /**
   * Create() creates a <code>PPB_PlatformVerification_Private</code> object.
   *
   * @pram[in] instance A <code>PP_Instance</code> identifying one instance of
   * a module.
   *
   * @return A <code>PP_Resource</code> corresponding to a
   * <code>PPB_PlatformVerification_Private</code> if successful, 0 if creation
   * failed.
   */
  PP_Resource (*Create)(PP_Instance instance);
  /**
   * IsPlatformVerification() determines if the provided resource is a
   * <code>PPB_PlatformVerification_Private</code>.
   *
   * @param[in] resource A <code>PP_Resource</code> corresponding to a
   * <code>PPB_PlatformVerification_Private</code>.
   *
   * @return <code>PP_TRUE</code> if the resource is a
   * <code>PPB_PlatformVerification_Private</code>, <code>PP_FALSE</code> if the
   * resource is invalid or some type other than
   * <code>PPB_PlatformVerification_Private</code>.
   */
  PP_Bool (*IsPlatformVerification)(PP_Resource resource);
  /**
   * Requests a platform challenge for a given service id.
   *
   * @param[in] service_id A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the service_id for the challenge.
   *
   * @param[in] challenge A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_ARRAY_BUFFER</code> that contains the challenge data.
   *
   * @param[out] signed_data A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_ARRAY_BUFFER</code> that contains the data signed by the
   * platform.
   *
   * @param[out] signed_data_signature A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_ARRAY_BUFFER</code> that contains the signature of the
   * signed data block.
   *
   * @param[out] platform_key_certificate A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> that contains the device specific
   * certificate for the requested service_id.
   *
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called after
   * the platform challenge has been completed. This callback will only run if
   * the return code is <code>PP_OK_COMPLETIONPENDING</code>.
   *
   * @return An int32_t containing an error code from <code>pp_errors.h</code>.
   */
  int32_t (*ChallengePlatform)(PP_Resource instance,
                               struct PP_Var service_id,
                               struct PP_Var challenge,
                               struct PP_Var* signed_data,
                               struct PP_Var* signed_data_signature,
                               struct PP_Var* platform_key_certificate,
                               struct PP_CompletionCallback callback);
};

typedef struct PPB_PlatformVerification_Private_0_2
    PPB_PlatformVerification_Private;
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_PLATFORM_VERIFICATION_PRIVATE_H_ */

