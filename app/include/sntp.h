#ifndef __SNTP_H__
#define __SNTP_H__

#include "lwip/ip_addr.h"
#include <time.h>

bool sntp_inits(int8_t UTC_offset) ICACHE_FLASH_ATTR;
void sntp_close(void) ICACHE_FLASH_ATTR;
time_t get_sntp_time(void) ICACHE_FLASH_ATTR;
time_t get_sntp_localtime(void) ICACHE_FLASH_ATTR;

extern ip_addr_t dhcp_sntp_server_address;

void sntp_send_request(ip_addr_t *server_addr) ICACHE_FLASH_ATTR;

#endif /* __SNTP_H__ */
