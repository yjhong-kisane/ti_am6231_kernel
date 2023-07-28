/**
@file       kisan_gpio_handler.c
@brief      AM623x System B/D Character Device Driver
@details    GPIO Handler (/dev/kisan_gpio_handler)
@date       2023-07-11
@author     hong.yongje@kisane.com
*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/gpio.h>          // gpio_set/get_value
#include <linux/input.h>         // system call
#include <linux/uaccess.h>       // copy_to_user, copy_from_user
#include <asm/io.h>              // ioremap
#include <linux/delay.h>         // mdelay

#include <kisan/kisan_common.h>
#include <kisan/cslr_soc_baseaddress.h>
#include <kisan/cslr_gpio.h>


struct StructKisanGpioHandlerData {
	struct platform_device *pdev;
	struct miscdevice mdev;
};

static int g_nDbgMsgFlag = __OFF__;

struct structGpioHandler {
    int nCmdType;   // 0=SetDirection, 1=GetDirection, 2=SetValue, 3=GetValue, 4=TogglePin
    int nGpioBank;
    int nGpioNum;
    int nCmdData;
    int nDbgMsgFlag;
};

typedef enum {
    CMD_TYPE_GPIO_HANDLER_SET_DIRECTION = 0,
    CMD_TYPE_GPIO_HANDLER_GET_DIRECTION,
    CMD_TYPE_GPIO_HANDLER_SET_VALUE,
    CMD_TYPE_GPIO_HANDLER_GET_VALUE,
    CMD_TYPE_GPIO_HANDLER_TOGGLE_PIN,
    MAX_NUM_OF_CMD_TYPE_GPIO_HANDLER
} eCommandTypeGpioHandler;


//extern void BitsDisplay(int nVal, int nBitLen);
extern void BitsDisplayU32(unsigned int unVal);


static int GetGpioBankNum(int nGpioNum);
static int GetGpioRegBit(int nGpioNum);
static int GetGpioDirection(int nGpioInstanceNum, int nGpioNum);
static int SetGpioDirection(int nGpioInstanceNum, int nGpioNum, int nDirection);
static int GetGpioValue(int nGpioInstanceNum, int nGpioNum);
static int SetGpioValue(int nGpioInstanceNum, int nGpioNum, int nLowHi);
static int ToggleGpioPin(int nGpioBank, int nGpioNum, int nMilliSec);


static int GetGpioBankNum(int nGpioNum)
{
	/*
		DATASHEET pg. 63~65, Table 6-22, 6-23
		TRM pg. 2375~2376, GPIO1 Register/Pin Mapping
	*/
	return nGpioNum / 16;
}

static int GetGpioRegBit(int nGpioNum)
{
	/*
		DATASHEET pg. 63~65, Table 6-22, 6-23
		TRM pg. 2375~2376, GPIO1 Register/Pin Mapping
	*/
	return nGpioNum % 32;
}

/*
	GPIO1_0(311)  : Bank(0), RegBit(0)
	GPIO1_1(312)  : Bank(0), RegBit(1)
	GPIO1_2(313)  : Bank(0), RegBit(2)
	GPIO1_3(314)  : Bank(0), RegBit(3)
	GPIO1_4(315)  : Bank(0), RegBit(4)
	GPIO1_5(316)  : Bank(0), RegBit(5)
	GPIO1_6(317)  : Bank(0), RegBit(6)
	GPIO1_7(318)  : Bank(0), RegBit(7)
	GPIO1_8(319)  : Bank(0), RegBit(8)
	GPIO1_9(320)  : Bank(0), RegBit(9)
	GPIO1_10(321) : Bank(0), RegBit(10)
	GPIO1_11(322) : Bank(0), RegBit(11)
	GPIO1_12(323) : Bank(0), RegBit(12)
	GPIO1_13(324) : Bank(0), RegBit(13)
	GPIO1_14(325) : Bank(0), RegBit(14)
	GPIO1_15(326) : Bank(0), RegBit(15)
	GPIO1_16(327) : Bank(1), RegBit(16)
	GPIO1_17(328) : Bank(1), RegBit(17)
	GPIO1_18(329) : Bank(1), RegBit(18)
	GPIO1_19(330) : Bank(1), RegBit(19)
	GPIO1_20(331) : Bank(1), RegBit(20)
	GPIO1_21(332) : Bank(1), RegBit(21)
	GPIO1_22(333) : Bank(1), RegBit(22)
	GPIO1_23(334) : Bank(1), RegBit(23)
	GPIO1_24(335) : Bank(1), RegBit(24)
	GPIO1_25(336) : Bank(1), RegBit(25)
	GPIO1_26(337) : Bank(1), RegBit(26)
	GPIO1_27(338) : Bank(1), RegBit(27)
	GPIO1_28(339) : Bank(1), RegBit(28)
	GPIO1_29(340) : Bank(1), RegBit(29)
	GPIO1_30(341) : Bank(1), RegBit(30)
	GPIO1_31(342) : Bank(1), RegBit(31)
	GPIO1_32(343) : Bank(2), RegBit(0)
	GPIO1_33(344) : Bank(2), RegBit(1)
	GPIO1_34(345) : Bank(2), RegBit(2)
	GPIO1_35(346) : Bank(2), RegBit(3)
	GPIO1_36(347) : Bank(2), RegBit(4)
	GPIO1_37(348) : Bank(2), RegBit(5)
	GPIO1_38(349) : Bank(2), RegBit(6)
	GPIO1_39(350) : Bank(2), RegBit(7)
	GPIO1_40(351) : Bank(2), RegBit(8)
	GPIO1_41(352) : Bank(2), RegBit(9)
	GPIO1_42(353) : Bank(2), RegBit(10)
	GPIO1_43(354) : Bank(2), RegBit(11)
	GPIO1_44(355) : Bank(2), RegBit(12)
	GPIO1_45(356) : Bank(2), RegBit(13)
	GPIO1_46(357) : Bank(2), RegBit(14)
	GPIO1_47(358) : Bank(2), RegBit(15)
	GPIO1_48(359) : Bank(3), RegBit(16)
	GPIO1_49(360) : Bank(3), RegBit(17)
	GPIO1_50(361) : Bank(3), RegBit(18)
	GPIO1_51(362) : Bank(3), RegBit(19)

	GPIO0_0(399)  : Bank(0), RegBit(0)
	GPIO0_1(400)  : Bank(0), RegBit(1)
	GPIO0_2(401)  : Bank(0), RegBit(2)
	GPIO0_3(402)  : Bank(0), RegBit(3)
	GPIO0_4(403)  : Bank(0), RegBit(4)
	GPIO0_5(404)  : Bank(0), RegBit(5)
	GPIO0_6(405)  : Bank(0), RegBit(6)
	GPIO0_7(406)  : Bank(0), RegBit(7)
	GPIO0_8(407)  : Bank(0), RegBit(8)
	GPIO0_9(408)  : Bank(0), RegBit(9)
	GPIO0_10(409) : Bank(0), RegBit(10)
	GPIO0_11(410) : Bank(0), RegBit(11)
	GPIO0_12(411) : Bank(0), RegBit(12)
	GPIO0_13(412) : Bank(0), RegBit(13)
	GPIO0_14(413) : Bank(0), RegBit(14)
	GPIO0_15(414) : Bank(0), RegBit(15)
	GPIO0_16(415) : Bank(1), RegBit(16)
	GPIO0_17(416) : Bank(1), RegBit(17)
	GPIO0_18(417) : Bank(1), RegBit(18)
	GPIO0_19(418) : Bank(1), RegBit(19)
	GPIO0_20(419) : Bank(1), RegBit(20)
	GPIO0_21(420) : Bank(1), RegBit(21)
	GPIO0_22(421) : Bank(1), RegBit(22)
	GPIO0_23(422) : Bank(1), RegBit(23)
	GPIO0_24(423) : Bank(1), RegBit(24)
	GPIO0_25(424) : Bank(1), RegBit(25)
	GPIO0_26(425) : Bank(1), RegBit(26)
	GPIO0_27(426) : Bank(1), RegBit(27)
	GPIO0_28(427) : Bank(1), RegBit(28)
	GPIO0_29(428) : Bank(1), RegBit(29)
	GPIO0_30(429) : Bank(1), RegBit(30)
	GPIO0_31(430) : Bank(1), RegBit(31)
	GPIO0_32(431) : Bank(2), RegBit(0)
	GPIO0_33(432) : Bank(2), RegBit(1)
	GPIO0_34(433) : Bank(2), RegBit(2)
	GPIO0_35(434) : Bank(2), RegBit(3)
	GPIO0_36(435) : Bank(2), RegBit(4)
	GPIO0_37(436) : Bank(2), RegBit(5)
	GPIO0_38(437) : Bank(2), RegBit(6)
	GPIO0_39(438) : Bank(2), RegBit(7)
	GPIO0_40(439) : Bank(2), RegBit(8)
	GPIO0_41(440) : Bank(2), RegBit(9)
	GPIO0_42(441) : Bank(2), RegBit(10)
	GPIO0_43(442) : Bank(2), RegBit(11)
	GPIO0_44(443) : Bank(2), RegBit(12)
	GPIO0_45(444) : Bank(2), RegBit(13)
	GPIO0_46(445) : Bank(2), RegBit(14)
	GPIO0_47(446) : Bank(2), RegBit(15)
	GPIO0_48(447) : Bank(3), RegBit(16)
	GPIO0_49(448) : Bank(3), RegBit(17)
	GPIO0_50(449) : Bank(3), RegBit(18)
	GPIO0_51(450) : Bank(3), RegBit(19)
	GPIO0_52(451) : Bank(3), RegBit(20)
	GPIO0_53(452) : Bank(3), RegBit(21)
	GPIO0_54(453) : Bank(3), RegBit(22)
	GPIO0_55(454) : Bank(3), RegBit(23)
	GPIO0_56(455) : Bank(3), RegBit(24)
	GPIO0_57(456) : Bank(3), RegBit(25)
	GPIO0_58(457) : Bank(3), RegBit(26)
	GPIO0_59(458) : Bank(3), RegBit(27)
	GPIO0_60(459) : Bank(3), RegBit(28)
	GPIO0_61(460) : Bank(3), RegBit(29)
	GPIO0_62(461) : Bank(3), RegBit(30)
	GPIO0_63(462) : Bank(3), RegBit(31)
	GPIO0_64(463) : Bank(4), RegBit(0)
	GPIO0_65(464) : Bank(4), RegBit(1)
	GPIO0_66(465) : Bank(4), RegBit(2)
	GPIO0_67(466) : Bank(4), RegBit(3)
	GPIO0_68(467) : Bank(4), RegBit(4)
	GPIO0_69(468) : Bank(4), RegBit(5)
	GPIO0_70(469) : Bank(4), RegBit(6)
	GPIO0_71(470) : Bank(4), RegBit(7)
	GPIO0_72(471) : Bank(4), RegBit(8)
	GPIO0_73(472) : Bank(4), RegBit(9)
	GPIO0_74(473) : Bank(4), RegBit(10)
	GPIO0_75(474) : Bank(4), RegBit(11)
	GPIO0_76(475) : Bank(4), RegBit(12)
	GPIO0_77(476) : Bank(4), RegBit(13)
	GPIO0_78(477) : Bank(4), RegBit(14)
	GPIO0_79(478) : Bank(4), RegBit(15)
	GPIO0_80(479) : Bank(5), RegBit(16)
	GPIO0_81(480) : Bank(5), RegBit(17)
	GPIO0_82(481) : Bank(5), RegBit(18)
	GPIO0_83(482) : Bank(5), RegBit(19)
	GPIO0_84(483) : Bank(5), RegBit(20)
	GPIO0_85(484) : Bank(5), RegBit(21)
	GPIO0_86(485) : Bank(5), RegBit(22)
GPIO0_87(486) : Bank(5), RegBit(23) <- MCU_GPIO0_0 중복(486)
GPIO0_88(487) : Bank(5), RegBit(24) <- MCU_GPIO0_1 중복(487)
GPIO0_89(488) : Bank(5), RegBit(25) <- MCU_GPIO0_2 중복(488)
GPIO0_90(489) : Bank(5), RegBit(26) <- MCU_GPIO0_3 중복(489)
GPIO0_91(490) : Bank(5), RegBit(27) <- MCU_GPIO0_4 중복(490)

	MCU_GPIO0_0(486)  : Bank(0), RegBit(0)
	MCU_GPIO0_1(487)  : Bank(0), RegBit(1)
	MCU_GPIO0_2(488)  : Bank(0), RegBit(2)
	MCU_GPIO0_3(489)  : Bank(0), RegBit(3)
	MCU_GPIO0_4(490)  : Bank(0), RegBit(4)
	MCU_GPIO0_5(491)  : Bank(0), RegBit(5)
	MCU_GPIO0_6(492)  : Bank(0), RegBit(6)
	MCU_GPIO0_7(493)  : Bank(0), RegBit(7)
	MCU_GPIO0_8(494)  : Bank(0), RegBit(8)
	MCU_GPIO0_9(495)  : Bank(0), RegBit(9)
	MCU_GPIO0_10(496) : Bank(0), RegBit(10)
	MCU_GPIO0_11(497) : Bank(0), RegBit(11)
	MCU_GPIO0_12(498) : Bank(0), RegBit(12)
	MCU_GPIO0_13(499) : Bank(0), RegBit(13)
	MCU_GPIO0_14(500) : Bank(0), RegBit(14)
	MCU_GPIO0_15(501) : Bank(0), RegBit(15)
	MCU_GPIO0_16(502) : Bank(1), RegBit(16)
	MCU_GPIO0_17(503) : Bank(1), RegBit(17)
	MCU_GPIO0_18(504) : Bank(1), RegBit(18)
	MCU_GPIO0_19(505) : Bank(1), RegBit(19)
	MCU_GPIO0_20(506) : Bank(1), RegBit(20)
	MCU_GPIO0_21(507) : Bank(1), RegBit(21)
	MCU_GPIO0_22(508) : Bank(1), RegBit(22)
	MCU_GPIO0_23(509) : Bank(1), RegBit(23)
*/

#if 1
static unsigned short m_ausGpio0[128];
static unsigned short m_ausGpio1[128];
//static unsigned short m_ausMCU_Gpio0[128];

static void SetGpioLUT(void);
static int GetGpioLUT(int nGpioInstanceNum, int nGpioNum);


static void SetGpioLUT(void)
{
	int i = 0;
	int nPinNum = 0;

	memset(m_ausGpio1, 0, sizeof(m_ausGpio1));
	memset(m_ausGpio0, 0, sizeof(m_ausGpio0));
	//memset(m_ausMCU_Gpio0, 0, sizeof(m_ausMCU_Gpio0));

	// gpiochip3: GPIOs 311-398, parent: platform/601000.gpio, 601000.gpio: GPIO1_0 ~ GPIO1_51
	nPinNum = 311;
	for(i = 0 ; i <= 51 ; i++)
	{
		if(!gpio_is_valid(nPinNum)) {
	    	printk(KERN_ERR "[%s:%4d] Invalid GPIO1_%d \n", __FILENAME__, __LINE__, nPinNum);
		}
		else {
			m_ausGpio1[i] = nPinNum;
			/*
			printk(KERN_DEBUG "[%s:%4d] GPIO1_%d(%d) : Bank(%d), RegBit(%d) \n", 
				__FILENAME__, __LINE__, i, nPinNum, GetGpioBankNum(i), GetGpioRegBit(i));
			*/
		}
		nPinNum++;
	}

	// gpiochip2: GPIOs 399-485, parent: platform/600000.gpio, 600000.gpio: GPIO0_0 ~ GPIO0_86
	nPinNum = 399;
	for(i = 0 ; i <= 86 ; i++)
	{
		if(!gpio_is_valid(nPinNum)) {
	    	printk(KERN_ERR "[%s:%4d] Invalid GPIO0_%d \n", __FILENAME__, __LINE__, nPinNum);
		}
		else {
			m_ausGpio0[i] = nPinNum;
			/*
			printk(KERN_DEBUG "[%s:%4d] GPIO0_%d(%d) : Bank(%d), RegBit(%d) \n", 
				__FILENAME__, __LINE__, i, nPinNum, GetGpioBankNum(i), GetGpioRegBit(i));
			*/
		}
		nPinNum++;
	}

#if 0
	// gpiochip1: GPIOs 486-509, parent: platform/4201000.gpio, 4201000.gpio: MCU_GPIO0_0 ~ MCU_GPIO0_23
	nPinNum = 486;
	for(i = 0 ; i <= 23 ; i++)
	{
		if(!gpio_is_valid(nPinNum)) {
	    	printk(KERN_ERR "[%s:%4d] Invalid MCU_GPIO0_%d \n", __FILENAME__, __LINE__, nPinNum);
		}
		else {
			m_ausMCU_Gpio0[i] = nPinNum;
			/*
			printk(KERN_DEBUG "[%s:%4d] MCU_GPIO0_%d(%d) : Bank(%d), RegBit(%d) \n", 
				__FILENAME__, __LINE__, i, nPinNum, GetGpioBankNum(i), GetGpioRegBit(i));
			*/
		}
		nPinNum++;
	}
#endif	// #if 0

	return;
}

static int GetGpioLUT(int nGpioInstanceNum, int nGpioNum)
{
	int nGpioNumLUT = -1;

	if(nGpioInstanceNum == 1)
	{
		// gpiochip3: GPIOs 311-398, parent: platform/601000.gpio, 601000.gpio: GPIO1_0 ~ GPIO1_51
		if( (nGpioNum < 0) || (nGpioNum > 51) ) {
			printk(KERN_ERR "[%s:%4d:%s] Wrong Gpio Num (%d) \n",
				__FILENAME__, __LINE__, __FUNCTION__, nGpioNum);
			return -1;
		}
		else {
			nGpioNumLUT = m_ausGpio1[nGpioNum];
		}
	}
	else if(nGpioInstanceNum == 0)
	{
		// gpiochip2: GPIOs 399-485, parent: platform/600000.gpio, 600000.gpio: GPIO0_0 ~ GPIO0_86
		if( (nGpioNum < 0) || (nGpioNum > 86) ) {
			printk(KERN_ERR "[%s:%4d:%s] Wrong Gpio Num (%d) \n",
				__FILENAME__, __LINE__, __FUNCTION__, nGpioNum);
			return -1;
		}
		else {
			nGpioNumLUT = m_ausGpio0[nGpioNum];
		}
	}
	else
	{
		printk(KERN_ERR "[%s:%4d:%s] Wrong Gpio Instance Number (%d) \n",
            __FILENAME__, __LINE__, __FUNCTION__, nGpioInstanceNum);
        return -1;
	}

	return nGpioNumLUT;
}
#endif	// #if 0

/*
	TRM pg. 8781, 12.2.5.1 GPIO Registers
	Table 12-366. mem, GPIO0_ Registers, Base Address=0060_0000H, Length=256
	----------------------------------------------------------------------
	Offset  Length Register Name  GPIO0          GPIO1          MCU_GPIO0
	----------------------------------------------------------------------
	...
	10h     32     GPIO_DIR01     0060_0010h     0060_1010h     0420_1010h
	...
	38h     32     GPIO_DIR23     0060_0038h     0060_1038h     0420_1038h
	...
	60h     32     GPIO_DIR45     0060_0060h     0060_1060h     0420_1060h
	...
	88h     32     GPIO_DIR67     0060_0088h     0060_1088h     0420_1088h
	...
	B0h     32     GPIO_DIR8      0060_00B0h     0060_10B0h     0420_10B0h
	...
*/
static int GetGpioDirection(int nGpioInstanceNum, int nGpioNum)
{
	int nRet = -1;
    void __iomem *iomem;
	unsigned int unRegVal = 0;
	unsigned int unOffset = 0;

	int nGpioBank = GetGpioBankNum(nGpioNum);
	int nGpioRegBit = GetGpioRegBit(nGpioNum);

	//TRM P.8781 - 12.2.5.1 GPIO Registers
	//Table 12-366. mem, GPIO0_ Registers, Base Address=0060 0000H, Length=256
	if(nGpioInstanceNum == 0) {
		iomem = ioremap(CSL_GPIO0_BASE, CSL_GPIO0_SIZE);	// 0x60_0000
	} 
	else if(nGpioInstanceNum == 1) {
		iomem = ioremap(CSL_GPIO1_BASE, CSL_GPIO1_SIZE);	// 0x60_1000
	}
	else {
		printk (KERN_ERR "[%s:%4d:%s] TRACE:\n",
            __FILENAME__, __LINE__, __FUNCTION__);
		return -1;
	}

	if(!iomem) {
        printk (KERN_ERR "[%s:%4d:%s] Failed to ioremap CSL_GPIO#_BASE.\n",
            __FILENAME__, __LINE__, __FUNCTION__);
        return -1;
    }
	
	unOffset = 0x10;	// GPIO_DIR01
	if(nGpioBank == 0 || nGpioBank == 1) {
		unOffset += (0x28 * 0);
	}
	else if(nGpioBank == 2 || nGpioBank == 3) {
		unOffset += (0x28 * 1);
	}
	else if(nGpioBank == 4 || nGpioBank == 5) {
		unOffset += (0x28 * 2);
	}
	else if(nGpioBank == 6 || nGpioBank == 7) {
		unOffset += (0x28 * 3);
	}
	else if(nGpioBank == 8) {
		unOffset += (0x28 * 4);
	}
	else {
		printk (KERN_ERR "[%s:%4d:%s] TRACE:\n",
            __FILENAME__, __LINE__, __FUNCTION__);
		iounmap(iomem);
		return -1;
	}

	unRegVal = ioread32(iomem + unOffset);
	iounmap(iomem);

	nRet = BITS_EXTRACT(BIT_CHECK(unRegVal, nGpioRegBit), 1, nGpioRegBit);

	//printk(KERN_DEBUG "[%s:%4d] Offset(0x%x), RegVal(0x%x), RegBit(%d), Ret(%d)\n", 
	//	__FILENAME__, __LINE__, unOffset, unRegVal, nGpioRegBit, nRet);

    return nRet;
}

static int SetGpioDirection(int nGpioInstanceNum, int nGpioNum, int nDirection)
{
#if 1
	// NOTE::2023-07-11
	// (참고) https://www.kernel.org/doc/html/v5.10/driver-api/gpio/legacy.html
	int nGpioNumLUT = GetGpioLUT(nGpioInstanceNum, nGpioNum);
	if(nGpioNumLUT <= 0) {
		printk (KERN_ERR "[%s:%4d:%s] TRACE:\n",
            __FILENAME__, __LINE__, __FUNCTION__);
		return -1;
	}
	
	if(nDirection == __OUT__) {
		gpio_direction_output(nGpioNumLUT, __LO__);
	}
	else if(nDirection == __IN__) {
		gpio_direction_input(nGpioNumLUT);
	}
	else {
		printk (KERN_ERR "[%s:%4d:%s] TRACE:\n",
            __FILENAME__, __LINE__, __FUNCTION__);
		return -1;
	}
#else
    void __iomem *iomem;
	unsigned int unRegVal = 0;
	unsigned int unOffset = 0;

	int nGpioBank = GetGpioBankNum(nGpioNum);
	int nGpioRegBit = GetGpioRegBit(nGpioNum);

	//TRM P.8781 - 12.2.5.1 GPIO Registers
	//Table 12-366. mem, GPIO0_ Registers, Base Address=0060 0000H, Length=256
	if(nGpioInstanceNum == 0) {
		iomem = ioremap(CSL_GPIO0_BASE, CSL_GPIO0_SIZE);	// 0x60_0000
	} 
	else if(nGpioInstanceNum == 1) {
		iomem = ioremap(CSL_GPIO1_BASE, CSL_GPIO1_SIZE);	// 0x60_1000
	}
	else {
		printk (KERN_ERR "[%s:%4d:%s] TRACE:\n",
            __FILENAME__, __LINE__, __FUNCTION__);
		return -1;
	}

	if(!iomem) {
        printk (KERN_ERR "[%s:%4d:%s] Failed to ioremap CSL_GPIO#_BASE.\n",
            __FILENAME__, __LINE__, __FUNCTION__);
        return -1;
    }
	
	unOffset = 0x10;	// GPIO_DIR01
	if(nGpioBank == 0 || nGpioBank == 1) {
		unOffset += (0x28 * 0);
	}
	else if(nGpioBank == 2 || nGpioBank == 3) {
		unOffset += (0x28 * 1);
	}
	else if(nGpioBank == 4 || nGpioBank == 5) {
		unOffset += (0x28 * 2);
	}
	else if(nGpioBank == 6 || nGpioBank == 7) {
		unOffset += (0x28 * 3);
	}
	else if(nGpioBank == 8) {
		unOffset += (0x28 * 4);
	}
	else {
		printk (KERN_ERR "[%s:%4d:%s] TRACE:\n",
            __FILENAME__, __LINE__, __FUNCTION__);
		iounmap(iomem);
		return -1;
	}

	unRegVal = ioread32(iomem + unOffset);

	// TRM P.8786 - 12.2.5.1.4.1 GPIO0_DIR01 Register (Offset = 10h) [reset = ffffffffh]
	// Direction of GPIO bank 1 bits, 0 = output, 1 = input.
	if(nDirection == __OUT__) {
		BIT_CLR(unRegVal, nGpioRegBit);
	}
	else if(nDirection == __IN__) {
		BIT_SET(unRegVal, nGpioRegBit);
	}
	else {
		printk (KERN_ERR "[%s:%4d:%s] TRACE:\n",
            __FILENAME__, __LINE__, __FUNCTION__);
		iounmap(iomem);
		return -1;
	}
	
	iowrite32(unRegVal, iomem + unOffset);
	iounmap(iomem);
#endif

    return 0;
}

static int GetGpioValue(int nGpioInstanceNum, int nGpioNum)
{
	int nRet = -1;
    void __iomem *iomem;
	unsigned int unRegVal = 0;
	unsigned int unOffset = 0;

	int nGpioBank = GetGpioBankNum(nGpioNum);
	int nGpioRegBit = GetGpioRegBit(nGpioNum);

	//TRM P.8781 - 12.2.5.1 GPIO Registers
	//Table 12-366. mem, GPIO0_ Registers, Base Address=0060 0000H, Length=256
	if(nGpioInstanceNum == 0) {
		iomem = ioremap(CSL_GPIO0_BASE, CSL_GPIO0_SIZE);	// 0x60_0000
	} 
	else if(nGpioInstanceNum == 1) {
		iomem = ioremap(CSL_GPIO1_BASE, CSL_GPIO1_SIZE);	// 0x60_1000
	}
	else {
		printk (KERN_ERR "[%s:%4d:%s] TRACE:\n",
            __FILENAME__, __LINE__, __FUNCTION__);
		return -1;
	}

	if(!iomem) {
        printk (KERN_ERR "[%s:%4d:%s] Failed to ioremap CSL_GPIO#_BASE.\n",
            __FILENAME__, __LINE__, __FUNCTION__);
        return -1;
    }
	
	unOffset = 0x14;	// GPIO_OUT_DATA01
	//unOffset = 0x18;	// GPIO_SET_DATA01
	//unOffset = 0x1c;	// GPIO_CLR_DATA01
	//unOffset = 0x20;	// GPIO_IN_DATA01
	if(nGpioBank == 0 || nGpioBank == 1) {
		unOffset += (0x28 * 0);
	}
	else if(nGpioBank == 2 || nGpioBank == 3) {
		unOffset += (0x28 * 1);
	}
	else if(nGpioBank == 4 || nGpioBank == 5) {
		unOffset += (0x28 * 2);
	}
	else if(nGpioBank == 6 || nGpioBank == 7) {
		unOffset += (0x28 * 3);
	}
	else if(nGpioBank == 8) {
		unOffset += (0x28 * 4);
	}
	else {
		printk (KERN_ERR "[%s:%4d:%s] TRACE:\n",
            __FILENAME__, __LINE__, __FUNCTION__);
		iounmap(iomem);
		return -1;
	}

	unRegVal = ioread32(iomem + unOffset);
	iounmap(iomem);

	nRet = BITS_EXTRACT(BIT_CHECK(unRegVal, nGpioRegBit), 1, nGpioRegBit);

	//printk(KERN_DEBUG "[%s:%4d] Offset(0x%x), RegVal(0x%x), RegBit(%d), Ret(%d)\n", 
	//	__FILENAME__, __LINE__, unOffset, unRegVal, nGpioRegBit, nRet);

    return nRet;
}

static int SetGpioValue(int nGpioInstanceNum, int nGpioNum, int nLowHi)
{
#if 1
	// NOTE::2023-07-11
	// (참고) https://www.kernel.org/doc/html/v5.10/driver-api/gpio/legacy.html
	int nGpioNumLUT = GetGpioLUT(nGpioInstanceNum, nGpioNum);
	if(nGpioNumLUT <= 0) {
		printk (KERN_ERR "[%s:%4d:%s] TRACE:\n",
            __FILENAME__, __LINE__, __FUNCTION__);
		return -1;
	}

	if(nLowHi == __LO__ || nLowHi == __HI__) {
		gpio_set_value(nGpioNumLUT, nLowHi);
	}
	else {
		printk (KERN_ERR "[%s:%4d:%s] TRACE:\n",
            __FILENAME__, __LINE__, __FUNCTION__);
		return -1;
	}
#else
    void __iomem *iomem;
	unsigned int unRegVal = 0;
	unsigned int unOffset = 0;

	int nGpioBank = GetGpioBankNum(nGpioNum);
	int nGpioRegBit = GetGpioRegBit(nGpioNum);

	//TRM P.8781 - 12.2.5.1 GPIO Registers
	//Table 12-366. mem, GPIO0_ Registers, Base Address=0060 0000H, Length=256
	if(nGpioInstanceNum == 0) {
		iomem = ioremap(CSL_GPIO0_BASE, CSL_GPIO0_SIZE);	// 0x60_0000
	} 
	else if(nGpioInstanceNum == 1) {
		iomem = ioremap(CSL_GPIO1_BASE, CSL_GPIO1_SIZE);	// 0x60_1000
	}
	else {
		printk (KERN_ERR "[%s:%4d:%s] TRACE:\n",
            __FILENAME__, __LINE__, __FUNCTION__);
		return -1;
	}

	if(!iomem) {
        printk (KERN_ERR "[%s:%4d:%s] Failed to ioremap CSL_GPIO#_BASE.\n",
            __FILENAME__, __LINE__, __FUNCTION__);
        return -1;
    }
	
	if(nLowHi == __HI__) {
		unOffset = 0x18;	// GPIO_SET_DATA01
	}
	else if(nLowHi == __LO__) {
		unOffset = 0x1c;	// GPIO_CLR_DATA01
	}
	else {
		printk (KERN_ERR "[%s:%4d:%s] TRACE:\n",
            __FILENAME__, __LINE__, __FUNCTION__);
		iounmap(iomem);
		return -1;
	}
	
	if(nGpioBank == 0 || nGpioBank == 1) {
		unOffset += (0x28 * 0);
	}
	else if(nGpioBank == 2 || nGpioBank == 3) {
		unOffset += (0x28 * 1);
	}
	else if(nGpioBank == 4 || nGpioBank == 5) {
		unOffset += (0x28 * 2);
	}
	else if(nGpioBank == 6 || nGpioBank == 7) {
		unOffset += (0x28 * 3);
	}
	else if(nGpioBank == 8) {
		unOffset += (0x28 * 4);
	}
	else {
		printk (KERN_ERR "[%s:%4d:%s] TRACE:\n",
            __FILENAME__, __LINE__, __FUNCTION__);
		iounmap(iomem);
		return -1;
	}

	if(nLowHi == __HI__) {
		unRegVal = ioread32(iomem + unOffset);
	}
	else if(nLowHi == __LO__) {
		unRegVal = 0;
	}

/*	
	printk(KERN_DEBUG "[%s:%4d] Offset(0x%x), RegVal(0x%x), RegBit(%d)\n", 
		__FILENAME__, __LINE__, unOffset, unRegVal, nGpioRegBit);
	BitsDisplayU32(unRegVal);
*/

	BIT_SET(unRegVal, nGpioRegBit);

/*
	BitsDisplayU32(unRegVal);
	printk(KERN_DEBUG "[%s:%4d] Offset(0x%x), RegVal(0x%x), RegBit(%d)\n", 
		__FILENAME__, __LINE__, unOffset, unRegVal, nGpioRegBit);
*/

	iowrite32(unRegVal, iomem + unOffset);
	iounmap(iomem);
#endif

	return 0;
}

static int ToggleGpioPin(int nGpioBank, int nGpioNum, int nMilliSec)
{
    int nRet = 0;
    int nVal = GetGpioValue(nGpioBank, nGpioNum);

    if(nVal == __LO__) {
		nRet = SetGpioValue(nGpioBank, nGpioNum, __HI__);
	}
	else if(nVal == __HI__) {
		nRet = SetGpioValue(nGpioBank, nGpioNum, __LO__);
	}
	else {
		printk (KERN_ERR "[%s:%4d:%s] GPIO%d_%d: %d ???\n",
            __FILENAME__, __LINE__, __FUNCTION__, nGpioBank, nGpioNum, nVal);
		return -1;
	}
	
	if(nRet >= 0 && nMilliSec > 0) {
        schedule_timeout_interruptible((nMilliSec * HZ) / 1000);
        nRet = SetGpioValue(nGpioBank, nGpioNum, nVal);
    }
	else {
		printk (KERN_ERR "[%s:%4d:%s] TRACE:\n",
            __FILENAME__, __LINE__, __FUNCTION__);
		return -1;
	}

    return nRet;
}

static int kisan_gpio_handler_open(struct inode *inode, struct file *file)
{
//  printk(KERN_DEBUG "[%s:%4d:%s] \n", __FILENAME__, __LINE__, __FUNCTION__);
    return 0;
}

static ssize_t kisan_gpio_handler_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    printk(KERN_DEBUG "[%s:%4d:%s] \n", __FILENAME__, __LINE__, __FUNCTION__);
    return 0;
} 

static ssize_t kisan_gpio_handler_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
    printk(KERN_DEBUG "[%s:%4d:%s] \n", __FILENAME__, __LINE__, __FUNCTION__);
	return 0;
}

static long kisan_gpio_handler_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
    void __user *argp = (void __user *) args;
    struct structGpioHandler stGpioHandler;

    int nCmdType = 0;   // 0= SetDirection, 1=GetDirection, 2=SetValue, 3=GetValue

    if(copy_from_user(&stGpioHandler, argp, sizeof(struct structGpioHandler))) {
        printk(KERN_ERR "[%s:%4d:%s] copy_from_user \n", __FILENAME__, __LINE__, __FUNCTION__);
        return -EFAULT;
    }

    nCmdType = stGpioHandler.nCmdType;
    g_nDbgMsgFlag = stGpioHandler.nDbgMsgFlag;
//    if(g_nDbgMsgFlag == __ON__) {
//        printk(KERN_DEBUG "[%s:%4d:%s] DbgMsg is ON. \n", __FILENAME__, __LINE__, __FUNCTION__);
//    }

    switch(nCmdType)
    {
        case CMD_TYPE_GPIO_HANDLER_SET_DIRECTION:
            if( (SetGpioDirection(stGpioHandler.nGpioBank, stGpioHandler.nGpioNum, stGpioHandler.nCmdData)) < 0 )
                return -EFAULT;
            break;

        case CMD_TYPE_GPIO_HANDLER_GET_DIRECTION:
            stGpioHandler.nCmdData = GetGpioDirection(stGpioHandler.nGpioBank, stGpioHandler.nGpioNum);
            if(copy_to_user(argp, &stGpioHandler, sizeof(struct structGpioHandler)))
                return -EFAULT;
            break;

        case CMD_TYPE_GPIO_HANDLER_SET_VALUE:
            if( (SetGpioValue(stGpioHandler.nGpioBank, stGpioHandler.nGpioNum, stGpioHandler.nCmdData)) < 0 )
                return -EFAULT;
            break;

        case CMD_TYPE_GPIO_HANDLER_GET_VALUE:
            stGpioHandler.nCmdData = GetGpioValue(stGpioHandler.nGpioBank, stGpioHandler.nGpioNum);
            if(copy_to_user(argp, &stGpioHandler, sizeof(struct structGpioHandler)))
                return -EFAULT;
            break;

        case CMD_TYPE_GPIO_HANDLER_TOGGLE_PIN:
            if( (ToggleGpioPin(stGpioHandler.nGpioBank, stGpioHandler.nGpioNum, stGpioHandler.nCmdData)) < 0 ) {
                printk(KERN_DEBUG "[%s:%4d:%s] %d %d %d\n", __FILENAME__, __LINE__, __FUNCTION__,
                    stGpioHandler.nGpioBank, stGpioHandler.nGpioNum, stGpioHandler.nCmdData);
                return -EFAULT;
            }
            break;

        default:
            printk(KERN_ERR "[%s:%4d:%s] Wrong CmdType (%d) \n",
                __FILENAME__, __LINE__, __FUNCTION__, nCmdType);
            return -EINVAL;
            break;
    }

    return 0;
}

static struct file_operations kisan_gpio_handler_fops = {
	.owner   		= THIS_MODULE,
    .open    		= kisan_gpio_handler_open,
    .read    		= kisan_gpio_handler_read,
	.write 			= kisan_gpio_handler_write,
//	.release 		= kisan_gpio_handler_release,
    .unlocked_ioctl = kisan_gpio_handler_ioctl,
//	.fasync 		= kisan_gpio_handler_fasync,
};

static int kisan_gpio_handler_probe(struct platform_device *pdev)
{
	struct StructKisanGpioHandlerData *pdata = NULL;
	int nErr = 0;

//	struct device_node *np = pdev->dev.of_node;
//	u32 u32_DT_Node = 0;

	pdata = devm_kzalloc(&pdev->dev, sizeof(struct StructKisanGpioHandlerData), GFP_KERNEL);
	if(!pdata) {
		printk(KERN_ERR "[%s:%4d] Failed to devm_kzalloc...\n", __FILENAME__, __LINE__);
		return -ENOMEM;
	}

 	// 자신의 platform_device 객체 저장
	pdata->pdev = pdev;
	pdata->mdev.minor = MISC_DYNAMIC_MINOR;
    pdata->mdev.name = "kisan_gpio_handler";
    pdata->mdev.fops = &kisan_gpio_handler_fops;

	/* register a minimal instruction set computer device (misc) */
	nErr = misc_register(&pdata->mdev);
	if(nErr) {
		dev_err(&pdev->dev,"failed to register misc device\n");
		devm_kfree(&pdev->dev, pdata);
		return nErr;
	}
	
 	// platform_device 에 사용할 고유의 구조체 설정
	platform_set_drvdata(pdev, pdata);

	SetGpioLUT();

	printk(KERN_DEBUG "[%s:%4d] Registered kisan_gpio_handler char driver. \n",
			__FILENAME__, __LINE__);

	return 0;
}

static int kisan_gpio_handler_remove(struct platform_device *pdev)
{
	struct StructKisanGpioHandlerData *pdata = platform_get_drvdata(pdev);

	misc_deregister(&pdata->mdev);
	devm_kfree(&pdev->dev, pdata);

    printk(KERN_INFO "[%s:%4d] Unloaded module: kisan_gpio_handler\n", __FILENAME__, __LINE__);

	return 0;
}

static int kisan_gpio_handler_suspend(struct platform_device *pdev, pm_message_t state)
{
	printk(KERN_DEBUG "[%s:%4d:%s]\n", __FILENAME__, __LINE__, __FUNCTION__);
	return 0;
}

static int kisan_gpio_handler_resume(struct platform_device *pdev)
{
	printk(KERN_DEBUG "[%s:%4d:%s]\n", __FILENAME__, __LINE__, __FUNCTION__);
	return 0;
}

#if IS_ENABLED(CONFIG_OF)  
static const struct of_device_id kisan_gpio_handler_dt_ids[] = {  
    {.compatible = "kisan_gpio_handler", },  
    {},  
};
MODULE_DEVICE_TABLE(of, kisan_gpio_handler_dt_ids);  
#endif // #if IS_ENABLED(CONFIG_OF)

static struct platform_driver kisan_gpio_handler_driver = {
	.probe   = kisan_gpio_handler_probe,
	.remove  = kisan_gpio_handler_remove,
	.suspend = kisan_gpio_handler_suspend,
	.resume  = kisan_gpio_handler_resume,
	.driver  = {
        .name           = "kisan_gpio_handler",
        .owner          = THIS_MODULE,
        .of_match_table = of_match_ptr(kisan_gpio_handler_dt_ids), 
    },
};
module_platform_driver(kisan_gpio_handler_driver);


MODULE_DESCRIPTION("AM623x Kisan System B/D GPIO Handling Driver");
MODULE_AUTHOR("hong.yongje@kisane.com");
MODULE_LICENSE("Dual BSD/GPL");
