/*
 * Copyright (C) ST-Ericsson AB 2009
 * Author:	Sjur Brendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/string.h>
#include <linux/skbuff.h>
#include <linux/hardirq.h>
#include <net/caif/generic/cfglue.h>
#include <net/caif/generic/cfpkt.h>

#define PKT_PREFIX CAIF_NEEDED_HEADROOM
#define PKT_POSTFIX CAIF_NEEDED_TAILROOM
#define PKT_LEN_WHEN_EXTENDING 128
#define PKT_ERROR(pkt, errmsg) do {	   \
    cfpkt_priv(pkt)->erronous = true;	   \
    skb_reset_tail_pointer(&pkt->skb);	   \
    pr_warning("CAIF: " errmsg);\
  } while (0)

struct cfpktq {
	struct sk_buff_head head;
	cfglu_atomic_t count;
	spinlock_t lock;
};

/*
 * net/caif/generic/ is generic and does not
 * understand SKB, so we do this typecast
 */
struct cfpkt {
	struct sk_buff skb;
};

/* Private data inside SKB */
struct cfpkt_priv_data {
	struct dev_info dev_info;
	bool erronous;
};

inline struct cfpkt_priv_data *cfpkt_priv(struct cfpkt *pkt)
{
	return (struct cfpkt_priv_data *) pkt->skb.cb;
}

inline bool is_erronous(struct cfpkt *pkt)
{
	return cfpkt_priv(pkt)->erronous;
}

inline struct sk_buff *pkt_to_skb(struct cfpkt *pkt)
{
	return &pkt->skb;
}

inline struct cfpkt *skb_to_pkt(struct sk_buff *skb)
{
	return (struct cfpkt *) skb;
}

cfglu_atomic_t cfpkt_packet_count;
EXPORT_SYMBOL(cfpkt_packet_count);

struct cfpkt *cfpkt_fromnative(enum caif_direction dir, void *nativepkt)
{
	struct cfpkt *pkt = skb_to_pkt(nativepkt);
	cfpkt_priv(pkt)->erronous = false;
	cfglu_atomic_inc(cfpkt_packet_count);
	return pkt;
}
EXPORT_SYMBOL(cfpkt_fromnative);

void *cfpkt_tonative(struct cfpkt *pkt)
{
	return (void *) pkt;
}
EXPORT_SYMBOL(cfpkt_tonative);

struct cfpkt *cfpkt_create_pfx(uint16 len, uint16 pfx)
{
	struct sk_buff *skb;

	if (likely(in_interrupt()))
		skb = alloc_skb(len + pfx, GFP_ATOMIC);
	else
		skb = alloc_skb(len + pfx, GFP_KERNEL);

	if (unlikely(skb == NULL))
		return NULL;

	skb_reserve(skb, pfx);
	cfglu_atomic_inc(cfpkt_packet_count);
	return skb_to_pkt(skb);
}

inline struct cfpkt *cfpkt_create(uint16 len)
{
	return cfpkt_create_pfx(len + PKT_POSTFIX, PKT_PREFIX);
}
EXPORT_SYMBOL(cfpkt_create);

void cfpkt_destroy(struct cfpkt *pkt)
{
	struct sk_buff *skb = pkt_to_skb(pkt);
	cfglu_atomic_dec(cfpkt_packet_count);
	caif_assert(cfglu_atomic_read(cfpkt_packet_count) >= 0);
	kfree_skb(skb);
}
EXPORT_SYMBOL(cfpkt_destroy);

inline bool cfpkt_more(struct cfpkt *pkt)
{
	struct sk_buff *skb = pkt_to_skb(pkt);
	return skb->len > 0;
}
EXPORT_SYMBOL(cfpkt_more);

int cfpkt_peek_head(struct cfpkt *pkt, void *data, uint16 len)
{
	struct sk_buff *skb = pkt_to_skb(pkt);
	if (skb->tail - skb->data >= len) {
		memcpy(data, skb->data, len);
		return CFGLU_EOK;
	}
	return !cfpkt_extr_head(pkt, data, len) &&
	    !cfpkt_add_head(pkt, data, len);
}
EXPORT_SYMBOL(cfpkt_peek_head);

int cfpkt_extr_head(struct cfpkt *pkt, void *data, uint16 len)
{
	struct sk_buff *skb = pkt_to_skb(pkt);
	uint8 *from;
	if (unlikely(is_erronous(pkt)))
		return CFGLU_EPKT;

	if (unlikely(len > skb->len)) {
		PKT_ERROR(pkt, "cfpkt_extr_head read beyond end of packet\n");
		return CFGLU_EPKT;
	}

	if (unlikely(len > skb_headlen(skb))) {
		if (unlikely(skb_linearize(skb) != 0)) {
			PKT_ERROR(pkt, "cfpkt_extr_head linearize failed\n");
			return CFGLU_EPKT;
		}
	}
	from = skb_pull(skb, len);
	from -= len;
	memcpy(data, from, len);
	return CFGLU_EOK;
}
EXPORT_SYMBOL(cfpkt_extr_head);

int cfpkt_extr_trail(struct cfpkt *pkt, void *dta, uint16 len)
{
	struct sk_buff *skb = pkt_to_skb(pkt);
	uint8 *data = dta;
	uint8 *from;
	if (unlikely(is_erronous(pkt)))
		return CFGLU_EPKT;

	if (unlikely(skb_linearize(skb) != 0)) {
		PKT_ERROR(pkt, "cfpkt_extr_trail linearize failed\n");
		return CFGLU_EPKT;
	}
	if (unlikely(skb->data + len > skb_tail_pointer(skb))) {
		PKT_ERROR(pkt, "cfpkt_extr_trail read beyond end of packet\n");
		return CFGLU_EPKT;
	}
	from = skb_tail_pointer(skb) - len;
	skb_trim(skb, skb->len - len);
	memcpy(data, from, len);
	return CFGLU_EOK;
}
EXPORT_SYMBOL(cfpkt_extr_trail);

int cfpkt_pad_trail(struct cfpkt *pkt, uint16 len)
{
	return cfpkt_add_body(pkt, NULL, len);
}
EXPORT_SYMBOL(cfpkt_pad_trail);

int cfpkt_add_body(struct cfpkt *pkt, const void *data, uint16 len)
{
	struct sk_buff *skb = pkt_to_skb(pkt);
	struct sk_buff *lastskb;
	uint8 *to;
	uint16 addlen = 0;


	if (unlikely(is_erronous(pkt)))
		return CFGLU_EPKT;

	lastskb = skb;

	/* Check whether we need to add space at the tail */
	if (unlikely(skb_tailroom(skb) < len)) {
		if (likely(len < PKT_LEN_WHEN_EXTENDING))
			addlen = PKT_LEN_WHEN_EXTENDING;
		else
			addlen = len;
	}

	/* Check whether we need to change the SKB before writing to the tail */
	if (unlikely((addlen > 0) || skb_cloned(skb) || skb_shared(skb))) {

		/* Make sure data is writable */
		if (unlikely(skb_cow_data(skb, addlen, &lastskb) < 0)) {
			PKT_ERROR(pkt, "cfpkt_add_body: cow failed\n");
			return CFGLU_EPKT;
		}
		/*
		 * Is the SKB non-linear after skb_cow_data()? If so, we are
		 * going to add data to the last SKB, so we need to adjust
		 * lengths of the top SKB.
		 */
		if (lastskb != skb) {
			pr_warning("CAIF: %s(): Packet is non-linear\n",
				   __func__);
			skb->len += len;
			skb->data_len += len;
		}
	}

	/* All set to put the last SKB and optionally write data there. */
	to = skb_put(lastskb, len);
	if (likely(data))
		memcpy(to, data, len);
	return CFGLU_EOK;
}
EXPORT_SYMBOL(cfpkt_add_body);

inline int cfpkt_addbdy(struct cfpkt *pkt, uint8 data)
{
	return cfpkt_add_body(pkt, &data, 1);
}
EXPORT_SYMBOL(cfpkt_addbdy);

int cfpkt_add_head(struct cfpkt *pkt, const void *data2, uint16 len)
{
	struct sk_buff *skb = pkt_to_skb(pkt);
	struct sk_buff *lastskb;
	uint8 *to;
	const uint8 *data = data2;
	if (unlikely(is_erronous(pkt)))
		return CFGLU_EPKT;
	if (unlikely(skb_headroom(skb) < len)) {
		PKT_ERROR(pkt, "cfpkt_add_head: no headroom\n");
		return CFGLU_EPKT;
	}

	/* Make sure data is writable */
	if (unlikely(skb_cow_data(skb, 0, &lastskb) < 0)) {
		PKT_ERROR(pkt, "cfpkt_add_head: cow failed\n");
		return CFGLU_EPKT;
	}

	to = skb_push(skb, len);
	memcpy(to, data, len);
	return CFGLU_EOK;
}
EXPORT_SYMBOL(cfpkt_add_head);

inline int cfpkt_add_trail(struct cfpkt *pkt, const void *data, uint16 len)
{
	return cfpkt_add_body(pkt, data, len);
}
EXPORT_SYMBOL(cfpkt_add_trail);

inline uint16 cfpkt_getlen(struct cfpkt *pkt)
{
	struct sk_buff *skb = pkt_to_skb(pkt);
	return skb->len;
}
EXPORT_SYMBOL(cfpkt_getlen);

inline uint16 cfpkt_iterate(struct cfpkt *pkt,
			    uint16 (*iter_func)(uint16, void *, uint16),
			    uint16 data)
{
	/*
	 * Don't care about the performance hit of linearizing,
	 * Checksum should not be used on high-speed interfaces anyway.
	 */
	if (unlikely(is_erronous(pkt)))
		return CFGLU_EPKT;
	if (unlikely(skb_linearize(&pkt->skb) != 0)) {
		PKT_ERROR(pkt, "cfpkt_iterate: linearize failed\n");
		return CFGLU_EPKT;
	}
	return iter_func(data, pkt->skb.data, cfpkt_getlen(pkt));
}
EXPORT_SYMBOL(cfpkt_iterate);

int cfpkt_setlen(struct cfpkt *pkt, uint16 len)
{
	struct sk_buff *skb = pkt_to_skb(pkt);


	if (unlikely(is_erronous(pkt)))
		return CFGLU_EPKT;

	if (likely(len <= skb->len)) {
		if (unlikely(skb->data_len))
			___pskb_trim(skb, len);
		else
			skb_trim(skb, len);

			return cfpkt_getlen(pkt);
	}

	/* Need to expand SKB */
	if (unlikely(!cfpkt_pad_trail(pkt, len - skb->len)))
		PKT_ERROR(pkt, "cfpkt_setlen: skb_pad_trail failed\n");

	return cfpkt_getlen(pkt);
}
EXPORT_SYMBOL(cfpkt_setlen);

struct cfpkt *cfpkt_create_uplink(const unsigned char *data, unsigned int len)
{
	struct cfpkt *pkt = cfpkt_create_pfx(len + PKT_POSTFIX, PKT_PREFIX);
	if (unlikely(data != NULL))
		cfpkt_add_body(pkt, data, len);
	return pkt;
}
EXPORT_SYMBOL(cfpkt_create_uplink);


struct cfpkt *cfpkt_append(struct cfpkt *dstpkt,
			     struct cfpkt *addpkt,
			     uint16 expectlen)
{
	struct sk_buff *dst = pkt_to_skb(dstpkt);
	struct sk_buff *add = pkt_to_skb(addpkt);
	uint16 addlen = add->tail - add->data;
	uint16 neededtailspace;
	struct sk_buff *tmp;
	uint16 dstlen;
	uint16 createlen;
	if (unlikely(is_erronous(dstpkt) || is_erronous(addpkt))) {
		cfpkt_destroy(addpkt);
		return dstpkt;
	}
	if (expectlen > addlen)
		neededtailspace = expectlen;
	else
		neededtailspace = addlen;

	if (dst->tail + neededtailspace > dst->end) {
		/* Create a dumplicate of 'dst' with more tail space */
		dstlen = dst->tail - dst->data;
		createlen = dstlen + neededtailspace;
		tmp = pkt_to_skb(
			cfpkt_create(createlen + PKT_PREFIX + PKT_POSTFIX));
		if (!tmp)
			return NULL;
		tmp->tail = tmp->data + dstlen;
		tmp->len = dstlen;
		memcpy(tmp->data, dst->data, dstlen);
		cfpkt_destroy(dstpkt);
		dst = tmp;
	}
	memcpy(dst->tail, add->data, add->tail - add->data);
	cfpkt_destroy(addpkt);
	dst->tail += addlen;
	dst->len += addlen;
	return skb_to_pkt(dst);
}
EXPORT_SYMBOL(cfpkt_append);

struct cfpkt *cfpkt_split(struct cfpkt *pkt, uint16 pos)
{
	struct sk_buff *skb2;
	struct sk_buff *skb = pkt_to_skb(pkt);
	uint8 *split = skb->data + pos;
	uint16 len2nd = skb->tail - split;

	if (unlikely(is_erronous(pkt)))
		return NULL;

	if (skb->data + pos > skb->tail) {
		PKT_ERROR(pkt,
			  "cfpkt_split: trying to split beyond end of packet");
		return NULL;
	}

	/* Create a new packet for the second part of the data */
	skb2 = pkt_to_skb(
		cfpkt_create_pfx(len2nd + PKT_PREFIX + PKT_POSTFIX,
				 PKT_PREFIX));

	if (skb2 == NULL)
		return NULL;

	/* Reduce the length of the original packet */
	skb->tail = split;
	skb->len = pos;

	memcpy(skb2->data, split, len2nd);
	skb2->tail += len2nd;
	skb2->len += len2nd;
	return skb_to_pkt(skb2);
}
EXPORT_SYMBOL(cfpkt_split);


char *cfpkt_log_pkt(struct cfpkt *pkt, char *buf, int buflen)
{
	struct sk_buff *skb = pkt_to_skb(pkt);
	char *p = buf;
	int i;

	/*
	 * Sanity check buffer length, it needs to be at least as large as
	 * the header info: ~=50+ bytes
	 */
	if (buflen < 50)
		return NULL;

	snprintf(buf, buflen, "%s: pkt:%p len:%d(%d+%d) {%d,%d} data: [",
		is_erronous(pkt) ? "ERRONOUS-SKB" :
		 (skb->data_len != 0 ? "COMPLEX-SKB" : "SKB"),
		skb,
		skb->len,
		skb->tail - skb->data,
		skb->data_len,
		skb->data - skb->head, skb->tail - skb->head);
	p = buf + strlen(buf);

	for (i = 0; i < skb->tail - skb->data && i < 300; i++) {
		if (p > buf + buflen - 10) {
			sprintf(p, "...");
			p = buf + strlen(buf);
			break;
		}
		sprintf(p, "%02x,", skb->data[i]);
		p = buf + strlen(buf);
	}
	sprintf(p, "]\n");
	return buf;
}
EXPORT_SYMBOL(cfpkt_log_pkt);

int cfpkt_raw_append(struct cfpkt *pkt, void **buf, unsigned int buflen)
{
	struct sk_buff *skb = pkt_to_skb(pkt);
	struct sk_buff *lastskb;

	caif_assert(buf != NULL);
	if (unlikely(is_erronous(pkt)))
		return CFGLU_EPKT;
	/* Make sure SKB is writable */
	if (unlikely(skb_cow_data(skb, 0, &lastskb) < 0)) {
		PKT_ERROR(pkt, "cfpkt_raw_append: skb_cow_data failed\n");
		return CFGLU_EPKT;
	}

	if (unlikely(skb_linearize(skb) != 0)) {
		PKT_ERROR(pkt, "cfpkt_raw_append: linearize failed\n");
		return CFGLU_EPKT;
	}

	if (unlikely(skb_tailroom(skb) < buflen)) {
		PKT_ERROR(pkt, "cfpkt_raw_append: buffer too short - failed\n");
		return CFGLU_EPKT;
	}

	*buf = skb_put(skb, buflen);
	return 1;
}
EXPORT_SYMBOL(cfpkt_raw_append);

int cfpkt_raw_extract(struct cfpkt *pkt, void **buf, unsigned int buflen)
{
	struct sk_buff *skb = pkt_to_skb(pkt);

	caif_assert(buf != NULL);
	if (unlikely(is_erronous(pkt)))
		return CFGLU_EPKT;

	if (unlikely(buflen > skb->len)) {
		PKT_ERROR(pkt, "cfpkt_raw_extract: buflen too large "
				"- failed\n");
		return CFGLU_EPKT;
	}

	if (unlikely(buflen > skb_headlen(skb))) {
		if (unlikely(skb_linearize(skb) != 0)) {
			PKT_ERROR(pkt, "cfpkt_raw_extract: linearize failed\n");
			return CFGLU_EPKT;
		}
	}

	*buf = skb->data;
	skb_pull(skb, buflen);

	return 1;
}
EXPORT_SYMBOL(cfpkt_raw_extract);

inline bool cfpkt_erroneous(struct cfpkt *pkt)
{
	return cfpkt_priv(pkt)->erronous;
}
EXPORT_SYMBOL(cfpkt_erroneous);

struct cfpkt *cfpkt_create_pkt(enum caif_direction dir,
			  const unsigned char *data, unsigned int len)
{
	struct cfpkt *pkt;
	if (dir == CAIF_DIR_OUT)
		pkt = cfpkt_create_pfx(len + PKT_POSTFIX, PKT_PREFIX);
	else
		pkt = cfpkt_create_pfx(len, 0);
	if (unlikely(!pkt))
		return NULL;
	if (unlikely(data))
		cfpkt_add_body(pkt, data, len);
	cfpkt_priv(pkt)->erronous = false;
	return pkt;
}
EXPORT_SYMBOL(cfpkt_create_pkt);

struct cfpktq *cfpktq_create()
{
	struct cfpktq *q = cfglu_alloc(sizeof(struct cfpktq));
	if (!q)
		return NULL;
	skb_queue_head_init(&q->head);
	cfglu_atomic_set(q->count, 0);
	spin_lock_init(&q->lock);
	return q;
}
EXPORT_SYMBOL(cfpktq_create);

void cfpkt_queue(struct cfpktq *pktq, struct cfpkt *pkt, unsigned short prio)
{
	cfglu_atomic_inc(pktq->count);
	spin_lock(&pktq->lock);
	skb_queue_tail(&pktq->head, pkt_to_skb(pkt));
	spin_unlock(&pktq->lock);

}
EXPORT_SYMBOL(cfpkt_queue);

struct cfpkt *cfpkt_qpeek(struct cfpktq *pktq)
{
	struct cfpkt *tmp;
	spin_lock(&pktq->lock);
	tmp = skb_to_pkt(skb_peek(&pktq->head));
	spin_unlock(&pktq->lock);
	return tmp;
}
EXPORT_SYMBOL(cfpkt_qpeek);

struct cfpkt *cfpkt_dequeue(struct cfpktq *pktq)
{
	struct cfpkt *pkt;
	spin_lock(&pktq->lock);
	pkt = skb_to_pkt(skb_dequeue(&pktq->head));
	if (pkt) {
		cfglu_atomic_dec(pktq->count);
		caif_assert(cfglu_atomic_read(pktq->count) >= 0);
	}
	spin_unlock(&pktq->lock);
	return pkt;
}
EXPORT_SYMBOL(cfpkt_dequeue);

int cfpkt_qcount(struct cfpktq *pktq)
{
	return cfglu_atomic_read(pktq->count);
}
EXPORT_SYMBOL(cfpkt_qcount);

struct cfpkt *cfpkt_clone_release(struct cfpkt *pkt)
{
	struct cfpkt *clone;
	clone  = skb_to_pkt(skb_clone(pkt_to_skb(pkt), GFP_ATOMIC));
	/* Free original packet. */
	cfpkt_destroy(pkt);
	if (!clone)
		return NULL;
	cfglu_atomic_inc(cfpkt_packet_count);
	return clone;
}
EXPORT_SYMBOL(cfpkt_clone_release);

struct payload_info *cfpkt_info(struct cfpkt *pkt)
{
	return (struct payload_info *)&pkt_to_skb(pkt)->cb;
}
EXPORT_SYMBOL(cfpkt_info);
