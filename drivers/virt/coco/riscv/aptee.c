// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2022 by Rivos Inc.

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <asm/sbi.h>

#include <uapi/linux/aptee.h>

#define DRIVER_NAME "aptee"
#define VERSION     "0.1"

#undef pr_fmt
#define pr_fmt(fmt) DRIVER_NAME": " fmt

static long aptee_get_evidence(void __user *argp)
{
	u8 *csr = NULL, *certificate = NULL;
	struct aptee_get_evidence_req req;
	long ret;

	if (copy_from_user(&req, argp, sizeof(req)))
		return -EFAULT;

	if (req.csr_len > MAX_CERTIFICATE_LEN ||
		req.certificate_len > MAX_CERTIFICATE_LEN)
		return -EINVAL;

	csr = kzalloc(req.csr_len, GFP_KERNEL);
	if (!csr) {
		ret = -ENOMEM;
		goto out;
	}

	if (copy_from_user(csr, u64_to_user_ptr(req.csr),
			   req.csr_len)) {
		ret = -EFAULT;
		goto out;
	}

	certificate = kzalloc(req.certificate_len, GFP_KERNEL);
	if (!certificate) {
		ret = -ENOMEM;
		goto out;
	}

	ret = sbi_atst_get_evidence(virt_to_phys(csr), req.csr_len,
				virt_to_phys(req.request_data), 0,
				virt_to_phys(certificate), req.certificate_len);

	if (copy_to_user(u64_to_user_ptr(req.certificate),
				certificate, req.certificate_len))
		ret = -EFAULT;

out:
	kfree(certificate);
	kfree(csr);
	return ret;
}

static long aptee_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	long ret = -EINVAL;

	switch (cmd) {
	case APTEE_GET_EVIDENCE:
		pr_warn("GetEvidence");
		ret = aptee_get_evidence(argp);
		break;
	default:
		pr_debug("cmd %d not supported\n", cmd);
		break;
	}

	return ret;
}

static const struct file_operations aptee_fops = {
	.owner	= THIS_MODULE,
	.unlocked_ioctl = aptee_ioctl,
};

static struct miscdevice aptee_miscdev = {
	.name	= "aptee",
	.fops	= &aptee_fops,
	.minor	= MISC_DYNAMIC_MINOR,
};

static int aptee_probe(struct platform_device *pdev)
{
	pr_warn("APTEE probe");
	return misc_register(&aptee_miscdev);
}

static int aptee_remove(struct platform_device *pdev)
{
	misc_deregister(&aptee_miscdev);

	return 0;
}

static const struct of_device_id aptee_of_match[] = {
	{ .compatible = "riscv,aptee", },
	{},
};
MODULE_DEVICE_TABLE(of, ap_tee_of_match);

static struct platform_driver aptee_driver = {
	.probe = aptee_probe,
	.remove = aptee_remove,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = aptee_of_match,
	},
};

module_platform_driver(aptee_driver);

MODULE_DESCRIPTION("RISC-V AP-TEE guest driver version " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
MODULE_ALIAS("devname:"DRIVER_NAME);
