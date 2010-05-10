/*
 * Copyright (C) ST-Ericsson AB 2010
 * Author:	Sjur Brendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#include <net/caif/cfpkt.h>

/* NOTE: Excluding physical layer, max is 10 bytes. Physical layer is
 * uncertain.
 */
#define PKT_PREFIX 16
#define PKT_POSTFIX 2

/* Minimum size of payload buffer */
#define PKT_MINSIZE 128

#define MAGIC_VALUE 0xbaadf00d
/* Return closest 32bit aligned address (by adding) */
#define ALIGN32(p) (((((u32)p)&0x3) == 0) ?\
		    ((u32) p) :\
		    (((u32)p) + (0x4 - (((u32)p)&0x3))))

/* Do division by 0 on failure - CRASH */
#define CHECK_MAGIC(pkt) \
	caif_assert(((pkt) != NULL && \
			((pkt)->magic1 == MAGIC_VALUE && \
					*(pkt)->magic2 == MAGIC_VALUE)))

/* Memory Layout:
 * Magic1
 * |   Buffer starts at start of page
 * |   |		  Pad until next 32bit aligned offset
 * |   |		  |   Magic2
 * |   |		  |   |	  Packet struct starts on first 32bit
 * |   |		  |   |	  | aligned offset after end of packet
 * V   V		  V   V	  V
 * +---+-----------------+---+---+-----------------+
 * |M2 | packet buffer ..|pad|M1 | struct cfpkt  |
 * +---+-----------------+---+---+-----------------+
 *     ^		 ^
 *     |		 |
 *     |		cfpkt._end_
 *     cfpkt._head_
 */

struct _payload {
	u32 magic2;
	u8 buf[1];
};
struct cfpkt {
	u32 magic1;
	u32 *magic2;		/* This will point to location before _head_ */
	struct caif_payload_info info;
	void *blob;
	struct cfpkt *next;
	const u8 *_head_;	/* Start of buffer, i.e. first legal
				 * pos for _data_
				 */
	u8 *_data_;		/* Start of payload */
	u8 *_tail_;		/* End of payload data */
	u8 *_end_;		/* End of buffer, i.e. last legal pos
				 * for _tail_
				 */
};

#define PKT_ERROR(pkt, errmsg) do {\
	   pkt->_data_ = pkt->_tail_ = pkt->_end_ = (u8 *)pkt->_head_;\
	   pr_err(errmsg); } while (0)

struct cfpktq {
	struct cfpkt *head;
	spinlock_t qlock;
	int count;
};

atomic_t cfpkt_packet_count;
EXPORT_SYMBOL(cfpkt_packet_count);

struct cfpkt *cfpkt_create_pfx(u16 len, u16 pfx)
{
	int pldlen;
	int bloblen;
	struct cfpkt *pkt;
	void *blob;
	struct _payload *pld;
	u8 *pktstruct;
	void *blobend, *structend;
	atomic_inc(&cfpkt_packet_count);

	/* (1) Compute payload length */
	pldlen = len + pfx;
	if (pldlen < PKT_MINSIZE)
		pldlen = PKT_MINSIZE;
	/* Make room for Magic before & after payload */
	pldlen += 2 * sizeof(u32);
	pldlen = ALIGN32(pldlen);

	/* (2) Compute blob length, payload + packet struct */
	bloblen = sizeof(struct _payload) + pldlen + sizeof(struct cfpkt);

	bloblen = ALIGN32(bloblen);

	/* (3) Allocate the blob */
	blob = cfglu_alloc(bloblen);

	blobend = (u8 *) blob + bloblen;

	/* Initialize payload struct */
	pld = (struct _payload *) blob;
	pld->magic2 = MAGIC_VALUE;

	/* Initialize packet struct */
	pktstruct = pld->buf + pldlen;
	pktstruct = (u8 *) ALIGN32(pktstruct);
	structend = pktstruct + sizeof(struct cfpkt);
	memset(pktstruct, 0, sizeof(struct cfpkt));
	caif_assert(structend <= blobend);
	pkt = (struct cfpkt *) pktstruct;
	pkt->blob = blob;
	pkt->_end_ = &pld->buf[pldlen];
	pkt->_head_ = &pld->buf[0];
	pkt->_data_ = (u8 *) pkt->_head_ + pfx;
	pkt->_tail_ = pkt->_data_;

	pkt->magic1 = MAGIC_VALUE;
	pkt->magic2 = (u32 *) &pld->buf[pldlen];
	*pkt->magic2 = MAGIC_VALUE;
	CHECK_MAGIC(pkt);
	return pkt;
}

struct cfpkt *cfpkt_create(u16 len)
{
	return cfpkt_create_pfx(len, PKT_PREFIX);
}

void cfpkt_destroy(struct cfpkt *pkt)
{
	CHECK_MAGIC(pkt);
	atomic_dec(&cfpkt_packet_count);
	caif_assert(atomic_read(&cfpkt_packet_count) >= 0);
	kfree(pkt->blob);
}

bool cfpkt_more(struct cfpkt *pkt)
{
	CHECK_MAGIC(pkt);
	return pkt->_data_ < pkt->_tail_;
}

int cfpkt_extr_head(struct cfpkt *pkt, void *dta, u16 len)
{
	register int i;
	u8 *data = dta;
	caif_assert(data != NULL);
	CHECK_MAGIC(pkt);
	if (pkt->_data_ + len > pkt->_tail_) {
		PKT_ERROR(pkt,
			  "cfpkt_extr_head would read beyond end of packet\n");
		return -EPROTO;
	}
	for (i = 0; i < len; i++) {
		data[i] = *pkt->_data_;
		pkt->_data_++;
	}
	CHECK_MAGIC(pkt);
	return 0;
}

int cfpkt_extr_trail(struct cfpkt *pkt, void *dta, u16 len)
{
	int i;
	u8 *data = dta;
	caif_assert(data != NULL);
	CHECK_MAGIC(pkt);
	if (pkt->_data_ + len > pkt->_tail_) {
		PKT_ERROR(pkt,
			"cfpkt_extr_trail would read beyond start of packet\n");
		return -EPROTO;
	}
	data += len;
	for (i = 0; i < len; i++) {
		data--;
		pkt->_tail_--;
		*data = *pkt->_tail_;
	}
	CHECK_MAGIC(pkt);
	return 0;
}

char *cfpkt_log_pkt(struct cfpkt *pkt, char *buf, int buflen)
{
	char *p = buf;
	int i;
	sprintf(buf, " pkt:%p len:%d {%d,%d} data: [",
		(void *) pkt,
		pkt->_tail_ - pkt->_data_,
		pkt->_data_ - pkt->_head_, pkt->_tail_ - pkt->_head_);
	p = buf + strlen(buf);

	for (i = 0; i < pkt->_tail_ - pkt->_data_; i++) {
		if (p > buf + buflen - 10) {
			sprintf(p, "...");
			p = buf + strlen(buf);
			break;
		}
		sprintf(p, "%02x,", pkt->_data_[i]);
		p = buf + strlen(buf);
	}
	sprintf(p, "]");
	return buf;
}

int cfpkt_add_body(struct cfpkt *pkt, const void *dta, u16 len)
{
	register int i;
	const u8 *data = dta;
	caif_assert(data != NULL);
	CHECK_MAGIC(pkt);
	if (pkt->_tail_ + len > pkt->_end_) {
		PKT_ERROR(pkt,
			  "cfpkt_add_body would write beyond end of packet\n");
		return -EPROTO;
	}

	for (i = 0; i < len; i++) {
		*pkt->_tail_ = data[i];
		pkt->_tail_++;
	}
	CHECK_MAGIC(pkt);
	return 0;
}

int cfpkt_addbdy(struct cfpkt *pkt, u8 data)
{
	CHECK_MAGIC(pkt);
	return cfpkt_add_body(pkt, &data, 1);
}

int cfpkt_add_head(struct cfpkt *pkt, const void *dta, u16 len)
{
	register int i;
	const u8 *data = dta;
	caif_assert(data != NULL);
	CHECK_MAGIC(pkt);
	if (pkt->_data_ - len < pkt->_head_) {
		PKT_ERROR(pkt, "cfpkt_add_head: write beyond start of packet\n");
		return -EPROTO;
	}
	for (i = len - 1; i >= 0; i--) {
		--pkt->_data_;
		*pkt->_data_ = data[i];
	}
	CHECK_MAGIC(pkt);
	return 0;
}

int cfpkt_add_trail(struct cfpkt *pkt, const void *data, u16 len)
{
	CHECK_MAGIC(pkt);
	caif_assert(data != NULL);
	return cfpkt_add_body(pkt, data, len);

}

u16 cfpkt_iterate(struct cfpkt *pkt,
		u16 (*func)(u16 chks, void *buf, u16 len),
		u16 data)
{
	return func(data, pkt->_data_, cfpkt_getlen(pkt));
}

int cfpkt_setlen(struct cfpkt *pkt, u16 len)
{
	CHECK_MAGIC(pkt);
	if (pkt->_data_ + len > pkt->_end_) {
		PKT_ERROR(pkt, "cfpkt_setlen: Erroneous packet\n");
		return -EPROTO;
	}
	pkt->_tail_ = pkt->_data_ + len;
	return cfpkt_getlen(pkt);
}

u16 cfpkt_getlen(struct cfpkt *pkt)
{
	CHECK_MAGIC(pkt);
	return pkt->_tail_ - pkt->_data_;
}

void cfpkt_extract(struct cfpkt *cfpkt, void *buf, unsigned int buflen,
		   unsigned int *actual_len)
{
	u16 pklen = cfpkt_getlen(cfpkt);
	caif_assert(buf != NULL);
	caif_assert(actual_len != NULL);
	CHECK_MAGIC(cfpkt);
	if (buflen < pklen)
		pklen = buflen;
	*actual_len = pklen;
	cfpkt_extr_head(cfpkt, buf, pklen);
	CHECK_MAGIC(cfpkt);
}

struct cfpkt *cfpkt_create_uplink(const unsigned char *data, unsigned int len)
{
	struct cfpkt *pkt = cfpkt_create_pfx(len, PKT_PREFIX);
	if (data != NULL)
		cfpkt_add_body(pkt, data, len);
	CHECK_MAGIC(pkt);
	return pkt;
}

struct cfpkt *cfpkt_append(struct cfpkt *dstpkt, struct cfpkt *addpkt,
				u16 expectlen)
{
	u16 addpktlen = addpkt->_tail_ - addpkt->_data_;
	u16 neededtailspace;
	CHECK_MAGIC(dstpkt);
	CHECK_MAGIC(addpkt);
	if (expectlen > addpktlen)
		neededtailspace = expectlen;
	else
		neededtailspace = addpktlen;
	if (dstpkt->_tail_ + neededtailspace > dstpkt->_end_) {
		struct cfpkt *tmppkt;
		u16 dstlen;
		u16 createlen;
		dstlen = dstpkt->_tail_ - dstpkt->_data_;
		createlen = dstlen + addpktlen;
		if (expectlen > createlen)
			createlen = expectlen;
		tmppkt = cfpkt_create(createlen + PKT_PREFIX + PKT_POSTFIX);
		tmppkt->_tail_ = tmppkt->_data_ + dstlen;
		memcpy(tmppkt->_data_, dstpkt->_data_, dstlen);
		cfpkt_destroy(dstpkt);
		dstpkt = tmppkt;
	}
	memcpy(dstpkt->_tail_, addpkt->_data_,
	       addpkt->_tail_ - addpkt->_data_);
	cfpkt_destroy(addpkt);
	dstpkt->_tail_ += addpktlen;
	CHECK_MAGIC(dstpkt);
	return dstpkt;
}

struct cfpkt *cfpkt_split(struct cfpkt *pkt, u16 pos)
{
	struct cfpkt *half;		/* FIXME: Rename half to pkt2 */
	u8 *split = pkt->_data_ + pos;
	u16 len2nd = pkt->_tail_ - split;


	CHECK_MAGIC(pkt);
	if (pkt->_data_ + pos > pkt->_tail_) {
		PKT_ERROR(pkt,
			  "cfpkt_split: trying to split beyond end of packet\n");
		return NULL;
	}

	/* Create a new packet for the second part of the data */
	half = cfpkt_create_pfx(len2nd + PKT_PREFIX + PKT_POSTFIX, PKT_PREFIX);


	if (half == NULL)
		return NULL;

	/* Reduce the length of the original packet */
	pkt->_tail_ = split;

	memcpy(half->_data_, split, len2nd);
	half->_tail_ += len2nd;
	CHECK_MAGIC(pkt);
	return half;
}

bool cfpkt_erroneous(struct cfpkt *pkt)
{
	/* Errors are marked by setting _end_ equal to _head_ (zero sized
	 *  packet)
	 */
	return pkt->_end_ == (u8 *) pkt->_head_;
}

int cfpkt_pad_trail(struct cfpkt *pkt, u16 len)
{
	CHECK_MAGIC(pkt);
	if (pkt->_tail_ + len > pkt->_end_) {
		PKT_ERROR(pkt, "cfpkt_pad_trail pads beyond end of packet\n");
		return -EPROTO;
	}
#if 1
	/* We're assuming that the modem doesn't require zero-padding */
	pkt->_tail_ += len;
#else
	while (len--)
		*pkt->_tail_++ = 0;
#endif

	CHECK_MAGIC(pkt);
	return 0;
}

int cfpkt_peek_head(struct cfpkt *const pkt, void *dta, u16 len)
{
	register int i;
	u8 *data = (u8 *) dta;
	CHECK_MAGIC(pkt);
	if (pkt->_data_ + len > pkt->_tail_) {
		PKT_ERROR(pkt,
			  "cfpkt_peek_head would read beyond end of packet\n");
		return -EPROTO;
	}
	for (i = 0; i < len; i++)
		data[i] = pkt->_data_[i];
	CHECK_MAGIC(pkt);
	return 0;

}

int cfpkt_raw_append(struct cfpkt *cfpkt, void **buf, unsigned int buflen)
{
	caif_assert(buf != NULL);
	if (cfpkt->_tail_ + buflen > cfpkt->_end_) {
		PKT_ERROR(cfpkt,
			  "cfpkt_raw_append would append beyond end of packet\n");
		return -EPROTO;
	}

	*buf = cfpkt->_tail_;
	cfpkt->_tail_ += buflen;
	return 0;
}

int cfpkt_raw_extract(struct cfpkt *cfpkt, void **buf, unsigned int buflen)
{
	caif_assert(buf != NULL);
	if (cfpkt->_data_ + buflen > cfpkt->_tail_) {
		PKT_ERROR(cfpkt,
			  "cfpkt_raw_extact would read beyond end of packet\n");
		return -EPROTO;
	}

	*buf = cfpkt->_data_;
	cfpkt->_data_ += buflen;
	return 0;
}

struct cfpktq *cfpktq_create(void)
{
	struct cfpktq *q = (struct cfpktq *) cfglu_alloc(sizeof(struct cfpktq));
	spin_lock_init(&q->qlock);
	q->head = NULL;
	q->count = 0;
	return q;
}

void cfpkt_queue(struct cfpktq *pktq, struct cfpkt *pkt, unsigned short prio)
{
	CHECK_MAGIC(pkt);
	spin_lock(&pktq->qlock);
	pkt->next = NULL;
	if (pktq->head == NULL)
		pktq->head = pkt;
	else {
		/* NOTE: Consider having a tail pointer in order to improve
		 * performance
		 */
		struct cfpkt *p = pktq->head;
		while (p->next != NULL) {
			CHECK_MAGIC(p);
			p = p->next;
		}
		p->next = pkt;
	}
	pktq->count++;
	spin_unlock(&pktq->qlock);
}

struct cfpkt *cfpkt_qpeek(struct cfpktq *pktq)
{
	struct cfpkt *pkt;

	spin_lock(&pktq->qlock);
	if (pktq->head != NULL) {
		/* NOTE: Sync is only needed due to this CHECK_MAGIC... */
		CHECK_MAGIC(pktq->head);
	}
	pkt = pktq->head;
	spin_unlock(&pktq->qlock);
	return pkt;
}

struct cfpkt *cfpkt_dequeue(struct cfpktq *pktq)
{
	struct cfpkt *ret;
	spin_lock(&pktq->qlock);
	if (pktq->head == NULL) {
		spin_unlock(&pktq->qlock);
		return NULL;
	}
	ret = pktq->head;
	pktq->head = pktq->head->next;
	CHECK_MAGIC(ret);
	pktq->count--;
	caif_assert(pktq->count >= 0);
	spin_unlock(&pktq->qlock);
	return ret;
}

int cfpkt_qcount(struct cfpktq *pktq)
{
	int count;

	spin_lock(&pktq->qlock);
	count = pktq->count;
	spin_unlock(&pktq->qlock);

	return count;
}

inline struct cfpkt *cfpkt_clone_release(struct cfpkt *pkt)
{
	/* Plain packets have no ownership. */
	return pkt;
}
#if 0
struct caif_packet_funcs cfpkt_get_packet_funcs()
{
	struct caif_packet_funcs f;
	memset(&f, 0, sizeof(f));
	f.cfpkt_destroy = cfpkt_destroy;
	f.cfpkt_extract = cfpkt_extract;
	f.cfpkt_create_xmit_pkt = cfpkt_create_uplink;	/* FIXME: Decide upon
							   which create
							   functions to export
							 */
	f.cfpkt_create_recv_pkt = cfpkt_create_uplink;
	f.cfpkt_raw_append = cfpkt_raw_append;
	f.cfpkt_raw_extract = cfpkt_raw_extract;
	f.cfpktq_create = cfpktq_create;
	f.cfpkt_queue = cfpkt_queue;
	f.cfpkt_qpeek = cfpkt_qpeek;
	f.cfpkt_dequeue = cfpkt_dequeue;
	f.cfpkt_getlen = cfpkt_getlen;
	return f;
}
#endif
struct caif_payload_info *cfpkt_info(struct cfpkt *pkt)
{
	return &pkt->info;
}
