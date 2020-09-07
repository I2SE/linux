/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Ported to 3.7 by Christoph G. Baumann <cgb@debian.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __FREESCALE_OTP__
#define __FREESCALE_OTP__

#if (defined(CONFIG_SOC_IMX28))

/* OCOTP registers and bits */
#define HW_OCOTP_CTRL		(0x00000000)
#define HW_OCOTP_CTRL_SET	(0x00000004)
#define HW_OCOTP_CTRL_CLR	(0x00000008)
#define HW_OCOTP_CTRL_TOG	(0x0000000c)

#define BM_OCOTP_CTRL_RELOAD_SHADOWS	0x00002000
#define BM_OCOTP_CTRL_RD_BANK_OPEN	0x00001000
#define BM_OCOTP_CTRL_ERROR		0x00000200
#define BM_OCOTP_CTRL_BUSY		0x00000100
#define BP_OCOTP_CTRL_ADDR		0
#define BM_OCOTP_CTRL_ADDR		0x0000003F
#define BP_OCOTP_CTRL_WR_UNLOCK		16
#define BM_OCOTP_CTRL_WR_UNLOCK		0xFFFF0000

#define HW_OCOTP_CUST_N(n)	(0x00000020 + (n) * 0x10)

/* Building up eight registers's names of a bank */
#define MXS_OTP_BANK(a, b, c, d, e, f, g, h)	\
	{\
	("HW_OCOTP_"#a), ("HW_OCOTP_"#b), ("HW_OCOTP_"#c), ("HW_OCOTP_"#d), \
	("HW_OCOTP_"#e), ("HW_OCOTP_"#f), ("HW_OCOTP_"#g), ("HW_OCOTP_"#h) \
	}

#define MXS_OTP_BANKS		(5)
#define MXS_OTP_BANK_ITEMS	(8)
static const char *bank_reg_desc[MXS_OTP_BANKS][MXS_OTP_BANK_ITEMS] = {
MXS_OTP_BANK(CUST0, CUST1, CUST2, CUST3, CRYPTO0, CRYPTO1, CRYPTO2, CRYPTO3),
MXS_OTP_BANK(HWCAP0, HWCAP1, HWCAP2, HWCAP3, HWCAP4, HWCAP5, SWCAP, CUSTCAP),
MXS_OTP_BANK(LOCK, OPS0, OPS1, OPS2, OPS3, UN0, UN1, UN2),
MXS_OTP_BANK(ROM0, ROM1, ROM2, ROM3, ROM4, ROM5, ROM6, ROM7),
MXS_OTP_BANK(SRK0, SRK1, SRK2, SRK3, SRK4, SRK5, SRK6, SRK7),
};

struct fsl_otp_data {
	char		**fuse_name;
	char		*regulator_name;
	unsigned int	fuse_num;
};

#endif /* defined(CONFIG_SOC_IMX28) */
#endif
