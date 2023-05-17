/**
@file       kisan_gpio_handler.c
@brief      AM623x System B/D Character Device Driver
@details    GPIO Interrupt (/dev/kisan_gpio_handler)
@date       2023-05-17
@author     hong.yongje@kisane.com
*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/gpio.h>          // gpio_set/get_value
#include <linux/input.h>                // system call
#include <linux/uaccess.h>              // copy_to_user, copy_from_user
#include <asm/io.h>              // ioremap
#include <linux/delay.h>            // mdelay

#include <kisan/kisan_common.h>
#include <kisan/cslr_soc_baseaddress.h>
#include <kisan/cslr_gpio.h>


struct StructKisanGpioHandlerData {
	struct platform_device *pdev;
	struct miscdevice mdev;
};


static void __iomem *g_ptrCM_REGS_WKUPAON;		// CSL_MPU_WKUPAON_CM_REGS
static void __iomem *g_ptrCM_REGS_L4PER;		// CSL_MPU_L4PER_CM_CORE_REGS

static void __iomem *g_ptrGpioBase1;
static void __iomem *g_ptrGpioBase2;
static void __iomem *g_ptrGpioBase3;
static void __iomem *g_ptrGpioBase4;
static void __iomem *g_ptrGpioBase5;
static void __iomem *g_ptrGpioBase6;
static void __iomem *g_ptrGpioBase7;
static void __iomem *g_ptrGpioBase8;


static int g_nDbgMsgFlag = __OFF__;
static int g_nToggleReset = __OFF__;

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



static int GetGpioDirection(int nGpioBank, int nGpioNum)
{
//    unsigned int unRegVal = 0x00000000;
    int nRet = 0;

    void __iomem *iomem = NULL;

    if( (nGpioBank < 1) || (nGpioBank > 8) ) {
        printk(KERN_ERR "[%s:%4d:%s] Wrong Gpio Bank (%d) \n",
            __FILENAME__, __LINE__, __FUNCTION__, nGpioBank);
        return -1;
    }

    if( (nGpioNum < 0) || (nGpioNum > 31) ) {
        printk(KERN_ERR "[%s:%4d:%s] Wrong Gpio Num (%d) \n",
            __FILENAME__, __LINE__, __FUNCTION__, nGpioNum);
        return -1;
    }

    switch(nGpioBank) {
        case 1:    iomem = g_ptrGpioBase1;    break;
        case 2:    iomem = g_ptrGpioBase2;    break;
        case 3:    iomem = g_ptrGpioBase3;    break;
		case 4:    iomem = g_ptrGpioBase4;	  break;
		case 5:    iomem = g_ptrGpioBase5;	  break;
		case 6:    iomem = g_ptrGpioBase6;	  break;
		case 7:    iomem = g_ptrGpioBase7;	  break;
		case 8:    iomem = g_ptrGpioBase8;	  break;
    }

/*
    unRegVal = ioread32 (iomem + GPIO_OE);
    nRet = BIT_CHECK(unRegVal, nGpioNum);

    if(g_nDbgMsgFlag == __ON__) {
        printk(KERN_DEBUG "[%s:%4d:%s] Read GPIO%d_%d direction: %d (0=Out, 1=In) \n",
            __FILENAME__, __LINE__, __FUNCTION__, nGpioBank, nGpioNum, nRet);
        BitsDisplayU32(unRegVal);
    }
*/

    return nRet;
}


static int SetGpioDirection(int nGpioBank, int nGpioNum, int nDirection)
{
    //unsigned int unRegVal = 0x00000000;
    void __iomem *iomem = NULL;
	//int i = 0;

    if( (nGpioBank < 1) || (nGpioBank > 8) ) {
        printk(KERN_ERR "[%s:%4d:%s] Wrong Gpio Bank (%d) \n",
            __FILENAME__, __LINE__, __FUNCTION__, nGpioBank);
        return -1;
    }

    if( (nGpioNum < 0) || (nGpioNum > 31) ) {
        printk(KERN_ERR "[%s:%4d:%s] Wrong Gpio Num (%d) \n",
            __FILENAME__, __LINE__, __FUNCTION__, nGpioNum);
        return -1;
    }

    if( (nDirection != __IN__) && (nDirection != __OUT__) ) {
        printk(KERN_ERR "[%s:%4d:%s] Wrong Gpio direction (%d) \n",
            __FILENAME__, __LINE__, __FUNCTION__, nDirection);
        return -1;
    }

    switch(nGpioBank) {
        case 1:    iomem = g_ptrGpioBase1;    break;
        case 2:    iomem = g_ptrGpioBase2;    break;
        case 3:    iomem = g_ptrGpioBase3;    break;
		case 4:    iomem = g_ptrGpioBase4;	  break;
		case 5:    iomem = g_ptrGpioBase5;	  break;
		case 6:    iomem = g_ptrGpioBase6;	  break;
		case 7:    iomem = g_ptrGpioBase7;	  break;
		case 8:    iomem = g_ptrGpioBase8;	  break;
    }

/*
    unRegVal = ioread32 (iomem + GPIO_OE);

    //unRegVal |= (nDirection << nGpioNum);
    if(nDirection == __OUT__)
	{
        BIT_CLR(unRegVal, nGpioNum);
	    iowrite32 (unRegVal, iomem + GPIO_OE);
		
		for(i = 0 ; i < 2 ; i++)
		{
//			unRegVal = ioread32 (iomem + GPIO_IRQWAKEN(i));
//			BIT_CLR(unRegVal, nGpioNum);
//			iowrite32 (unRegVal, iomem + GPIO_IRQWAKEN(i)); 
			
			unRegVal = ioread32 (iomem + GPIO_IRQSTATUS_SET(i));
			BIT_CLR(unRegVal, nGpioNum);
			iowrite32 (unRegVal, iomem + GPIO_IRQSTATUS_SET(i)); 
		}
	}
    else if(nDirection == __IN__)
	{
        BIT_SET(unRegVal, nGpioNum);
	    iowrite32 (unRegVal, iomem + GPIO_OE); 

		for(i = 0 ; i < 2 ; i++)
		{
//		    unRegVal = ioread32 (iomem + GPIO_IRQWAKEN(i));
//	        BIT_SET(unRegVal, nGpioNum);
//		    iowrite32 (unRegVal, iomem + GPIO_IRQWAKEN(i)); 
			
			unRegVal = ioread32 (iomem + GPIO_IRQSTATUS_SET(i));
			BIT_SET(unRegVal, nGpioNum);
			iowrite32 (unRegVal, iomem + GPIO_IRQSTATUS_SET(i)); 
		}
	}


	if(g_nDbgMsgFlag == __ON__) {
		printk(KERN_DEBUG "[%s:%4d:%s] GPIO%d_%d set direction to %d (0=Out, 1=In) \n",
			__FILENAME__, __LINE__, __FUNCTION__, nGpioBank, nGpioNum, nDirection);
		BitsDisplayU32(unRegVal);
	}
	else {
		if(g_nToggleReset == __ON__)
		{
			mdelay(2);
		}
	}

*/

    return 0;
}


static int GetGpioValue(int nGpioBank, int nGpioNum)
{
    //unsigned int unRegVal = 0x00000000;
    //void __iomem *iomem = NULL;

    int nRet = 0;

    if( (nGpioBank < 1) || (nGpioBank > 8) ) {
        printk(KERN_ERR "[%s:%4d:%s] Wrong Gpio Bank (%d) \n",
            __FILENAME__, __LINE__, __FUNCTION__, nGpioBank);
        return -1;
    }

    if( (nGpioNum < 0) || (nGpioNum > 31) ) {
        printk(KERN_ERR "[%s:%4d:%s] Wrong Gpio Num (%d) \n",
            __FILENAME__, __LINE__, __FUNCTION__, nGpioNum);
        return -1;
    }

/*
#if 1
    switch(nGpioBank) {
        case 1:    iomem = g_ptrGpioBase1;    break;
        case 2:    iomem = g_ptrGpioBase2;    break;
        case 3:    iomem = g_ptrGpioBase3;    break;
		case 4:    iomem = g_ptrGpioBase4;	  break;
		case 5:    iomem = g_ptrGpioBase5;	  break;
		case 6:    iomem = g_ptrGpioBase6;	  break;
		case 7:    iomem = g_ptrGpioBase7;	  break;
		case 8:    iomem = g_ptrGpioBase8;	  break;
    }

	unRegVal = ioread32(iomem + GPIO_OE);

//    if( (BIT_CHECK(unRegVal, nGpioNum)) != __OUT__ ) {
//        printk(KERN_DEBUG "[%s:%4d:%s] GPIO%d_%d direction is not out, can not set value. \n",
//            __FILENAME__, __LINE__, __FUNCTION__, nGpioBank, nGpioNum);
//        return -1;
//    }

	if( (BIT_CHECK(unRegVal, nGpioNum)) == __OUT__ )
	{
	    unRegVal = ioread32(iomem + GPIO_DATAOUT);
	}
	else
	{
	    unRegVal = ioread32(iomem + GPIO_DATAIN);
	}
	

    if(BIT_CHECK(unRegVal, nGpioNum))
        nRet = __HI__;
    else
        nRet = __LO__;

    if(g_nDbgMsgFlag == __ON__) {
        printk(KERN_DEBUG "[%s:%4d:%s] Get GPIO%d_%d value < %s \n",
            __FILENAME__, __LINE__, __FUNCTION__, nGpioBank, nGpioNum, (nRet == __LO__ ? "Low(0)" : "Hi(1)"));
        BitsDisplayU32(unRegVal);
    }

#else
	// API 사용 시 레지스터를 직접 제어하는 것에 비해 느림
    nRet = gpio_get_value((32 * (nGpioBank - 1) + nGpioNum);
#endif
*/

    return nRet;
}


static int SetGpioValue(int nGpioBank, int nGpioNum, int nLowHi)
{
#if 1
    //unsigned int unRegVal = 0x00000000;
    //void __iomem *iomem = NULL;

    if( (nGpioBank < 1) || (nGpioBank > 8) ) {
        printk(KERN_ERR "[%s:%4d:%s] Wrong Gpio Bank (%d) \n",
            __FILENAME__, __LINE__, __FUNCTION__, nGpioBank);
        return -1;
    }

    if( (nGpioNum < 0) || (nGpioNum > 31) ) {
        printk(KERN_ERR "[%s:%4d:%s] Wrong Gpio Num (%d) \n",
            __FILENAME__, __LINE__, __FUNCTION__, nGpioNum);
        return -1;
    }

    if( (nLowHi != __LO__) && (nLowHi != __HI__) ) {
        printk(KERN_ERR "[%s:%4d:%s] Wrong Gpio Value (%d) \n",
            __FILENAME__, __LINE__, __FUNCTION__, nLowHi);
        return -1;
    }
/*
    switch(nGpioBank) {
        case 1:    iomem = g_ptrGpioBase1;    break;
        case 2:    iomem = g_ptrGpioBase2;    break;
        case 3:    iomem = g_ptrGpioBase3;    break;
		case 4:    iomem = g_ptrGpioBase4;	  break;
		case 5:    iomem = g_ptrGpioBase5;	  break;
		case 6:    iomem = g_ptrGpioBase6;	  break;
		case 7:    iomem = g_ptrGpioBase7;	  break;
		case 8:    iomem = g_ptrGpioBase8;	  break;
    }

	// TRM Pg. 7103

    unRegVal = ioread32(iomem + GPIO_OE);
    if( (BIT_CHECK(unRegVal, nGpioNum)) != __OUT__ ) {
        printk(KERN_DEBUG "[%s:%4d:%s] GPIO%d_%d direction is not out, can not set value. \n",
            __FILENAME__, __LINE__, __FUNCTION__, nGpioBank, nGpioNum);
        return -1;
    }

    unRegVal = ioread32(iomem + GPIO_DATAOUT);

    //unRegVal |= (nLowHi << nGpioNum);
    if(nLowHi == __LO__)
        BIT_CLR(unRegVal, nGpioNum);
    else if(nLowHi == __HI__)
        BIT_SET(unRegVal, nGpioNum);

    iowrite32(unRegVal, iomem + GPIO_DATAOUT);

    if(g_nDbgMsgFlag == __ON__) {
        printk(KERN_DEBUG "[%s:%4d:%s] GPIO%d_%d set value > %s *** \n",
            __FILENAME__, __LINE__, __FUNCTION__, nGpioBank, nGpioNum, (nLowHi == __LO__ ? "Low(0)" : "Hi(1)"));
        BitsDisplayU32(unRegVal);
    }
    else {        
        if(g_nToggleReset == __ON__)
        {
            mdelay(2);
        }
    }
    */
#else
	// API 사용 시 레지스터를 직접 제어하는 것에 비해 느림
    gpio_set_value(GPIO_TO_PIN(nGpioBank, nGpioNum), nLowHi);
#endif


    return 0;
}


static int ToggleGpioPin(int nGpioBank, int nGpioNum, int nTime)
{
    int nRet = 0;
    int nVal = 0;
    unsigned long ulEndTime = 0;

    int nIsDirectionChanged = __NO__;

    if(GetGpioDirection(nGpioBank, nGpioNum) != __OUT__) {
        SetGpioDirection(nGpioBank, nGpioNum, __OUT__);
        nIsDirectionChanged = __YES__;
    }

    nVal = GetGpioValue(nGpioBank, nGpioNum);
//    nVal = gpio_get_value((32*nGpioBank)+nGpioNum);

    switch(nVal)
    {
        case __LO__:        nRet = SetGpioValue(nGpioBank, nGpioNum, __HI__);        break;
        case __HI__:        nRet = SetGpioValue(nGpioBank, nGpioNum, __LO__);        break;
//        case __LO__:        gpio_set_value(((32*nGpioBank)+nGpioNum), __HI__);       break;
//        case __HI__:        gpio_set_value(((32*nGpioBank)+nGpioNum), __LO__);       break;

        default:
            printk(KERN_ERR "[%s:%4d:%s] Wrong GPIO%d_%d value: %d \n",
                __FILENAME__, __LINE__, __FUNCTION__, nGpioBank, nGpioNum, nVal);
            nRet = -1;
            break;
    }

    if(nRet >= 0 && nTime > 0)
	{
#if 0
        if( (nTime > 0) && (nTime < 1000) ) {
            udelay(nTime);
        }
        else if( (nTime >= 1000) && (nTime <= 5000) ) {
            mdelay(nTime / 1000);
        }
        else {
            printk(KERN_ERR "[%s:%4d:%s] Wrong GPIO%d_%d toggle delay time: %d \n",
                __FILENAME__, __LINE__, __FUNCTION__, nGpioBank, nGpioNum, nTime);
        }
#else
        // Delay N ms
        ulEndTime = get_jiffies_64() + ((HZ/1000)*nTime);
        while(jiffies < ulEndTime);
#endif

        nRet = SetGpioValue(nGpioBank, nGpioNum, nVal);
//        gpio_set_value(((32*nGpioBank)+nGpioNum), nVal);
    }

    if(nIsDirectionChanged == __YES__)
        SetGpioDirection(nGpioBank, nGpioNum, __IN__);

    return nRet;
}


static int EnableClockGpioBank(int nBankNum)
{
	unsigned int unOffset = 0x0;
//	unsigned int unRegVal = 0x0;


	switch(nBankNum)
	{
		/*
			TRM 404 L4_WKUP Memory Map
			GPIO1	TP_GPIO1_TARG	0x4ae1_0000 	4 KB
		
			TRM 459, 472
			Clock Domain : CD_WKUPAON
		
			TRM 1596
			CM_WKUPAON_GPIO1_CLKCTRL	Offset(0x38),	PhyAddr(0x4ae0_7838)	// CSL_MPU_WKUPAON_CM_REGS
		*/
		case 1:
			{
//				printk(KERN_DEBUG "[%s:%4d:%s] Trying to enable clock: GPIO1\n", __FILENAME__, __LINE__, __FUNCTION__);
				unOffset = 0x38;
#if 1
				iowrite32(0x01, g_ptrCM_REGS_WKUPAON + unOffset);
#else
				unRegVal = ioread32 (g_ptrCM_REGS_WKUPAON + unOffset);
//				BitsDisplayU32(unRegVal);
				BIT_SET(unRegVal,  0);
				BIT_CLR(unRegVal,  1);
				BIT_CLR(unRegVal, 16);
				BIT_CLR(unRegVal, 17);
//				BitsDisplayU32(unRegVal);
				iowrite32 (unRegVal, g_ptrCM_REGS_WKUPAON + unOffset); 
#endif
			}
			break;

		
		/*
		 TRM Pg. 1072, 1073, 1074, 1075
		
		 Table 3-1113. CM_L4PER_GPIO2_CLKCTRL: Offset(0x60), Physical Address(0x4A00_9760)
		 Table 3-1115. CM_L4PER_GPIO3_CLKCTRL: Offset(0x68), Physical Address(0x4A00_9768)
		 Table 3-1117. CM_L4PER_GPIO4_CLKCTRL: Offset(0x70), Physical Address(0x4A00_9770)
		 Table 3-1119. CM_L4PER_GPIO5_CLKCTRL: Offset(0x78), Physical Address(0x4A00_9778)
		 Table 3-1121. CM_L4PER_GPIO6_CLKCTRL: Offset(0x80), Physical Address(0x4A00_9770)
		 Table 3-1155. CM_L4PER_GPIO7_CLKCTRL: Offset(0x110), Physical Address(0x4A00_9810)
		 Table 3-1157. CM_L4PER_GPIO8_CLKCTRL: Offset(0x118), Physical Address(0x4A00_9818)
			 
		 Enable GPIO 2~8 clock
		*/
		case 2:
			{
//				printk(KERN_DEBUG "[%s:%4d:%s] Trying to enable clock: GPIO2\n", __FILENAME__, __LINE__, __FUNCTION__);
				unOffset = 0x60;
#if 1
				iowrite32(0x01, g_ptrCM_REGS_L4PER + unOffset);
#else
				unRegVal = ioread32 (g_ptrCM_REGS_L4PER + unOffset);
//				BitsDisplayU32(unRegVal);
				BIT_SET(unRegVal,  0);
				BIT_CLR(unRegVal,  1);
				BIT_CLR(unRegVal, 16);
				BIT_CLR(unRegVal, 17);
//				BitsDisplayU32(unRegVal);
				iowrite32 (unRegVal, g_ptrCM_REGS_L4PER + unOffset);
#endif
			}
			break;
		case 3:
			{
//				printk(KERN_DEBUG "[%s:%4d:%s] Trying to enable clock: GPIO3\n", __FILENAME__, __LINE__, __FUNCTION__);
				unOffset = 0x68;
#if 1
				iowrite32(0x01, g_ptrCM_REGS_L4PER + unOffset);
#else
				unRegVal = ioread32 (g_ptrCM_REGS_L4PER + unOffset);
//				BitsDisplayU32(unRegVal);
				BIT_SET(unRegVal,  0);
				BIT_CLR(unRegVal,  1);
				BIT_CLR(unRegVal, 16);
				BIT_CLR(unRegVal, 17);
//				BitsDisplayU32(unRegVal);
				iowrite32 (unRegVal, g_ptrCM_REGS_L4PER + unOffset); 
#endif
			}
			break;
		case 4:
			{						
//				printk(KERN_DEBUG "[%s:%4d:%s] Trying to enable clock: GPIO4\n", __FILENAME__, __LINE__, __FUNCTION__);
				unOffset = 0x70;
#if 1
				iowrite32(0x01, g_ptrCM_REGS_L4PER + unOffset);
#else
				unRegVal = ioread32 (g_ptrCM_REGS_L4PER + unOffset);
//				BitsDisplayU32(unRegVal);
				BIT_SET(unRegVal,  0);
				BIT_CLR(unRegVal,  1);
				BIT_CLR(unRegVal, 16);
				BIT_CLR(unRegVal, 17);
//				BitsDisplayU32(unRegVal);
				iowrite32 (unRegVal, g_ptrCM_REGS_L4PER + unOffset); 
#endif
			}
			break;
		case 5:
			{
//				printk(KERN_DEBUG "[%s:%4d:%s] Trying to enable clock: GPIO5\n", __FILENAME__, __LINE__, __FUNCTION__);
				unOffset = 0x78;
#if 1
				iowrite32(0x01, g_ptrCM_REGS_L4PER + unOffset);
#else
				unRegVal = ioread32 (g_ptrCM_REGS_L4PER + unOffset);
//				BitsDisplayU32(unRegVal);
				BIT_SET(unRegVal,  0);
				BIT_CLR(unRegVal,  1);
				BIT_CLR(unRegVal, 16);
				BIT_CLR(unRegVal, 17);
//				BitsDisplayU32(unRegVal);
				iowrite32 (unRegVal, g_ptrCM_REGS_L4PER + unOffset); 
#endif
			}
			break;
		case 6:
			{
//				printk(KERN_DEBUG "[%s:%4d:%s] Trying to enable clock: GPIO6\n", __FILENAME__, __LINE__, __FUNCTION__);
				unOffset = 0x80;
#if 1
				iowrite32(0x01, g_ptrCM_REGS_L4PER + unOffset);
#else
				unRegVal = ioread32 (g_ptrCM_REGS_L4PER + unOffset);
//				BitsDisplayU32(unRegVal);
				BIT_SET(unRegVal,  0);
				BIT_CLR(unRegVal,  1);
				BIT_CLR(unRegVal, 16);
				BIT_CLR(unRegVal, 17);
//				BitsDisplayU32(unRegVal);
				iowrite32 (unRegVal, g_ptrCM_REGS_L4PER + unOffset); 
#endif
			}
			break;
		case 7:
			{		
//				printk(KERN_DEBUG "[%s:%4d:%s] Trying to enable clock: GPIO7\n", __FILENAME__, __LINE__, __FUNCTION__);
				unOffset = 0x110;
#if 1
				iowrite32(0x01, g_ptrCM_REGS_L4PER + unOffset);
#else
				unRegVal = ioread32 (g_ptrCM_REGS_L4PER + unOffset);
//				BitsDisplayU32(unRegVal);
				BIT_SET(unRegVal,  0);
				BIT_CLR(unRegVal,  1);
				BIT_CLR(unRegVal, 16);
				BIT_CLR(unRegVal, 17);
//				BitsDisplayU32(unRegVal);
				iowrite32 (unRegVal, g_ptrCM_REGS_L4PER + unOffset); 
#endif
			}
			break;
		case 8:
			{
//				printk(KERN_DEBUG "[%s:%4d:%s] Trying to enable clock: GPIO8\n", __FILENAME__, __LINE__, __FUNCTION__);
				unOffset = 0x118;
#if 1
				iowrite32(0x01, g_ptrCM_REGS_L4PER + unOffset);
#else
				unRegVal = ioread32 (g_ptrCM_REGS_L4PER + unOffset);
//				BitsDisplayU32(unRegVal);
				BIT_SET(unRegVal,  0);
				BIT_CLR(unRegVal,  1);
				BIT_CLR(unRegVal, 16);
				BIT_CLR(unRegVal, 17);
//				BitsDisplayU32(unRegVal);
				iowrite32 (unRegVal, g_ptrCM_REGS_L4PER + unOffset); 
#endif
			}
			break;		

		default:			
			printk(KERN_ERR "[%s:%4d:%s] Unhandled GPIO Bank: %d !\n", __FILENAME__, __LINE__, __FUNCTION__, nBankNum);
			break;
	}

	return 0;
}

static int InitGpioRegisterValues(void)
{
	//int i = 0;

#if 0
	g_ptrCM_REGS_WKUPAON = ioremap(CSL_MPU_WKUPAON_CM_REGS, CSL_MPU_WKUPAON_CM_SIZE);
	if(!g_ptrCM_REGS_WKUPAON) {
	  printk (KERN_ERR "[%s:%4d:%s] Failed to remap memory for CSL_MPU_WKUPAON_CM_REGS.\n",
		__FILENAME__, __LINE__, __FUNCTION__);
//	  return -1;
	}

	g_ptrCM_REGS_L4PER = ioremap(CSL_MPU_L4PER_CM_CORE_REGS, CSL_MPU_L4PER_CM_CORE_SIZE);
	if(!g_ptrCM_REGS_L4PER) {
	  printk (KERN_ERR "[%s:%4d:%s] Failed to remap memory for CSL_MPU_L4PER_CM_CORE_REGS.\n",
		__FILENAME__, __LINE__, __FUNCTION__);
//	  return -1;
	}
			

	for(i = 1 ; i <= 8 ; i++)
	{
		EnableClockGpioBank(i);
	}


	/*	 
 	 TRM Pg. 7109 ~ 7111
	*/
    g_ptrGpioBase1 = ioremap(CSL_MPU_GPIO1_REGS, CSL_MPU_GPIO1_SIZE);
    if(!g_ptrGpioBase1) {
        printk (KERN_ERR "[%s:%4d:%s] Failed to ioremap g_ptrGpioBase1.\n",
            __FILENAME__, __LINE__, __FUNCTION__);
//        return -1;
    }

    /*
	 TRM Pg. 1072, 1073, 1074, 1075

	 Table 3-1113. CM_L4PER_GPIO2_CLKCTRL: Offset(0x60), Physical Address(0x4A00_9760)
	 Table 3-1115. CM_L4PER_GPIO3_CLKCTRL: Offset(0x68), Physical Address(0x4A00_9768)
	 Table 3-1117. CM_L4PER_GPIO4_CLKCTRL: Offset(0x70), Physical Address(0x4A00_9770)
	 Table 3-1119. CM_L4PER_GPIO5_CLKCTRL: Offset(0x78), Physical Address(0x4A00_9778)
	 Table 3-1121. CM_L4PER_GPIO6_CLKCTRL: Offset(0x80), Physical Address(0x4A00_9770)
 	 Table 3-1155. CM_L4PER_GPIO7_CLKCTRL: Offset(0x110), Physical Address(0x4A00_9810)
	 Table 3-1157. CM_L4PER_GPIO8_CLKCTRL: Offset(0x118), Physical Address(0x4A00_9818)
	 	 
     Enable GPIO 2~8 clock
    */
		
    g_ptrGpioBase2 = ioremap(CSL_MPU_GPIO2_REGS, CSL_MPU_GPIO2_SIZE);
    if(!g_ptrGpioBase2) {
        printk (KERN_ERR "[%s:%4d:%s] Failed to ioremap g_ptrGpioBase2.\n",
            __FILENAME__, __LINE__, __FUNCTION__);
//        return -1;
    }
    g_ptrGpioBase3 = ioremap(CSL_MPU_GPIO3_REGS, CSL_MPU_GPIO3_SIZE);
    if(!g_ptrGpioBase3) {
        printk (KERN_ERR "[%s:%4d:%s] Failed to ioremap g_ptrGpioBase3.\n",
            __FILENAME__, __LINE__, __FUNCTION__);
//        return -1;
    }
    g_ptrGpioBase4 = ioremap(CSL_MPU_GPIO4_REGS, CSL_MPU_GPIO4_SIZE);
    if(!g_ptrGpioBase4) {
        printk (KERN_ERR "[%s:%4d:%s] Failed to ioremap g_ptrGpioBase4.\n",
            __FILENAME__, __LINE__, __FUNCTION__);
//        return -1;
    }
    g_ptrGpioBase5 = ioremap(CSL_MPU_GPIO5_REGS, CSL_MPU_GPIO5_SIZE);
    if(!g_ptrGpioBase5) {
        printk (KERN_ERR "[%s:%4d:%s] Failed to ioremap g_ptrGpioBase5.\n",
            __FILENAME__, __LINE__, __FUNCTION__);
//        return -1;
    }
    g_ptrGpioBase6 = ioremap(CSL_MPU_GPIO6_REGS, CSL_MPU_GPIO6_SIZE);
    if(!g_ptrGpioBase6) {
        printk (KERN_ERR "[%s:%4d:%s] Failed to ioremap g_ptrGpioBase6.\n",
            __FILENAME__, __LINE__, __FUNCTION__);
//        return -1;
    }
    g_ptrGpioBase7 = ioremap(CSL_MPU_GPIO7_REGS, CSL_MPU_GPIO7_SIZE);
    if(!g_ptrGpioBase7) {
        printk (KERN_ERR "[%s:%4d:%s] Failed to ioremap g_ptrGpioBase7.\n",
            __FILENAME__, __LINE__, __FUNCTION__);
//        return -1;
    }
    g_ptrGpioBase8 = ioremap(CSL_MPU_GPIO8_REGS, CSL_MPU_GPIO8_SIZE);
    if(!g_ptrGpioBase8) {
        printk (KERN_ERR "[%s:%4d:%s] Failed to ioremap g_ptrGpioBase8.\n",
            __FILENAME__, __LINE__, __FUNCTION__);
//        return -1;
    } 

#endif
    return 0;
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

    // Reset Flag For CPU Toggle
    g_nToggleReset = __OFF__;


	/*
	// NOTE::20210705
	// Userspace 에서 GPIO 2, 4, 5, 8 접근 시 아래의 커널 에러 발생
		...
		[  121.554350] WARNING: CPU: 0 PID: 845 at drivers/bus/omap_l3_noc.c:147 l3_interrupt_handler+0x330/0x380
		[  121.563711] 44000000.ocp:L3 Custom Error: MASTER MPU TARGET L4_PER1_P3 (Read): Data Access in User mode during Functional access
		...
	*/

/*
	EnableClockGpioBank(stGpioHandler.nGpioBank);

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
            if(((stGpioHandler.nGpioBank == 3) && (stGpioHandler.nGpioNum == 31))        // Reset GPIO FPGA_N	(NOTE::20210705, S2 System B/D V01 기준)
                || ((stGpioHandler.nGpioBank == 3) && (stGpioHandler.nGpioNum == 5))     // Reset GPIO FPGA_T	(NOTE::20210705, S2 System B/D V01 기준)
                ) 
            {
                g_nToggleReset = __ON__;
            }
            
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
*/
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


static void TestGpio(void)
{
/*
	// NOTE::2023-05-17
	// GPIO 작업 시 확인해야 할 문서

	Datasheet P.18 ~
		Pin Attributes
	Datasheet P.63 ~
		GPIO# Signal Descriptions

	TRM P.34 
		Memory Map
			PADCFG_CTRL0_CFG0 : 0x000F0000
			GPIO0 : 0x00600000
			GPIO1 : 0x00601000

	TRM P.3645~~ 
		6.1.2.3 Pad Configuration Ball Names
			TX_DIS bit (TRM P.3643, 3644)
				0=Driver is enabled, 1=Driver is disabled

	TRM P.2375 ~
		4.7.1.4 GPIO Register / Pin Mapping

	TRM P.8781 ~
		12.2.5.1 GPIO Registers
			GPIO_DIR01
			GPIO_SET_DATA01
			GPIO_CLR_DATA01

*/

/*
	// NOTE::2023-05-17
	// gpio 할당 번호 / 상태 확인
	root@am62xx-evm:~# cat /sys/kernel/debug/gpio
	gpiochip3: GPIOs 311-398, parent: platform/601000.gpio, 601000.gpio:
	gpio-318 (                    |enable              ) out lo

	gpiochip2: GPIOs 399-485, parent: platform/600000.gpio, 600000.gpio:
	gpio-431 (                    |regulator-3         ) out lo
	gpio-441 (                    |tps65219-LDO1-SEL-SD) out lo

	gpiochip1: GPIOs 486-509, parent: platform/4201000.gpio, 4201000.gpio:

	gpiochip0: GPIOs 510-511, parent: platform/3b000000.memory-controller, omap-gpmc:

	// NOTE::2023-05-17
	// sysfs 에서 gpio handling 시 경로 참고
	root@am62xx-evm:~# ll /sys/class/gpio/
	total 0
	drwxr-xr-x  2 root root    0 Jan  1  1970 .
	drwxr-xr-x 66 root root    0 Jan  1  1970 ..
	--w-------  1 root root 4096 Jan  1  1970 export
	lrwxrwxrwx  1 root root    0 Jan  1  1970 gpiochip311 -> ../../devices/platform/bus@f0000/601000.gpio/gpio/gpiochip311
	lrwxrwxrwx  1 root root    0 Jan  1  1970 gpiochip399 -> ../../devices/platform/bus@f0000/600000.gpio/gpio/gpiochip399
	lrwxrwxrwx  1 root root    0 Jan  1  1970 gpiochip486 -> ../../devices/platform/bus@f0000/bus@f0000:bus@4000000/4201000.gpio/gpio/gpiochip486
	lrwxrwxrwx  1 root root    0 Jan  1  1970 gpiochip510 -> ../../devices/platform/bus@f0000/3b000000.memory-controller/gpio/gpiochip510
	--w-------  1 root root 4096 Jan  1  1970 unexport

	// NOTE::2023-05-17
	// gpio0_11 상태 변경 시 예시
	#gpio0_11 (AM6232_STATUS_LED1)
	echo 410 > /sys/class/gpio/export
	echo out > /sys/class/gpio/gpio410/direction
	echo 0 > /sys/class/gpio/gpio410/value
*/

#if 0
	// NOTE::2023-05-17
	// 커널 빌트인으로 이미지에 탑재하여 실행하면 동작하지 않고,
	// 커널 모듈로 Init Process 에서 실행해야만 동작하는 이슈가 있음

	int nIrq = 0;
	int i = 0;

	for(i = 310 ; i < 512 ; i++)
	{
		if(gpio_is_valid(i))
		{
			//if(gpio_request_one(i, GPIOF_IN, "TEST") < 0)
			if(devm_gpio_request(&pdev->dev, i, "TEST") < 0)
			{
				printk(KERN_ERR "[%s:%4d] Failed to gpio(%d)_request...\n", __FILENAME__, __LINE__, i);
				continue;
			}

			nIrq = gpio_to_irq(i);
			if (nIrq < 0)
			{
				printk(KERN_ERR "[%s:%4d] Failed to gpio(%d)_to_irq(%d).\n", __FILENAME__, __LINE__, i, nIrq);
			}
			else
			{
				printk(KERN_ERR "[%s:%4d] ***********************************gpio(%d)_to_irq(%d).\n", __FILENAME__, __LINE__, i, nIrq);
			}	
		}
		else
		{
			printk(KERN_ERR "[%s:%4d] Failed to gpio(%d)_is_valid.\n", __FILENAME__, __LINE__, i);
		}
	}
#endif	// #if 0

#if 0
	// NOTE::2023-05-17
	// 디바이스 트리에서 Pinmux 시, 실행할 필요 없음
	unsigned int unRegVal = 0x00000000;
    void __iomem *iomem = NULL;

	iomem = ioremap(CSL_PADCFG_CTRL0_CFG0_BASE, CSL_PADCFG_CTRL0_CFG0_SIZE);	// 0xf_0000
    if(!iomem) {
        printk (KERN_ERR "[%s:%4d:%s] Failed to ioremap CSL_PADCFG_CTRL0_CFG0_BASE.\n",
            __FILENAME__, __LINE__, __FUNCTION__);
//        return -1;
    }
	else
	{
		unRegVal = ioread32(iomem + 0x4004);
		printk(KERN_ERR "[%s:%s:%d] GPIO0_1 PADCFG: 0x%08x\n", __FILENAME__, __FUNCTION__, __LINE__, unRegVal);
		BIT_CLR(unRegVal, 21);	// TX_DIS

		iowrite32(unRegVal, iomem + 0x4004);
		unRegVal = ioread32(iomem + 0x4004);
		printk(KERN_ERR "[%s:%s:%d] GPIO0_1 PADCFG: 0x%08x\n", __FILENAME__, __FUNCTION__, __LINE__, unRegVal);

		unRegVal = ioread32(iomem + 0x402c);
		printk(KERN_ERR "[%s:%s:%d] GPIO0_11 PADCFG: 0x%08x\n", __FILENAME__, __FUNCTION__, __LINE__, unRegVal);
		//BIT_CLR(unRegVal, 21);	// TX_DIS

		unRegVal = 0x08014007;

		iowrite32(unRegVal, iomem + 0x402c);
		unRegVal = ioread32(iomem + 0x402c);
		printk(KERN_ERR "[%s:%s:%d] GPIO0_11 PADCFG: 0x%08x\n", __FILENAME__, __FUNCTION__, __LINE__, unRegVal);

		iowrite32(unRegVal, iomem + 0x4034);
		iowrite32(unRegVal, iomem + 0x4038);
		iowrite32(unRegVal, iomem + 0x4114);
		
		iounmap(iomem);
	}

	iomem = ioremap(CSL_GPIO0_BASE, CSL_GPIO0_SIZE);	// 0x60_0000
    if(!iomem) {
        printk (KERN_ERR "[%s:%4d:%s] Failed to ioremap CSL_GPIO0_BASE.\n",
            __FILENAME__, __LINE__, __FUNCTION__);
//        return -1;
    }
	else
	{
	/*
		unRegVal = ioread32(iomem + CSL_GPIO_DIR(0));
		printk(KERN_ERR "[%s:%s:%d] GPIO0_DIR01: 0x%08x\n", __FILENAME__, __FUNCTION__, __LINE__, unRegVal);
		BIT_CLR(unRegVal, 11);	//GPIO0_11
		BIT_CLR(unRegVal, 13);	//GPIO0_13
		BIT_CLR(unRegVal, 14);	//GPIO0_14
		iowrite32(unRegVal, iomem + CSL_GPIO_DIR(0));
		printk(KERN_ERR "[%s:%s:%d] GPIO0_DIR01: 0x%08x\n", __FILENAME__, __FUNCTION__, __LINE__, unRegVal);
	*/
		
		// NOTE::2023-05-17
		// STATUS LED 켜기 테스트
	//	for(i = 0 ; i < 3 ; i ++)
	//	{
			unRegVal = ioread32(iomem + CSL_GPIO_SET_DATA(0));
			printk(KERN_ERR "[%s:%s:%d] GPIO_SET_DATA01: 0x%08x\n", __FILENAME__, __FUNCTION__, __LINE__, unRegVal);
			BIT_SET(unRegVal, 11);	//GPIO0_11
			BIT_SET(unRegVal, 13);	//GPIO0_13
			BIT_SET(unRegVal, 14);	//GPIO0_14
			iowrite32(unRegVal, iomem + CSL_GPIO_SET_DATA(0));
			unRegVal = ioread32(iomem + CSL_GPIO_SET_DATA(0));
			printk(KERN_ERR "[%s:%s:%d] GPIO_SET_DATA01: 0x%08x\n", __FILENAME__, __FUNCTION__, __LINE__, unRegVal);

			mdelay(1000);

			unRegVal = ioread32(iomem + CSL_GPIO_CLR_DATA(0));
			printk(KERN_ERR "[%s:%s:%d] GPIO_CLR_DATA01: 0x%08x\n", __FILENAME__, __FUNCTION__, __LINE__, unRegVal);
			BIT_SET(unRegVal, 11);	//GPIO0_11
			BIT_SET(unRegVal, 13);	//GPIO0_13
			BIT_SET(unRegVal, 14);	//GPIO0_14
			iowrite32(unRegVal, iomem + CSL_GPIO_CLR_DATA(0));
			unRegVal = ioread32(iomem + CSL_GPIO_CLR_DATA(0));
			printk(KERN_ERR "[%s:%s:%d] GPIO_CLR_DATA01: 0x%08x\n", __FILENAME__, __FUNCTION__, __LINE__, unRegVal);

	//		mdelay(1000);
	//	}

		iounmap(iomem);
	}
#endif	// #if 0
}


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


    //TestGpio();

/*
    if( (InitGpioRegisterValues()) < 0 ) {
        printk(KERN_ERR "[%s:%4d:%s] Failed to InitGpioRegisterValues \n",
            __FILENAME__, __LINE__, __FUNCTION__);
        return -ENOMEM;
    }
*/

	printk(KERN_DEBUG "[%s:%4d] Registered kisan_gpio_handler char driver. \n",
			__FILENAME__, __LINE__);

	return 0;
}

static int kisan_gpio_handler_remove(struct platform_device *pdev)
{
	struct StructKisanGpioHandlerData *pdata = platform_get_drvdata(pdev);

/*
    iounmap(g_ptrCM_REGS_WKUPAON);
    iounmap(g_ptrCM_REGS_L4PER);

    iounmap(g_ptrGpioBase1);
    iounmap(g_ptrGpioBase2);
    iounmap(g_ptrGpioBase3);
    iounmap(g_ptrGpioBase4);
    iounmap(g_ptrGpioBase5);
    iounmap(g_ptrGpioBase6);
    iounmap(g_ptrGpioBase7);
    iounmap(g_ptrGpioBase8);
*/

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
