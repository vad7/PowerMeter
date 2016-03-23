
#define DEFAULT_TC_HOST_PORT 80
#define TCP_REQUEST_TIMEOUT	15 // sec

extern uint8 iot_get_request_tpl[];
extern uint8 key_http_ok[];
extern uint8 iot_cloud_ini[];

char *iot_server_name; // = NULL; // set from ini: iot_server=
typedef struct _IOT_DATA {
	uint32	min_interval; 	// msec
	uint32	last_run;		// system_get_time()
	struct _IOT_DATA *next;
	char	iot_request[];		// set from ini: iot_add=
} IOT_DATA;
IOT_DATA *iot_data_first; // = NULL;
IOT_DATA *iot_data_processing; // = NULL;

