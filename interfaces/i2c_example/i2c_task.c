 /****************************************************************************************
 *
 * @file i2c_tack.c
 *
 * @brief I2C master tasks
 *
 * Copyright (C) 2020-2023 Renesas Electronics Corporation and/or its affiliates
 * The MIT License (MIT)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 ****************************************************************************************
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <ad_i2c.h>

#include "peripheral_setup.h"
#include "platform_devices.h"
#include "osal.h"
#include "i2c_task.h"

/* Retained symbols */
__RETAINED static OS_EVENT i2c_event;
__RETAINED static HW_I2C_ABORT_SOURCE I2C_error_code;


void _i2c_master_async_write_done(void *user_data, HW_I2C_ABORT_SOURCE error)
{
        /* Read the error status code */
        I2C_error_code = error;

        /* Signal the master task that time for resuming has elapsed. */
        OS_EVENT_SIGNAL_FROM_ISR(i2c_event);
}

void _i2c_master_async_read_done(void *user_data, HW_I2C_ABORT_SOURCE error)
{
        /* Read the error status code */
        I2C_error_code = error;

        /* Signal the master task that time for resuming has elapsed. */
        OS_EVENT_SIGNAL_FROM_ISR(i2c_event);

}


OS_TASK_FUNCTION (i2c_master_task, pvParameters )
{

        printf("I2C init \n\r");
        uint8_t resp[I2C_RESPONSE_SIZE];
        uint16_t hum_data, temp_data;
        ad_i2c_handle_t _master_handle = ad_i2c_open(I2C_MASTER_DEVICE);

        OS_TICK_TIME xNextWakeTime;
        static uint32_t loop_counter=0;
        static uint32_t transaction_counter=0;


        /* Initialise i2c_event semaphore*/
        OS_EVENT_CREATE(i2c_event);

        /* Initialise xNextWakeTime - this only needs to be done once. */
        xNextWakeTime = OS_GET_TICK_COUNT();

        for ( ;; ) {

                vTaskDelayUntil( &xNextWakeTime, COUNTER_FREQUENCY_MS );
                loop_counter++;
                if (loop_counter % (1000 / OS_TICKS_2_MS(COUNTER_FREQUENCY_MS)) == 0) {
                        transaction_counter++;
                        unsigned char * _req = (unsigned char *)"0";
                        printf("Write I2C: %s \n\r", _req); //Wake Up HS3001

#if (I2C_ASYNC_EN)
                        I2C_error_code = ad_i2c_write_async(_master_handle,
                                                 _req,
                                                 I2C_REQUEST_SIZE,
                                                 _i2c_master_async_write_done,
                                                 NULL,
                                                 HW_I2C_F_ADD_STOP);

        /*
         * In the meantime and while I2C transactions are performed in the background,
         *ad_i2c_user_cb application task can proceed to other operations/calculation.
         * It is essential that, the new operations do not involve I2C transactions
         * on the already occupied bus!!!
         */
                        OS_EVENT_WAIT(i2c_event, OS_EVENT_FOREVER);
			vTaskDelayUntil(&xNextWakeTime, OS_TICKS_2_MS(40)); //delay wait as per datasheet

			I2C_error_code = ad_i2c_read_async(_master_handle,
                                                   resp,
                                                   I2C_RESPONSE_SIZE,
                                                   _i2c_master_async_read_done,
                                                   NULL,
                                                   HW_I2C_F_ADD_STOP);
			OS_EVENT_WAIT(i2c_event, OS_EVENT_FOREVER);
#else
                 I2C_error_code = ad_i2c_write_read(_master_handle,
                                                   _req,
                                                   I2C_REQUEST_SIZE,//reduce this to simulate incomplete send
                                                   resp,
                                                   I2C_RESPONSE_SIZE,
                                                   HW_I2C_F_ADD_STOP);
#endif
                        if(0 == I2C_error_code){
                                /*
                                 * Data conversion according to the formulas provided
                                 * in the HS3001 datasheet.
                                */
                                hum_data = (resp[0]<<8) + resp[1];
                                uint32_t hum_data_result = hum_data*100000/16383;
                                hum_data_result /=1000;
                                temp_data = (resp[2]<<8) + resp[3];
                                temp_data >>=2;
                                uint32_t temp_data_result = temp_data*1000*165/16383;
                                temp_data_result /= 1000;
                                temp_data_result -=40;
                                printf("Successfully read I2C \n\r");
                                printf("Hum: %ld Temp: %ld \n\r", hum_data_result, temp_data_result);

                        } else {
                                printf("i2c read error(%d)\n", I2C_error_code);
                        }
                        fflush(stdout);
                }
        }
        ad_i2c_close(_master_handle, true);
        OS_TASK_DELETE( NULL ); //should never get here
}
