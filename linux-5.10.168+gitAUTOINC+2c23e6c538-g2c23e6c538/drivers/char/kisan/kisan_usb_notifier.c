/**
@file       kisan_usb_notifier.c
@brief      AM623x System B/D Character Device Driver
@details    S1P Base board (/dev/kisan_usb_notifier)
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

#include <linux/usb.h>
#include <linux/notifier.h>
#include <linux/kmod.h>		// call_usermodehelper


#define DBG_MSG_ON_KISAN_USB_NOTIFIER


struct StructKisanUsbNotifierData {
	struct platform_device *pdev;
	struct miscdevice mdev;
};

struct structUsbNotifier {
    unsigned char ucOperationMode;
};

static unsigned char g_ucOperationMode = 0;

static int g_unUsbWifiDongleAttaced = FALSE;
static int g_unUsbLTEDongleAttaced = FALSE;

//extern void execute_shell_command(char* strPath, char* strCommand, char* strArgv1, char* strArgv2, char* strArgv3);

extern int SendUdpMsg(unsigned char *buf, int len);
#define UDP_SEND_LENGTH_USB_NOTIFIER		(2)


static int CheckUsbDeviceVendor(struct usb_device *dev)
{
    int nRet = TRUE;
    int nVendor = dev->descriptor.idVendor;
    int nProductID = dev->descriptor.idProduct;

    unsigned char aucSendBuf[UDP_SEND_LENGTH_USB_NOTIFIER] = { 0xff, 0xff };

    if(!g_unUsbWifiDongleAttaced)
    {
        if((nVendor == 0x0bda)    // Realtek Semiconductor (WIFI dongle)
            && (nProductID == 0x8176))
        {
            printk(KERN_EMERG ">>>>>>>>>> Realtek Semiconductor Corp. RTL8188CUS 802.11n WLAN Adapter is attached.\n");
			//execute_shell_command("/kisan/", "kisan_wifi.sh", "on", NULL, NULL);

            aucSendBuf[0] = UDP_CMD_KISAN_WIFI_ATTACHED;
			aucSendBuf[1] = 0x00;
            printk(KERN_DEBUG "[%s:%4d] SendUdpMsg [ %2x %2x ]\n", 
				__FILENAME__, __LINE__, aucSendBuf[0], aucSendBuf[1]);
			SendUdpMsg(&aucSendBuf[0], UDP_SEND_LENGTH_USB_NOTIFIER);

            g_unUsbWifiDongleAttaced = TRUE;
            nRet = TRUE;
        }
		else if((nVendor == 0x1286)    // Ublox Lily-W13x (WIFI module)
            && (nProductID == 0x204a || nProductID == 0x2049))
        {
			/*
				[ 4523.112874] usb 1-1.1: New USB device found, idVendor=1286, idProduct=204a
				[ 4523.120083] usb 1-1.1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
				[ 4523.127738] usb 1-1.1: Product: Marvell Wireless Device
				[ 4523.133191] usb 1-1.1: Manufacturer: Marvell
				[ 4523.137667] usb 1-1.1: SerialNumber: 6C1DEB47CC22
			*/
            printk(KERN_EMERG ">>>>>>>>>> Ublox Lily-W13x (Marvell Avastar 88w8801 chipset) WiFi Module is attached.\n");
			//execute_shell_command("/kisan/", "kisan_wifi.sh", "on", NULL, NULL);

            aucSendBuf[0] = UDP_CMD_KISAN_WIFI_ATTACHED;
			aucSendBuf[1] = 0x01;
            printk(KERN_DEBUG "[%s:%4d] SendUdpMsg [ %2x %2x ]\n", 
				__FILENAME__, __LINE__, aucSendBuf[0], aucSendBuf[1]);
			SendUdpMsg(&aucSendBuf[0], UDP_SEND_LENGTH_USB_NOTIFIER);

            g_unUsbWifiDongleAttaced = TRUE;            
            nRet = TRUE;
        }
    }

    if(!g_unUsbLTEDongleAttaced)
    {
        if((nVendor == 0x258d)    // HiveMotion HS2300 LTE USB Dongle
            && (nProductID == 0x2000))
        {
            printk(KERN_EMERG ">>>>>>>>>> HiveMotion HS2300 LTE USB Dongle is attached. \n");
			//execute_shell_command("/kisan/", "kisan_wifi.sh", "on", NULL, NULL);

            aucSendBuf[0] = UDP_CMD_KISAN_LTE_ATTACHED;
			aucSendBuf[1] = 0x00;
            printk(KERN_DEBUG "[%s:%4d] SendUdpMsg [ %2x %2x ]\n", 
				__FILENAME__, __LINE__, aucSendBuf[0], aucSendBuf[1]);
			SendUdpMsg(&aucSendBuf[0], UDP_SEND_LENGTH_USB_NOTIFIER);

            g_unUsbLTEDongleAttaced = TRUE;
            nRet = TRUE;
        }
    }

	if((nVendor == 0x1cbe)	// TI USB Generic Bulk Device
		&& (nProductID == 0x0003))
	{
		/*
			[ 5495.231767] usb 1-1.2.1: New USB device found, idVendor=1cbe, idProduct=0003
			[ 5495.239178] usb 1-1.2.1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
			[ 5495.247018] usb 1-1.2.1: Product: Generic Bulk Device
			[ 5495.252290] usb 1-1.2.1: Manufacturer: Texas Instruments
			[ 5495.257860] usb 1-1.2.1: SerialNumber: 12345678
		*/
		printk(KERN_EMERG ">>>>>>>>>> Generic Bulk Device (Luminary Micro Inc.) is attached. \n");

		aucSendBuf[0] = UDP_CMD_KISAN_USB_BULK_COMM;
		aucSendBuf[1] = 0x01;
		printk(KERN_DEBUG "[%s:%4d] SendUdpMsg [ %2x %2x ]\n", 
				__FILENAME__, __LINE__, aucSendBuf[0], aucSendBuf[1]);
		SendUdpMsg(&aucSendBuf[0], UDP_SEND_LENGTH_USB_NOTIFIER);
		nRet = TRUE;
	}

    return nRet;
}

static void usb_detect_func(struct usb_device *dev)
{
    if(dev != NULL)
	{
        if( !(CheckUsbDeviceVendor(dev)) )
		{
            printk(KERN_DEBUG "[%s:%3d:%s] Unknown USB device: %s, %s, 0x%04X, 0x%04X \n", 
                __FILENAME__, __LINE__, __FUNCTION__,
                dev->manufacturer, dev->product, dev->descriptor.idVendor, dev->descriptor.idProduct);
        }        
    }
    else
	{
		printk(KERN_DEBUG "[%s:%3d:%s] Unknown USB Device. \n", __FILENAME__, __LINE__, __FUNCTION__);
    }
}

static void usb_remove_func(struct  usb_device *dev)
{
    int nVendor = dev->descriptor.idVendor;
    int nProductID = dev->descriptor.idProduct;

    unsigned char aucSendBuf[UDP_SEND_LENGTH_USB_NOTIFIER] = { 0xff, 0xff };

    if((nVendor == 0x0bda)
        && (nProductID == 0x8176))
    {
        printk(KERN_EMERG "<<<<<<<<<< Realtek Semiconductor Corp. RTL8188CUS 802.11n WLAN Adapter is dettached. \n");
		//execute_shell_command("/kisan/", "kisan_wifi.sh", "off", NULL, NULL);
        
        aucSendBuf[0] = UDP_CMD_KISAN_WIFI_DETACHED;
        aucSendBuf[1] = 0x00;
        printk(KERN_DEBUG "[%s:%4d] SendUdpMsg [ %2x %2x ]\n", 
				__FILENAME__, __LINE__, aucSendBuf[0], aucSendBuf[1]);
		SendUdpMsg(&aucSendBuf[0], UDP_SEND_LENGTH_USB_NOTIFIER);

        g_unUsbWifiDongleAttaced = FALSE;
    }
	else if((nVendor == 0x1286)    // Ublox Lily-W13x (WIFI module)
		&& (nProductID == 0x204a || nProductID == 0x2049))
	{
		/*
			[ 4523.112874] usb 1-1.1: New USB device found, idVendor=1286, idProduct=204a
			[ 4523.120083] usb 1-1.1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
			[ 4523.127738] usb 1-1.1: Product: Marvell Wireless Device
			[ 4523.133191] usb 1-1.1: Manufacturer: Marvell
			[ 4523.137667] usb 1-1.1: SerialNumber: 6C1DEB47CC22
		*/
		printk(KERN_EMERG "<<<<<<<<<< Ublox Lily-W13x (Marvell Avastar 88w8801 chipset) WiFi Module is dettached. \n");
		//execute_shell_command("/kisan/", "kisan_wifi.sh", "off", NULL, NULL);
		
		aucSendBuf[0] = UDP_CMD_KISAN_WIFI_DETACHED;
		aucSendBuf[1] = 0x01;
        printk(KERN_DEBUG "[%s:%4d] SendUdpMsg [ %2x %2x ]\n", 
			__FILENAME__, __LINE__, aucSendBuf[0], aucSendBuf[1]);
		SendUdpMsg(&aucSendBuf[0], UDP_SEND_LENGTH_USB_NOTIFIER);

		g_unUsbWifiDongleAttaced = FALSE;
	}
	else if((nVendor == 0x258d)    // HiveMotion HS2300 LTE USB Dongle
        && (nProductID == 0x2000))
    {
        printk(KERN_EMERG "<<<<<<<<<< HiveMotion HS2300 LTE USB Dongle is dettached. \n");
		//execute_shell_command("/kisan/", "kisan_wifi.sh", "off", NULL, NULL);
        
        aucSendBuf[0] = UDP_CMD_KISAN_LTE_DETACHED;
        aucSendBuf[1] = 0x00;
        printk(KERN_DEBUG "[%s:%4d] SendUdpMsg [ %2x %2x ]\n", 
			__FILENAME__, __LINE__, aucSendBuf[0], aucSendBuf[1]);
		SendUdpMsg(&aucSendBuf[0], UDP_SEND_LENGTH_USB_NOTIFIER);

        g_unUsbLTEDongleAttaced = FALSE;
    }
	else if((nVendor == 0x1cbe)		// TI USB Generic Bulk Device
		&& (nProductID == 0x0003))
	{
		/*
			[ 5495.231767] usb 1-1.2.1: New USB device found, idVendor=1cbe, idProduct=0003
			[ 5495.239178] usb 1-1.2.1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
			[ 5495.247018] usb 1-1.2.1: Product: Generic Bulk Device
			[ 5495.252290] usb 1-1.2.1: Manufacturer: Texas Instruments
			[ 5495.257860] usb 1-1.2.1: SerialNumber: 12345678
		*/
		printk(KERN_EMERG "<<<<<<<<<< Generic Bulk Device (Luminary Micro Inc.) is dettached. \n");

		aucSendBuf[0] = UDP_CMD_KISAN_USB_BULK_COMM;
		aucSendBuf[1] = 0x00;
        printk(KERN_DEBUG "[%s:%4d] SendUdpMsg [ %2x %2x ]\n", 
			__FILENAME__, __LINE__, aucSendBuf[0], aucSendBuf[1]);
		SendUdpMsg(&aucSendBuf[0], UDP_SEND_LENGTH_USB_NOTIFIER);
	}
}

static int kisan_usb_notify_subscriber(struct notifier_block *self, unsigned long action, void *dev)
{
    //printk(KERN_DEBUG "[%s:%3d:%s] USB action: %ld ========================\n", __FILENAME__, __LINE__, __FUNCTION__, action);
	switch (action) {
		case USB_DEVICE_ADD:        // USB Device Detect
    		usb_detect_func(dev);
            break;
        case USB_DEVICE_REMOVE:     // USB Device Remove
            usb_remove_func(dev);
            break;
        case USB_BUS_ADD:
			break;
        case USB_BUS_REMOVE:
		 	break;
	}

	return NOTIFY_OK;
}

static struct notifier_block kisan_usb_nb = {
	.notifier_call = kisan_usb_notify_subscriber,
};

static int kisan_usb_notifier_open(struct inode *inode, struct file *file)
{
#if defined( DBG_MSG_ON_KISAN_USB_NOTIFIER )
    printk(KERN_DEBUG "[%s:%4d:%s] \n", __FILENAME__, __LINE__, __FUNCTION__);
#endif  // #if defined( DBG_MSG_ON_KISAN_USB_NOTIFIER )

    return 0;
}

static ssize_t kisan_usb_notifier_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
#if defined( DBG_MSG_ON_KISAN_USB_NOTIFIER )
    printk(KERN_DEBUG "[%s:%4d:%s] \n", __FILENAME__, __LINE__, __FUNCTION__);
#endif  // #if defined( DBG_MSG_ON_KISAN_USB_NOTIFIER )

    return 0;
} 

static long kisan_usb_notifier_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
	void __user *argp = (void __user *) args;
	struct structUsbNotifier stUsbNotifier;

	if(copy_from_user(&stUsbNotifier, argp, sizeof(struct structUsbNotifier))) {
		printk(KERN_ERR "[%s:%4d:%s] copy_from_user. \n", __FILENAME__, __LINE__, __FUNCTION__);
		return -EFAULT;
	}

	g_ucOperationMode = stUsbNotifier.ucOperationMode;

#if defined( DBG_MSG_ON_KISAN_USB_NOTIFIER )
	printk(KERN_DEBUG "[%s:%4d:%s] Operation Mode: %d \n", __FILENAME__, __LINE__, __FUNCTION__, g_ucOperationMode);
#endif  // #if defined( DBG_MSG_ON_KISAN_USB_NOTIFIER )

    return 0;
}

static struct file_operations kisan_usb_notifier_fops = {
	.owner   		= THIS_MODULE,
    .open    		= kisan_usb_notifier_open,
    .read    		= kisan_usb_notifier_read,    
    .unlocked_ioctl = kisan_usb_notifier_ioctl,
};

static int kisan_usb_notifier_probe(struct platform_device *pdev)
{
	struct StructKisanUsbNotifierData *pdata = NULL;
	//struct device_node *np = pdev->dev.of_node;
	//unsigned int unDeviceTreeNode = 0;
	int nErr = 0;

	pdata = devm_kzalloc(&pdev->dev, sizeof(struct StructKisanUsbNotifierData), GFP_KERNEL);
	if(!pdata) {
		printk(KERN_ERR "[%s:%4d] Failed to devm_kzalloc...\n", __FILENAME__, __LINE__);
		return -ENOMEM;
	}

 	// 자신의 platform_device 객체 저장
	pdata->pdev = pdev;
	pdata->mdev.minor = MISC_DYNAMIC_MINOR;
    pdata->mdev.name = "kisan_usb_notifier";
    pdata->mdev.fops = &kisan_usb_notifier_fops;

	/* register a minimal instruction set computer device (misc) */
	nErr = misc_register(&pdata->mdev);
	if(nErr) {
		dev_err(&pdev->dev,"failed to register misc device\n");
		devm_kfree(&pdev->dev, pdata);
		return nErr;
	}
	
 	// platform_device 에 사용할 고유의 구조체 설정
	platform_set_drvdata(pdev, pdata);

	usb_register_notify(&kisan_usb_nb);
	
	printk(KERN_DEBUG "[%s:%4d] Registered kisan_usb_notifier char driver. \n",
			__FILENAME__, __LINE__);

	return 0;
}

static int kisan_usb_notifier_remove(struct platform_device *pdev)
{
	struct StructKisanUsbNotifierData *pdata = platform_get_drvdata(pdev);

	usb_unregister_notify(&kisan_usb_nb);

	misc_deregister(&pdata->mdev);
	devm_kfree(&pdev->dev, pdata);

    printk(KERN_INFO "[%s:%4d] Unloaded module: kisan_usb_notifier\n", __FILENAME__, __LINE__);

	return 0;
}

static int kisan_usb_notifier_suspend(struct platform_device *pdev, pm_message_t state)
{
	printk(KERN_DEBUG "[%s:%4d:%s]\n", __FILENAME__, __LINE__, __FUNCTION__);
	return 0;
}

static int kisan_usb_notifier_resume(struct platform_device *pdev)
{
	printk(KERN_DEBUG "[%s:%4d:%s]\n", __FILENAME__, __LINE__, __FUNCTION__);
	return 0;
}


#if IS_ENABLED(CONFIG_OF)  
static const struct of_device_id kisan_usb_notifier_dt_ids[] = {  
    {.compatible = "kisan_usb_notifier", },  
    {},  
};
MODULE_DEVICE_TABLE(of, kisan_usb_notifier_dt_ids);  
#endif // #if IS_ENABLED(CONFIG_OF)

static struct platform_driver kisan_usb_notifier_driver = {
	.probe   = kisan_usb_notifier_probe,
	.remove  = kisan_usb_notifier_remove,
	.suspend = kisan_usb_notifier_suspend,
	.resume  = kisan_usb_notifier_resume,
	.driver  = {
        .name           = "kisan_usb_notifier",
        .owner          = THIS_MODULE,
        .of_match_table = of_match_ptr(kisan_usb_notifier_dt_ids), 
    },
};
module_platform_driver(kisan_usb_notifier_driver);


MODULE_DESCRIPTION("AM623x Kisan System B/D USB Notifier Driver");
MODULE_AUTHOR("hong.yongje@kisane.com");
MODULE_LICENSE("Dual BSD/GPL");
