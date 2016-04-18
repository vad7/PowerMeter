/*
 * Debug to RAM
 * Written by vad7@yahoo.com
 *
*/

#include "c_types.h"

#define DEBUG_RAM_BUF_MAX	32768
extern uint32 Debug_RAM_size;
extern uint8 *Debug_RAM_addr;
extern uint32 Debug_RAM_len;
extern uint8  Debug_level;
extern char print_mem_buf[1024];
void ICACHE_FLASH_ATTR dbg_printf_out(char c);
void ICACHE_FLASH_ATTR dbg_printf(const char *format, ...);
void ICACHE_FLASH_ATTR dbg_set(uint8 level, uint32 size);
void ICACHE_FLASH_ATTR dbg_tcp_send(void * ts_conn);
