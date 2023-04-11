#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <kisan/kisan_common.h>

#include <net/sock.h>                        // UDP, netlink
#include <linux/kmod.h>                      // call_usermodehelper


static u32 g_unUdpPortNum = 0;

static struct socket *g_sock_UDP;
//EXPORT_SYMBOL(g_sock_IPC_UDP);

static struct sockaddr_in g_addr_UDP;
//EXPORT_SYMBOL(g_addr_IPC_UDP);


//==================================================================================================================================
// NOTE::20210729
// Mailbox 관련
//==================================================================================================================================
#include <asm/siginfo.h>		// siginfo
#include <linux/rcupdate.h>		// rcu_read_lock, rcu_read_unlock

//#define SIG_MAILBOX5_QUEUE1_RX (44)
static u32 g_unSigNum = 0;

static unsigned char g_ucDbgFlag = FALSE;
static int g_nMboxProcID = 0;

//static struct siginfo m_info;
static struct kernel_siginfo m_info;
static struct task_struct *m_t;

unsigned int g_unMboxRxCnt = 0;
EXPORT_SYMBOL(g_unMboxRxCnt);
//==================================================================================================================================


struct StructKisanCommonData {
	struct platform_device *pdev;
	struct miscdevice mdev;
};



// 16비트 정수를 2진수 문자열로 변환
char *U16ToS8(unsigned short usVal)
{
    static char cStr[16 + 1] = {'0', };
    int count = 16;

    do { cStr[--count] = '0' + (char) (usVal & 1);
        usVal = usVal >> 1;
    } while (count);

    return cStr;
}
EXPORT_SYMBOL(U16ToS8);


void BitsDisplay(int nVal, int nBitLen)
{
    int i = 0;
    nBitLen--;

    for(i=nBitLen ; i >=0 ; i--)
        printk(" [%2d] %d", i, BIT_CHECK(nVal, i)? 1:0);

    printk("\n");
}
EXPORT_SYMBOL(BitsDisplay);


void BitsDisplayU32(unsigned int unVal)
{
 	printk(" 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0\n");
	printk("  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d  %d\n",
		(BIT_CHECK(unVal, 31)? 1:0),
		(BIT_CHECK(unVal, 30)? 1:0),
		(BIT_CHECK(unVal, 29)? 1:0),
		(BIT_CHECK(unVal, 28)? 1:0),
		(BIT_CHECK(unVal, 27)? 1:0),
		(BIT_CHECK(unVal, 26)? 1:0),
		(BIT_CHECK(unVal, 25)? 1:0),
		(BIT_CHECK(unVal, 24)? 1:0),
		(BIT_CHECK(unVal, 23)? 1:0),
		(BIT_CHECK(unVal, 22)? 1:0),
		(BIT_CHECK(unVal, 21)? 1:0),
		(BIT_CHECK(unVal, 20)? 1:0),
		(BIT_CHECK(unVal, 19)? 1:0),
		(BIT_CHECK(unVal, 18)? 1:0),
		(BIT_CHECK(unVal, 17)? 1:0),
		(BIT_CHECK(unVal, 16)? 1:0),
		(BIT_CHECK(unVal, 15)? 1:0),
		(BIT_CHECK(unVal, 14)? 1:0),
		(BIT_CHECK(unVal, 13)? 1:0),
		(BIT_CHECK(unVal, 12)? 1:0),
		(BIT_CHECK(unVal, 11)? 1:0),
		(BIT_CHECK(unVal, 10)? 1:0),
		(BIT_CHECK(unVal,  9)? 1:0),
		(BIT_CHECK(unVal,  8)? 1:0),
		(BIT_CHECK(unVal,  7)? 1:0),
		(BIT_CHECK(unVal,  6)? 1:0),
		(BIT_CHECK(unVal,  5)? 1:0),
		(BIT_CHECK(unVal,  4)? 1:0),
		(BIT_CHECK(unVal,  3)? 1:0),
		(BIT_CHECK(unVal,  2)? 1:0),
		(BIT_CHECK(unVal,  1)? 1:0),
		(BIT_CHECK(unVal,  0)? 1:0));
}
EXPORT_SYMBOL(BitsDisplayU32);


int ksocket_send(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len)
{
    struct msghdr msg;
    struct iovec iov;
    mm_segment_t oldfs;
    int size = 0;

    if (sock->sk == NULL)
       return 0;

    iov.iov_base = buf;
    iov.iov_len = len;

    msg.msg_flags = 0;
    msg.msg_name = addr;
    msg.msg_namelen  = sizeof(struct sockaddr_in);
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
//#if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
//	msg.msg_iov = &iov;
//	msg.msg_iovlen = 1;
//#else
	iov_iter_init(&msg.msg_iter, READ, &iov, 1, len);
//#endif
    msg.msg_control = NULL;

    oldfs = get_fs();
    set_fs(KERNEL_DS);
//#if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
//    size = sock_sendmsg(sock, &msg, len);
//#else
	size = sock_sendmsg(sock, &msg);
//#endif
    set_fs(oldfs);

    return size;
}
EXPORT_SYMBOL(ksocket_send);


int SendUdpMsg(unsigned char *buf, int len)
{
    struct msghdr msg;
    struct iovec iov;
    mm_segment_t oldfs;
    int size = 0;

    if (g_sock_UDP->sk == NULL)
       return 0;

    iov.iov_base = buf;
    iov.iov_len = len;

    msg.msg_flags = 0;
    msg.msg_name = &g_addr_UDP;
    msg.msg_namelen  = sizeof(struct sockaddr_in);
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
//#if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
//	msg.msg_iov = &iov;
//	msg.msg_iovlen = 1;
//#else
	iov_iter_init(&msg.msg_iter, READ, &iov, 1, len);
//#endif
    msg.msg_control = NULL;

    oldfs = get_fs();
    set_fs(KERNEL_DS);
//#if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
//    size = sock_sendmsg(sock, &msg, len);
//#else
	size = sock_sendmsg(g_sock_UDP, &msg);
//#endif
    set_fs(oldfs);

    return size;
}
EXPORT_SYMBOL(SendUdpMsg);


int ksocket_receive(struct socket* sock, struct sockaddr_in* addr, unsigned char* buf, int len)
{
    struct msghdr msg;
    struct iovec iov;
    mm_segment_t oldfs;
    int size = 0;

    if (sock->sk == NULL)
        return 0;

    iov.iov_base = buf;
    iov.iov_len = len;

    msg.msg_flags = 0;
    msg.msg_name = addr;
    msg.msg_namelen  = sizeof(struct sockaddr_in);
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
//#if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
//	msg.msg_iov = &iov;
//	msg.msg_iovlen = 1;
//#else
	iov_iter_init(&msg.msg_iter, READ, &iov, 1, len);
//#endif
    msg.msg_control = NULL;

    oldfs = get_fs();
    set_fs(KERNEL_DS);

//#if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
//    size = sock_recvmsg(sock, &msg, len, msg.msg_flags);
//#else
    size = sock_recvmsg(sock, &msg, msg.msg_flags);
//#endif

    set_fs(oldfs);

    return size;
}
EXPORT_SYMBOL(ksocket_receive);


int SaveUdpMsgToSharedMem(unsigned char *pucBuf, int nSaveDataLen)
{
    void __iomem *iomemAddr;
    unsigned char *pucPtr;

    if(nSaveDataLen > 0x100000) { // 1 MB
        printk(KERN_ERR "[%s:%4d:%s] Over Size to DDR shared memory: (0x%08X) \n",
            __FILENAME__, __LINE__, __FUNCTION__, nSaveDataLen);
        return -1;
    }

    iomemAddr = ioremap(ADDR_DDR_CMEM_SHARED_DATA_BUF, nSaveDataLen);
    if(!iomemAddr) {
        printk(KERN_ERR "[%s:%4d:%s] Failed to remap memory for iomemAddr. \n",
            __FILENAME__, __LINE__, __FUNCTION__);
        return -1;
    }
    pucPtr = (unsigned char *)iomemAddr;
    memcpy(pucPtr, pucBuf, nSaveDataLen);

    return 0;
}
EXPORT_SYMBOL(SaveUdpMsgToSharedMem);


int SendUdpMsgToUserSpace(unsigned char *pucBuf, int nSendMsgLen)
{
    struct socket *sock;
    struct sockaddr_in addr;
    int nErr = 0;

    struct msghdr msg;
    struct iovec iov;
    mm_segment_t oldfs;
    int size = 0;

    /* create a socket */
    if ( ((nErr = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock)) < 0) ) {
        printk(KERN_ERR "[%s:%4d:%s] Could not create a datagram socket, err(%d) \n",
            __FILENAME__, __LINE__, __FUNCTION__, -ENXIO);
        return -ENXIO;
    }

    memset(&addr, 0, sizeof(struct sockaddr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(g_unUdpPortNum);//(DEFAULT_UDP_PORT);


    /* send socket */
    if (sock->sk == NULL)
       return -1;

    iov.iov_base = pucBuf;
    iov.iov_len = nSendMsgLen;

    msg.msg_flags = 0;
    msg.msg_name = &addr;
    msg.msg_namelen  = sizeof(struct sockaddr_in);
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
//#if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
//	msg.msg_iov = &iov;
//	msg.msg_iovlen = 1;
//#else
	iov_iter_init(&msg.msg_iter, READ, &iov, 1, nSendMsgLen);
//#endif
    msg.msg_control = NULL;

    oldfs = get_fs();
    set_fs(KERNEL_DS);
//#if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
//	  size = sock_sendmsg(sock, &msg, len);
//#else
	size = sock_sendmsg(sock, &msg);
//#endif
    set_fs(oldfs);

    sock_release(sock);
    sock = NULL;

    return 0;
}
EXPORT_SYMBOL(SendUdpMsgToUserSpace);


void execute_shell_command(char* strPath, char* strCommand, char* strArgv1, char* strArgv2, char* strArgv3)
{
    char *argv[5] = { NULL, NULL, NULL, NULL, NULL };
    static char *envp[4] = { "HOME=/", "TERM=linux", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };

    argv[0] = strPath;
    argv[1] = strCommand;
    argv[2] = strArgv1;
    argv[3] = strArgv2;
    argv[4] = strArgv3;

    printk(KERN_DEBUG "[%s:%4d] Execute shell command > %s, %s, %s, %s, %s \n", 
        __FILENAME__, __LINE__,  argv[0], argv[1], argv[2], argv[3], argv[4]);
    
    call_usermodehelper( argv[0], argv, envp, UMH_WAIT_EXEC );
}
EXPORT_SYMBOL(execute_shell_command);    


static int kisan_common_open(struct inode *inode, struct file *file)
{
    printk(KERN_DEBUG "[%s:%4d:%s] \n", __FILENAME__, __LINE__, __FUNCTION__);            
    return 0;
}


static ssize_t kisan_common_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    printk(KERN_DEBUG "[%s:%4d:%s] \n", __FILENAME__, __LINE__, __FUNCTION__);            
    return 0;
} 


static ssize_t kisan_common_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
    printk(KERN_DEBUG "[%s:%4d:%s] \n", __FILENAME__, __LINE__, __FUNCTION__);            
	return 0;
}


int SendSignalToUserSpace_Mbox_Rx(int nValue)
{
	if(g_nMboxProcID <= 0 || m_t == NULL) {
/*
		printk(KERN_DEBUG "[%s:%4d:%s] Wrong pid: (%d)\n", 
			__FILENAME__, __LINE__, __FUNCTION__, g_nMboxProcID);
*/
	}
	else
	{	
		m_info.si_int = nValue;
		
	    if(send_sig_info(g_unSigNum/*SIG_MAILBOX5_QUEUE1_RX*/, &m_info, m_t) < 0) {
	        printk(KERN_DEBUG "[%s:%4d:%s] Error sending signal: (%d)\n",
				__FILENAME__, __LINE__, __FUNCTION__, g_unSigNum);//SIG_MAILBOX5_QUEUE1_RX);
	    }
		else
		{
		    if(g_ucDbgFlag == __ON__) {
			    printk(KERN_DEBUG ">mb[%d][%08x]\n", g_unMboxRxCnt, nValue);
			}
		}
	}
    return 0;
}
EXPORT_SYMBOL(SendSignalToUserSpace_Mbox_Rx);


struct structKisanCommon {
    int nKernelModule;
	int nCmdForKModule;
	int nProcessID;
	unsigned char aucData[32];
    unsigned char ucDbgFlag;
};

typedef enum {
    KERNEL_MODULE_OMAP_MAILBOX = 0,
    MAX_NUM_OF_KERNEL_MODULE
} eKisanCommonKernelModule;

typedef enum {
	OMAP_MAILBOX_CMD_SET_PID = 0,
    MAX_NUM_OF_OMAP_MAILBOX_CMD
} eOmapMailboxCommands;


static long kisan_common_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
	void __user *argp = (void __user *) args;
	int nRet = 0;

	struct structKisanCommon stKisanCommon;
	int nKModule = 0;
	int nCmd = 0;
	int nProcID = 0;
	unsigned char aucData[32];
	
	if(copy_from_user(&stKisanCommon, argp, sizeof(struct structKisanCommon))) {
		printk(KERN_ERR "[%s:%4d:%s] copy_from_user \n", __FILENAME__, __LINE__, __FUNCTION__);
		return -EFAULT;
	}

	nKModule = stKisanCommon.nKernelModule;
	nCmd = stKisanCommon.nCmdForKModule;
	nProcID = stKisanCommon.nProcessID;
	memcpy(aucData, stKisanCommon.aucData, 32);
	g_ucDbgFlag = stKisanCommon.ucDbgFlag;

    if(g_ucDbgFlag == __ON__) {
	    printk(KERN_DEBUG "[%s:%4d:%s] DbgMsg is ON. \n", __FILENAME__, __LINE__, __FUNCTION__);
    }

	switch(nKModule)
	{
		case KERNEL_MODULE_OMAP_MAILBOX:
			if(nCmd == OMAP_MAILBOX_CMD_SET_PID)
			{
				// NOTE::20210203
				// Mailbox Rx 시그널을 받을 Userspace의 Process ID 를 설정
				printk(KERN_DEBUG "[%s:%4d:%s] Set Omap-Mailbox Process ID: (%d)\n", 
					__FILENAME__, __LINE__, __FUNCTION__, nProcID);


				if(nProcID <= 0) {
					if(g_ucDbgFlag == __ON__) {
						printk(KERN_DEBUG "[%s:%4d:%s] Wrong pid: (%d)\n", 
							__FILENAME__, __LINE__, __FUNCTION__, nProcID);
					}
					return -EFAULT;
				}

				memset(&m_info, 0 ,sizeof(struct siginfo));
				m_info.si_signo = g_unSigNum;//SIG_MAILBOX5_QUEUE1_RX;
				m_info.si_code = SI_QUEUE;
				m_info.si_int = 0;
			
				rcu_read_lock();

				//m_t = find_task_by_vpid(nProcID);
				m_t = pid_task(find_vpid(nProcID), PIDTYPE_PID);
				if(m_t == NULL) {
					printk(KERN_DEBUG "[%s:%4d:%s] Failed to find_task_by_vpid...\n", __FILENAME__, __LINE__, __FUNCTION__);
					rcu_read_unlock();
					return -ENODEV;
				}
				rcu_read_unlock();

				g_nMboxProcID = nProcID;				
				g_unMboxRxCnt = 0;
			}
			else
			{
				nRet = -EFAULT;
			}
			break;

		default:
			printk(KERN_ERR "[%s:%4d:%s] Wrong Kernel Module (%d) \n",
				__FILENAME__, __LINE__, __FUNCTION__, nKModule);
			nRet = -EINVAL;
			break;
	}

    return nRet;
}


static struct file_operations kisan_common_fops = {
	.owner            = THIS_MODULE,
    .open             = kisan_common_open,
    .read             = kisan_common_read,
    .write            = kisan_common_write,
    .unlocked_ioctl   = kisan_common_ioctl,
};

static int CreateUdpSocket(void)
{
	int nErr = 0;

	/* create a socket */
	if ( ((nErr = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &g_sock_UDP)) < 0) ) {
		printk(KERN_INFO "[%s:%4d:%s] Could not create a datagram socket, error = %d \n",
			__FILENAME__, __LINE__, __FUNCTION__, -ENXIO);
		return -ENXIO;
	}

	memset(&g_addr_UDP, 0, sizeof(struct sockaddr));
	g_addr_UDP.sin_family	   = AF_INET;
	g_addr_UDP.sin_addr.s_addr = htonl(INADDR_ANY);
	g_addr_UDP.sin_port 	   = htons(g_unUdpPortNum);//(DEFAULT_UDP_PORT);	// 7890

	printk(KERN_DEBUG "[%s:%4d] Created UDP Socket for IPC (Port: %d)\n",
			__FILENAME__, __LINE__, g_unUdpPortNum);

	return 0;
}


#if 0	// #if defined(USE_NETLINK_SOCKET)
#define MAX_PAYLOAD (16)
#define NL_EXAMPLE 	(19)
#define NL_GROUP 	(1)

static struct sock *nl_sk = NULL;

void NetlinkSendMsg(unsigned char *buf, int len)
{
    struct sk_buff *skb = NULL;
    struct nlmsghdr *nlh;

	skb = alloc_skb(NLMSG_SPACE (MAX_PAYLOAD), GFP_KERNEL);
	nlh = (struct nlmsghdr *)skb_put (skb, NLMSG_SPACE (MAX_PAYLOAD));
	nlh->nlmsg_len = NLMSG_SPACE (MAX_PAYLOAD);
	nlh->nlmsg_pid = 0;
	nlh->nlmsg_flags = 0;
	
	memcpy (NLMSG_DATA (nlh), buf, len);
	
	NETLINK_CB (skb).dst_group = NL_GROUP;
	netlink_broadcast (nl_sk, skb, 0, NL_GROUP, GFP_KERNEL);
}
EXPORT_SYMBOL(NetlinkSendMsg);

static int CreateNetlinkSocket(void)
{
	nl_sk = netlink_kernel_create(&init_net, NL_EXAMPLE, NULL);	
	if (nl_sk == NULL) {
        printk(KERN_ERR "[%s:%4d:%s] Failed to create netlink socket... \n",
            __FILENAME__, __LINE__, __FUNCTION__);
		return -1;
	}

	return 0;
}
#endif	// #if defined(USE_NETLINK_SOCKET)



static int kisan_common_probe(struct platform_device *pdev)
{
	struct StructKisanCommonData *pdata = NULL;
	struct device_node *np = pdev->dev.of_node;
	int nErr = 0;
	u32 unDeviceTreeNode = 0;

	pdata = devm_kzalloc(&pdev->dev, sizeof(struct StructKisanCommonData), GFP_KERNEL);
	if(!pdata) {
		printk(KERN_ERR "[%s:%4d] Failed to devm_kzalloc...\n", __FILENAME__, __LINE__);
		return -ENOMEM;
	}

	// 자신의 platform_device 객체 저장
	pdata->pdev = pdev;
	pdata->mdev.minor = MISC_DYNAMIC_MINOR;
    pdata->mdev.name = "kisan_common";
    pdata->mdev.fops = &kisan_common_fops;

	/* register a minimal instruction set computer device (misc) */
	nErr = misc_register(&pdata->mdev);
	if(nErr) {
		dev_err(&pdev->dev,"failed to register misc device\n");
		devm_kfree(&pdev->dev, pdata);
		return nErr;
	}

	// platform_device 에 사용할 고유의 구조체 설정
	platform_set_drvdata(pdev, pdata);


	if (!of_property_read_u32(np, "UDP_PORT", &unDeviceTreeNode)) {
		printk(KERN_DEBUG "[%s:%4d] Read DT: UDP_PORT(%d)\n", __FILENAME__, __LINE__, unDeviceTreeNode);
		g_unUdpPortNum = unDeviceTreeNode;
	}
	else {
		printk(KERN_ERR "[%s:%4d] Failed to Read DT: UDP_PORT(%d)\n", __FILENAME__, __LINE__, unDeviceTreeNode);
	}

	if(g_unUdpPortNum > 0) {
		if(CreateUdpSocket() < 0) {
			printk(KERN_ERR "[%s:%4d:%s] Failed to CreateUdpSocket... \n", __FILENAME__, __LINE__, __FUNCTION__);
		}
	}
	else {
		printk(KERN_ERR "[%s:%4d] Wrong UDP_PORT(%d)\n", __FILENAME__, __LINE__, unDeviceTreeNode);
	}

#if defined (USE_NETLINK_SOCKET)
	if(CreateNetlinkSocket() < 0) {
		printk(KERN_ERR "[%s:%4d:%s] Failed to CreateNetlinkSocket... \n", __FILENAME__, __LINE__, __FUNCTION__);
	}
#endif	// #if defined (USE_NETLINK_SOCKET)


	if (!of_property_read_u32(np, "SIG_NUM", &unDeviceTreeNode)) {
		printk(KERN_DEBUG "[%s:%4d] Read DT: SIG_NUM(%d)\n", __FILENAME__, __LINE__, unDeviceTreeNode);
		g_unSigNum = unDeviceTreeNode;
	}
	else {
		printk(KERN_ERR "[%s:%4d] Failed to Read DT: SIG_NUM(%d)\n", __FILENAME__, __LINE__, unDeviceTreeNode);
	}

	printk(KERN_DEBUG "[%s:%4d] Registered kisan_common char driver.\n", __FILENAME__, __LINE__);
	return 0;

}

static int kisan_common_remove(struct platform_device *pdev)
{
	struct StructKisanCommonData *pdata = platform_get_drvdata(pdev);

    printk(KERN_DEBUG "[%s:%4d:%s]\n", __FILENAME__, __LINE__, __FUNCTION__);

	sock_release(g_sock_UDP);
	g_sock_UDP = NULL;

#if defined (USE_NETLINK_SOCKET)
	sock_release (nl_sk->sk_socket);
#endif	// #if defined (USE_NETLINK_SOCKET)

	misc_deregister(&pdata->mdev);
	devm_kfree(&pdev->dev, pdata);

	printk(KERN_INFO "[%s:%4d] Unloaded module: kisan_common\n", __FILENAME__, __LINE__);
	return 0;
}

static int kisan_common_suspend(struct platform_device *pdev, pm_message_t state)
{
    printk(KERN_DEBUG "[%s:%4d:%s]\n", __FILENAME__, __LINE__, __FUNCTION__);
	return 0;
}

static int kisan_common_resume(struct platform_device *pdev)
{
    printk(KERN_DEBUG "[%s:%4d:%s]\n", __FILENAME__, __LINE__, __FUNCTION__);
	return 0;
}


#if IS_ENABLED(CONFIG_OF)  
static const struct of_device_id kisan_common_dt_ids[] = {  
    {.compatible = "kisan_common", },  
    {},  
};
MODULE_DEVICE_TABLE(of, kisan_common_dt_ids);  
#endif // #if IS_ENABLED(CONFIG_OF)

static struct platform_driver kisan_common_driver = {
	.probe   = kisan_common_probe,
	.remove  = kisan_common_remove,
	.suspend = kisan_common_suspend,
	.resume  = kisan_common_resume,
	.driver  = {
        .name           = "kisan_common",
        .owner          = THIS_MODULE,
        .of_match_table = of_match_ptr(kisan_common_dt_ids), 
    },
};
module_platform_driver(kisan_common_driver);


MODULE_DESCRIPTION("AM623x Kisan System B/D Common Driver");
MODULE_AUTHOR("hong.yongje@kisane.com");
MODULE_LICENSE("Dual BSD/GPL");

