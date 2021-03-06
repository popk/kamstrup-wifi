#include <stddef.h>
#include <esp8266.h>

#include "mqtt.h"
#include "config.h"
#include "user_config.h"
#include "debug.h"
#include "utils.h"
#include "tinyprintf.h"
#include "driver/ext_spi_flash.h"

syscfg_t sys_cfg;
SAVE_FLAG saveFlag;

#define EXT_CFG_LOCATION		0x0

#ifdef EXT_SPI_RAM_IS_NAND
#define EXT_SPI_RAM_SEC_SIZE	0x1000	// 4 kB block size for FLASH RAM
#else
#define EXT_SPI_RAM_SEC_SIZE	0x200	// we only need 512 bytes for config when FeRAM is used
#endif

#define SAVE_DEFER_TIME 2000
static os_timer_t config_save_timer;
char config_save_timer_running;

void ICACHE_FLASH_ATTR
cfg_save() {
#ifdef IMPULSE
	uint32_t impulse_meter_count_temp;
#endif // IMPULSE
	
#ifdef IMPULSE	
	do {
		// try to save until sys_cfg.impulse_meter_count does not change
		impulse_meter_count_temp = sys_cfg.impulse_meter_count;

		// calculate checksum on sys_cfg struct without ccit_crc16
		sys_cfg.ccit_crc16 = ccit_crc16((uint8_t *)&sys_cfg, offsetof(syscfg_t, ccit_crc16) - offsetof(syscfg_t, cfg_holder));	
		ext_spi_flash_read((EXT_CFG_LOCATION + 2) * EXT_SPI_RAM_SEC_SIZE,
		                   (uint32 *)&saveFlag, sizeof(SAVE_FLAG));
	
		if (saveFlag.flag == 0) {
#ifdef EXT_SPI_RAM_IS_NAND
			ext_spi_flash_erase_sector(EXT_CFG_LOCATION + 1);
#endif	// EXT_SPI_RAM_IS_NAND
			ext_spi_flash_write((EXT_CFG_LOCATION + 1) * EXT_SPI_RAM_SEC_SIZE,
							(uint32 *)&sys_cfg, sizeof(syscfg_t));
			saveFlag.flag = 1;
#ifdef EXT_SPI_RAM_IS_NAND
			ext_spi_flash_erase_sector(EXT_CFG_LOCATION + 2);
#endif	// EXT_SPI_RAM_IS_NAND
			ext_spi_flash_write((EXT_CFG_LOCATION + 2) * EXT_SPI_RAM_SEC_SIZE,
							(uint32 *)&saveFlag, sizeof(SAVE_FLAG));
		} else {
#ifdef EXT_SPI_RAM_IS_NAND
			ext_spi_flash_erase_sector(EXT_CFG_LOCATION + 0);
#endif	// EXT_SPI_RAM_IS_NAND
			ext_spi_flash_write((EXT_CFG_LOCATION + 0) * EXT_SPI_RAM_SEC_SIZE,
							(uint32 *)&sys_cfg, sizeof(syscfg_t));
			saveFlag.flag = 0;
#ifdef EXT_SPI_RAM_IS_NAND
			ext_spi_flash_erase_sector(EXT_CFG_LOCATION + 2);
#endif	// EXT_SPI_RAM_IS_NAND
			ext_spi_flash_write((EXT_CFG_LOCATION + 2) * EXT_SPI_RAM_SEC_SIZE,
							(uint32 *)&saveFlag, sizeof(SAVE_FLAG));
		}
	} while (sys_cfg.impulse_meter_count != impulse_meter_count_temp);
#else
	// calculate checksum on sys_cfg struct without ccit_crc16
	sys_cfg.ccit_crc16 = ccit_crc16((uint8_t *)&sys_cfg, offsetof(syscfg_t, ccit_crc16) - offsetof(syscfg_t, cfg_holder));
	system_param_save_with_protect(CFG_LOCATION, &sys_cfg, sizeof(syscfg_t));
#endif	// IMPULSE
}

void ICACHE_FLASH_ATTR
cfg_load() {
#ifdef IMPULSE
	// DEBUG: we suppose nothing else is touching sys_cfg while saving otherwise checksum becomes wrong
	INFO("\r\nload ...\r\n");
	
	ext_spi_flash_read((EXT_CFG_LOCATION + 2) * EXT_SPI_RAM_SEC_SIZE,
				   (uint32 *)&saveFlag, sizeof(SAVE_FLAG));
	if (saveFlag.flag == 0) {
		ext_spi_flash_read((EXT_CFG_LOCATION + 0) * EXT_SPI_RAM_SEC_SIZE,
					   (uint32 *)&sys_cfg, sizeof(syscfg_t));
	} else {
		ext_spi_flash_read((EXT_CFG_LOCATION + 1) * EXT_SPI_RAM_SEC_SIZE,
					   (uint32 *)&sys_cfg, sizeof(syscfg_t));
	}

	// if checksum fails...
	if (sys_cfg.ccit_crc16 != ccit_crc16((uint8_t *)&sys_cfg, offsetof(syscfg_t, ccit_crc16) - offsetof(syscfg_t, cfg_holder))) {
#ifdef DEBUG
		os_printf("config crc error, default conf loaded\n");
#endif // DEBUG
		// if first time config load default conf
		os_memset(&sys_cfg, 0x00, sizeof(syscfg_t));

		tfp_snprintf(sys_cfg.sta_ssid, 64, "%s", STA_SSID);
		tfp_snprintf(sys_cfg.sta_pwd, 64, "%s", STA_PASS);
		sys_cfg.sta_type = STA_TYPE;

		tfp_snprintf(sys_cfg.device_id, 16, MQTT_CLIENT_ID, system_get_chip_id());
		tfp_snprintf(sys_cfg.mqtt_host, 64, "%s", MQTT_HOST);
		sys_cfg.mqtt_port = MQTT_PORT;
		tfp_snprintf(sys_cfg.mqtt_user, 32, "%s", MQTT_USER);
		tfp_snprintf(sys_cfg.mqtt_pass, 32, "%s", MQTT_PASS);

		sys_cfg.security = DEFAULT_SECURITY;	//default non ssl
		
		memcpy(sys_cfg.key, key, sizeof(key));

		sys_cfg.mqtt_keepalive = MQTT_KEEPALIVE;
#ifndef IMPULSE
		sys_cfg.ac_thermo_state = 0;
		memset(&sys_cfg.cron_jobs, 0, sizeof(cron_job_t));
#endif
		tfp_snprintf(sys_cfg.impulse_meter_serial, IMPULSE_METER_SERIAL_LEN, DEFAULT_METER_SERIAL);
		tfp_snprintf(sys_cfg.impulse_meter_energy, 2, "0");
		tfp_snprintf(sys_cfg.impulses_per_kwh, 4, "100");
		sys_cfg.impulse_meter_count = 0;
		INFO(" default configuration\r\n");

		cfg_save();
	}
	else {
#ifdef DEBUG
		os_printf("config crc ok\n");
#endif // DEBUG
	}
#else
	// DEBUG: we suppose nothing else is touching sys_cfg while saving otherwise checksum becomes wrong
	INFO("\r\nload ...\r\n");
	system_param_load(CFG_LOCATION, 0, &sys_cfg, sizeof(syscfg_t));

	// if checksum fails...
	if (sys_cfg.ccit_crc16 != ccit_crc16((uint8_t *)&sys_cfg, offsetof(syscfg_t, ccit_crc16) - offsetof(syscfg_t, cfg_holder))) {
#ifdef DEBUG
		os_printf("config crc error, default conf loaded\n");
#endif // DEBUG
		// if first time config load default conf
		os_memset(&sys_cfg, 0x00, sizeof(syscfg_t));

		tfp_snprintf(sys_cfg.sta_ssid, 64, "%s", STA_SSID);
		tfp_snprintf(sys_cfg.sta_pwd, 64, "%s", STA_PASS);
		sys_cfg.sta_type = STA_TYPE;

		tfp_snprintf(sys_cfg.device_id, 16, MQTT_CLIENT_ID, system_get_chip_id());
		tfp_snprintf(sys_cfg.mqtt_host, 64, "%s", MQTT_HOST);
		sys_cfg.mqtt_port = MQTT_PORT;
		tfp_snprintf(sys_cfg.mqtt_user, 32, "%s", MQTT_USER);
		tfp_snprintf(sys_cfg.mqtt_pass, 32, "%s", MQTT_PASS);

		sys_cfg.security = DEFAULT_SECURITY;	//default non ssl
		
		memcpy(sys_cfg.key, key, sizeof(key));

		sys_cfg.mqtt_keepalive = MQTT_KEEPALIVE;
		
		sys_cfg.ac_thermo_state = 0;
		memset(&sys_cfg.cron_jobs, 0, sizeof(cron_job_t));
		INFO(" default configuration\r\n");

		cfg_save();
	}
	else {
#ifdef DEBUG
		os_printf("config crc ok\n");
#endif // DEBUG
	}
#endif // IMPULSE
}

void ICACHE_FLASH_ATTR
cfg_save_defered() {
	os_timer_disarm(&config_save_timer);
	os_timer_setfn(&config_save_timer, (os_timer_func_t *)config_save_timer_func, NULL);
	os_timer_arm(&config_save_timer, SAVE_DEFER_TIME, 0);
}

ICACHE_FLASH_ATTR
void config_save_timer_func(void *arg) {
	if (config_save_timer_running) {
		// reschedule
		os_timer_disarm(&config_save_timer);
		os_timer_setfn(&config_save_timer, (os_timer_func_t *)config_save_timer_func, NULL);
		os_timer_arm(&config_save_timer, SAVE_DEFER_TIME, 0);
	}
	else {
		// stop timer
		os_timer_disarm(&config_save_timer);
		config_save_timer_running = 0;

		cfg_save();		
	}
}
