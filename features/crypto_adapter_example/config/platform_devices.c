/**
 ****************************************************************************************
 *
 * @file platform_devices.c
 *
 * @brief Configuration of devices connected to board.
 *
 * Copyright (C) 2017-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#include "ad_crypto.h"
#include "platform_devices.h"

#if (dg_configCRYPTO_ADAPTER == 1)

/* An arbitrary 128-bit Initialization Vector (IV) required for AES-CBC operations */
const uint8 iv_128b[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };


/* An arbitrary 128-bit Initialization Counter (IC) required for AES-CTR operations */
const uint8 ic_128b[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };


/* An arbitrary 128-bit key (base) used for AES operations  */
const uint8 key_128b[16] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                             0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };


/* An arbitrary 192-bit key (base) used for AES operations  */
const uint8 key_192b[24] = { 0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52,
                             0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5,
                             0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b };

ad_crypto_config_t crypto_aes_cfg = {
        .algo = AD_CRYPTO_ALGO_AES,
        .engine = {
                .aes = {
                        .mode = HW_AES_MODE_CBC,
                        .operation = HW_AES_OPERATION_ENCRYPT,
                        .key_size = HW_AES_KEY_SIZE_128,
                        .key_expand = HW_AES_KEY_EXPAND_BY_HW,
                        .keys_addr = (uint32_t)key_128b,
                        .output_data_mode = HW_AES_OUTPUT_DATA_MODE_ALL,
                        .wait_more_input = false,
                        .iv_cnt_ptr = iv_128b
                },
        },
};

ad_crypto_config_t crypto_hash_cfg = {
        .algo = AD_CRYPTO_ALGO_HASH,
        .engine = {
                .hash = {
                        .type = HW_HASH_TYPE_SHA_256,
                        .wait_more_input = false,
                        .output_data_len = HW_HASH_OUTPUT_LEN_MAX_SHA_256
                },
        },
};

#endif /* dg_configCRYPTO_ADAPTER */
