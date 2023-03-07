# ti_am6231_kernel

## SDK 정보

https://www.ti.com/tool/PROCESSOR-SDK-AM62X

ti-processor-sdk-linux-am62xx-evm-08.05.00.21 기반



## 참고 링크

https://software-dl.ti.com/processor-sdk-linux/esd/AM62X/08_05_00_21/exports/docs/linux/Overview_Building_the_SDK.html


## 의존성 패키지 설치

sudo apt-get install build-essential autoconf automake bison flex libssl-dev bc u-boot-tools python diffstat texinfo gawk chrpath dos2unix wget unzip socat doxygen libc6:i386 libncurses5:i386 libstdc++6:i386 libz1:i386 g++-multilib git python3-distutils python3-apt

sudo apt-get install libelf-dev dwarves


## 빌드 스크립트

SDK 설치 경로의 Makefile 과 Rules.make 파일을 참조하여 작성

```bash
#!/bin/bash

PATH_PROJ=/home/yjhong/kisan/TI_AM6231
PATH_KERNEL=${PATH_PROJ}/kernel/trunk/linux-5.10.153
PATH_RFS=${PATH_PROJ}/rootfs/trunk
PATH_IMG=${PATH_PROJ}/image
PATH_UTIL=${PATH_PROJ}/util

CC_ARMV8=/home/yjhong/kisan/TI_AM6231/toolchain/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-
CC=${CC_ARMV8}

SQUASHFS_FILE=filesystem.squashfs


#Points to the root of the TI SDK
TI_SDK_PATH=/home/yjhong/kisan/TI_AM6231/ti-processor-sdk-linux-am62xx-evm-08.05.00.21

#DEFCONFIG=tisdk_am62xx-evm_defconfig
DEFCONFIG=kisan_s1_defconfig

INSTALL_MOD_STRIP=1


echo "### Start AM6231 Auto Making Script ###"

echo "### Checking permission"

AMIROOT=`whoami | awk {'print $1'}`
if [ "$AMIROOT" != "root" ] ; then
	echo "  **** Error *** must run script with sudo"
	echo ""
	exit
fi

cd ${PATH_KERNEL}

echo "### Cleaning & Configuring Kisan S1(AM6231) Kernel"
#make ARCH=arm64 CROSS_COMPILE=${CC} mrproper
make ARCH=arm64 CROSS_COMPILE=${CC} ${DEFCONFIG}


echo =====================================
echo     Building the Linux Kernel DTBs
echo =====================================
for DTB in ti/k3-am625-sk.dtb ti/k3-am625-skeleton.dtb ti/k3-am625-sk-lpmdemo.dtb ti/k3-am625-sk-csi2-ov5640.dtbo
do
    #echo $DTB
    make -j 2 ARCH=arm64 CROSS_COMPILE=${CC} $DTB
done


echo =================================
echo     Building the Linux Kernel
echo =================================
#./scripts/config --disable SYSTEM_TRUSTED_KEY
#./scripts/config --disable SYSTEM_REVOCATION_KEYS
make -j 2 ARCH=arm64 CROSS_COMPILE=${CC} Image


if [ "$1" = '-m' ] ; then
    echo "### Building Modules"
    make -j 2 ARCH=arm64 CROSS_COMPILE=${CC} modules
    sync
    echo "### Copying Modules to Rootfs (~/rootfs/lib/modules/...)"
    make INSTALL_MOD_PATH=${PATH_RFS} INSTALL_MOD_STRIP=${INSTALL_MOD_STRIP} ARCH=arm64 CROSS_COMPILE=${CC} modules_install
    sync
else
	echo "--- Skip Building Modules..."
fi


cp ${PATH_KERNEL}/arch/arm64/boot/dts/ti/*.dtb* ${PATH_RFS}/boot
cp ${PATH_KERNEL}/arch/arm64/boot/Image ${PATH_RFS}/boot
sync
cp ${PATH_RFS}/boot/* ${PATH_IMG}/mmcblk0p2/boot
sync

echo "### End AM6231 Auto Making Script ###"

```



## SD 카드 부팅 체크

```bash
root@am62xx-evm:~# dmesg
[    0.000000] Booting Linux on physical CPU 0x0000000000 [0x410fd034]
[    0.000000] Linux version 5.10.153 (root@ubuntu18) (aarch64-none-linux-gnu-gcc (GNU Toolchain for the A-profile Architecture 9.2-2019.12 (arm-9.10)) 9.2.1 20191025, GNU ld (GNU Toolchain for the A-profile Architecture 9.2-2019.12 (arm-9.10)) 2.33.1.20191209) #3 SMP PREEMPT Mon Mar 6 16:25:36 KST 2023
[    0.000000] Machine model: Texas Instruments AM625 SK
[    0.000000] earlycon: ns16550a0 at MMIO32 0x0000000002800000 (options '')
[    0.000000] printk: bootconsole [ns16550a0] enabled
[    0.000000] efi: UEFI not found.
[    0.000000] Reserved memory: created DMA memory pool at 0x000000009c800000, size 3 MiB
[    0.000000] OF: reserved mem: initialized node ipc-memories@9c800000, compatible id shared-dma-pool
[    0.000000] Reserved memory: created DMA memory pool at 0x000000009cb00000, size 1 MiB
[    0.000000] OF: reserved mem: initialized node m4f-dma-memory@9cb00000, compatible id shared-dma-pool
[    0.000000] Reserved memory: created DMA memory pool at 0x000000009cc00000, size 14 MiB
[    0.000000] OF: reserved mem: initialized node m4f-memory@9cc00000, compatible id shared-dma-pool
[    0.000000] Reserved memory: created DMA memory pool at 0x000000009da00000, size 1 MiB
[    0.000000] OF: reserved mem: initialized node r5f-dma-memory@9da00000, compatible id shared-dma-pool
[    0.000000] Reserved memory: created DMA memory pool at 0x000000009db00000, size 12 MiB
[    0.000000] OF: reserved mem: initialized node r5f-memory@9db00000, compatible id shared-dma-pool
[    0.000000] Zone ranges:
[    0.000000]   DMA      [mem 0x0000000080000000-0x00000000ffffffff]
[    0.000000]   DMA32    empty
[    0.000000]   Normal   empty
[    0.000000] Movable zone start for each node
[    0.000000] Early memory node ranges
[    0.000000]   node   0: [mem 0x0000000080000000-0x000000009c7fffff]
[    0.000000]   node   0: [mem 0x000000009c800000-0x000000009e6fffff]
[    0.000000]   node   0: [mem 0x000000009e700000-0x000000009e77ffff]
[    0.000000]   node   0: [mem 0x000000009e780000-0x000000009fffffff]
[    0.000000]   node   0: [mem 0x00000000a0000000-0x00000000ffffffff]
[    0.000000] Initmem setup node 0 [mem 0x0000000080000000-0x00000000ffffffff]
[    0.000000] On node 0 totalpages: 524288
[    0.000000]   DMA zone: 8192 pages used for memmap
[    0.000000]   DMA zone: 0 pages reserved
[    0.000000]   DMA zone: 524288 pages, LIFO batch:63
[    0.000000] cma: Reserved 512 MiB at 0x00000000dd000000
[    0.000000] psci: probing for conduit method from DT.
[    0.000000] psci: PSCIv1.1 detected in firmware.
[    0.000000] psci: Using standard PSCI v0.2 function IDs
[    0.000000] psci: Trusted OS migration not required
[    0.000000] psci: SMC Calling Convention v1.2
[    0.000000] percpu: Embedded 22 pages/cpu s50008 r8192 d31912 u90112
[    0.000000] pcpu-alloc: s50008 r8192 d31912 u90112 alloc=22*4096
[    0.000000] pcpu-alloc: [0] 0 [0] 1 [0] 2 [0] 3
[    0.000000] Detected VIPT I-cache on CPU0
[    0.000000] CPU features: detected: ARM erratum 845719
[    0.000000] CPU features: detected: GIC system register CPU interface
[    0.000000] Built 1 zonelists, mobility grouping on.  Total pages: 516096
[    0.000000] Kernel command line: console=ttyS2,115200n8 earlycon=ns16550a,mmio32,0x02800000 root=PARTUUID=af84bedf-02 rw rootfstype=ext4 rootwait
[    0.000000] Dentry cache hash table entries: 262144 (order: 9, 2097152 bytes, linear)
[    0.000000] Inode-cache hash table entries: 131072 (order: 8, 1048576 bytes, linear)
[    0.000000] mem auto-init: stack:off, heap alloc:off, heap free:off
[    0.000000] Memory: 1456392K/2097152K available (10880K kernel code, 1148K rwdata, 4216K rodata, 1792K init, 432K bss, 116472K reserved, 524288K cma-reserved)
[    0.000000] SLUB: HWalign=64, Order=0-3, MinObjects=0, CPUs=4, Nodes=1
[    0.000000] rcu: Preemptible hierarchical RCU implementation.
[    0.000000] rcu:     RCU event tracing is enabled.
[    0.000000] rcu:     RCU restricting CPUs from NR_CPUS=256 to nr_cpu_ids=4.
[    0.000000]  Trampoline variant of Tasks RCU enabled.
[    0.000000] rcu: RCU calculated value of scheduler-enlistment delay is 25 jiffies.
[    0.000000] rcu: Adjusting geometry for rcu_fanout_leaf=16, nr_cpu_ids=4
[    0.000000] NR_IRQS: 64, nr_irqs: 64, preallocated irqs: 0
[    0.000000] GICv3: GIC: Using split EOI/Deactivate mode
[    0.000000] GICv3: 256 SPIs implemented
[    0.000000] GICv3: 0 Extended SPIs implemented
[    0.000000] GICv3: Distributor has no Range Selector support
[    0.000000] GICv3: 16 PPIs implemented
[    0.000000] GICv3: CPU0: found redistributor 0 region 0:0x0000000001880000
[    0.000000] ITS [mem 0x01820000-0x0182ffff]
[    0.000000] GIC: enabling workaround for ITS: Socionext Synquacer pre-ITS
[    0.000000] ITS@0x0000000001820000: Devices Table too large, reduce ids 20->19
[    0.000000] ITS@0x0000000001820000: allocated 524288 Devices @80800000 (flat, esz 8, psz 64K, shr 0)
[    0.000000] ITS: using cache flushing for cmd queue
[    0.000000] GICv3: using LPI property table @0x0000000080030000
[    0.000000] GIC: using cache flushing for LPI property table
[    0.000000] GICv3: CPU0: using allocated LPI pending table @0x0000000080040000
[    0.000000] arch_timer: cp15 timer(s) running at 200.00MHz (phys).
[    0.000000] clocksource: arch_sys_counter: mask: 0xffffffffffffff max_cycles: 0x2e2049d3e8, max_idle_ns: 440795210634 ns
[    0.000004] sched_clock: 56 bits at 200MHz, resolution 5ns, wraps every 4398046511102ns
[    0.008510] Console: colour dummy device 80x25
[    0.013101] Calibrating delay loop (skipped), value calculated using timer frequency.. 400.00 BogoMIPS (lpj=800000)
[    0.023782] pid_max: default: 32768 minimum: 301
[    0.028588] LSM: Security Framework initializing
[    0.033372] Mount-cache hash table entries: 4096 (order: 3, 32768 bytes, linear)
[    0.040952] Mountpoint-cache hash table entries: 4096 (order: 3, 32768 bytes, linear)
[    0.050694] rcu: Hierarchical SRCU implementation.
[    0.055883] Platform MSI: msi-controller@1820000 domain created
[    0.062169] PCI/MSI: /bus@f0000/interrupt-controller@1800000/msi-controller@1820000 domain created
[    0.071406] EFI services will not be available.
[    0.076313] smp: Bringing up secondary CPUs ...
[    0.089590] Detected VIPT I-cache on CPU1
[    0.089626] GICv3: CPU1: found redistributor 1 region 0:0x00000000018a0000
[    0.089641] GICv3: CPU1: using allocated LPI pending table @0x0000000080050000
[    0.089708] CPU1: Booted secondary processor 0x0000000001 [0x410fd034]
[    0.098357] Detected VIPT I-cache on CPU2
[    0.098381] GICv3: CPU2: found redistributor 2 region 0:0x00000000018c0000
[    0.098391] GICv3: CPU2: using allocated LPI pending table @0x0000000080060000
[    0.098427] CPU2: Booted secondary processor 0x0000000002 [0x410fd034]
[    0.107042] Detected VIPT I-cache on CPU3
[    0.107063] GICv3: CPU3: found redistributor 3 region 0:0x00000000018e0000
[    0.107074] GICv3: CPU3: using allocated LPI pending table @0x0000000080070000
[    0.107110] CPU3: Booted secondary processor 0x0000000003 [0x410fd034]
[    0.107183] smp: Brought up 1 node, 4 CPUs
[    0.186893] SMP: Total of 4 processors activated.
[    0.191704] CPU features: detected: 32-bit EL0 Support
[    0.196970] CPU features: detected: CRC32 instructions
[    0.209507] CPU: All CPU(s) started at EL2
[    0.213714] alternatives: patching kernel code
[    0.219420] devtmpfs: initialized
[    0.228645] KASLR disabled due to lack of seed
[    0.233391] clocksource: jiffies: mask: 0xffffffff max_cycles: 0xffffffff, max_idle_ns: 7645041785100000 ns
[    0.243366] futex hash table entries: 1024 (order: 4, 65536 bytes, linear)
[    0.264825] pinctrl core: initialized pinctrl subsystem
[    0.270854] DMI not present or invalid.
[    0.275468] NET: Registered protocol family 16
[    0.281679] DMA: preallocated 256 KiB GFP_KERNEL pool for atomic allocations
[    0.289026] DMA: preallocated 256 KiB GFP_KERNEL|GFP_DMA pool for atomic allocations
[    0.297093] DMA: preallocated 256 KiB GFP_KERNEL|GFP_DMA32 pool for atomic allocations
[    0.305812] thermal_sys: Registered thermal governor 'step_wise'
[    0.305818] thermal_sys: Registered thermal governor 'power_allocator'
[    0.312517] hw-breakpoint: found 6 breakpoint and 4 watchpoint registers.
[    0.326238] ASID allocator initialised with 65536 entries
[    0.355650] HugeTLB registered 1.00 GiB page size, pre-allocated 0 pages
[    0.362528] HugeTLB registered 32.0 MiB page size, pre-allocated 0 pages
[    0.369381] HugeTLB registered 2.00 MiB page size, pre-allocated 0 pages
[    0.376232] HugeTLB registered 64.0 KiB page size, pre-allocated 0 pages
[    0.384122] cryptd: max_cpu_qlen set to 1000
[    0.391366] k3-chipinfo 43000014.chipid: Family:AM62X rev:SR1.0 JTAGID[0x0bb7e02f] Detected
[    0.400426] vcc_5v0: supplied by vmain_pd
[    0.404951] vcc_3v3_sys: supplied by vmain_pd
[    0.409840] vcc_1v8: supplied by vcc_3v3_sys
[    0.415244] iommu: Default domain type: Translated
[    0.420580] SCSI subsystem initialized
[    0.424818] mc: Linux media interface: v0.10
[    0.429208] videodev: Linux video capture interface: v2.00
[    0.434878] pps_core: LinuxPPS API ver. 1 registered
[    0.439952] pps_core: Software ver. 5.3.6 - Copyright 2005-2007 Rodolfo Giometti <giometti@linux.it>
[    0.449301] PTP clock support registered
[    0.453339] EDAC MC: Ver: 3.0.0
[    0.457198] omap-mailbox 29000000.mailbox: omap mailbox rev 0x66fc9100
[    0.464369] FPGA manager framework
[    0.467937] Advanced Linux Sound Architecture Driver Initialized.
[    0.475088] clocksource: Switched to clocksource arch_sys_counter
[    0.481539] VFS: Disk quotas dquot_6.6.0
[    0.485604] VFS: Dquot-cache hash table entries: 512 (order 0, 4096 bytes)
[    0.497955] NET: Registered protocol family 2
[    0.502702] IP idents hash table entries: 32768 (order: 6, 262144 bytes, linear)
[    0.511405] tcp_listen_portaddr_hash hash table entries: 1024 (order: 2, 16384 bytes, linear)
[    0.520220] TCP established hash table entries: 16384 (order: 5, 131072 bytes, linear)
[    0.528426] TCP bind hash table entries: 16384 (order: 6, 262144 bytes, linear)
[    0.536152] TCP: Hash tables configured (established 16384 bind 16384)
[    0.543052] UDP hash table entries: 1024 (order: 3, 32768 bytes, linear)
[    0.549948] UDP-Lite hash table entries: 1024 (order: 3, 32768 bytes, linear)
[    0.557429] NET: Registered protocol family 1
[    0.562347] RPC: Registered named UNIX socket transport module.
[    0.568422] RPC: Registered udp transport module.
[    0.573260] RPC: Registered tcp transport module.
[    0.578074] RPC: Registered tcp NFSv4.1 backchannel transport module.
[    0.584667] PCI: CLS 0 bytes, default 64
[    0.589494] hw perfevents: enabled with armv8_cortex_a53 PMU driver, 7 counters available
[    0.601583] Initialise system trusted keyrings
[    0.606351] workingset: timestamp_bits=46 max_order=19 bucket_order=0
[    0.616529] squashfs: version 4.0 (2009/01/31) Phillip Lougher
[    0.623095] NFS: Registering the id_resolver key type
[    0.628312] Key type id_resolver registered
[    0.632590] Key type id_legacy registered
[    0.636751] nfs4filelayout_init: NFSv4 File Layout Driver Registering...
[    0.643608] nfs4flexfilelayout_init: NFSv4 Flexfile Layout Driver Registering...
[    0.651367] 9p: Installing v9fs 9p2000 file system support
[    0.691646] Key type asymmetric registered
[    0.695843] Asymmetric key parser 'x509' registered
[    0.700867] Block layer SCSI generic (bsg) driver version 0.4 loaded (major 243)
[    0.708432] io scheduler mq-deadline registered
[    0.713062] io scheduler kyber registered
[    0.719289] pinctrl-single 4084000.pinctrl: 34 pins, size 136
[    0.725629] pinctrl-single f4000.pinctrl: 171 pins, size 684
[    0.738943] Serial: 8250/16550 driver, 10 ports, IRQ sharing enabled
[    0.758985] brd: module loaded
[    0.769230] loop: module loaded
[    0.773376] megasas: 07.714.04.00-rc1
[    0.780806] tun: Universal TUN/TAP device driver, 1.6
[    0.786562] igbvf: Intel(R) Gigabit Virtual Function Network Driver
[    0.792979] igbvf: Copyright (c) 2009 - 2012 Intel Corporation.
[    0.799091] sky2: driver version 1.30
[    0.803805] VFIO - User Level meta-driver version: 0.3
[    0.810067] i2c /dev entries driver
[    0.815343] sdhci: Secure Digital Host Controller Interface driver
[    0.821673] sdhci: Copyright(c) Pierre Ossman
[    0.826459] sdhci-pltfm: SDHCI platform and OF driver helper
[    0.833340] ledtrig-cpu: registered to indicate activity on CPUs
[    0.839847] SMCCC: SOC_ID: ARCH_SOC_ID not implemented, skipping ....
[    0.847995] optee: probing for conduit method.
[    0.852584] optee: revision 3.19 (d6c5d003)
[    0.852863] optee: dynamic shared memory is enabled
[    0.862515] optee: initialized driver
[    0.868303] NET: Registered protocol family 17
[    0.873027] 9pnet: Installing 9P2000 support
[    0.877481] Key type dns_resolver registered
[    0.882056] Loading compiled-in X.509 certificates
[    0.897559] ti-sci 44043000.system-controller: ti,ctx-memory-region is required for suspend but not provided.
[    0.907727] ti-sci 44043000.system-controller: ti_sci_init_suspend failed, mem suspend will be non-functional.
[    0.917962] ti-sci 44043000.system-controller: ABI: 3.1 (firmware rev 0x0008 '8.5.3--v08.05.03 (Chill Capybar')
[    0.983433] davinci-mcasp 2b10000.mcasp: IRQ common not found
[    0.991884] omap-gpmc 3b000000.memory-controller: GPMC revision 6.0
[    0.998344] gpmc_mem_init: disabling cs 0 mapped at 0x0-0x1000000
[    1.007465] omap_i2c 20000000.i2c: bus 0 rev0.12 at 400 kHz
[    1.015076] omap_i2c 20010000.i2c: bus 1 rev0.12 at 100 kHz
[    1.021381] ti-sci-intr 4210000.interrupt-controller: Interrupt Router 5 domain created
[    1.029740] ti-sci-intr bus@f0000:interrupt-controller@a00000: Interrupt Router 3 domain created
[    1.038993] ti-sci-inta 48000000.interrupt-controller: Interrupt Aggregator domain 28 created
[    1.048467] ti-udma 485c0100.dma-controller: Number of rings: 82
[    1.056527] ti-udma 485c0100.dma-controller: Channels: 48 (bchan: 18, tchan: 12, rchan: 18)
[    1.067591] ti-udma 485c0000.dma-controller: Number of rings: 150
[    1.077428] ti-udma 485c0000.dma-controller: Channels: 35 (tchan: 20, rchan: 15)
[    1.087599] printk: console [ttyS2] disabled
[    1.092044] 2800000.serial: ttyS2 at MMIO 0x2800000 (irq = 27, base_baud = 3000000) is a 8250
[    1.100812] printk: console [ttyS2] enabled
[    1.109254] printk: bootconsole [ns16550a0] disabled
[    1.124029] spi-nor spi0.0: s28hs512t (65536 Kbytes)
[    1.129049] 7 fixed-partitions partitions found on MTD device fc40000.spi.0
[    1.136002] Creating 7 MTD partitions on "fc40000.spi.0":
[    1.141402] 0x000000000000-0x000000080000 : "ospi.tiboot3"
[    1.148124] 0x000000080000-0x000000280000 : "ospi.tispl"
[    1.154512] 0x000000280000-0x000000680000 : "ospi.u-boot"
[    1.160935] 0x000000680000-0x0000006c0000 : "ospi.env"
[    1.167121] 0x0000006c0000-0x000000700000 : "ospi.env.backup"
[    1.173826] 0x000000800000-0x000003fc0000 : "ospi.rootfs"
[    1.180271] 0x000003fc0000-0x000004000000 : "ospi.phypattern"
[    1.193093] davinci_mdio 8000f00.mdio: Configuring MDIO in manual mode
[    1.239092] davinci_mdio 8000f00.mdio: davinci mdio revision 9.7, bus freq 1000000
[    1.248772] davinci_mdio 8000f00.mdio: phy[0]: device 8000f00.mdio:00, driver TI DP83867
[    1.256870] davinci_mdio 8000f00.mdio: phy[1]: device 8000f00.mdio:01, driver TI DP83867
[    1.265047] am65-cpsw-nuss 8000000.ethernet: initializing am65 cpsw nuss version 0x6BA01103, cpsw version 0x6BA81103 Ports: 3 quirks:00000002
[    1.277891] am65-cpsw-nuss 8000000.ethernet: initialized cpsw ale version 1.5
[    1.285017] am65-cpsw-nuss 8000000.ethernet: ALE Table size 512
[    1.291675] am65-cpsw-nuss 8000000.ethernet: CPTS ver 0x4e8a010c, freq:500000000, add_val:1 pps:0
[    1.304057] rtc-ti-k3 2b1f0000.rtc: registered as rtc0
[    1.309250] rtc-ti-k3 2b1f0000.rtc: setting system clock to 1970-01-01T00:00:08 UTC (8)
[    1.421713] mmc0: CQHCI version 5.10
[    1.423519] davinci-mcasp 2b10000.mcasp: IRQ common not found
[    1.443353] pca953x 1-0022: supply vcc not found, using dummy regulator
[    1.450079] pca953x 1-0022: using AI
[    1.471796] mmc0: SDHCI controller on fa10000.mmc [fa10000.mmc] using ADMA 64-bit
[    1.482609] sii902x 1-003b: supply iovcc not found, using dummy regulator
[    1.489565] sii902x 1-003b: supply cvcc12 not found, using dummy regulator
[    1.499421] i2c i2c-1: Added multiplexed i2c bus 2
[    1.506298] [drm] Initialized tidss 1.0.0 20180215 for 30200000.dss on minor 0
[    1.514046] tidss 30200000.dss: [drm] Cannot find any crtc or sizes
[    1.527493] vdd_mmc1: supplied by vcc_3v3_sys
[    1.533749] wlan_lten: supplied by vcc_3v3_sys
[    1.539907] debugfs: Directory 'pd:53' with parent 'pm_genpd' already present!
[    1.547244] wlan_en: supplied by wlan_lten
[    1.547262] debugfs: Directory 'pd:52' with parent 'pm_genpd' already present!
[    1.547566] mmc1: CQHCI version 5.10
[    1.551571] mmc2: CQHCI version 5.10
[    1.558625] debugfs: Directory 'pd:51' with parent 'pm_genpd' already present!
[    1.573145] mmc0: Command Queue Engine enabled
[    1.573370] debugfs: Directory 'pd:182' with parent 'pm_genpd' already present!
[    1.577612] mmc0: new HS200 MMC card at address 0001
[    1.585778] mmc1: SDHCI controller on fa00000.mmc [fa00000.mmc] using ADMA 64-bit
[    1.590842] mmcblk0: mmc0:0001 S0J56X 14.8 GiB
[    1.601714] mmc2: SDHCI controller on fa20000.mmc [fa20000.mmc] using ADMA 64-bit
[    1.602279] mmcblk0boot0: mmc0:0001 S0J56X partition 1 31.5 MiB
[    1.615796] mmcblk0boot1: mmc0:0001 S0J56X partition 2 31.5 MiB
[    1.622024] mmcblk0rpmb: mmc0:0001 S0J56X partition 3 4.00 MiB, chardev (237:0)
[    1.629813] sdhci-am654 fa20000.mmc: card claims to support voltages below defined range
[    1.632884]  mmcblk0: p1 p2
[    1.644771] ALSA device list:
[    1.647753]   No soundcards found.
[    1.651653] Waiting for root device PARTUUID=af84bedf-02...
[    1.652505] mmc2: new high speed SDIO card at address 0001
[    1.674043] mmc1: new ultra high speed DDR50 SDHC card at address 0001
[    1.681254] mmcblk1: mmc1:0001 00000 29.8 GiB
[    1.687315]  mmcblk1: p1 p2
[    1.700491] EXT4-fs (mmcblk1p2): mounted filesystem with ordered data mode. Opts: (null)
[    1.708682] VFS: Mounted root (ext4 filesystem) on device 179:98.
[    1.716075] devtmpfs: mounted
[    1.720308] Freeing unused kernel memory: 1792K

```
