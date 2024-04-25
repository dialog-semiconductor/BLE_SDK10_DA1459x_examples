# Crypto Adapter Demonstration Example

This sample code demonstrates using the crypto engine. The DA1459x family of devices incorporates a single crypto accelerator capable of performing AES and HASH cryptographic operations. Please note that these two modes can work complementary. Therefore, either AES or HASH cryptographic algorithms can be computed at a time. The crypto engine integrates a dedicated DMA engine and so no CPU intervention is required for cryptographic computations. One interesting feature of the crypto engine is the ability to process fragmented data. This means that data to be processed, namely input vectors, can be located at different locations in memory. This demonstration example exhibits two OS tasks; one task demonstrates performing AES CBC encryption/decryption algorithms and another task demonstrates  running SHA-256 hashing algorithms. Each of the two tasks should demonstrate the `fragmented` functionality by setting the the following configuration macros in `crypto_task.c`:

```
#define CRYPTO_HASH_FRAGMENTED      ( 1 )
#define CRYPTO_AES_FRAGMENTED       ( 1 )
```

The key vectors used for cryptographic operations can also be stored in the secure key area in eFLASH. Currently, up to 8 key vectors, each consisting of up to 32 bytes, can be accommodated in the user keys area. The latter can be protected via sticky bits. In such a case, only the secure DMA channel #5 can access the secure keys area in eFLASH. Please note that any attempt to read the key vectors directly from the crypto's registers file should return all zeros. The sample code demonstrates using key vectors stored in the secure keys area with the help of the following macros in `cryto_task.c`

```
#define CRYPTO_AES_SECURE_KEY     ( 0 )
/* Up to 8 user data keys can be stored in secure are in eFLASH. */
#define CRYPTO_AES_SECURE_KEY_IDX ( 0 )
```

For demonstration purposes a valid secure key is read and its contents are exercised if CPU is allowed to do so (secure keys area is unprotected). If the secure key is read all ones then it is assumed that the selected key index is empty and its corresponding position in eFLASH is written with some dummy data.

## HW and SW Configuration

  - **Hardware Configuration**
    - This example runs on DA14592 family of devices.
    - A Pro development kit is needed for this example.
  - **Software Configuration**
    - Download the latest SDK version for the target family of devices.

    - SEGGER J-Link tools are normally downloaded and installed as part of the e2 Studio installation.

## How to run the example

### Initial Setup

- Download the source code from the Support Website.
- Import the project into your workspace (there should be no path dependencies).
- Connect the target device to your host PC via USB1. The mentioned port is used to power the device via VBAT and also to support debugging functionality (UART and JTAG).
- Compile the code and load it into the chip.
- Open a serial terminal (115200/8 - N - 1).
- Press the reset button on the daughterboard to start executing the application. 
- HASH and AES cryptographic operations should execute and their output status should be displayed on the console indicating whether or note operations were run successfully. 

## Known Limitations

There are no known limitations for this sample code.

**************************************************************************************
