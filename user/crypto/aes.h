#ifndef _AES_H_
#define _AES_H_

#include <esp8266.h>
#include <stdint.h>


// #define the macros below to 1/0 to enable/disable the mode of operation.
//
// CBC enables AES128 encryption in CBC-mode of operation and handles 0-padding.
// ECB enables the basic ECB 16-byte block algorithm. Both can be enabled simultaneously.

// The #ifndef-guard allows it to be configured before #include'ing or at compile time.
#ifndef CBC
  #define CBC 1
#endif

#ifndef ECB
  #define ECB 1
#endif



#if defined(ECB) && ECB

ICACHE_FLASH_ATTR void AES128_ECB_encrypt(const uint8_t* input, const uint8_t* key, uint8_t *output);
ICACHE_FLASH_ATTR void AES128_ECB_decrypt(const uint8_t* input, const uint8_t* key, uint8_t *output);

#endif // #if defined(ECB) && ECB


#if defined(CBC) && CBC

ICACHE_FLASH_ATTR void AES128_CBC_encrypt_buffer(uint8_t* output, const uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);
ICACHE_FLASH_ATTR void AES128_CBC_decrypt_buffer(uint8_t* output, const uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);

#endif // #if defined(CBC) && CBC



#endif //_AES_H_
