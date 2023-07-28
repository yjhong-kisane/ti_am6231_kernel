/**
@file       kisan_common.h
@brief      am623x device driver 공통 헤더
@details    am623x device driver 공통 헤더
@date       2023-04-11
@author     hong.yongje@kisane.com
*/

#ifndef _LINUX_KISAN_H
#define _LINUX_KISAN_H


#include <linux/miscdevice.h>			// miscdevice

#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>     // module_platform_driver for Device Tree




#if !defined(TRUE)
#define TRUE 	(1)
#endif	// #if !defined(TRUE)
#if !defined(FALSE)
#define FALSE 	(0)
#endif	// #if !defined(FALSE)

#define __ON__      (1)
#define __OFF__     (0)

#define __YES__      (1)
#define __NO__     (0)

#define __HI__      (1)
#define __LO__     (0)

#define __IN__     (1)
#define __OUT__      (0)

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)



/**
@def        Macros
@date       2016-10-19
@author     hong.yongje@kisane.com
*/

/* 한 비트 클리어 */
#define BIT_CLR(data, loc)    ((data) &= ~(0x1 << (loc)))
/* 연속된 여러 비트 클리어 */
#define BITS_CLR(data, area, loc)     ((data) &= ~((area) << (loc)))

/* 한 비트 설정 */
#define BIT_SET(data, loc)      ((data) |= (0x1 << (loc)))
/* 연속된 여러 비트 설정 */
#define BITS_SET(data, area, loc)  ((data) |= ((area) << (loc)))

/* 한 비트 반전 */
#define BIT_INVERT(data, loc)   ((data) ^= (0x1 << (loc)))
/* 연속된 여러 비트 반전 */
#define BITS_INVERT(data, area, loc)    ((data) ^= ((area) << (loc)))

/* 비트 검사 */
#define BIT_CHECK(data, loc)    ((data) & (0x1 << (loc)))

/* 비트 추출 */
#define BITS_EXTRACT(data, area, loc)   (((data) >> (loc)) & (area))




//#define GPIO_TO_BANK(gpio)	((gpio) / 32)
//#define GPIO_TO_PINS(gpio)	((gpio) % 32)




/**
@def        AM572x gpio pin number definitions
@date       2020-08-31
@author     hong.yongje@kisane.com
*/

// NOTE::20200831
// GPIO Numbering을 기존 AM335x 에서 (32 * n) + x 였던 것과 달리
// AM572x 에서는 (32 * (n - 1)) + x 로 계산해야 함
//#define GPIO_TO_PIN(bank, bank_gpio) (32 * (bank - 1) + (bank_gpio))


//-----------------------------------------------------------------------------------------------

// For ~/arch/arm/mach-omap2/board-generic.c
//#define GPIO_PIN_NUM_LAN8710_RESET		GPIO_TO_PIN(3, 30)





/**
@def        am572x Shared Memory(DDR) UDP Protocol definitions
@date       2022-01-06
@author     hong.yongje@kisane.com
*/

#define DEFAULT_UDP_PORT                        (7890)

#define UDP_CMD_KISAN_APL_NOTI                  (0x01)

#define UDP_CMD_KISAN_WIFI_ATTACHED             (0x02)
#define UDP_CMD_KISAN_WIFI_DETACHED             (0x03)

#define UDP_CMD_KISAN_LTE_ATTACHED              (0x04)
#define UDP_CMD_KISAN_LTE_DETACHED              (0x05)

#define UDP_CMD_KISAN_USB_BULK_COMM				(0x06)

//#define UDP_CMD_KISAN_COOLING_FAN_STATUS        (0x06)	// NOTE::20220422 Qt Apl 에서 검사하므로 미사용

//#define UDP_CMD_KISAN_USB1_OVER_CURRENT         (0x07)


/**
@def        am572x Shared Memory(DDR) Structure definitions
@date       2022-01-06
@author     hong.yongje@kisane.com
*/

#define ADDR_DDR_CMEM_SHARED_DATA_BUF                  (0x9f000000)	// NOTE:: 20210415, 0x9f00_0000 부터 16 MB (CMEM 0xa000_0000 전 까지) 가용 영역
#define LENGTH_OF_DDR_SHM_DATA_KEY                     (3)    // 3Bytes


#define LENGTH_OF_DDR_SHM_DATA_FAN_STATUS   		(2)    // 2Bytes


#endif	/* _LINUX_KISAN_H */


