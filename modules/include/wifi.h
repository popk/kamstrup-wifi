/*
 * wifi.h
 *
 *  Created on: Dec 30, 2014
 *      Author: Minh
 */

#ifndef USER_WIFI_H_
#define USER_WIFI_H_
typedef void (*WifiCallback)(uint8_t);

void wifi_handle_event_cb(System_Event_t *evt);
void ICACHE_FLASH_ATTR wifi_scan_done_cb(void *arg, STATUS status);
void ICACHE_FLASH_ATTR wifi_default();
void ICACHE_FLASH_ATTR wifi_fallback();
void ICACHE_FLASH_ATTR wifi_connect(uint8_t* ssid, uint8_t* pass, WifiCallback cb);
sint8_t ICACHE_FLASH_ATTR wifi_get_rssi();
void ICACHE_FLASH_ATTR wifi_start_scan();
void ICACHE_FLASH_ATTR wifi_stop_scan();
bool ICACHE_FLASH_ATTR wifi_scan_is_running();
bool ICACHE_FLASH_ATTR wifi_fallback_is_present();

#endif /* USER_WIFI_H_ */
