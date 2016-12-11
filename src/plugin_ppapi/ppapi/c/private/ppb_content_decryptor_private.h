/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_content_decryptor_private.idl,
 *   modified Mon Mar 30 22:35:33 2015.
 */

#ifndef PPAPI_C_PRIVATE_PPB_CONTENT_DECRYPTOR_PRIVATE_H_
#define PPAPI_C_PRIVATE_PPB_CONTENT_DECRYPTOR_PRIVATE_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/private/pp_content_decryptor.h"

#define PPB_CONTENTDECRYPTOR_PRIVATE_INTERFACE_0_14 \
    "PPB_ContentDecryptor_Private;0.14"
#define PPB_CONTENTDECRYPTOR_PRIVATE_INTERFACE \
    PPB_CONTENTDECRYPTOR_PRIVATE_INTERFACE_0_14

/**
 * @file
 * This file defines the <code>PPB_ContentDecryptor_Private</code>
 * interface. Note: This is a special interface, only to be used for Content
 * Decryption Modules, not normal plugins.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * <code>PPB_ContentDecryptor_Private</code> structure contains the function
 * pointers the browser must implement to support plugins implementing the
 * <code>PPP_ContentDecryptor_Private</code> interface. This interface provides
 * browser side support for the Content Decryption Module (CDM) for Encrypted
 * Media Extensions: http://www.w3.org/TR/encrypted-media/
 */
struct PPB_ContentDecryptor_Private_0_14 {
  /**
   * A promise has been resolved by the CDM.
   *
   * @param[in] promise_id Identifies the promise that the CDM resolved.
   */
  void (*PromiseResolved)(PP_Instance instance, uint32_t promise_id);
  /**
   * A promise that resulted in a new session has been resolved by the CDM.
   *
   * @param[in] promise_id Identifies the promise that the CDM resolved.
   *
   * @param[in] session_id A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the session's ID attribute.
   */
  void (*PromiseResolvedWithSession)(PP_Instance instance,
                                     uint32_t promise_id,
                                     struct PP_Var session_id);
  /**
   * A promise has been rejected by the CDM due to an error.
   *
   * @param[in] promise_id Identifies the promise that the CDM rejected.
   *
   * @param[in] exception_code A <code>PP_CdmExceptionCode</code> containing
   * the exception code.
   *
   * @param[in] system_code A system error code.
   *
   * @param[in] error_description A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the error description.
   */
  void (*PromiseRejected)(PP_Instance instance,
                          uint32_t promise_id,
                          PP_CdmExceptionCode exception_code,
                          uint32_t system_code,
                          struct PP_Var error_description);
  /**
   * A message or request has been generated for key_system in the CDM, and
   * must be sent to the web application.
   *
   * For example, when the browser invokes <code>CreateSession()</code>
   * on the <code>PPP_ContentDecryptor_Private</code> interface, the plugin
   * must send a message containing the license request.
   *
   * Note that <code>SessionMessage()</code> can be used for purposes other than
   * responses to <code>CreateSession()</code> calls. See also the text
   * in the comment for <code>SessionReady()</code>, which describes a sequence
   * of <code>UpdateSession()</code> and <code>SessionMessage()</code> calls
   * required to prepare for decryption.
   *
   * @param[in] session_id A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the ID of a session for
   * which this message is intended.
   *
   * @param[in] message_type A <code>PP_CdmMessageType</code> containing the
   * message type.
   *
   * @param[in] message A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_ARRAY_BUFFER</code> that contains the message.
   *
   * @param[in] legacy_destination_url A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the destination URL for the
   * message.
   */
  void (*SessionMessage)(PP_Instance instance,
                         struct PP_Var session_id,
                         PP_CdmMessageType message_type,
                         struct PP_Var message,
                         struct PP_Var legacy_destination_url);
  /**
   * The keys for a session have changed.
   *
   * @param[in] session_id A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the ID of the session that has
   * a change in keys.
   *
   * @param[in] has_additional_usable_key A <code>PP_Bool</code> indicating if
   * a new usable key has been added.
   *
   * @param[in] key_count The number of arguments contained in
   * <code>key_information</code>
   *
   * @param[in] key_information An array of type <code>PP_KeyInformation</code>
   * that are the session's key IDs and their status.
   */
  void (*SessionKeysChange)(PP_Instance instance,
                            struct PP_Var session_id,
                            PP_Bool has_additional_usable_key,
                            uint32_t key_count,
                            const struct PP_KeyInformation key_information[]);
  /**
   * The expiration time for a session has changed.
   *
   * @param[in] session_id A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the ID of the session that has
   * a new expiration time.
   *
   * @param[in] new_expiry_time A <code>PP_Time</code> indicating the new
   * expiry time of the session. The value is defined as the number of seconds
   * since the Epoch (00:00:00 UTC, January 1, 1970).
   */
  void (*SessionExpirationChange)(PP_Instance instance,
                                  struct PP_Var session_id,
                                  PP_Time new_expiry_time);
  /**
   * The session has been closed as the result of a call to the
   * <code>ReleaseSession()</code> method on the
   * <code>PPP_ContentDecryptor_Private</code> interface, or due to other
   * factors as determined by the CDM.
   *
   * @param[in] session_id A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the session's ID attribute of
   * the session that is now closed.
   */
  void (*SessionClosed)(PP_Instance instance, struct PP_Var session_id);
  /**
   * An error occurred in a <code>PPP_ContentDecryptor_Private</code> method,
   * or within the plugin implementing the interface.
   *
   * @param[in] session_id A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the session's ID attribute of
   * the session that caused the error.
   *
   * @param[in] exception_code A <code>PP_CdmExceptionCode</code> containing
   * the exception code.
   *
   * @param[in] system_code A system error code.
   *
   * @param[in] error_description A <code>PP_Var</code> of type
   * <code>PP_VARTYPE_STRING</code> containing the error description.
   */
  void (*LegacySessionError)(PP_Instance instance,
                             struct PP_Var session_id,
                             PP_CdmExceptionCode exception_code,
                             uint32_t system_code,
                             struct PP_Var error_description);
  /**
   * Called after the <code>Decrypt()</code> method on the
   * <code>PPP_ContentDecryptor_Private</code> interface completes to
   * deliver decrypted_block to the browser for decoding and rendering.
   *
   * The plugin must not hold a reference to the encrypted buffer resource
   * provided to <code>Decrypt()</code> when it calls this method. The browser
   * will reuse the buffer in a subsequent <code>Decrypt()</code> call.
   *
   * @param[in] decrypted_block A <code>PP_Resource</code> corresponding to a
   * <code>PPB_Buffer_Dev</code> resource that contains a decrypted data
   * block.
   *
   * @param[in] decrypted_block_info A <code>PP_DecryptedBlockInfo</code> that
   * contains the result code and tracking info associated with the
   * <code>decrypted_block</code>.
   */
  void (*DeliverBlock)(
      PP_Instance instance,
      PP_Resource decrypted_block,
      const struct PP_DecryptedBlockInfo* decrypted_block_info);
  /**
   * Called after the <code>InitializeAudioDecoder()</code> or
   * <code>InitializeVideoDecoder()</code> method on the
   * <code>PPP_ContentDecryptor_Private</code> interface completes to report
   * decoder initialization status to the browser.
   *
   * @param[in] success A <code>PP_Bool</code> that is set to
   * <code>PP_TRUE</code> when the decoder initialization request associated
   * with <code>request_id</code> was successful.
   *
   * @param[in] decoder_type A <code>PP_DecryptorStreamType</code> identifying
   * the decoder type for which this initialization status response was sent.
   *
   * @param[in] request_id The <code>request_id</code> value passed to
   * <code>InitializeAudioDecoder</code> or <code>InitializeVideoDecoder</code>
   * in <code>PP_AudioDecoderConfig</code> or
   * <code>PP_VideoDecoderConfig</code>.
   */
  void (*DecoderInitializeDone)(PP_Instance instance,
                                PP_DecryptorStreamType decoder_type,
                                uint32_t request_id,
                                PP_Bool success);
  /**
   * Called after the <code>DeinitializeDecoder()</code> method on the
   * <code>PPP_ContentDecryptor_Private</code> interface completes to report
   * decoder de-initialization completion to the browser.
   *
   * @param[in] decoder_type The <code>PP_DecryptorStreamType</code> passed to
   * <code>DeinitializeDecoder()</code>.
   *
   * @param[in] request_id The <code>request_id</code> value passed to
   * <code>DeinitializeDecoder()</code>.
   */
  void (*DecoderDeinitializeDone)(PP_Instance instance,
                                  PP_DecryptorStreamType decoder_type,
                                  uint32_t request_id);
  /**
   * Called after the <code>ResetDecoder()</code> method on the
   * <code>PPP_ContentDecryptor_Private</code> interface completes to report
   * decoder reset completion to the browser.
   *
   * @param[in] decoder_type The <code>PP_DecryptorStreamType</code> passed to
   * <code>ResetDecoder()</code>.
   *
   * @param[in] request_id The <code>request_id</code> value passed to
   * <code>ResetDecoder()</code>.
   */
  void (*DecoderResetDone)(PP_Instance instance,
                           PP_DecryptorStreamType decoder_type,
                           uint32_t request_id);
  /**
   * Called after the <code>DecryptAndDecode()</code> method on the
   * <code>PPP_ContentDecryptor_Private</code> interface completes to deliver
   * a decrypted and decoded video frame to the browser for rendering.
   *
   * The plugin must not hold a reference to the encrypted buffer resource
   * provided to <code>DecryptAndDecode()</code> when it calls this method. The
   * browser will reuse the buffer in a subsequent
   * <code>DecryptAndDecode()</code> call.
   *
   * @param[in] decrypted_frame A <code>PP_Resource</code> corresponding to a
   * <code>PPB_Buffer_Dev</code> resource that contains a video frame.
   *
   * @param[in] decrypted_frame_info A <code>PP_DecryptedFrameInfo</code> that
   * contains the result code, tracking info, and buffer format associated with
   * <code>decrypted_frame</code>.
   */
  void (*DeliverFrame)(
      PP_Instance instance,
      PP_Resource decrypted_frame,
      const struct PP_DecryptedFrameInfo* decrypted_frame_info);
  /**
   * Called after the <code>DecryptAndDecode()</code> method on the
   * <code>PPP_ContentDecryptor_Private</code> interface completes to deliver
   * a buffer of decrypted and decoded audio samples to the browser for
   * rendering.
   *
   * The plugin must not hold a reference to the encrypted buffer resource
   * provided to <code>DecryptAndDecode()</code> when it calls this method. The
   * browser will reuse the buffer in a subsequent
   * <code>DecryptAndDecode()</code> call.
   *
   * <code>audio_frames</code> can contain multiple audio output buffers. Each
   * buffer is serialized in this format:
   *
   * |<------------------- serialized audio buffer ------------------->|
   * | int64_t timestamp | int64_t length | length bytes of audio data |
   *
   * For example, with three audio output buffers, |audio_frames| will look
   * like this:
   *
   * |<---------------- audio_frames ------------------>|
   * | audio buffer 0 | audio buffer 1 | audio buffer 2 |
   *
   * @param[in] audio_frames A <code>PP_Resource</code> corresponding to a
   * <code>PPB_Buffer_Dev</code> resource that contains a decrypted buffer
   * of decoded audio samples.
   *
   * @param[in] decrypted_sample_info A <code>PP_DecryptedSampleInfo</code> that
   * contains the tracking info and result code associated with the decrypted
   * samples.
   */
  void (*DeliverSamples)(
      PP_Instance instance,
      PP_Resource audio_frames,
      const struct PP_DecryptedSampleInfo* decrypted_sample_info);
};

typedef struct PPB_ContentDecryptor_Private_0_14 PPB_ContentDecryptor_Private;
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_CONTENT_DECRYPTOR_PRIVATE_H_ */

