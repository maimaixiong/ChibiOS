/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    hal_crypto.h
 * @brief   Cryptographic Driver macros and structures.
 *
 * @addtogroup CRYPTO
 * @{
 */

#ifndef HAL_CRYPTO_H
#define HAL_CRYPTO_H

#if (HAL_USE_CRY == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver state machine possible states.
 */
typedef enum {
  CRY_UNINIT = 0,                           /**< Not initialized.           */
  CRY_STOP = 1,                             /**< Stopped.                   */
  CRY_READY = 2                             /**< Ready.                     */
} crystate_t;

/**
 * @brief   Driver error codes.
 */
typedef enum {
  CRY_NOERROR = 0,                          /**< No error.                  */
  CRY_ERR_INV_ALGO = 1,                     /**< Invalid cypher/mode.       */
  CRY_ERR_INV_KEY_SIZE = 2,                 /**< Invalid key size.          */
  CRY_ERR_INV_KEY_TYPE = 3,                 /**< Invalid key type.          */
  CRY_ERR_INV_KEY_ID = 4                    /**< Invalid key type.          */
} cryerror_t;

/**
 * @brief   Type of an algorithm identifier.
 * @note    It is only used to determine the key required for operations.
 */
typedef enum {
  cry_algo_none = 0,
  cry_algo_aes,
  cry_algo_des,
  cry_algo_tripledes
} cryalgorithm_t;

#include "hal_crypto_lld.h"

#if !defined(CRY_LLD_SUPPORTS_AES_ECB) ||                                   \
    !defined(CRY_LLD_SUPPORTS_AES_CBC) ||                                   \
    !defined(CRY_LLD_SUPPORTS_AES_CFB) ||                                   \
    !defined(CRY_LLD_SUPPORTS_AES_CTR)
#error "CRYPTO LLD does not export required switches"
#endif

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    Low level driver helper macros
 * @{
 */
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void cryInit(void);
  void cryObjectInit(CRYDriver *cryp);
  void cryStart(CRYDriver *cryp, const CRYConfig *config);
  void cryStop(CRYDriver *cryp);

  cryerror_t cryLoadTransientKey(CRYDriver *cryp,
                                 cryalgorithm_t algorithm,
                                 size_t size,
                                 const uint8_t *keyp);

#if CRY_LLD_SUPPORTS_AES_ECB == TRUE
  cryerror_t cryEncryptAES_ECB(CRYDriver *cryp,
                               crykey_t key_id,
                               size_t size,
                               const uint8_t *in,
                               uint8_t *out);
  cryerror_t cryDecryptAES_ECB(CRYDriver *cryp,
                               crykey_t key_id,
                               size_t size,
                               const uint8_t *in,
                               uint8_t *out);
#endif /* CRY_LLD_SUPPORTS_AES_ECB == TRUE */

#if CRY_LLD_SUPPORTS_AES_CBC == TRUE
  cryerror_t cryEncryptAES_CBC(CRYDriver *cryp,
                               crykey_t key_id,
                               size_t size,
                               const uint8_t *in,
                               uint8_t *out,
                               const uint8_t *iv);
  cryerror_t cryDecryptAES_CBC(CRYDriver *cryp,
                               crykey_t key_id,
                               size_t size,
                               const uint8_t *in,
                               uint8_t *out,
                               const uint8_t *iv);
#endif /* CRY_LLD_SUPPORTS_AES_CBC == TRUE */

#if CRY_LLD_SUPPORTS_AES_CFB == TRUE
  cryerror_t cryEncryptAES_CFB(CRYDriver *cryp,
                               crykey_t key_id,
                               size_t size,
                               const uint8_t *in,
                               uint8_t *out,
                               const uint8_t *iv);
  cryerror_t cryDecryptAES_CFB(CRYDriver *cryp,
                               crykey_t key_id,
                               size_t size,
                               const uint8_t *in,
                               uint8_t *out,
                               const uint8_t *iv);
#endif /* CRY_LLD_SUPPORTS_AES_CFB == TRUE */

#if CRY_LLD_SUPPORTS_AES_CTR == TRUE
  cryerror_t cryEncryptAES_CTR(CRYDriver *cryp,
                               crykey_t key_id,
                               size_t size,
                               const uint8_t *in,
                               uint8_t *out,
                               const uint8_t *nonce,
                               uint8_t *cnt);
  cryerror_t cryDecryptAES_CTR(CRYDriver *cryp,
                               crykey_t key_id,
                               size_t size,
                               const uint8_t *in,
                               uint8_t *out,
                               const uint8_t *nonce,
                               uint8_t *cnt);
#endif /* CRY_LLD_SUPPORTS_AES_CTR == TRUE */
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_CRYPTO == TRUE */

#endif /* HAL_CRYPTO_H */

/** @} */