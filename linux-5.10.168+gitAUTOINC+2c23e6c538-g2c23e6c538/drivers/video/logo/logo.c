// SPDX-License-Identifier: GPL-2.0-only

/*
 *  Linux logo to be displayed on boot
 *
 *  Copyright (C) 1996 Larry Ewing (lewing@isc.tamu.edu)
 *  Copyright (C) 1996,1998 Jakub Jelinek (jj@sunsite.mff.cuni.cz)
 *  Copyright (C) 2001 Greg Banks <gnb@alphalink.com.au>
 *  Copyright (C) 2001 Jan-Benedict Glaw <jbglaw@lug-owl.de>
 *  Copyright (C) 2003 Geert Uytterhoeven <geert@linux-m68k.org>
 */

#include <linux/linux_logo.h>
#include <linux/stddef.h>
#include <linux/module.h>

#ifdef CONFIG_M68K
#include <asm/setup.h>
#endif


//NOTE::2023-05-10
// Kisan 부팅 로고 출력을 위한 정의 추가 (KISAN_CUSTOM_BOOT_LOGO)
// logo_linux_clut224.ppm 파일을 800*480 기준으로 변경하여 컴파일 후
//  logo_linux_clut224.c 가 변경되었는지 확인
#define KISAN_CUSTOM_BOOT_LOGO

#if defined(KISAN_CUSTOM_BOOT_LOGO)
#include <asm/io.h> // ioremap
#endif	// #if defined(KISAN_CUSTOM_BOOT_LOGO)


static bool nologo;
module_param(nologo, bool, 0);
MODULE_PARM_DESC(nologo, "Disables startup logo");

/*
 * Logos are located in the initdata, and will be freed in kernel_init.
 * Use late_init to mark the logos as freed to prevent any further use.
 */

static bool logos_freed;

static int __init fb_logo_late_init(void)
{
	logos_freed = true;
	return 0;
}

late_initcall_sync(fb_logo_late_init);

/* logo's are marked __initdata. Use __ref to tell
 * modpost that it is intended that this function uses data
 * marked __initdata.
 */
const struct linux_logo * __ref fb_find_logo(int depth)
{
#if defined(KISAN_CUSTOM_BOOT_LOGO)
	struct linux_logo *logo = NULL;
	struct linux_logo kisan_logo;
	
	unsigned char *pucLogo;
	unsigned char *pucClut;
	unsigned char *pucData;
	
	unsigned int unPhyAddr = 0;
	unsigned int unPhyLen = 0;

	memset(&kisan_logo, 0x00, sizeof(struct linux_logo));

	// NOTE::2023-05-10
	// 커널 부팅 인자 (Kernel command line: ... loglevel=8 mem=952M) 중 mem 항목에 따라
	// AM6232 : 1G 기준 설정 리눅스 영역을 952MB (0x8000_0000 ~ 0xbb7f_ffff) 로 설정하여
	// 바로 이어지는 0xbb80_0000 을 Boot Logo File DDR Physical Offset Address 로 설정함
	
	if(pfn_valid(__phys_to_pfn(0xbb800000)) == 0) {
		// 0xbb80_0000 이 할당된 커널 영역에 포함 안되는 경우
		// 4MB (0xbb80_0000 ~ 0xbbbf_ffff) - 1GB DDR / 리눅스 952M 기준 -> 계수기 보드
		unPhyAddr = 0xbb800000;
		unPhyLen = 0x400000;
	}
	else {
		// 0xbb80_0000 이 할당된 커널 영역에 포함 되는 경우
		// 4MB (0xf6c0_0000 ~ 0xf6ff_ffff) - 2GB DDR / 리눅스 1900M 기준 -> 입금기 보드
		unPhyAddr = 0xf6c00000;
		unPhyLen = 0x400000;
	}
	pucLogo = (unsigned char *)ioremap(unPhyAddr, unPhyLen);
	pucClut = (unsigned char *)pucLogo + 16;
	pucData = pucClut + (224 * 3);

	kisan_logo.type = (((pucLogo[3] & 0xff) << 24) |
						((pucLogo[2] & 0xff) << 16) |
						((pucLogo[1] & 0xff) <<  8) |
						(pucLogo[0] & 0xff));

	kisan_logo.width = (((pucLogo[7] & 0xff) << 24) |
						((pucLogo[6] & 0xff) << 16) |
						((pucLogo[5] & 0xff) <<  8) |
						(pucLogo[4] & 0xff));

	kisan_logo.height = (((pucLogo[11] & 0xff) << 24) |
						((pucLogo[10] & 0xff) << 16) |
						((pucLogo[9] & 0xff) <<  8) |
						(pucLogo[8] & 0xff));

	kisan_logo.clutsize = (((pucLogo[15] & 0xff) << 24) |
						((pucLogo[14] & 0xff) << 16) |
						((pucLogo[13] & 0xff) <<  8) |
						(pucLogo[12] & 0xff));
	
	printk(KERN_DEBUG "[%s:%4d:%s] Kisan Boot Logo (%d x %d) @ 0x%x \n",
		__FILE__, __LINE__, __FUNCTION__, kisan_logo.width, kisan_logo.height, unPhyAddr);
#else
	const struct linux_logo *logo = NULL;
#endif	// #if defined(KISAN_CUSTOM_BOOT_LOGO)


	if (nologo || logos_freed)
		return NULL;

	if (depth >= 1) {
#ifdef CONFIG_LOGO_LINUX_MONO
		/* Generic Linux logo */
		logo = &logo_linux_mono;
#endif
#ifdef CONFIG_LOGO_SUPERH_MONO
		/* SuperH Linux logo */
		logo = &logo_superh_mono;
#endif
	}
	
	if (depth >= 4) {
#ifdef CONFIG_LOGO_LINUX_VGA16
		/* Generic Linux logo */
		logo = &logo_linux_vga16;
#endif
#ifdef CONFIG_LOGO_SUPERH_VGA16
		/* SuperH Linux logo */
		logo = &logo_superh_vga16;
#endif
	}
	
	if (depth >= 8) {
#ifdef CONFIG_LOGO_LINUX_CLUT224
		/* Generic Linux logo */
		logo = &logo_linux_clut224;

#if defined(KISAN_CUSTOM_BOOT_LOGO)
		if (logo->type == kisan_logo.type)
//			  && (logo->width == kisan_logo.width)
//			  && (logo->height == kisan_logo.height)
//			  && (logo->clutsize == kisan_logo.clutsize))
		{
			memcpy(&logo->type, &kisan_logo.type, 4);
			memcpy(&logo->width, &kisan_logo.width, 4);
			memcpy(&logo->height, &kisan_logo.height, 4);
			memcpy(&logo->clutsize, &kisan_logo.clutsize, 4);
	
			memcpy((void *)logo->clut, pucClut, (224*3));
			memcpy((void *)logo->data, pucData, (logo->width*logo->height));
		}
		else {
			printk(KERN_ERR "[%s:%4d:%s] Check a Logo File in FAT32 Partition...\n",
				__FILE__, __LINE__, __FUNCTION__);
			printk(KERN_ERR "[%s:%4d:%s]  Logo_type 	: %3d, %3d \n",
				__FILE__, __LINE__, __FUNCTION__, logo->type, kisan_logo.type);
			printk(KERN_ERR "[%s:%4d:%s]  Logo_width	: %3d, %3d \n",
				__FILE__, __LINE__, __FUNCTION__, logo->width, kisan_logo.width);
			printk(KERN_ERR "[%s:%4d:%s]  Logo_height	: %3d, %3d \n",
				__FILE__, __LINE__, __FUNCTION__, logo->height, kisan_logo.height);
			printk(KERN_ERR "[%s:%4d:%s]  Logo_clutsize : %3d, %3d \n",
				__FILE__, __LINE__, __FUNCTION__, logo->clutsize, kisan_logo.clutsize);
		}

		iounmap(pucLogo);
#endif  // #if defined( KISAN_CUSTOM_BOOT_LOGO )
#endif	// #ifdef CONFIG_LOGO_LINUX_CLUT224


#ifdef CONFIG_LOGO_DEC_CLUT224
		/* DEC Linux logo on MIPS/MIPS64 or ALPHA */
		logo = &logo_dec_clut224;
#endif
#ifdef CONFIG_LOGO_MAC_CLUT224
		/* Macintosh Linux logo on m68k */
		if (MACH_IS_MAC)
			logo = &logo_mac_clut224;
#endif
#ifdef CONFIG_LOGO_PARISC_CLUT224
		/* PA-RISC Linux logo */
		logo = &logo_parisc_clut224;
#endif
#ifdef CONFIG_LOGO_SGI_CLUT224
		/* SGI Linux logo on MIPS/MIPS64 */
		logo = &logo_sgi_clut224;
#endif
#ifdef CONFIG_LOGO_SUN_CLUT224
		/* Sun Linux logo */
		logo = &logo_sun_clut224;
#endif
#ifdef CONFIG_LOGO_SUPERH_CLUT224
		/* SuperH Linux logo */
		logo = &logo_superh_clut224;
#endif
	}
	return logo;
}
EXPORT_SYMBOL_GPL(fb_find_logo);
