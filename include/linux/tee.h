/*
 * TEE driver for Trust Zone enabled CPUs.
 *
 * Copyright (C) ST-Ericsson SA 2010
 * Author: Shujuan Chen <shujuan.chen@stericsson.com>
 * Author: Martin Hovang <martin.xm.hovang@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef TEE_SERVICE_H
#define TEE_SERVICE_H

/**
 * tee_cmd id values
 */
#define TEED_OPEN_SESSION    0x00000000
#define TEED_CLOSE_SESSION   0x00000001
#define TEED_INVOKE          0x00000002

/**
 * tee_retval id values
 */
#define TEED_SUCCESS                0x00000000
#define TEED_ERROR_GENERIC          0xFFFF0000
#define TEED_ERROR_ACCESS_DENIED    0xFFFF0001
#define TEED_ERROR_CANCEL           0xFFFF0002
#define TEED_ERROR_ACCESS_CONFLICT  0xFFFF0003
#define TEED_ERROR_EXCESS_DATA      0xFFFF0004
#define TEED_ERROR_BAD_FORMAT       0xFFFF0005
#define TEED_ERROR_BAD_PARAMETERS   0xFFFF0006
#define TEED_ERROR_BAD_STATE        0xFFFF0007
#define TEED_ERROR_ITEM_NOT_FOUND   0xFFFF0008
#define TEED_ERROR_NOT_IMPLEMENTED  0xFFFF0009
#define TEED_ERROR_NOT_SUPPORTED    0xFFFF000A
#define TEED_ERROR_NO_DATA          0xFFFF000B
#define TEED_ERROR_OUT_OF_MEMORY    0xFFFF000C
#define TEED_ERROR_BUSY             0xFFFF000D
#define TEED_ERROR_COMMUNICATION    0xFFFF000E
#define TEED_ERROR_SECURITY         0xFFFF000F
#define TEED_ERROR_SHORT_BUFFER     0xFFFF0010

/**
 * TEE origin codes
 */
#define TEED_ORIGIN_DRIVER          0x00000002
#define TEED_ORIGIN_TEE             0x00000003
#define TEED_ORIGIN_TEE_APPLICATION 0x00000004

#define TEE_UUID_CLOCK_SIZE 8

#define TEEC_CONFIG_PAYLOAD_REF_COUNT 4

/* Different naming convention to comply with
   Global Platforms TEE Client API spec */
struct tee_uuid {
	uint32_t timeLow;
	uint16_t timeMid;
	uint16_t timeHiAndVersion;
	uint8_t clockSeqAndNode[TEE_UUID_CLOCK_SIZE];
};

struct tee_sharedmemory {
	void *buffer;
	size_t size;
	uint32_t flags;
};

struct tee_operation {
	struct tee_sharedmemory shm[TEEC_CONFIG_PAYLOAD_REF_COUNT];
	uint32_t flags;
};

struct tee_session {
	uint32_t state;
	uint32_t err;
	uint32_t origin;
	uint32_t id;
	void *ta;
	struct tee_uuid *uuid;
	unsigned int cmd;
	unsigned int ta_size;
	struct tee_operation *idata;
	struct mutex *sync;
};

struct tee_write {
	struct tee_uuid *uuid;
	unsigned int id;
	unsigned int cmd;
	void *ta;
	size_t ta_size;
	void *inData;
	size_t inSize;
};

struct tee_read {
	unsigned int id;	/* return value */
	unsigned int origin;	/* error origin */
};

/**
 * This function handles the function calls to trusted applications.
 */
int call_sec_world(struct tee_session *ts);

#endif
