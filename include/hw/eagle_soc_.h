/*
 * ESP8266 registers
 */

//DPORT{{
#define HOST_INF_SEL                (0x28)
#define DPORT_LINK_DEVICE_SEL       0x000000FF
#define DPORT_LINK_DEVICE_SEL_S     8
#define DPORT_PERI_IO_SWAP          0x000000FF
#define DPORT_PERI_IO_SWAP_S        0
#define PERI_IO_CSPI_OVERLAP        (BIT(7)) // two spi masters on cspi
#define PERI_IO_HSPI_OVERLAP        (BIT(6)) // two spi masters on hspi
#define PERI_IO_HSPI_PRIO           (BIT(5)) // hspi is with the higher prior
#define PERI_IO_UART1_PIN_SWAP      (BIT(3)) // swap uart1 pins (u1rxd <-> u1cts), (u1txd <-> u1rts)
#define PERI_IO_UART0_PIN_SWAP      (BIT(2)) // swap uart0 pins (u0rxd <-> u0cts), (u0txd <-> u0rts)
#define PERI_IO_SPI_PORT_SWAP       (BIT(1)) // swap two spi
#define PERI_IO_UART_PORT_SWAP      (BIT(0)) // swap two uart
//}}

//RTC reg {{
#define REG_RTC_BASE                PERIPHS_RTC_BASEADDR

#define RTC_SLP_VAL                     (REG_RTC_BASE + 0x004) // the target value of RTC_COUNTER for wakeup from light-sleep/deep-sleep
#define RTC_SLP_CNT_VAL                 (REG_RTC_BASE + 0x01C) // the current value of RTC_COUNTER

#define RTC_SCRATCH0                    (REG_RTC_BASE + 0x030) // the register for software to save some values for watchdog reset
#define RTC_SCRATCH1                    (REG_RTC_BASE + 0x034) // the register for software to save some values for watchdog reset
#define RTC_SCRATCH2                    (REG_RTC_BASE + 0x038) // the register for software to save some values for watchdog reset
#define RTC_SCRATCH3                    (REG_RTC_BASE + 0x03C) // the register for software to save some values for watchdog reset

#define RTC_GPIO_OUT                    (REG_RTC_BASE + 0x068) // used by gpio16
#define RTC_GPIO_ENABLE                 (REG_RTC_BASE + 0x074)
#define RTC_GPIO_IN_DATA                (REG_RTC_BASE + 0x08C)
#define RTC_GPIO_CONF                   (REG_RTC_BASE + 0x090)
#define PAD_XPD_DCDC_CONF               (REG_RTC_BASE + 0x0A0)
//}}

