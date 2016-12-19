/*
 * Mercury power meter driver (http://www.incotexcom.ru/counters.htm)
 *
 * Use IrDa interface (UART0)
 *
 * Written by vad7
 */
#ifdef USE_MERCURY
#include "time.h"

typedef struct {
	uint32 W; 		// +W*h
	uint32 W_T1; 	// Tariff #1, +W*h
	//uint32 W_T2;  // = W - W_T1
} POWER;

typedef struct {
	uint32 W1; // Moment power Phase 1
	uint32 W2; // Moment power Phase 2
	uint32 W3; // Moment power Phase 3
	uint16 U1; // Voltage 1
	uint16 U2; // Voltage 2
	uint16 U3; // Voltage 3
	uint16 I1; // Current 1
	uint16 I2; // Current 2
	uint16 I3; // Current 3
	POWER Total;
	POWER Year;
	POWER Day;
	uint32 TotalPhase1; // W*h
	uint32 TotalPhase2; // W*h
	uint32 TotalPhase3; // W*h
	time_t Time;
} PWMT_CURRENT;
PWMT_CURRENT __attribute__((aligned(4))) pwmt_cur;

#define PWMT_CONNECT_PAUSE		5	// ms
#define PWMT_CONNECT_TIMEOUT	150 // ms
uint8 pwmt_connect_status; 	// 0 - not connected, 1 - connecting, 2 - connected
uint8 pwmt_connect_pause;
uint8 pwmt_connect_timeout;

typedef struct {
	POWER PreviousYear;
	POWER Month[12];
	POWER PreviousDay;
} PWMT_ARCHIVE;
PWMT_ARCHIVE __attribute__((aligned(4))) pwmt_arch;

/*
X0h  Все нормально.
X1h  Недопустимая команда или параметр.
X2h  Внутренняя ошибка счетчика.
X3h  Не достаточен уровень доступа для удовлетворения запроса.
X4h  Внутренние часы счетчика уже корректировались в течение текущих суток.
X5h  Не открыт канал связи
*/

#endif
