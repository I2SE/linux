/*
 * Freescale i.MX28 On-Chip OTP driver
 *
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
 * Huang Shijie <b32955 at freescale.com>
 *
 * Ported to 3.x by Christoph G. Baumann <cgb@debian.org>
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
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/fcntl.h>
#include <linux/mutex.h>
#include <linux/clk.h>
#include <linux/of_address.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#include "mx28_otp.h"

static DEFINE_MUTEX(otp_mutex);
static struct kobject *otp_kobj;
static struct attribute **attrs;
static struct kobj_attribute *kattr;
static struct attribute_group attr_group;
static void __iomem *otp_base;

static struct fsl_otp_data otp_data = {
	.fuse_name	= (char **)bank_reg_desc,
	.fuse_num	= MXS_OTP_BANKS * MXS_OTP_BANK_ITEMS,
};

static ssize_t otp_show(struct kobject *kobj, struct kobj_attribute *attr,
		      char *buf)
{
	int timeout = 0x400;
	int index = 0;
	u32 value = 0;

	if (kattr == NULL)
	{
		pr_err("%s: kattr is null\n", __func__);
		return 0;
	}

	index = attr - kattr;

	if ((index < 0) && (index >= otp_data.fuse_num))
	{
		pr_err("%s: index %d is out of range\n",
			__func__, index);
		return 0;
	}

	mutex_lock(&otp_mutex);

	/* try to clear ERROR bit */
	__raw_writel(BM_OCOTP_CTRL_ERROR, otp_base + HW_OCOTP_CTRL_CLR);

	/* check both BUSY and ERROR cleared */
	while ((__raw_readl(otp_base) &
		(BM_OCOTP_CTRL_BUSY | BM_OCOTP_CTRL_ERROR)) && --timeout)
		cpu_relax();

	if (unlikely(!timeout))
		goto error_unlock;

	/* open OCOTP banks for read */
	__raw_writel(BM_OCOTP_CTRL_RD_BANK_OPEN, otp_base + HW_OCOTP_CTRL_SET);

	/* approximately wait 32 hclk cycles */
	udelay(1);

	/* poll BUSY bit becoming cleared */
	timeout = 0x400;
	while ((__raw_readl(otp_base) & BM_OCOTP_CTRL_BUSY) && --timeout)
		cpu_relax();

	if (unlikely(!timeout))
		goto error_unlock;

	value = __raw_readl(otp_base + HW_OCOTP_CUST_N(index));

	/* close banks for power saving */
	__raw_writel(BM_OCOTP_CTRL_RD_BANK_OPEN, otp_base + HW_OCOTP_CTRL_CLR);

	mutex_unlock(&otp_mutex);

	return sprintf(buf, "0x%x\n", value);

error_unlock:
	mutex_unlock(&otp_mutex);
	pr_err("%s: timeout in reading OCOTP\n", __func__);
	return 0;
}

static void free_otp_attr(void)
{
	kfree(attrs);
	attrs = NULL;

	kfree(kattr);
	kattr = NULL;
}

static int alloc_otp_attr(void)
{
	int i;

	/* The last one is NULL, which is used to detect the end */
	attrs = kzalloc((otp_data.fuse_num + 1) * sizeof(attrs[0]),
			GFP_KERNEL);
	kattr = kzalloc(otp_data.fuse_num * sizeof(struct kobj_attribute),
			GFP_KERNEL);

	if (!attrs || !kattr)
		goto error_out;

	for (i = 0; i < otp_data.fuse_num; i++) {
		kattr[i].attr.name = otp_data.fuse_name[i];
		kattr[i].attr.mode = 0400;
		kattr[i].show  = otp_show;
		attrs[i] = &kattr[i].attr;
		sysfs_attr_init(attrs[i]);
	}
	memset(&attr_group, 0, sizeof(attr_group));
	attr_group.attrs = attrs;
	return 0;

error_out:
	free_otp_attr();
	return -ENOMEM;
}

static int fsl_otp_probe(struct platform_device *pdev)
{
	struct fsl_otp_data *pdata;
	int retval;

	/* get device base address */
	if (pdev->dev.of_node) {
		otp_base = of_iomap(pdev->dev.of_node, 0);

		if (!otp_base)
			return -ENXIO;

		dev_info(&pdev->dev, "OCOTP found\n");
	}

	pdata = &otp_data;
	if (pdev->dev.platform_data == NULL)
		pdev->dev.platform_data = &otp_data;

	retval = alloc_otp_attr();
	if (retval)
		return retval;

	otp_kobj = kobject_create_and_add("fsl_otp", NULL);
	if (!otp_kobj) {
		retval = -ENOMEM;
		goto error;
	}

	retval = sysfs_create_group(otp_kobj, &attr_group);
	if (retval)
		goto error;

	mutex_init(&otp_mutex);
	return 0;
error:
	kobject_put(otp_kobj);
	otp_kobj = NULL;
	free_otp_attr();
	dev_err(&pdev->dev, "%s returns %d\n", __func__, retval);
	return retval;
}

static int otp_remove(struct platform_device *pdev)
{
	kobject_put(otp_kobj);
	otp_kobj = NULL;

	free_otp_attr();
	return 0;
}

static const struct of_device_id mxs_otp_dt_ids[] = {
	{ .compatible = "fsl,ocotp", .data = NULL, },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mxs_otp_dt_ids);

static struct platform_driver ocotp_driver = {
	.probe		= fsl_otp_probe,
	.remove		= otp_remove,
	.driver		= {
		.name   = "ocotp",
		.owner	= THIS_MODULE,
		.of_match_table = mxs_otp_dt_ids,
	},
};

module_platform_driver(ocotp_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Christoph G. Baumann <cgb@debian.org>");
MODULE_DESCRIPTION("driver for OCOTP in i.MX28");
