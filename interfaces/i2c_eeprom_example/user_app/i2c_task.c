 /****************************************************************************************
 *
 * @file i2c_tack.c
 *
 * @brief I2C master tasks
 *
 * Copyright (c) 2023 Renesas Electronics Corporation and/or its affiliates
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
 *
 ****************************************************************************************
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <ad_i2c.h>
#include "osal.h"

#include "peripheral_setup.h"
#include "platform_devices.h"
#include "i2c_task.h"

#include "app_gpio.h"

static HW_I2C_ABORT_SOURCE I2C_error_code;

/* Semaphore for when EEPROM is ready for the next i2c transaction */
__RETAINED static OS_EVENT i2c_eeprom_event;

/*
 * DEFINES
 ****************************************************************************************
 */
#define BUFFER_SIZE             (256)

// Struct for storing the write data and handles to write to the EEPROM
typedef struct {
	ad_i2c_handle_t* master_handle;
	uint8_t current_write_address_and_bytes[17];
    const uint8_t* write_byte_array;
    const uint8_t* write_start_address;
    const uint16_t* write_byte_amount;
    OS_TIMER* write_delay_handle;
} str_writing_bytes_i2c;

/*
 * VARIABLE DEFINITIONS
 ****************************************************************************************
 */
// variable for the write using the ad_i2c functions
static str_writing_bytes_i2c str_wr_bytes;

// variables for the read using the ad_i2c functions
static uint8_t i2c_eeprom_read_address[1];

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
bool write_bytes_eeprom_async(str_writing_bytes_i2c* _str_wr_bytes);

/**
 ****************************************************************************************
 * @brief I2C EEPROM timer done ISR.
 * I2C EEPROM is ready for next read or write transaction.
 * @param[in|out] osTimer    	Timer handle not used
 ****************************************************************************************
 */
void _i2c_eeprom_write_ready(OS_TIMER osTimer){
	/* Signal that the EEPROM is ready. */
	OS_EVENT_SIGNAL_FROM_ISR(i2c_eeprom_event);
}

/**
 ****************************************************************************************
 * @brief I2C write done ISR.
 * I2C bus is free. If delay after write is not set there will be signaled that
 * EEPROM is ready for next read or write transaction.
 ****************************************************************************************
 */
void _i2c_master_async_write_done(void *user_data, HW_I2C_ABORT_SOURCE error) {
	/* Read the error status code */
	I2C_error_code = error;

	str_writing_bytes_i2c *_str_wr_bytes = user_data;

	if (I2C_error_code == HW_I2C_ABORT_NONE) {
		// Only if delay after write is set
		if (_str_wr_bytes->write_delay_handle != NULL) {
			// Start the timer to start next write, when the timer runs out.
			OS_TIMER_START_FROM_ISR(*_str_wr_bytes->write_delay_handle);
		} else {
			// Start next write with no delay between the writes
			/* Signal that the EEPROM is ready. */
			OS_EVENT_SIGNAL_FROM_ISR(i2c_eeprom_event);
		}
	} else {
		/* Signal that the EEPROM is ready. */
		OS_EVENT_SIGNAL_FROM_ISR(i2c_eeprom_event);
	}
}

/**
 ****************************************************************************************
 * @brief I2C read done ISR.
 * I2C bus is free and ready for new read or write
 ****************************************************************************************
 */
void _i2c_master_async_read_done(void *user_data, HW_I2C_ABORT_SOURCE error)
{
	/* Read the error status code */
	I2C_error_code = error;

	/* Signal that the EEPROM is ready. */
	OS_EVENT_SIGNAL_FROM_ISR(i2c_eeprom_event);
}

/**
 ****************************************************************************************
 * @brief Write byte non blocking.
 * @param[in] _str_wr_bytes    	Struct for storing the write data and handles to write
 * 								to the EEPROM
 ****************************************************************************************
 */
bool write_bytes_eeprom_async(str_writing_bytes_i2c* _str_wr_bytes){
	static uint16_t bytes_written = 0;

	uint16_t byte_amount_to_write = *_str_wr_bytes->write_byte_amount - bytes_written;

	// Write start address
	_str_wr_bytes->current_write_address_and_bytes[0] = *_str_wr_bytes->write_start_address + bytes_written;

	uint8_t i;
	for (i = 0; i < 16 && i < byte_amount_to_write; i++){
		_str_wr_bytes->current_write_address_and_bytes[1 + i] = _str_wr_bytes->write_byte_array[bytes_written+i];
	}

	bytes_written = bytes_written + i;

	I2C_error_code = ad_i2c_write_async(*_str_wr_bytes->master_handle,					// I2C Handle
									_str_wr_bytes->current_write_address_and_bytes,		// Write data
									1 + i,												// Write size
									_i2c_master_async_write_done,						// Callback function
									_str_wr_bytes,										// Callback argument
									HW_I2C_F_ADD_STOP);									// Add stop

	if (I2C_error_code != HW_I2C_ABORT_NONE) {
		printf("ad_i2c_write_async I2C_error_code: 0x%02X", I2C_error_code);
	}
	if (bytes_written >= *_str_wr_bytes->write_byte_amount){
		bytes_written = 0;
	}

	// return 1 when all bytes are transmitted
	return bytes_written == 0;
}

/**
 ****************************************************************************************
 * @brief Blocking write bytes.
 * @param[in] _str_wr_bytes    	Struct for storing the write data and handles to write
 * 								to the EEPROM
 ****************************************************************************************
 */
void write_bytes_eeprom_blocking(str_writing_bytes_i2c* _str_wr_bytes){
	uint16_t bytes_written = 0;

	for (uint16_t byte_amount_to_write = *_str_wr_bytes->write_byte_amount;
			byte_amount_to_write > 0;
			byte_amount_to_write = *_str_wr_bytes->write_byte_amount - bytes_written) {

		// Write start address
		_str_wr_bytes->current_write_address_and_bytes[0] = *_str_wr_bytes->write_start_address + bytes_written;

		// Amount of bytes going to be written on i2c bus in this transaction
		uint8_t i2c_write_amount;
		for (i2c_write_amount = 0;
				i2c_write_amount < 16 && i2c_write_amount < byte_amount_to_write;
				i2c_write_amount++) {
			_str_wr_bytes->current_write_address_and_bytes[1 + i2c_write_amount] =
					_str_wr_bytes->write_byte_array[bytes_written
							+ i2c_write_amount];
		}

		bytes_written = bytes_written + i2c_write_amount;

		// write bytes
		I2C_error_code = ad_i2c_write(*_str_wr_bytes->master_handle,						// I2C Handle
										_str_wr_bytes->current_write_address_and_bytes,		// Write data
										1 + i2c_write_amount,								// Write size
										HW_I2C_F_ADD_STOP);									// Add stop

		if (I2C_error_code != HW_I2C_ABORT_NONE){
			break;
		}

		// Delay needed for EEPROM, because of the write cycle time, when new write is
		// started before write cycle time is done the EEPROM will respond with NAK
		OS_DELAY_MS(12); //Use 12 ms delay instead of 5 ms, because of inaccurate delay
	}
}

/**
 ****************************************************************************************
 * @brief Test write bytes.
 * @param[in] _master_handle    		ad_i2c handle
 * @param[in] _write_byte_array         Pointer to the array of bytes to be written
 * @param[in] _write_start_address      Starting address of the write process
 * @param[in] _write_byte_amount        Size of the data to be written
 * @param[in] _write_delay_handle		Set so that consecutive writes will be done after
 * 										the set byte write cycle time. NULL to skip the
 * 										delay.
 ****************************************************************************************
 */
void test_write_bytes(ad_i2c_handle_t *_master_handle,
		uint8_t *_write_byte_array, uint8_t *_write_start_address,
		uint16_t *_write_byte_amount, OS_TIMER *_write_delay_handle) {
	// address to write out of memory EEPROM.
	if ((*_write_start_address + *_write_byte_amount) > BUFFER_SIZE) {
		printf(
				" - Test error! - writing at address: 0x%02X not possible, lower address or size\n\r",
				*_write_start_address + *_write_byte_amount);
	}
	else {
#if (I2C_ASYNC_EN)
		/* Wait until the EEPROM is ready. */
		OS_EVENT_WAIT(i2c_eeprom_event, OS_EVENT_FOREVER);

#endif // #if (I2C_ASYNC_EN)

		// set writing data
		str_wr_bytes.master_handle = _master_handle;
		str_wr_bytes.write_byte_array = _write_byte_array;
		str_wr_bytes.write_byte_amount = _write_byte_amount;
		str_wr_bytes.write_start_address = _write_start_address;
		str_wr_bytes.write_delay_handle = _write_delay_handle;

#if (I2C_ASYNC_EN)
		while (write_bytes_eeprom_async(&str_wr_bytes) != 1){
			// Let OS scheduler yield the current task immediately
			OS_DELAY_MS(1);
			/* Wait until the EEPROM is ready. */
			OS_EVENT_WAIT(i2c_eeprom_event, OS_EVENT_FOREVER);
		}
#else
		write_bytes_eeprom_blocking(&str_wr_bytes);
#endif // #if (I2C_ASYNC_EN)
	}
}

/**
 ****************************************************************************************
 * @brief Test read bytes.
 * When I2C_ASYNC_EN is set 1, this function is non blocking, if EEPROM is ready,
 * otherwise will wait at OS_EVENT_WAIT, until EEPROM is ready.
 * @param[in] _master_handle    		ad_i2c handle
 * @param[in] _read_bytes         		Pointer to store the read bytes
 * @param[in] _read_address      		Starting address of the read bytes
 * @param[in] _read_size        		Size of the data to be read
 ****************************************************************************************
 */
void read_bytes_eeprom(ad_i2c_handle_t *_master_handle, uint8_t *_read_bytes,
		uint8_t _read_address, uint16_t _read_size) {
#if (I2C_ASYNC_EN)
	/* Wait until the EEPROM is ready. */
	OS_EVENT_WAIT(i2c_eeprom_event, OS_EVENT_FOREVER);
#endif // #if (I2C_ASYNC_EN)

	/* the reading address is set by writing to the EEPROM, writing only the address
	 * (this address is now stored in the read register), and then switching to read mode
	 * and reading for the amount of addresses needed to be read (the read address in the
	 * EEPROM is auto incremented after every read. */
	i2c_eeprom_read_address[0] = _read_address;

#if (I2C_ASYNC_EN)
	I2C_error_code = ad_i2c_write_read_async(*_master_handle,	// I2C Handle
								 i2c_eeprom_read_address,		// Write start address to read
								 1,								// Write size
								 _read_bytes,					// Pointer to store the read bytes
								 _read_size,					// Amount of bytes to read
								 _i2c_master_async_read_done,	// Callback
								 NULL,							// Callback argument
								 HW_I2C_F_ADD_STOP);			// Add stop condition after read

#else
	I2C_error_code = ad_i2c_write_read(*_master_handle,			// I2C Handle
								 i2c_eeprom_read_address,		// Write start address to read
								 1,								// Write size
								 _read_bytes,					// Pointer to store the read bytes
								 _read_size,					// Amount of bytes to read
								 HW_I2C_F_ADD_STOP);			// Add stop condition after read
#endif // #if (I2C_ASYNC_EN)
}

/**
 ****************************************************************************************
 * @brief Test print read bytes.
 * This function will not check if data from EEPROM is still being written to data array
 * @param[in] _read_bytes         		Pointer to the read bytes
 * @param[in] _read_address      		Starting address of the read bytes
 * @param[in] _read_size        		Size of the data to be printed
 ****************************************************************************************
 */
void test_print_read_data(uint8_t *_read_bytes, uint8_t _read_address, uint16_t _read_size) {
	// Display Results
	printf("Read data, from address 0x%02X:", _read_address);
	for (uint16_t i = 0; i < _read_size; i++) {
		printf(" %02X", _read_bytes[i]);
	}
	printf("\n\r");
}

OS_TASK_FUNCTION (i2c_master_task, pvParameters )
{
	printf("\n\r\n\r   I2C init\n\r");

    /* Turn off write protection. */
    app_gpio_set_inactive(I2C_WRITE_PROTECT, 1);

	ad_i2c_handle_t _master_handle = ad_i2c_open(I2C_MASTER_DEVICE);

#if (I2C_ASYNC_EN)
	// create semaphore for when EEPROM is ready for the next i2c transaction.
	OS_EVENT_CREATE(i2c_eeprom_event);

	/* Signal that the I2C transaction is completed. Because i2c master
	 * is initialized, and no transaction is started */
	OS_EVENT_SIGNAL(i2c_eeprom_event);
#endif // #if (I2C_ASYNC_EN)

	// Variables used, for calling the test functions
	uint8_t test_address = 0;
	uint16_t test_size = 0;
	uint16_t test_wr_size = BUFFER_SIZE; // the desired write size
	uint16_t test_rd_size = BUFFER_SIZE; // the desired read size
	uint8_t test_wr_data[BUFFER_SIZE];
	uint8_t test_rd_data[BUFFER_SIZE];

	// Populate write vector
	for (uint16_t i = 0; i < test_wr_size; i++) {
		test_wr_data[i] = i; // so this will write values 0x00-FF to EEPROM.
	}
	// Reset read vector
	for (uint16_t i = 0; i < test_rd_size; i++) {
		test_rd_data[i] = 0;
	}

	OS_TIMER eeprom_write_delay;
	/* Initialise OS_TIMER_CREATE - this only needs to be done once.
	 * Delay for EEPROM so that consecutive writes will be done after the write cycle time delay. */
	eeprom_write_delay = OS_TIMER_CREATE(
    	        "i2c_write_delay",              // Name for the timer (optional).
				OS_MS_2_TICKS(12),    			// Timer period in ticks, use 12 ms delay
													// instead of 5 ms, because of inaccurate delay
				OS_TIMER_ONCE,                 	// Set timer as one-shot.
				( void * ) 0,    				// Timer ID
				_i2c_eeprom_write_ready);		// Callback function to execute.
	if (eeprom_write_delay == NULL) {
		printf("\n\rTimer error, timer will not be used\n\r");
	}

	uint8_t counter_test_transaction = 0;

	for (;;) {
		OS_DELAY_MS(TASK_FREQUENCY_MS);
		counter_test_transaction++;

		switch (counter_test_transaction) {
		case 1:
			// Page Write
			printf("\n\r*** Test 1 ***\n\r");
			printf("Writing page to EEPROM (values 0x00-FF)...\n\r");
			test_address = 0;
			test_write_bytes(&_master_handle, test_wr_data, &test_address,
					&test_wr_size, &eeprom_write_delay);
			break;

		case 2:
			// Page Read
			printf("\n\r*** Test 2 ***\n\r");
			printf("Reading page from EEPROM\n\r");
			test_address = 0;
			read_bytes_eeprom(&_master_handle, test_rd_data, test_address, test_rd_size);
			test_print_read_data(test_rd_data, test_address, test_rd_size);
			break;

		case 3:
			// Populate write vector
			for (uint16_t i = 0; i < test_wr_size; i++) {
				// so this will write value 0x00 to EEPROM.
				test_wr_data[i] = 0;
			}

			// Page Write
			printf("\n\r*** Test 3 ***\n\r");
			printf("Writing page to EEPROM (value: 0x00)...\n\r");
			test_address = 0;
			test_write_bytes(&_master_handle, test_wr_data, &test_address,
					&test_wr_size, &eeprom_write_delay);
			break;

		case 4:
			// Page Read
			printf("\n\r*** Test 4 ***\n\r");
			printf("Reading page from EEPROM\n\r");
			test_address = 0;
			read_bytes_eeprom(&_master_handle, test_rd_data, test_address, test_rd_size);
			test_print_read_data(test_rd_data, test_address, test_rd_size);
			break;

		case 5:
			// Random write Byte
			printf("\n\r*** Test 5 ***\n\r");
			test_size = 1;

			test_address = 22;
			test_wr_data[0] = 0xAA;
			printf("Write byte, to address 0x%02X: %02X\n\r", test_address, test_wr_data[0]);
			test_write_bytes(&_master_handle, test_wr_data, &test_address,
					&test_size, &eeprom_write_delay);

			test_address = 32;
			test_wr_data[0] = 0xA9;
			printf("Write byte, to address 0x%02X: %02X\n\r", test_address, test_wr_data[0]);
			test_write_bytes(&_master_handle, test_wr_data, &test_address,
					&test_size, &eeprom_write_delay);

			test_address = 12;
			test_wr_data[0] = 0xA8;
			printf("Write byte, to address 0x%02X: %02X\n\r", test_address, test_wr_data[0]);
			test_write_bytes(&_master_handle, test_wr_data, &test_address,
					&test_size, &eeprom_write_delay);
			break;

		case 6:
			// Random Read Byte
			printf("\n\r*** Test 6 ***\n\r");
			test_size = 1;

			test_address = 22;
			read_bytes_eeprom(&_master_handle, test_rd_data, test_address, test_size);
			test_print_read_data(test_rd_data, test_address, test_size);

			test_address = 32;
			read_bytes_eeprom(&_master_handle, test_rd_data, test_address, test_size);
			test_print_read_data(test_rd_data, test_address, test_size);

			test_address = 12;
			read_bytes_eeprom(&_master_handle, test_rd_data, test_address, test_size);
			test_print_read_data(test_rd_data, test_address, test_size);
			break;

		case 7:
		    /* Turn on write protection. */
		    app_gpio_set_active(I2C_WRITE_PROTECT, 1);
		    break;

		default:
			counter_test_transaction = 10;
			printf(".");
			break;
		}
	}
	OS_TIMER_DELETE(eeprom_write_delay, 0);
	OS_EVENT_DELETE(i2c_eeprom_event);
	ad_i2c_close(_master_handle, true);
	OS_TASK_DELETE(NULL); //should never get here
}
