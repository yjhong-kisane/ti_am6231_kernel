/**
@file       kisan_board_s1p.c
@brief      AM623x System B/D Character Device Driver
@details    S1P Base board (/dev/kisan_board_s1p)
@date       2023-07-06
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


#define DBG_MSG_ON_KISAN_BOARD_S1P


struct StructKisanBoardS1pData {
	struct platform_device *pdev;
	struct miscdevice mdev;
};

struct structBoardS1P {
    unsigned char ucOperationMode;
};

static unsigned char g_ucOperationMode = 0;


static int kisan_board_s1p_open(struct inode *inode, struct file *file)
{
#if defined( DBG_MSG_ON_KISAN_BOARD_S1P )
    printk(KERN_DEBUG "[%s:%4d:%s] \n", __FILENAME__, __LINE__, __FUNCTION__);
#endif  // #if defined( DBG_MSG_ON_KISAN_BOARD_S1P )

    return 0;
}

static ssize_t kisan_board_s1p_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
#if defined( DBG_MSG_ON_KISAN_BOARD_S1P )
    printk(KERN_DEBUG "[%s:%4d:%s] \n", __FILENAME__, __LINE__, __FUNCTION__);
#endif  // #if defined( DBG_MSG_ON_KISAN_BOARD_S1P )

    return 0;
} 

static long kisan_board_s1p_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
	void __user *argp = (void __user *) args;
	struct structBoardS1P stBoardS1P;

	if(copy_from_user(&stBoardS1P, argp, sizeof(struct structBoardS1P))) {
		printk(KERN_ERR "[%s:%4d:%s] copy_from_user. \n", __FILENAME__, __LINE__, __FUNCTION__);
		return -EFAULT;
	}

	g_ucOperationMode = stBoardS1P.ucOperationMode;

#if defined( DBG_MSG_ON_KISAN_BOARD_S1P )
	printk(KERN_DEBUG "[%s:%4d:%s] Operation Mode: %d \n", __FILENAME__, __LINE__, __FUNCTION__, g_ucOperationMode);
#endif  // #if defined( DBG_MSG_ON_KISAN_BOARD_S1P )

    return 0;
}


static struct file_operations kisan_board_s1p_fops = {
	.owner   		= THIS_MODULE,
    .open    		= kisan_board_s1p_open,
    .read    		= kisan_board_s1p_read,    
    .unlocked_ioctl = kisan_board_s1p_ioctl,
};

static int kisan_board_s1p_probe(struct platform_device *pdev)
{
	struct StructKisanBoardS1pData *pdata = NULL;
	//struct device_node *np = pdev->dev.of_node;
	//unsigned int unDeviceTreeNode = 0;
	int nErr = 0;

	pdata = devm_kzalloc(&pdev->dev, sizeof(struct StructKisanBoardS1pData), GFP_KERNEL);
	if(!pdata) {
		printk(KERN_ERR "[%s:%4d] Failed to devm_kzalloc...\n", __FILENAME__, __LINE__);
		return -ENOMEM;
	}

 	// 자신의 platform_device 객체 저장
	pdata->pdev = pdev;
	pdata->mdev.minor = MISC_DYNAMIC_MINOR;
    pdata->mdev.name = "kisan_board_s1p";
    pdata->mdev.fops = &kisan_board_s1p_fops;

	/* register a minimal instruction set computer device (misc) */
	nErr = misc_register(&pdata->mdev);
	if(nErr) {
		dev_err(&pdev->dev,"failed to register misc device\n");
		devm_kfree(&pdev->dev, pdata);
		return nErr;
	}
	
 	// platform_device 에 사용할 고유의 구조체 설정
	platform_set_drvdata(pdev, pdata);

	printk(KERN_DEBUG "[%s:%4d] Registered kisan_board_s1p char driver. \n",
			__FILENAME__, __LINE__);

	return 0;
}

static int kisan_board_s1p_remove(struct platform_device *pdev)
{
	struct StructKisanBoardS1pData *pdata = platform_get_drvdata(pdev);

	misc_deregister(&pdata->mdev);
	devm_kfree(&pdev->dev, pdata);

    printk(KERN_INFO "[%s:%4d] Unloaded module: kisan_board_s1p\n", __FILENAME__, __LINE__);

	return 0;
}

static int kisan_board_s1p_suspend(struct platform_device *pdev, pm_message_t state)
{
	printk(KERN_DEBUG "[%s:%4d:%s]\n", __FILENAME__, __LINE__, __FUNCTION__);
	return 0;
}

static int kisan_board_s1p_resume(struct platform_device *pdev)
{
	printk(KERN_DEBUG "[%s:%4d:%s]\n", __FILENAME__, __LINE__, __FUNCTION__);
	return 0;
}


#if IS_ENABLED(CONFIG_OF)  
static const struct of_device_id kisan_board_s1p_dt_ids[] = {  
    {.compatible = "kisan_board_s1p", },  
    {},  
};
MODULE_DEVICE_TABLE(of, kisan_board_s1p_dt_ids);  
#endif // #if IS_ENABLED(CONFIG_OF)

static struct platform_driver kisan_board_s1p_driver = {
	.probe   = kisan_board_s1p_probe,
	.remove  = kisan_board_s1p_remove,
	.suspend = kisan_board_s1p_suspend,
	.resume  = kisan_board_s1p_resume,
	.driver  = {
        .name           = "kisan_board_s1p",
        .owner          = THIS_MODULE,
        .of_match_table = of_match_ptr(kisan_board_s1p_dt_ids), 
    },
};
module_platform_driver(kisan_board_s1p_driver);


MODULE_DESCRIPTION("AM623x Kisan System B/D For S1P Driver");
MODULE_AUTHOR("hong.yongje@kisane.com");
MODULE_LICENSE("Dual BSD/GPL");
