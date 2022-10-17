/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */
/*
 * Userspace interface for the RISC-V AP-TEE guest driver.
 *
 * Copyright (c) 2022 by Rivos Inc.
 */

#ifndef __UAPI_LINUX_APTEE_H_
#define __UAPI_LINUX_APTEE_H_

#include <linux/types.h>
#include <linux/ioctl.h>

/* Reuse the TEE ioctl type */
#define APTEE_MAGIC 0xA4

/* Largest acceptable certificate and certificate request length */
#define MAX_CERTIFICATE_LEN 4096

struct aptee_get_evidence_req {
	__u8 request_data[64];
	__u64 csr;
	__u32 csr_len;
	__u64 certificate;
	__u32 certificate_len;
};

/* Get AP-TEE Attestation Evidence */
#define APTEE_GET_EVIDENCE_NR 0x0
#define APTEE_GET_EVIDENCE _IOWR(APTEE_MAGIC, APTEE_GET_EVIDENCE_NR, struct aptee_get_evidence_req)

#endif /* __UAPI_LINUX_SEV_GUEST_H_ */
