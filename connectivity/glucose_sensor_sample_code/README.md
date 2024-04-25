# Glucose Service (GLS) Sample Code

This sample code provides support for the Bluetooth Glucose Service (GLS) and demonstrates a Glucose Profile (GLP) demonstration example. The following screenshot depicts the file structure of the sample code:

 <img src="assets\gls_file_structure.PNG" style="zoom: 20%;" />

1. The Bluetooth service has been implemented in such a way so that optional features/properties can be enabled/disabled at build time, per demand. By default, all optional features/properties are disabled. This file contains configuration macros to handle all optional features/properties. Please note that this file should not be modified by users in case one or more optional properties are to be modified.
2. These files contain the core implementation of the Bluetooth Glucose Service and provides the API needed for the application to setup the Glucose Service database and properly handle requests raised by peer devices (GLS collectors). 
3. The GLS database should be handled on application level by users. That is the, core service implementation is not responsible to store or handle any characteristic values. This gives the flexibility to end users to construct the database based on their application needs and select the storage medium that best fits their application. These files contain a simple database framework that uses linked lists to dynamically add a record in the database  and handle various requests raised by peer devices (GLS collectors).  Customers can modify the existing implementation to meet their application needs, if needed.
4. This contains the main application task that sets up a Glucose Service database and handles the various requests raised by peer devices with the help of the demonstrated database framework API. 
5. This file should be used to overwrite default macro configurations values related to optional properties/features as defined in `glucose_service_default.h`. 

### Glucose Service API Usage

This section describes how the Bluetooth Glucose Service database can be set up and the steps required for an application to follow in order to handle the various incoming requests related to the Record Access Control Point characteristic.

1. First step for a device that implements the Glucose Service meter role is to register the Glucose Service to the underlying Bluetooth stack. In theory, multiple Glucose Services instances can be created but in practice only one instance is needed. The following code snippet depicts how a Glucose Service instance can be created and registered. 

        __RETAINED_RW static gls_callbacks_t gls_callbacks = {
            .event_sent = event_sent_cb,
        #if GLS_FEATURE_INDICATION_PROPERTY
            .get_features = get_features_cb,
        #endif
            .racp_callbacks = {
        		.report_num_of_records = report_num_of_records_cb,
        		.report_records = report_records_cb,
        		.abort_operation = abort_operation_cb,
        #if GLS_RACP_COMMAND_DELETE_STORED_RECORDS_SUPPORT
        		.delete_records = delete_records_cb,
        #endif
                },
        };
        
        gls = gls_init((const gls_callbacks_t *)&gls_callbacks);
        OS_ASSERT(gls != NULL);

As depicted above the initialization routine requires a structure of callback functions. Some of these functions are mandatory, others are mandatory when specific criteria are met and others are optional and can be omitted. To facilitate users, different callback functions are exposed per RACP op codes/commands so users do not have to parse the op code values on their own. The following section further analyzes the conditions under which these callback functions are triggered and what actions are expected to take place.

##### RACP - Report Number of Stored Records

a. When a command that requests the number of stored records is received and is valid then the server will call the registered `report_num_of_records` callback function. 

b. It is strongly advised that request handling is differed to the main application task by sending notifications.  The `gls_racp_t` structure that contains the requested operator and operand values should be stored so they are available in the main task's context.

c. Application should report back to the peer device the number of stored records based on the operator and filter type values. As per the specifications, the value should be reported in form of indications. The latter can be achieved by calling `gls_indicate_number_of_stored_records`. 

Following is a screenshot  that depicts the key parts of handling requests that deal with reporting the number of stored records.   

    /* This callback will be called by server upon reception of a command that requests
       the number of stored records. */
    static void report_num_of_records_cb(...)
    {
    	...
        /* Parsing should be done in the application's task context so BLE Manager's task            can process other BLE events. Defer further processing by sending    
           notifications. */
        OS_TASK_NOTIFY(...);
    }
    
    OS_TASK_FUNCTION(glucose_sensor_task, params)
    {
    	...
    	
    	for (;;) {
    		...
    	    if (notif & RACP_REPORT_NUM_NOTIF) {
                 /* Application should indicate the number of records based on
                    the operator and filter type values. */
                    gls_indicate_number_of_stored_records(...);
            }
    	}
    }
> Callback functions are called within the Bluetooth manager task which has higher priority compared to application task. It's  strongly recommended that handling is differed to the main application task so the manager task is freed and is able to execute other incoming Bluetooth events. 

##### RACP - Delete Stored Records

a. When a command that requests to delete stored records is received and is valid then the server will call the registered `delete_records` callback function. 

b. It is strongly advised that request handling is differed to the main application task by sending notifications.  The `gls_racp_t` structure that contains the requested operator and operand values should be stored so they are available in the main task's context.

c. Application should delete all records that meet the criteria specified by the operator and filter type values. As per the specifications, deleting stored records is optional and application might not proceed with deleting any record. Once all deletions are performed the application should indicate the success or failure of deletion. The latter can be achieved by calling `gls_indicate_delete_records_status`. 

The command is optional and the callback function is expected to be called when:

```
#define GLS_RACP_COMMAND_DELETE_STORED_RECORDS_SUPPORT  ( 1 )
```

Following is a screenshot  that depicts the key parts of handling requests that deal with deleting stored records:	

	/* This callback will be called by server upon reception of a command that requests
	   deletion of stored records. */
	static void delete_records_cb(...)
	{
		...
	    /* Parsing should be done in the application's task context so BLE Manager's task            can process other BLE events. Defer further processing by sending    
	       notifications. */
	    OS_TASK_NOTIFY(...);
	}
	
	OS_TASK_FUNCTION(glucose_sensor_task, params)
	{
		...
	    for (;;) {
	        ...
	        if (notif & RACP_DELETE_RECORDS_NOTIF) {
	             /* Application should delete stored records based on
	                the operator and filter type values. Once all requested records
	                are deleted status should be indicated to the peer device. */
	                gls_indicate_delete_records_status(...);
	        }
	    }
	}
##### RACP - Abort an Ongoing  Command

a. When a command that requests to abort the current processing of a command is received the server will call the registered `abort_operation` callback function. 

b. It is strongly advised that request handling is differed to the main application task by sending notifications. 

c. As per the specification the application should do its best to abort the current processing of a request as soon as possible. Once abortion is completed the application should indicate the success of failure of abortion. This can be achieved by calling `gls_indicate_abort_operation_status`.

Following is a screenshot  that depicts the key parts of handling requests that deal with aborting ongoing requests:

```
/* This callback will be called by server upon reception of a command that requests
   aborting the current command processing. */
static void abort_operation_cb(...)
{
	...
    /* Parsing should be done in the application's task context so BLE Manager's task            can process other BLE events. Defer further processing by sending    
       notifications. */
    OS_TASK_NOTIFY(...);
}

OS_TASK_FUNCTION(glucose_sensor_task, params)
{
	...
    for (;;) {
        ...
        if (notif & RACP_ABORT_NOTIF) {
             /* Application should abort processing the current command
                at the earliest convenience. Once abortion is completed
                status should be indicated to the peer device. */
                gls_indicate_abort_operation_status(...);
        }
    }
}
```

##### RACP - Report Stored Records

a. When a command that requests to report stored records is received and is valid the server will call the registered `report_records` callback function. 

b. It is strongly advised that request handling is differed to the main application task by sending notifications.  The `gls_racp_t` structure that contains the requested operator and operand values should be stored so they are available in main task's context.

c. Application should report all records that meet the criteria specified by the operator and filter type values. Reporting a record is done by sending notifications to the peer device via `gls_notify_record.` It is expected that this API is called for each record that is to be reported. It's worth mentioning that the status returned by the mentioned API should be examined and in case of failure notification for the same record should be re-sent. Following is a helper macro that can be used:

```
/* Helper macro to call a function until success is returned */
#define APP_CALL_FUNCTION_UNTILL_SUCCESS(_f, _s, args...) \
        typeof(_s) status;                                \
        do {                                              \
                status = _f(args);                        \
        } while (status != _s);                           \
```

Once all records are notified application should indicate the success or failure of the reporting processing. This can be achieved by calling `gls_indicate_report_records_status`. 

Following is a screenshot  that depicts the key parts of handling requests that deal with reporting stored records:

```
/* This callback will be called by server upon reception of a command that requests
   reporting stored records. */
static void report_records_cb(...)
{
	...
    /* Parsing should be done in the application's task context so BLE Manager's task            can process other BLE events. Deferfurther processing by sending    
       notifications. */
    OS_TASK_NOTIFY(...);
}

OS_TASK_FUNCTION(glucose_sensor_task, params)
{
	...
    for (;;) {
        ...
        if (notif & RACP_REPORT_RECORDS_NOTIF) {
             /* Application should report stored records based on the operator
                and filter type values. Is expected that a notification will be 
                sent to peer device for each record.
                for (...) {
                	/**/
                	APP_CALL_FUNCTION_UNTILL_SUCCESS(gls_notify_record, (bool)true, ...);
                }
                
                /* Once all records are reported the status
                   should be indicated to the peer device. */
                gls_indicate_report_records_status(...);
        }
    }
}
```

##### Read/Indicate Feature Characteristic Value

As per specifications the value of the feature characteristic can optionally be changed over the lifetime of the glucose meter device. In such a case, the application should handle the associated read requests and be able to explicitly send indications to peer devices when the device feature value is updated. 

a. If a read event associated to the feature characteristic value is received the server will call the `get_features` callback function. 

b. Application is expected to provided the 16-bit feature value by calling  `gls_get_feature_cfm()`. Though the callback function is called within the BLE manager's task context it is expected that responding with the current feature value is done immediately and does not require significant effort that might introduce delays. As a result, deferring the handling request is not advised. 

Following is a code snippet that demonstrates handling feature read requests:

```
/* This callacbk has a meaning only if the associated configuration macro is enabled. */
#if GLS_FEATURE_INDICATION_PROPERTY
static void get_features_cb(ble_service_t *svc, uint16_t conn_idx)
{
        gls_get_feature_cfm(svc, conn_idx, ATT_ERROR_OK, 
        					GLS_FEATURE_CHARACTERISTIC_VALUE);
}
#endif
```

If the characteristic value is changed over the device lifetime then it is expected that the new value is indicated to peer devices with the help of `gls_indicate_feature_all`. The latter will actually invoke the `gls_indicate_feature` API for each connected and bonded peer device that has its Client Characteristic Configuration Description (CCCD) enabled. 

##### Optional Callback Functions

1. Application can optionally register a callback function that will be invoked for each notification/indication sent by application. This callback can be used to get that status of a previous notification/indication (success or failure). Following is the function prototype of the mentioned callback function:

```
typedef void (*gls_event_sent_cb_t)(ble_service_t *svc, const ble_evt_gatts_event_sent_t *evt);
```

2. Application can optionally register a callback function that will be invoked following write requests to the Client Characteristic Configuration Descriptors associated to the Glucose Measurement and Glucose Measurement Context characteristics. Please read comments on the corresponding function signature as depicted below:

```
/* As per GLS specifications, if CCC descriptor has been configured to enable   
   notifications and the peer collector does not perform a RACP procedure, then after 
   waiting for an idle time of 5'', the sensor device should perform the GAP Terminate 
   Connection procedure */
typedef void (*gls_ccc_enabled_cb_t)(ble_service_t *svc, const ble_evt_gatts_write_req_t *evt);
```

### Glucose Service Database Framework

The Glucose Service database containing the various patient records should be handled explicitly on application level. To facilitate users, a database framework is introduced and is expected that modifications be performed by developers to meet their application needs. The database makes use of linked lists by dynamically allocating record spaces of fixed size each time one or more records are requested to be generated. That means that patients records are not stored in some non-volatile storage medium. Following is a short description of the current database framework API.

1.  The database should first be initialized once before used by calling `app_db_init`. By default the max. number of records that can be allocated is ten and can be changed via `APP_DB_MAX_LIST_LEN`. As per specifications if the max. storage capacity is reached and a new record is to created the oldest record should be overwritten. **Please note that if the max. number of records is increased then the heap size (`configTOTAL_HEAP_SIZE`) might need to be increased accordingly.** 
2. When a new patient record is to be  generated application should call `app_db_add_record_entry` with a callback function of type `app_db_add_record_entry_cb_t`. This callback will be invoked by the framework once the record space is allocated. The purpose of this callback function is for the application to initialize the various record parameters  with the values of interest.  In the glucose meter sample code an OS timer is setup to expire every `GLS_DATABASE_UPDATE_MS` (default value is 10'').  A notification is then sent to application task and the latter requests the creation of a new record entry passing `init_record_entry_cb` as callback function.
3. As mentioned above, following a write request to the Record Access Control Point (RACP) characteristic should result in invoking a registered callback function based on the requested command (abort, delete or report). Application, and within the callback function's context should call `app_db_update_racp_request`so the database gets informed on the current RACP request (operator, filter type etc.).
4. Application should call the appropriate framework API so the database framework can handle the current RACP request. Following is a code snippet that demonstrate using the available APIs:                        

       OS_TASK_FUNCTION(glucose_sensor_task, params)
       {
       	...
           for (;;) {
               ...
               if (notif & RACP_REPORT_RECORDS_NOTIF) {
               	/* The dabase will notify the records entries that match the operator and 
               	   filter type criteria. The status will be indicated before exiting. */
               	app_db_report_records_handle();
               }
               
              if (notif & RACP_DELETE_RECORDS_NOTIF) {
              		/* The dabase will delete the records entries that match the operator and 
                 	filter type criteria. The status will be indicated before exiting. */
               	app_db_delete_records_handle();
               }
           
              if (notif & RACP_REPORT_NUM_NOTIF) {
                 /* The dabase will calculate the number of records entries that match the
                 operator and filter type criteria. */
                 app_db_report_num_of_records_handle();
               }
           ...
       	}
       }
   **Please not that the database framework does not provide support for User Facing Time filter types. If the associated configuration macro is enabled via `GLS_RACP_FILTER_USER_FACING_TIME_SUPPORT` then it is expected that the database is modified accordingly. Otherwise an assertion is expected to be thrown at compile time (`C_ASSERT(0)`).** 

## HW and SW Configuration

  - **Hardware Configuration**
    - This example runs on DA14592 family of devices.
    - A Pro development kit is needed for this example.
  - **Software Configuration**
    - Download the latest SDK version for the target family of devices.

    - SEGGER J-Link tools are normally downloaded and installed as part of the e2 Studio installation.

## How to run the example

### Initial Setup

- Download the source code from Renesas GitHub

- Import the project into your workspace (there should be no path dependencies).

- Connect the target device to your host PC via USB1. The mentioned port is used to power the device via VBAT and also to support debugging functionality (UART and JTAG).

- Compile the code and load it into the chip.

- Open a serial terminal (115200/8 - N - 1).

- Press the reset button on the daughterboard to start executing the application. 

- Open a Bluetooth scanner APP and connect to the target device that should be displayed as Renesas Glucose Sensor (scan response).  

- If this is the first time connecting to the meter device, any attempt to write to the Record Access Control Point (RACP) characteristic will first trigger the authentication and bonding process as illustrated in the screenshot below:

  <img src="assets\authentication.png" style="zoom: 20%;" />

  1.  A 6-digit passkey should be displayed on the serial console.
  2.  A pop-up window should be displayed on the scanner APP and is expected that the displayed passkey in entered.
  3.  Once entered, pairing and bonding status should be displayed on the serial console.

**The Glucose Service implementation has been tested using the Profile Testing Suite v8.5.1 with the help of a PTS USB dongle. All mandatory and optional features have been tested expect for tests that concern User Facing Time filter types as the latter are not supported by the framework database out of the box.** 

## Known Limitations

There are no known limitations for this sample code.

**************************************************************************************
