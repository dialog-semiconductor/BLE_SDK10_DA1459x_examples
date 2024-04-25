/**
 ****************************************************************************************
 *
 * @file platform_devices.h
 *
 * @brief Configuration of devices connected to board.
 *
 * Copyright (C) 2016-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef PLATFORM_DEVICES_H_
#define PLATFORM_DEVICES_H_

#include "ad_crypto.h"

#if (dg_configCRYPTO_ADAPTER == 1)

extern ad_crypto_config_t crypto_aes_cfg;
extern ad_crypto_config_t crypto_hash_cfg;

#define CRYPTO_AES_DEVICE   (&crypto_aes_cfg)
#define CRYPTO_HASH_DEVICE   (&crypto_hash_cfg)

/*
 * @brief Structure used for holding the fragmented vectors
 */
typedef struct app_crypto_frag_input {
       uint32_t src;
       uint32_t len;
       bool wait_more_input;
} app_crypto_frag_input_t;

#endif

#endif /* PLATFORM_DEVICES_H_ */
