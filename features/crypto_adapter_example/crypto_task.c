/**
 ****************************************************************************************
 *
 * @file crypto_task.c
 *
 * @brief Crypto engine demonstration example
 *
 * Copyright (C) 2015-2023 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#include <string.h>
#include "osal.h"
#include "sys_watchdog.h"
#include "sys_clock_mgr.h"
#include "sys_power_mgr.h"
#include "misc.h"
#include "platform_devices.h"
#include "vectors.h" // Must be called only here!

#define CRYPTO_HASH_FRAGMENTED    ( 0 )
#define CRYPTO_AES_FRAGMENTED     ( 0 )
#define CRYPTO_AES_SECURE_KEY     ( 0 )
/* Up to 8 user data keys can be stored in secure are in eFLASH. */
#define CRYPTO_AES_SECURE_KEY_IDX ( 0 )

#define CRYPTO_AES_TASK_NOTIF    ( 1 << 0 )
#define CRYPTO_HASH_TASK_NOTIF   ( 1 << 0 )

__RETAINED OS_TASK crypto_aes_task_h;
__RETAINED OS_TASK crypto_hash_task_h;

static void ad_crypto_hash_cb(uint32_t status)
{
        /* Notify only if the last block has been processed. If more data are to be processed
         * the crypto engine should not be inactive. */
        if (status & HW_AES_HASH_IRQ_MASK_INACTIVE) {
                OS_TASK_NOTIFY_FROM_ISR(crypto_hash_task_h, CRYPTO_HASH_TASK_NOTIF, OS_NOTIFY_SET_BITS);
        }
}

static void ad_crypto_aes_cb(uint32_t status)
{
        /* Notify only if the last block has been processed. If more data are to be processed
         * the crypto engine should not be inactive. */
        if (status & HW_AES_HASH_IRQ_MASK_INACTIVE) {
                OS_TASK_NOTIFY_FROM_ISR(crypto_aes_task_h, CRYPTO_AES_TASK_NOTIF, OS_NOTIFY_SET_BITS);
        }
}

#if CRYPTO_AES_SECURE_KEY
#include "hw_fcu.h"
#include "eflash_automode.h"

extern const uint8 key_128b[16];

static uint32 crypto_aes_construct_key(const uint8 *data)
{
        if ((uint32)data & 0x3) {
                uint32 internal_buf;
                uint8 *p = (uint8 *)&internal_buf + 3;
                unsigned int i;

                for (i = 0; i < 4; i++) {
                        *(p--) = *(data++);
                }
                return internal_buf;
        } else {
                return SWAP32(*(uint32 *)data);
        }
}

static uint32_t crypto_aes_secure_key_read(uint8_t key_idx)
{
        /* For demonstration purposes we select the first user key index in eFLASH. Developers can select
         * any of the 8 user key entries. A zero address should be returned if the selected secure key
         * index is revoked (key index is zeroed) or an invalid key index is requested. */
        uint32_t key_address = hw_aes_key_address_get(CRYPTO_AES_SECURE_KEY_IDX);

        if (key_address) {
                /* If protected, only the secure DMA channel #5 can access the user keys area and
                 * the only valid destination is the crypto engine!!! */
                if (!hw_dma_is_aes_key_protection_enabled()) {
                        bool is_key_empty = true;
                        uint32_t user_data[MEMORY_EFLASH_USER_DATA_KEY_SIZE >> 2];

                        /* Get the contents of the selected secure key index. */
                        ASSERT_WARNING(hw_fcu_read(key_address - MEMORY_EFLASH_S_BASE, user_data,
                                                MEMORY_EFLASH_USER_DATA_KEY_SIZE >> 2, NULL) == HW_FCU_ERROR_NONE);

                        /* Check if the selected secure key has some contents. We assume that
                         * values other than 0xFFFFFFFF designate valid secure key payload. */
                        for (int i = 0; i < MEMORY_EFLASH_USER_DATA_KEY_SIZE >> 2; i++) {
                                if (user_data[i] != 0xFFFFFFFF) {
                                        is_key_empty = false;
                                        break;
                                }
                        }

                        /* If secure key payload is not written, write some contents. */
                        if (is_key_empty) {
                                /* Format key payload data. */
                                for (int i = 0, j = 0; i < sizeof(key_128b) >> 2 && j < sizeof(key_128b); i++, j += 4) {
                                        user_data[i] = crypto_aes_construct_key(key_128b + j);
                                }

                                ASSERT_WARNING(eflash_automode_write_page(key_address - MEMORY_EFLASH_S_BASE,
                                                      (const uint8_t *)user_data, sizeof(key_128b)) == sizeof(key_128b));

                                DBG_PRINTF("User keys area is unprotected and the selected key index is empty. Writing some contents...\n\r");
                        }
                }
        } else {
                DBG_PRINTF("\n\rThe selected key index is revoked or invalid.\n\r");
                ASSERT_WARNING(0);
        }

        return key_address;
}
#endif /* CRYPTO_AES_SECURE_KEY */

/**
 * @brief Task responsible for performing AES operations
 */
OS_TASK_FUNCTION(prvCRYPTO_AES, pvParameters)
{
        OS_BASE_TYPE ret;
        uint32_t notif;
        int8_t wdog_id;

        /* Buffer used to hold hash data */
        uint8_t crypto_aes_out[sizeof(crypto_aes_non_frag_vector)] = { 0 };

        ad_crypto_handle_t ad_crypto_aes_h;

        crypto_aes_task_h = OS_GET_CURRENT_TASK();

        /* Update the crypto driver's settings that can be changed by application */
#if CRYPTO_AES_SECURE_KEY
        CRYPTO_AES_DEVICE->engine.aes.keys_addr = crypto_aes_secure_key_read(0);
#endif
        CRYPTO_AES_DEVICE->engine.aes.callback = ad_crypto_aes_cb;
        CRYPTO_AES_DEVICE->engine.aes.output_data_addr = (uint32_t)crypto_aes_out;
        CRYPTO_AES_DEVICE->engine.aes.input_data_addr =
                        CRYPTO_AES_FRAGMENTED ? (uint32_t)crypto_aes_frag_vector_1 :
                                                (uint32_t)crypto_aes_non_frag_vector;
        CRYPTO_AES_DEVICE->engine.aes.input_data_len =
                        CRYPTO_AES_FRAGMENTED ? sizeof(crypto_aes_frag_vector_1) :
                                                sizeof(crypto_aes_non_frag_vector);
        CRYPTO_AES_DEVICE->engine.aes.wait_more_input =
                        CRYPTO_AES_FRAGMENTED ? true : false;

        /*
         * Open/initialize the crypto engine. The target SoC integrates a single crypto accelerator.
         * Keep in mind that the device cannot enter the extended sleep state as long as
         * an adapter instance is opened. Instead, the ARM WFI state is used when possible
         * e.g. waiting for the engine to perform its current cryptographic operation.
         */
        ad_crypto_aes_h = ad_crypto_open(CRYPTO_AES_DEVICE, OS_MUTEX_FOREVER);
        ASSERT_WARNING(ad_crypto_aes_h);

        /*
         * The crypto accelerator should not be configured based on its drivers settings
         * i.e.CRYPTO_AES_DEVICE. Initiate the cryptographic operation and wait for the
         * engine to complete its task. */
        ret = ad_crypto_perform_operation(OS_EVENT_FOREVER);
        ASSERT_WARNING(ret == OS_EVENT_SIGNALED);

#if CRYPTO_AES_FRAGMENTED
        app_crypto_frag_input_t aes_frag_in_vectors[] = {
                { (uint32_t)crypto_aes_frag_vector_2, sizeof(crypto_aes_frag_vector_2), true /*more data will follow*/ },
                { (uint32_t)crypto_aes_frag_vector_3, sizeof(crypto_aes_frag_vector_3), true /*more data will follow*/ },
                { (uint32_t)crypto_aes_frag_vector_4, sizeof(crypto_aes_frag_vector_4), false /*this should be the last block to be processed*/ }
        };

        /* Scan for all of the blocks (fragmented data) that should be processed */
        for (int i = 0; i < ARRAY_LENGTH(aes_frag_in_vectors); i++) {
                /* Update driver's settings required to process a next data block. */
                CRYPTO_AES_DEVICE->engine.aes.input_data_addr = aes_frag_in_vectors[i].src;
                CRYPTO_AES_DEVICE->engine.aes.input_data_len = aes_frag_in_vectors[i].len;
                CRYPTO_AES_DEVICE->engine.aes.wait_more_input = aes_frag_in_vectors[i].wait_more_input;

                ad_crypto_configure_for_next_fragment(CRYPTO_AES_DEVICE);

                ret = ad_crypto_perform_operation(OS_EVENT_FOREVER);
                ASSERT_WARNING(ret == OS_EVENT_SIGNALED);
        }
#endif

        /* The requested input vector should now be encrypted and the results should be stored in
         * 'crypto_aes_out'. Now, it's time to perform the opposite operation so encrypted data
         * are translated back to raw/plain data. */
        CRYPTO_AES_DEVICE->engine.aes.operation = HW_AES_OPERATION_DECRYPT;
        /* It's OK for the source and destination buffers to match! */
        CRYPTO_AES_DEVICE->engine.aes.input_data_addr = (uint32_t)crypto_aes_out;
        CRYPTO_AES_DEVICE->engine.aes.input_data_len = sizeof(crypto_aes_out);
        CRYPTO_AES_DEVICE->engine.aes.wait_more_input = false;

        /* Should be called when the crypto adapter is already opened and driver's settings
         * should be updated. */
        ad_crypto_configure(CRYPTO_AES_DEVICE);

        ret = ad_crypto_perform_operation(OS_EVENT_FOREVER);
        ASSERT_WARNING(ret == OS_EVENT_SIGNALED);

        /* The crypto engine is no longer used. Close it so the device is allowed to enter
         * the sleep state and it's SW resources are released.  */
        ad_crypto_close();

        wdog_id = sys_watchdog_register(false);

        for (;;) {

                sys_watchdog_notify(wdog_id);
                /* Remove task id from the monitoring list as long as a task remains in the blocking state. */
                sys_watchdog_suspend(wdog_id);

                ret = OS_TASK_NOTIFY_WAIT(OS_TASK_NOTIFY_ALL_BITS, 0, &notif, OS_TASK_NOTIFY_FOREVER);
                OS_ASSERT(ret == OS_TASK_NOTIFY_SUCCESS);

                sys_watchdog_notify_and_resume(wdog_id);

                if (notif & CRYPTO_AES_TASK_NOTIF) {
                        /* Compare the output of de-cryption with the dummy vector */
                        if (!memcmp(crypto_aes_out, crypto_aes_non_frag_vector, sizeof(crypto_aes_out))) {
                                DBG_PRINTF("\n\rSuccessful AES Operation\n\r");
                        } else {
                                DBG_PRINTF("\n\rUnsuccessful AES Operation\n\r");
                        }
                }
        }
}


/**
 * @brief Task responsible for performing HASH operations
 */
OS_TASK_FUNCTION(prvCRYPTO_HASH, pvParameters)
{
        OS_BASE_TYPE ret;
        uint32_t notif;
        int8_t wdog_id;
        uint8 crypto_hash_out[HW_HASH_OUTPUT_LEN_MAX_SHA_256] = { 0 };
        ad_crypto_handle_t ad_crypto_hash_h;

        crypto_hash_task_h = OS_GET_CURRENT_TASK();

        /* Update the crypto driver's settings that can be changed by application */
        CRYPTO_HASH_DEVICE->engine.hash.callback = ad_crypto_hash_cb;
        CRYPTO_HASH_DEVICE->engine.hash.output_data_addr = (uint32_t)crypto_hash_out;
        CRYPTO_HASH_DEVICE->engine.hash.input_data_addr =
                        CRYPTO_HASH_FRAGMENTED ? (uint32_t)crypto_hash_frag_vector_1 :
                                                 (uint32_t)crypto_hash_non_frag_vector;
        CRYPTO_HASH_DEVICE->engine.hash.input_data_len =
                        CRYPTO_HASH_FRAGMENTED ? (uint32_t)sizeof(crypto_hash_frag_vector_1) - 1 :
                                                 (uint32_t)sizeof(crypto_hash_non_frag_vector) - 1;
        CRYPTO_HASH_DEVICE->engine.hash.wait_more_input =
                        CRYPTO_HASH_FRAGMENTED ? true : false;

        /*
         * Open/initialize the crypto engine. The target SoC integrates a single crypto accelerator.
         * Keep in mind that the device cannot enter the extended sleep state as long as
         * an adapter instance is opened. Instead, the ARM WFI state is used when possible
         * e.g. waiting for the engine to perform its current cryptographic operation.
         */
        ad_crypto_hash_h = ad_crypto_open(CRYPTO_HASH_DEVICE, OS_MUTEX_FOREVER);
        ASSERT_WARNING(ad_crypto_hash_h);

        /*
         * The crypto accelerator should not be configured based on its drivers settings
         * i.e.CRYPTO_HASH_DEVICE. Initiate the hashing operation and wait for the
         * engine to complete its task. */
        ret = ad_crypto_perform_operation(OS_EVENT_FOREVER);
        ASSERT_WARNING(ret == OS_EVENT_SIGNALED);

#if CRYPTO_HASH_FRAGMENTED
        app_crypto_frag_input_t hash_frag_in_vectors[] = {
                { (uint32_t)crypto_hash_frag_vector_2, (uint32_t)sizeof(crypto_hash_frag_vector_2) - 1, true },
                { (uint32_t)crypto_hash_frag_vector_3, (uint32_t)sizeof(crypto_hash_frag_vector_3) - 1, false },
        };

        /* Scan for all of the blocks (fragmented data) that should be processed */
        for (int i = 0; i < ARRAY_LENGTH(hash_frag_in_vectors); i++) {
                /* Update driver's settings required to process a next data block. */
                CRYPTO_HASH_DEVICE->engine.hash.input_data_addr = hash_frag_in_vectors[i].src;
                CRYPTO_HASH_DEVICE->engine.hash.input_data_len = hash_frag_in_vectors[i].len;
                CRYPTO_HASH_DEVICE->engine.hash.wait_more_input = hash_frag_in_vectors[i].wait_more_input;

                ad_crypto_configure_for_next_fragment(CRYPTO_HASH_DEVICE);

                ret = ad_crypto_perform_operation(OS_EVENT_FOREVER);
                ASSERT_WARNING(ret == OS_EVENT_SIGNALED);
        }
#endif

        /* The crypto engine is no longer used. Close it so the device is allowed to enter
         * the sleep state and it's SW resources are released.  */
        ad_crypto_close();

        wdog_id = sys_watchdog_register(false);

        for (;;) {

                sys_watchdog_notify(wdog_id);
                /* Remove task id from the monitoring list as long as a task remains in the blocking state. */
                sys_watchdog_suspend(wdog_id);

                ret = OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_TASK_NOTIFY_FOREVER);
                OS_ASSERT(ret == OS_TASK_NOTIFY_SUCCESS);

                sys_watchdog_notify_and_resume(wdog_id);

                if (notif & CRYPTO_HASH_TASK_NOTIF) {
                        /* Compare the results with the pre-calculated hashing data */
                        if (!memcmp(crypto_hash_out, hash_sha_256_precomputed_32_bytes, HW_HASH_OUTPUT_LEN_MAX_SHA_256)) {
                                DBG_PRINTF("\n\rSuccessful HASH Operation\n\r");
                        } else {
                                DBG_PRINTF("\n\rUnsuccessful HASH Operation\n\r");
                        }
                        DBG_PRINTF("\n\r");
                }
        }
}
