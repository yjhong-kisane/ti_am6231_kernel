/**
@file       kisan_gpio_intr.c
@brief      AM623x System B/D Character Device Driver
@details    GPIO Interrupt (/dev/kisan_gpio_intr)
@date       2023-04-26
@author     hong.yongje@kisane.com
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/mm.h>       // mmap

#include <asm/uaccess.h>
#include <asm/io.h> // ioremap


#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/completion.h>

#include <linux/errno.h>
#include <linux/types.h>

#include <kisan/kisan_common.h>
#include <kisan/cslr_soc_baseaddress.h>
#include <kisan/cslr_gpio.h>


#define DBG_MSG_ON_KISAN_GPIO_INTR


struct StructKisanGpioIntrData {
	struct platform_device *pdev;
	struct miscdevice mdev;
};


struct structGpioIntr {
    unsigned char ucOperationMode;
};

static unsigned char g_ucOperationMode = 0;


struct stTimer {
	int nGpioNum;
	int nKeyNum;
	int nDebounceTime;
	struct work_struct work;
	struct timer_list timer;
};

static struct stTimer g_stTimerWork;


static int g_nGpioNum_FPGA_INTR = 0;
static int g_nGpioToIrq_FPGA_INTR = 0;
static char *g_strIntrName_FPGA_INTR = "gpio:FrontKey_FPGA";


extern int SendUdpMsg(unsigned char *buf, int len);




static void GpioIntr_FrontKey_Timer(struct timer_list *t)
{
	struct stTimer *te = from_timer(te, t, timer);
	schedule_work(&te->work);
}


static void GpioIntr_FrontKey_WorkFunc(struct work_struct *work)
{
	int nGpioNum = 0, nKeyNum = 0;
	unsigned char ucGpioVal = 0;
    unsigned char aucSendBuf[LENGTH_OF_DDR_SHM_DATA_KEY] = {0x00, };

	struct stTimer *tt = container_of(work, struct stTimer, work);

	nGpioNum = tt->nGpioNum;
	nKeyNum = tt->nKeyNum;

	ucGpioVal = (gpio_get_value(nGpioNum) ? __HI__ : __LO__);
	
    aucSendBuf[0] = 0x01;       // Userspace에서 구분하는데 사용할 UDP CMD
    aucSendBuf[1] = ucGpioVal;	// Status (Push(0)/Release(1))
	aucSendBuf[2] = 0;

	printk(KERN_DEBUG "[%s:%4d:%s] SendUdpMsg [ %2x %2x %2x ]\n", 
		__FILENAME__, __LINE__, __FUNCTION__, 
		aucSendBuf[0], aucSendBuf[1], aucSendBuf[2]);

	SendUdpMsg(&aucSendBuf[0], LENGTH_OF_DDR_SHM_DATA_KEY);
}


static irqreturn_t IsrForFrontKey(int irq, void *dev_id)
{
#if defined( DBG_MSG_ON_KISAN_GPIO_INTR )
	unsigned char ucStatus = 0;
	int nGpioNum = 0, nKeyNum = 0; 
#endif	// #if defined( DBG_MSG_ON_KISAN_GPIO_INTR )

	struct stTimer *tt = dev_id;
	mod_timer(&tt->timer, jiffies + msecs_to_jiffies(tt->nDebounceTime));

#if defined( DBG_MSG_ON_KISAN_GPIO_INTR )
	nGpioNum = tt->nGpioNum;
	nKeyNum = tt->nKeyNum;
	
	if((gpio_get_value(nGpioNum) ? __HI__ : __LO__))
    {
        printk(KERN_DEBUG "[%s:%4d] Key-%d is released (GPIO_%d_%d: HI)\n", __FILENAME__, __LINE__, nKeyNum, nGpioNum/32+1, nGpioNum%32);
		ucStatus = __HI__;
//        irq_set_irq_type(irq, IRQ_TYPE_EDGE_FALLING);
	}
	else {
        printk(KERN_DEBUG "[%s:%4d] Key-%d is pushed (GPIO_%d_%d: LO) \n", __FILENAME__, __LINE__, nKeyNum, nGpioNum/32+1, nGpioNum%32);
		ucStatus = __LO__;
//        irq_set_irq_type(irq, IRQ_TYPE_EDGE_RISING);
	}
#endif	// #if defined( DBG_MSG_ON_KISAN_GPIO_INTR )

	return IRQ_HANDLED;
}


static int kisan_gpio_intr_open(struct inode *inode, struct file *file)
{
#if defined( DBG_MSG_ON_KISAN_GPIO_INTR )
    printk(KERN_DEBUG "[%s:%4d:%s] \n", __FILENAME__, __LINE__, __FUNCTION__);
#endif  // #if defined( DBG_MSG_ON_KISAN_GPIO_INTR )

    return 0;
}


static ssize_t kisan_gpio_intr_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
#if defined( DBG_MSG_ON_KISAN_GPIO_INTR )
    printk(KERN_DEBUG "[%s:%4d:%s] \n", __FILENAME__, __LINE__, __FUNCTION__);
#endif  // #if defined( DBG_MSG_ON_KISAN_GPIO_INTR )

    return 0;
} 


static long kisan_gpio_intr_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
	void __user *argp = (void __user *) args;
	struct structGpioIntr stGpioIntr;

	if(copy_from_user(&stGpioIntr, argp, sizeof(struct structGpioIntr))) {
		printk(KERN_ERR "[%s:%4d:%s] copy_from_user. \n", __FILENAME__, __LINE__, __FUNCTION__);
		return -EFAULT;
	}

	g_ucOperationMode = stGpioIntr.ucOperationMode;

#if defined( DBG_MSG_ON_KISAN_GPIO_INTR )
	printk(KERN_DEBUG "[%s:%4d:%s] Operation Mode: %s \n", __FILENAME__, __LINE__, __FUNCTION__,
		g_ucOperationMode ? "FPGA Interrupt x1" : "GPIO Direct x10");
#endif  // #if defined( DBG_MSG_KEY_ON )

    return 0;
}


static struct file_operations kisan_gpio_intr_fops = {
	.owner   		= THIS_MODULE,
    .open    		= kisan_gpio_intr_open,
    .read    		= kisan_gpio_intr_read,    
    .unlocked_ioctl = kisan_gpio_intr_ioctl,
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
	// (G25) GPIO0_1 (CPU_BOARD_VER1) 상태 변경 시 예시
	echo 400 > /sys/class/gpio/export
	echo out > /sys/class/gpio/gpio400/direction
	echo 0 > /sys/class/gpio/gpio400/value
	echo 1 > /sys/class/gpio/gpio400/value
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


static int InitGpioKeyFPGA(int nPinNum)
{
	int nErr = 0;
	//int nPinNum = g_nGpioNum_FPGA_INTR;

	if(!gpio_is_valid(nPinNum)) {
	    printk(KERN_ERR "[%s:%4d:%s] Invalid GPIO... (%d) \n", 
    	        __FILENAME__, __LINE__, __FUNCTION__, nPinNum);
	    return -EIO;
	}
	else {
	    nErr = gpio_request_one(nPinNum, GPIOF_IN, g_strIntrName_FPGA_INTR);
	    if( nErr != 0 ) {
	        printk(KERN_ERR "[%s:%4d:%s] Failed to request GPIO pin#%i for IRQ (err %i) \n", 
           	        __FILENAME__, __LINE__, __FUNCTION__, nPinNum, nErr);
            return -EIO;
        }
    }

	g_nGpioToIrq_FPGA_INTR = gpio_to_irq(nPinNum);
	if(g_nGpioToIrq_FPGA_INTR < 0) {
	    printk(KERN_ERR "[%s:%4d:%s] Failed to gpio #%d to irq (%s) \n", 
    	       	__FILENAME__, __LINE__, __FUNCTION__, nPinNum, g_strIntrName_FPGA_INTR);
	    gpio_free(nPinNum);
	    return -EIO;
	}

	g_stTimerWork.nGpioNum = nPinNum;
	g_stTimerWork.nKeyNum = 1;
	g_stTimerWork.nDebounceTime = 25;

	INIT_WORK(&(g_stTimerWork.work), GpioIntr_FrontKey_WorkFunc);
	timer_setup(&(g_stTimerWork.timer), GpioIntr_FrontKey_Timer, 0);

	nErr = request_irq(g_nGpioToIrq_FPGA_INTR, IsrForFrontKey, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, 
	            		g_strIntrName_FPGA_INTR, &g_stTimerWork);
    //irq_set_irq_type ( g_nGpioToIrq_FPGA_INTR, IRQ_TYPE_EDGE_FALLING );

    if (nErr) {
        printk(KERN_ERR "[%s:%4d:%s] Failed to request IRQ line#%i from GPIO#%i (err %i)\n",
			    __FILENAME__, __LINE__, __FUNCTION__, g_nGpioToIrq_FPGA_INTR, nPinNum, nErr);
        return -EIO;
    }
	else
	{
		printk(KERN_DEBUG "[%s:%4d] IRQ: %i from GPIO(%i)\n",
				__FILENAME__, __LINE__,
				g_nGpioToIrq_FPGA_INTR, nPinNum);
	}

	return nErr;
}


static int kisan_gpio_intr_probe(struct platform_device *pdev)
{
	struct StructKisanGpioIntrData *pdata = NULL;
	struct device_node *np = pdev->dev.of_node;
	int nErr = 0;

 	unsigned int unDeviceTreeNode = 0;

	pdata = devm_kzalloc(&pdev->dev, sizeof(struct StructKisanGpioIntrData), GFP_KERNEL);
	if(!pdata) {
		printk(KERN_ERR "[%s:%4d] Failed to devm_kzalloc...\n", __FILENAME__, __LINE__);
		return -ENOMEM;
	}

 	// 자신의 platform_device 객체 저장
	pdata->pdev = pdev;
	pdata->mdev.minor = MISC_DYNAMIC_MINOR;
    pdata->mdev.name = "kisan_gpio_intr";
    pdata->mdev.fops = &kisan_gpio_intr_fops;

	/* register a minimal instruction set computer device (misc) */
	nErr = misc_register(&pdata->mdev);
	if(nErr) {
		dev_err(&pdev->dev,"failed to register misc device\n");
		devm_kfree(&pdev->dev, pdata);
		return nErr;
	}
	
 	// platform_device 에 사용할 고유의 구조체 설정
	platform_set_drvdata(pdev, pdata);

	if (!of_property_read_u32(np, "mode", &unDeviceTreeNode)) {
#if defined( DBG_MSG_ON_KISAN_GPIO_INTR )
		printk(KERN_DEBUG "[%s:%4d] Read DT: mode(%d)\n", __FILENAME__, __LINE__, unDeviceTreeNode);
#endif	// #if defined( DBG_MSG_ON_KISAN_GPIO_INTR )
		g_ucOperationMode = (unsigned char)unDeviceTreeNode;
	}
	else {
		printk(KERN_ERR "[%s:%4d] Failed to Read DT: mode\n", __FILENAME__, __LINE__);
	}

	if (!of_property_read_u32(np, "gpio-key-fpga", &unDeviceTreeNode)) {
#if defined( DBG_MSG_ON_KISAN_GPIO_INTR )
		printk(KERN_DEBUG "[%s:%4d] Read DT: gpio-key-fpga(%d)\n", __FILENAME__, __LINE__, unDeviceTreeNode);
#endif	// #if defined( DBG_MSG_ON_KISAN_GPIO_INTR )
		g_nGpioNum_FPGA_INTR = unDeviceTreeNode;
	}
	else {
		printk(KERN_ERR "[%s:%4d] Failed to Read DT: gpio-key-fpga\n", __FILENAME__, __LINE__);
	}
	
	// NOTE::2023-05-17
	// 커널 빌트인으로 이미지에 탑재하여 실행하면 동작하지 않고,
	// 커널 모듈로 Init Process 에서 실행해야만 동작하는 이슈가 있음
	if((g_nGpioNum_FPGA_INTR >= 311 && g_nGpioNum_FPGA_INTR <= 362) ||		// gpiochip3: GPIOs 311-398, parent: platform/601000.gpio, 601000.gpio: GPIO1_0 ~ GPIO1_51
		(g_nGpioNum_FPGA_INTR >= 399 && g_nGpioNum_FPGA_INTR <= 485) ||		// gpiochip2: GPIOs 399-485, parent: platform/600000.gpio, 600000.gpio: GPIO0_0 ~ GPIO0_86
		(g_nGpioNum_FPGA_INTR >= 486 && g_nGpioNum_FPGA_INTR <= 509))		// gpiochip1: GPIOs 486-509, parent: platform/4201000.gpio, 4201000.gpio: MCU_GPIO0_0 ~ MCU_GPIO0_23
	{
		nErr = InitGpioKeyFPGA(g_nGpioNum_FPGA_INTR);
		if(nErr == -EIO) {
			printk(KERN_ERR "[%s:%4d] Failed to set Front Key gpio(s).\n", __FILENAME__, __LINE__);
		}
		else
		{
			printk(KERN_DEBUG "[%s:%4d] Success to set Front Key IRQ(s).\n", __FILENAME__, __LINE__);
		}
	}
	else
	{
		printk(KERN_ERR "[%s:%4d] Failed to assign: gpio-key-fpga to GPIO(%d)\n", __FILENAME__, __LINE__, g_nGpioNum_FPGA_INTR);
	}

	// TestGpio();

	printk(KERN_DEBUG "[%s:%4d] Registered kisan_gpio_intr char driver. \n",
			__FILENAME__, __LINE__);

	return 0;
}

static int kisan_gpio_intr_remove(struct platform_device *pdev)
{
	struct StructKisanGpioIntrData *pdata = platform_get_drvdata(pdev);

	free_irq(g_nGpioToIrq_FPGA_INTR, NULL);
	gpio_free(g_nGpioNum_FPGA_INTR);
	del_timer(&(g_stTimerWork.timer));

	misc_deregister(&pdata->mdev);
	devm_kfree(&pdev->dev, pdata);

    printk(KERN_INFO "[%s:%4d] Unloaded module: kisan_gpio_intr\n", __FILENAME__, __LINE__);

	return 0;
}

static int kisan_gpio_intr_suspend(struct platform_device *pdev, pm_message_t state)
{
	printk(KERN_DEBUG "[%s:%4d:%s]\n", __FILENAME__, __LINE__, __FUNCTION__);
	return 0;
}

static int kisan_gpio_intr_resume(struct platform_device *pdev)
{
	printk(KERN_DEBUG "[%s:%4d:%s]\n", __FILENAME__, __LINE__, __FUNCTION__);
	return 0;
}


#if IS_ENABLED(CONFIG_OF)  
static const struct of_device_id kisan_gpio_intr_dt_ids[] = {  
    {.compatible = "kisan_gpio_intr", },  
    {},  
};
MODULE_DEVICE_TABLE(of, kisan_gpio_intr_dt_ids);  
#endif // #if IS_ENABLED(CONFIG_OF)

static struct platform_driver kisan_gpio_intr_driver = {
	.probe   = kisan_gpio_intr_probe,
	.remove  = kisan_gpio_intr_remove,
	.suspend = kisan_gpio_intr_suspend,
	.resume  = kisan_gpio_intr_resume,
	.driver  = {
        .name           = "kisan_gpio_intr",
        .owner          = THIS_MODULE,
        .of_match_table = of_match_ptr(kisan_gpio_intr_dt_ids), 
    },
};
module_platform_driver(kisan_gpio_intr_driver);


MODULE_DESCRIPTION("AM623x Kisan System B/D GPIO Interrupt Driver");
MODULE_AUTHOR("hong.yongje@kisane.com");
MODULE_LICENSE("Dual BSD/GPL");
