/*
 * Send data to IoT cloud, like thingspeak.com
 * Channels max: 2
 * Setup - cfg_meter.IoT_*
 *
 * reworked by vad7
 */

#include "user_config.h"
#include "bios.h"
#include "sdk/add_func.h"
#include "c_types.h"
#include "osapi.h"
#include "user_interface.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "tcp_srv_conn.h"
#include "web_utils.h"
#include "wifi_events.h"
#include "webfs.h"
#include "web_utils.h"
#include "power_meter.h"
#include "web_fs_init.h"

#define DEFAULT_TC_HOST_PORT 80
#define TCP_REQUEST_TIMEOUT	15 // sec

/* iot_cloud.ini:
iot_server=<имя http сервера>\n
iot_add_n=<параметры для GET запроса №1>\n
iot_add_n=<параметры для GET запроса №2>\n
iot_add_n=<параметры для GET запроса №3>\n
...

переменные обрамляются "~",
n - минимальный интервал между запросами в мсек, 0 - без ожидания
если при парсинге установлен SCB_RETRYCB, то пропускаем запрос
Итоговый запрос: http://<имя сервера><параметры>
"\n" = 0x0A
*/


uint8 get_request_tpl[] = "GET %s HTTP/1.0/\r\nHost: %s\r\n\r\n";
uint8 key_http_ok[] = "HTTP/1.? 200 OK\r\n";
//uint8 key_chunked[] = "Transfer-Encoding: chunked";

uint8 key_td_info[] = "<td class=\"info\">";
//uint32 next_start_time;

//uint32 content_chunked DATA_IRAM_ATTR;
os_timer_t error_timer;

ip_addr_t tc_remote_ip;
int tc_init_flg; // внутренние флаги инициализации
#define TC_INITED 		(1<<0)
#define TC_INIT_WIFI_CB (1<<1)
#define TC_FLG_RUN_ON 	(1<<2)

uint32 tc_head_size; // используется для разбора HTTP ответа

#define mMIN(a, b)  ((a<b)?a:b)

TCP_SERV_CFG * tc_servcfg;

void tc_go_next(void);

//-------------------------------------------------------------------------------
// run_error_timer
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR run_error_timer(uint32 tsec)
{
	ets_timer_disarm(&error_timer);
	ets_timer_setfn(&error_timer, (os_timer_func_t *)tc_go_next, NULL);
	ets_timer_arm_new(&error_timer, tsec*1000, 0, 1); // таймер на x секунд
}
//-------------------------------------------------------------------------------
// tc_close
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR tc_close(void)
{
	if(tc_servcfg != NULL) {
		tcpsrv_close(tc_servcfg);
		tc_servcfg = NULL;
	}
}
//-------------------------------------------------------------------------------
// TCP sent_cb
//-------------------------------------------------------------------------------
/*
err_t ICACHE_FLASH_ATTR tc_sent_cb(TCP_SERV_CONN *ts_conn) {
#if DEBUGSOO > 1
	tcpsrv_sent_callback_default(ts_conn);
#endif
	tc_sconn = ts_conn;
	return ERR_OK;
}
*/
//-------------------------------------------------------------------------------
// TCP receive response from server
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tc_recv(TCP_SERV_CONN *ts_conn) {
#if DEBUGSOO > 1
	tcpsrv_received_data_default(ts_conn);
#endif
	tcpsrv_unrecved_win(ts_conn);
	tc_init_flg &= ~TC_FLG_RUN_ON; // clear run flag
    uint8 *pstr = ts_conn->pbufi;
    sint32 len = ts_conn->sizei;
    if(tc_head_size == 0) {
        if(len < sizeof(key_http_ok) +2 -1) {
        	return ERR_OK;
        }
        if(str_cmp_wildcards(pstr, key_http_ok)) {
        	tc_head_size = sizeof(key_http_ok)-1;
        	len -= sizeof(key_http_ok)-1;
        	pstr += sizeof(key_http_ok)-1;
        } else {
        	tc_close();
        	return ERR_OK;
        }
        if(tc_head_size != 0 && len > sizeof(key_td_info) + 1) {
			#if DEBUGSOO > 5
				os_printf("content[%d] ", len);
			#endif
		   	ts_conn->flag.rx_null = 1; // stop receiving data
        }
    }
	return ERR_OK;
}
//-------------------------------------------------------------------------------
// TCP listen, put GET request to the server
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tc_listen(TCP_SERV_CONN *ts_conn) {
	uint16 len = 0;
	uint8 *buf = NULL;
	struct buf_fini *p = NULL;
	if(iot_data_processing != NULL) {
		p = web_fini_init(1); // prepare empty buffer, filled with 0
		if(p == NULL) return 1;
		uint16 len = os_strlen(iot_data_processing->iot_url_params);
		os_memcpy(p->buf, iot_data_processing->iot_url_params, len);
		p->web_conn.msgbuflen = len;
		webserver_parse_buf(&p->ts_conn);
		if(p->web_conn.webflag & SCB_RETRYCB) { // cancel send
			#if DEBUGSOO > 4
				os_printf("iot-skip!\n");
			#endif
			tc_close();
			os_free(p);
			return 2;
		}
		buf = p->buf;
		len = p->web_conn.msgbuflen;
	}
#if DEBUGSOO > 1
	tcpsrv_print_remote_info(ts_conn);
	os_printf("send %d bytes\n", len);
#endif
	err_t err = tcpsrv_int_sent_data(ts_conn, buf, len);
	os_free(p);
	return err;
}
//-------------------------------------------------------------------------------
// TCP disconnect
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR tc_disconnect(TCP_SERV_CONN *ts_conn) {
#if DEBUGSOO > 1
	tcpsrv_disconnect_calback_default(ts_conn);
#endif
}
//-------------------------------------------------------------------------------
// tc_start
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tc_init(void)
{
	err_t err = ERR_USE;
	tc_close();

	TCP_SERV_CFG * p = tcpsrv_init_client3();  // tcpsrv_init(3)
	if (p != NULL) {
		// изменим конфиг на наше усмотрение:
		p->max_conn = 3; // =0 - вечная попытка соединения
		p->flag.rx_buf = 1; // прием в буфер с его автосозданием.
		p->flag.nagle_disabled = 1; // отмена nagle
//		p->time_wait_rec = tc_twrec; // по умолчанию 5 секунд
//		p->time_wait_cls = tc_twcls; // по умолчанию 5 секунд
#if DEBUGSOO > 4
		os_printf("TC: Max retry connection %d, time waits %d & %d, min heap size %d\n",
					p->max_conn, p->time_wait_rec, p->time_wait_cls, p->min_heap);
#endif
		p->func_discon_cb = tc_disconnect;
		p->func_listen = tc_listen;
		p->func_sent_cb = NULL;
		p->func_recv = tc_recv;
		err = ERR_OK;
	}
	tc_servcfg = p;
	return err;
}
//-------------------------------------------------------------------------------
// dns_found_callback
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR tc_dns_found_callback(uint8 *name, ip_addr_t *ipaddr, void *callback_arg)
{
#if DEBUGSOO > 4
	os_printf("clb:%s, " IPSTR " ", name, IP2STR(ipaddr));
#endif
	if(tc_servcfg != NULL) {
		if(ipaddr != NULL && ipaddr->addr != 0) {
			tc_remote_ip = *ipaddr;
			tc_head_size = 0;
			err_t err = tcpsrv_client_start(tc_servcfg, tc_remote_ip.addr, DEFAULT_TC_HOST_PORT);
			if (err != ERR_OK) {
#if DEBUGSOO > 4
				os_printf("goerr=%d ", err);
#endif
				tc_close();
			}
		}
	}
}
//-------------------------------------------------------------------------------
// close_dns_found
//-------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR close_dns_finding(void){
	ets_timer_disarm(&error_timer);
	if(tc_init_flg & TC_FLG_RUN_ON) { // ожидание dns_found_callback() ?
		// убить вызов  tc_dns_found_callback()
		int i;
		for (i = 0; i < DNS_TABLE_SIZE; ++i) {
			if(dns_table[i].found == (dns_found_callback)tc_dns_found_callback) {
				/* flush this entry */
				dns_table[i].found = NULL;
				dns_table[i].state = DNS_STATE_UNUSED;
			}
		}
		tc_init_flg &= ~TC_FLG_RUN_ON;
	}
	tc_close();
}
//-------------------------------------------------------------------------------
// TCP client start
//-------------------------------------------------------------------------------
err_t ICACHE_FLASH_ATTR tc_go(void)
{
	err_t err = ERR_USE;
	if((tc_init_flg & TC_FLG_RUN_ON) || iot_data_processing == NULL) return err; // выход, если процесс запущен или нечего запускать
	run_error_timer(TCP_REQUEST_TIMEOUT); // обработать ошибки и продолжение
	err = tc_init(); // инициализация TCP
	if(err == ERR_OK) {
		tc_init_flg |= TC_FLG_RUN_ON; // процесс запущен
		err = dns_gethostbyname(iot_server_name, &tc_remote_ip, (dns_found_callback)tc_dns_found_callback, NULL);
#if DEBUGSOO > 4
		os_printf("dns_gethostbyname(%s)=%d ", iot_server_name, err);
#endif
		if(err == ERR_OK) {	// Адрес разрешен из кэша или локальной таблицы
			tc_head_size = 0;
			err = tcpsrv_client_start(tc_servcfg, tc_remote_ip.addr, DEFAULT_TC_HOST_PORT);
			if(err != ERR_OK) {
				tc_init_flg &= ~TC_FLG_RUN_ON; // процесс не запущен
			}
		} else if(err == ERR_INPROGRESS) { // Запущен процесс разрешения имени с внешнего DNS
			err = ERR_OK;
		}
		if (err != ERR_OK) {
			tc_init_flg &= ~TC_FLG_RUN_ON; // процесс не запущен
//				tc_close();
		}
	}
	return err;
}
// next iot_data connection
void tc_go_next(void)
{
	if(tc_init_flg & TC_FLG_RUN_ON) { // Timeout
		close_dns_finding();
		tc_init_flg &= ~TC_FLG_RUN_ON; // clear
	}
	if(iot_data_processing != NULL) {
		// next
		while((iot_data_processing = iot_data_processing->next) != NULL) {
			if(iot_data_processing->last_run + iot_data_processing->min_interval <= system_get_time()) { // если рано - пропускаем
				if(tc_go() == ERR_OK) break;
			}
		}
	}
	if(iot_data_processing == NULL) ets_timer_disarm(&error_timer); // stop timer
}

// return 0 - ok
uint8_t ICACHE_FLASH_ATTR iot_cloud_init(void)
{
	uint8_t retval;
	if(tc_init_flg) { // already init - restart
		close_dns_finding();
		tc_init_flg = 0;
	}
	if(iot_data_first != NULL) { // data exist - clear
		iot_data_clear();
	}
	retval = web_fini(cfg_meter.iot_ini);
	if(retval == 0) tc_init_flg |= TC_INITED;
	return retval;
}

void ICACHE_FLASH_ATTR iot_data_clear(void)
{
	if(iot_data_first != NULL) {
		IOT_DATA *next, *iot = iot_data_first;
		do {
			next = iot->next;
			os_free(iot);
		} while((iot = next) != NULL);
		os_free(iot_server_name);
		iot_data_first = NULL;
		iot_data_processing = NULL;
		iot_server_name = NULL;
	}
}

// 1 - start, 0 - end
void ICACHE_FLASH_ATTR iot_cloud_send(uint8 fwork)
{
	if((tc_init_flg & TC_INITED) == 0) return;
	if(fwork == 0) { // end
		close_dns_finding();
		return;
	}
	if(iot_data_first == NULL || !flg_open_all_service || wifi_station_get_connect_status() != STATION_GOT_IP) return; // st connected?
	if(iot_data_first != NULL) {
		tc_go_next();
	}
}
