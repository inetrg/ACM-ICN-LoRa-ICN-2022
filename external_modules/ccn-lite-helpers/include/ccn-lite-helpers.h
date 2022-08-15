/*
 * Copyright (C) 2022 HAW Hamburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup
 * @ingroup
 * @brief
 * @{
 *
 * @file
 * @brief
 *
 * @author  Peter Kietzmann <peter.kietzmann@haw-hamburg.de>
 */
#ifndef CCN_LITE_HELPERS_H
#define CCN_LITE_HELPERS_H

#include <stdio.h>

#include "msg.h"
#include "shell.h"
#include "random.h"
#include "ccn-lite-riot.h"
#include "ccnl-callbacks.h"
#include "ccnl-producer.h"
#include "ccnl-pkt-builder.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/pktdump.h"

#ifdef MODULE_OPENDSME
#include "opendsme/opendsme.h"
gnrc_netif_t *opendsme_get_netif(void);
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ADDR_LEN            (GNRC_NETIF_L2ADDR_MAXLEN)

#ifndef REGPFX
#define REGPFX      "reg"
#endif

#ifndef ACK
#define ACK         "ACK"
#endif

#ifndef NACK
#define NACK        "NCK"
#endif

#ifndef RFX_NAME_PFX
#define RFX_NAME_PFX         "RNP"
#endif

#ifndef WAIT
#define WAIT        "WAIT"
#endif

#ifndef RVS_INT_PFX
#define RVS_INT_PFX         "RPT"
#endif

#ifdef MODULE_OPENDSME
#define PWRTWO(EXP) (1 << (EXP))
#define SF_IN_MSF (CONFIG_DSME_MAC_MULTISUPERFRAME_ORDER - CONFIG_DSME_MAC_SUPERFRAME_ORDER)
#define SF_DURATION (8) // actually 7.68, but we are conservative
#define WAITTIME PWRTWO(SF_IN_MSF) * SF_DURATION
#else
#define WAITTIME    (20)
#endif

enum {
    ADD_CONTENT,
    ADD_PIT,
    CS_TIMEOUT,
    PIT_TIMEOUT,
};


int gw_on_propagate_cb(struct ccnl_relay_s *relay,
                       struct ccnl_face_s *from,
                       struct ccnl_interest_s *ccnl_int);

int rfx_on_data_cb(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   struct ccnl_pkt_s *pkt);

int rfx_on_interest_cb(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   struct ccnl_pkt_s *pkt);

int local_producer_on_interest_cb(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   struct ccnl_pkt_s *pkt);

int register_on_data_cb(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   struct ccnl_pkt_s *pkt);

int on_data_cb(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   struct ccnl_pkt_s *pkt);

int filter_ack_nack_wait_on_data_cb(struct ccnl_relay_s *relay, struct ccnl_content_s *c);

int cache_strategy_static_cb(struct ccnl_relay_s *relay, struct ccnl_content_s *c);

int _on_retrans_cb(struct ccnl_interest_s *ccnl_int);

int register_node(char *regpfx, char *testid, char *addr_str);

int send_data_push(char *name, char *data);

void print_l3info_l2(uint8_t *payload, unsigned len, bool direction);

void print_l3info(int type, struct ccnl_prefix_s *pfx);

#ifdef __cplusplus
}
#endif
#endif /* CCN_LITE_RIOT_H */
/** @} */
