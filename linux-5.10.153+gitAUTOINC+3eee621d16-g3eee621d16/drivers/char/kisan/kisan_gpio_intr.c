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

    printk(KERN_DEBUG "[%s:%4d:%s] TRACE:!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", __FILENAME__, __LINE__, __FUNCTION__);

	nGpioNum = tt->nGpioNum;
	nKeyNum = tt->nKeyNum;

	ucGpioVal = (gpio_get_value(nGpioNum) ? __HI__ : __LO__);
	
    aucSendBuf[0] = 0x01;       // Userspace에서 구분하는데 사용할 UDP CMD
    aucSendBuf[1] = ucGpioVal;	// Status (Push(0)/Release(1))
	aucSendBuf[2] = 0;

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

    printk(KERN_DEBUG "[%s:%4d:%s] TRACE:!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", __FILENAME__, __LINE__, __FUNCTION__);

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
root@am62xx-evm:~# cat /sys/kernel/debug/gpio
gpiochip3: GPIOs 311-398, parent: platform/601000.gpio, 601000.gpio:
 gpio-318 (                    |enable              ) out lo

gpiochip2: GPIOs 399-485, parent: platform/600000.gpio, 600000.gpio:
 gpio-431 (                    |regulator-3         ) out lo
 gpio-441 (                    |tps65219-LDO1-SEL-SD) out lo

gpiochip1: GPIOs 486-509, parent: platform/4201000.gpio, 4201000.gpio:

gpiochip0: GPIOs 510-511, parent: platform/3b000000.memory-controller, omap-gpmc:


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
*/




    int nErr = 0;
    //int nPinNum = 0;
    //for (nPinNum = 311 ; nPinNum < 512 ; nPinNum++)
	int nPinNum = 467; //(AM6232_PRU_STATUS -> LAN8710_RESET)
    {
        if(!gpio_is_valid(nPinNum)) {
            printk(KERN_ERR "[%s:%4d:%s] Invalid GPIO... (%d) \n", 
                    __FILENAME__, __LINE__, __FUNCTION__, nPinNum);
            //continue;
        }
        else {
            nErr = gpio_request(nPinNum, "");
            if (nErr) {
                printk(KERN_ERR "[%s:%4d:%s] Failed to request gpio#%d\n", 
                    __FILENAME__, __LINE__, __FUNCTION__, nPinNum);
                //continue;
            }
            else
            {
				#if 0
				gpio_free(nPinNum);
				//printk(KERN_ERR "[%s:%4d:%s] Free gpio#%d\n", __FILENAME__, __LINE__, __FUNCTION__, nPinNum);
				#else
                gpio_set_value(nPinNum, 0);
                printk(KERN_ERR "[%s:%4d:%s] GPIO#%d Value: %d \n", 
                        __FILENAME__, __LINE__, __FUNCTION__, nPinNum, gpio_get_value(nPinNum));
                mdelay(100);

                gpio_set_value(nPinNum, 1);
                printk(KERN_ERR "[%s:%4d:%s] GPIO#%d Value: %d \n", 
                        __FILENAME__, __LINE__, __FUNCTION__, nPinNum, gpio_get_value(nPinNum));
                mdelay(100);
				#endif
            }
        }
    }
}

static int ResetPhyLAN8710A(void)
{
    int nErr = 0;
	int nPinNum = 467;

	if(!gpio_is_valid(nPinNum)) {
	    printk(KERN_ERR "[%s:%4d:%s] Invalid GPIO... (%d) \n", 
    	        __FILENAME__, __LINE__, __FUNCTION__, nPinNum);
	    return -EIO;
	}
	else {
        nErr = gpio_request(nPinNum, "ResetPhyLAN8710A");
        if (nErr) {
		    printk(KERN_ERR "[%s:%4d:%s] Failed to request gpio#%d\n", 
                __FILENAME__, __LINE__, __FUNCTION__, nPinNum);
		    return -EIO;
	    }



	    nErr = gpio_request_one(nPinNum, GPIOF_OUT_INIT_HIGH, "ResetPhyLAN8710A");
	    if( nErr != 0 ) {
	        printk(KERN_ERR "[%s:%4d:%s] Failed to request GPIO pin#%i...\n", 
           	        __FILENAME__, __LINE__, __FUNCTION__, nPinNum);
            return -EIO;
        }
        else
        {
            printk(KERN_ERR "[%s:%4d:%s] GPIO Value: %d \n", 
           	        __FILENAME__, __LINE__, __FUNCTION__, gpio_get_value(nPinNum));

            gpio_set_value(nPinNum, 0);

            printk(KERN_ERR "[%s:%4d:%s] GPIO Value: %d \n", 
           	        __FILENAME__, __LINE__, __FUNCTION__, gpio_get_value(nPinNum));

            gpio_set_value(nPinNum, 1);

            printk(KERN_ERR "[%s:%4d:%s] GPIO Value: %d \n", 
           	        __FILENAME__, __LINE__, __FUNCTION__, gpio_get_value(nPinNum));
        }
    }

    return nErr;
}

static int InitGpioKeyFPGA(void)
{
	int nErr = 0;
	int nPinNum = g_nGpioNum_FPGA_INTR;

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
        printk(KERN_ERR "[%s:%4d:%s] The requested IRQ line#%i from GPIO#%i (err %i)\n",
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

    
    TestGpio();

/*

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
	
    nErr = InitGpioKeyFPGA();
	if(nErr == -EIO) {
		printk(KERN_ERR "[%s:%4d] Failed to set Front Key gpio(s).\n", __FILENAME__, __LINE__);
	}
	else
	{
		printk(KERN_DEBUG "[%s:%4d] Success to set Front Key IRQ(s).\n", __FILENAME__, __LINE__);
	}
    




    ResetPhyLAN8710A();


*/







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

