/*
 * Copyright (C) 2022 HAW Hamburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup
 * @{
 *
 * @file
 * @brief
 * @note
 *
 * @author      Peter Kietzmann <peter.kietzmann@haw-hamburg.de>
 *
 * @}
 */

#include "ccn-lite-helpers.h"
extern void heap_stats(void);


extern uint8_t i_am_sinkfx;
extern char my_id[];

static unsigned char _buf[64];

// global struct to hold gateway 'from' face on nodes. set on registration ACK.
struct ccnl_face_s *from_gw_global = NULL;

// global struct to hold 'previous hop' face on rfx sink. set on incoming interest indication.
struct ccnl_face_s *from_fwd_global = NULL;

static inline void _print_pfx_comps(struct ccnl_prefix_s *pfx)
{
    printf("pfx->compcnt: %"PRIu32"\n", pfx->compcnt);
    for (unsigned i=0;i<pfx->compcnt;i++) {
        printf("%.*s \n", pfx->complen[i], pfx->comp[i]);
    }
}


static struct ccnl_face_s *_intern_face_get(char *addr_str)
{
    /* initialize address with 0xFF for broadcast */
    uint8_t relay_addr[MAX_ADDR_LEN];
    memset(relay_addr, UINT8_MAX, MAX_ADDR_LEN);
    size_t addr_len = gnrc_netif_addr_from_str(addr_str, relay_addr);

    if (addr_len == 0) {
        printf("Error: %s is not a valid link layer address\n", addr_str);
        return NULL;
    }

    sockunion sun;
    sun.sa.sa_family = AF_PACKET;
    memcpy(&(sun.linklayer.sll_addr), relay_addr, addr_len);
    sun.linklayer.sll_halen = addr_len;
    sun.linklayer.sll_protocol = htons(ETHERTYPE_NDN);

    /* TODO: set correct interface instead of always 0 */
    struct ccnl_face_s *fibface = ccnl_get_face_or_create(&ccnl_relay, 0, &sun.sa, sizeof(sun.linklayer));

    return fibface;
}

static int _intern_fib_add(char *pfx, char *addr_str)
{
    int suite = CCNL_SUITE_NDNTLV;
    struct ccnl_prefix_s *prefix = ccnl_URItoPrefix(pfx, suite, NULL);
    if (!prefix) {
        puts("Error: prefix could not be created!");
        return -1;
    }

    struct ccnl_face_s *fibface = _intern_face_get(addr_str);
    if (fibface == NULL) {
        return -1;
    }
    fibface->flags |= CCNL_FACE_FLAGS_STATIC;

    if (ccnl_fib_add_entry(&ccnl_relay, prefix, fibface) != 0) {
        printf("Error adding (%s : %s) to the FIB\n", pfx, addr_str);
        return -1;
    }

    return 0;
}

// ret 0   -> aborts handle interest function (ccnl_fwd_handleInterest)
// ret > 0 -> continues handle interest function (ccnl_fwd_handleInterest)
int register_on_data_cb(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   struct ccnl_pkt_s *pkt)
{
    (void)relay;
    (void)from;
    (void)pkt;
    puts("GOT DATA");

    if(pkt->pfx->compcnt == 2) {
        if ( !memcmp(pkt->pfx->comp[0], REGPFX, pkt->pfx->complen[0]) ) {
            // additionally check own node ID here?
            if(!from_gw_global) {
                if (!memcmp(pkt->content, ACK, pkt->contlen)) {
                    from_gw_global = (struct ccnl_face_s *) ccnl_calloc(1, sizeof(struct ccnl_face_s));
                    memcpy(from_gw_global, from, sizeof(struct ccnl_face_s));
                    printf("added GW: %s\n", ccnl_addr2ascii(&from_gw_global->peer));
                }
            }
            else {
                printf("from_gw_global already set: %s\n",ccnl_addr2ascii(&from_gw_global->peer));
                return 1;
            }
        }
    }
    return 0;
}

int on_data_cb(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   struct ccnl_pkt_s *pkt)
{
    (void)relay;
    (void)from;
    (void)pkt;

    puts("GW GOT DATA");

    // case to remove fib on reflexive data
    if (!memcmp(pkt->pfx->comp[0], RVS_INT_PFX, pkt->pfx->complen[0])) {
        printf("DATA WITH %s PREFIX\n", RVS_INT_PFX);
        // check if this prefix is in FIB, if yes, cache
        struct ccnl_forward_s *fwd;
        for (fwd = ccnl_relay.fib; fwd; fwd = fwd->next) {
            if (!fwd->prefix) {
                continue;
            }
            int rc = ccnl_prefix_cmp(fwd->prefix, NULL, pkt->pfx, CMP_LONGEST);
            if(rc > 0) {
                puts("PFX in FIB, remove...");

                int res = ccnl_fib_rem_entry(&ccnl_relay, fwd->prefix, NULL);

                if (res < 0) {
                    puts("ERROR removing FIB entry");
                }
                else {
                    puts("removed FIB entry");
                }
            }
        }
    }
    else if (!memcmp(pkt->pfx->comp[0], RFX_NAME_PFX, pkt->pfx->complen[0])) {
        // this case is just to prevent the "else" case from looping
        printf("DATA WITH %s PREFIX\n", RFX_NAME_PFX);
    }
    // case to allow lora data push if registered
    else
    {
        // check if this prefix is in FIB, if yes, cache
        struct ccnl_forward_s *fwd;
        for (fwd = ccnl_relay.fib; fwd; fwd = fwd->next) {
            if (!fwd->prefix) {
                continue;
            }
            int rc = ccnl_prefix_cmp(fwd->prefix, NULL, pkt->pfx, CMP_LONGEST);
            if(rc > 0) {
                puts("PLACE DATA PUSH IN CACHE");

                // add the RPT (reverse) prefix before adding to cache
                snprintf((char *)&_buf, sizeof(_buf),
                    "/%s/%.*s/%.*s", RVS_INT_PFX, pkt->pfx->complen[0], pkt->pfx->comp[0], pkt->pfx->complen[1], pkt->pfx->comp[1]);
                struct ccnl_prefix_s *prefix = ccnl_URItoPrefix((char *)_buf, CCNL_SUITE_NDNTLV, NULL);
                struct ccnl_content_s *c = ccnl_mkContentObject(prefix, pkt->content, pkt->contlen, NULL);
                ccnl_prefix_free(prefix);

                if (c) {
                    puts("REALLY ADD TO CACHE");
                    c->flags |= CCNL_CONTENT_FLAGS_STATIC; // really needed here?
                    ccnl_content_add2cache(&ccnl_relay, c);

                    snprintf((char *)&_buf, sizeof(_buf),"/%s/%.*s/%.*s", RFX_NAME_PFX, pkt->pfx->complen[0], pkt->pfx->comp[0], pkt->pfx->complen[1], pkt->pfx->comp[1]);
                    struct ccnl_prefix_s *prefix = ccnl_URItoPrefix((char *)_buf, CCNL_SUITE_NDNTLV, NULL);
                    ccnl_send_interest(prefix, (unsigned char *)_buf, sizeof(_buf), NULL);
                    ccnl_prefix_free(prefix);  // TODO: check do i need to free more memory?

                    return 1;
                }
            }
        }
    }

    return 0;
}


// return 0: dont cache
// return 1: cache
int filter_ack_nack_wait_on_data_cb(struct ccnl_relay_s *relay, struct ccnl_content_s *c)
{
    (void)relay;

    if (strstr((const char *)c->pkt->content, ACK)) {
        return 0;
    }
    if (strstr((const char *)c->pkt->content, NACK)) {
        return 0;
    }
    if (strstr((const char *)c->pkt->content, WAIT)) {
        return 0;
    }

    return 1;
}

int cache_strategy_static_cb(struct ccnl_relay_s *relay, struct ccnl_content_s *c){
    (void)relay;
    c->flags |= CCNL_CONTENT_FLAGS_STATIC;
    return 1;
}


// ret > 0   -> aborts handle interest function (ccnl_fwd_handleInterest)
// ret  0 -> continues handle interest function (ccnl_fwd_handleInterest)
int rfx_on_interest_cb(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   struct ccnl_pkt_s *pkt)
{
    // _print_pfx_comps(pkt->pfx);

    if(pkt->pfx->compcnt == 3) {
        if (!memcmp(pkt->pfx->comp[0], RVS_INT_PFX, pkt->pfx->complen[0])) {
            printf("received interest with %s\n",RVS_INT_PFX);

            // check PIT for second name part. if local, give app. otherwise, fwd
            struct ccnl_interest_s *i;
            for (i = relay->pit; i; i = i->next)
            {

                // WHICH CONDITION TO ADD HERE?
                // compare PIT prefix and incoming prefix. if equal, proceed here
                if (!memcmp(pkt->pfx->comp[1], i->pkt->pfx->comp[1], pkt->pfx->complen[1])) {

                    // cannot parse the pit address? then node is originator and answers reflex interest
                    char *from_as_str = ccnl_addr2ascii(&(i->from->peer));
                    if (!from_as_str) {

                    }
                }
            }
        }

        if (!memcmp(pkt->pfx->comp[0], RFX_NAME_PFX, pkt->pfx->complen[0])) {
            printf("received interest with %s\n", RFX_NAME_PFX);
            // if not exists, add tmp FIB towards sender of interest and remove ... when?=
            snprintf((char *)&_buf, sizeof(_buf),
                    "/%s/%.*s", RVS_INT_PFX, pkt->pfx->complen[1], pkt->pfx->comp[1]);

            sockunion *su = &(from->peer);
            if(su->sa.sa_family == AF_PACKET) { // basically copy from sockunion.c
                struct sockaddr_ll *ll = &su->linklayer;
                static char result[CCNL_MAX_ADDRESS_LEN];
                strcpy(result, ll2ascii(ll->sll_addr, ll->sll_halen & 0x0f));
                printf("add to FIB: %s over: %s\n", _buf, result);

                // this adds a static fib entry
                _intern_fib_add((char *)&_buf, result);

            }

            // if sink,  memorize face (workaround case ll_RX has no face) and send back int,
            if (i_am_sinkfx)
            {
                from_fwd_global = (struct ccnl_face_s *) ccnl_calloc(1, sizeof(struct ccnl_face_s));
                memcpy(from_fwd_global, from, sizeof(struct ccnl_face_s));
                // printf("fwd added FWD: %s\n", ccnl_addr2ascii(&from_gw_global->peer));

                printf("I SINK SEND REFLEXIVE INTEREST\n");
                snprintf((char *)&_buf, sizeof(_buf),
                    "/%s/%.*s/%.*s", RVS_INT_PFX, pkt->pfx->complen[1], pkt->pfx->comp[1], pkt->pfx->complen[2], pkt->pfx->comp[2]);
                struct ccnl_prefix_s *prefix = ccnl_URItoPrefix((char *)_buf, CCNL_SUITE_NDNTLV, NULL);
                ccnl_send_interest(prefix, (unsigned char *)_buf, sizeof(_buf), NULL);
                ccnl_prefix_free(prefix);  // TODO: check do i need to free more memory?

                return 1; // abort handle interest function
            }
        }
    }
    // naming scheme: /REGPFX/ID
    if(pkt->pfx->compcnt == 2) {
        // match registration prefix REGPFX
        if (!memcmp(pkt->pfx->comp[0], REGPFX, pkt->pfx->complen[0])) {

            // add a FIB entry for the prefix /ID
            static char scratch[32];
            struct ccnl_prefix_s *prefix;

            snprintf(scratch, sizeof(scratch)/sizeof(scratch[0]),
                    "/%.*s", pkt->pfx->complen[1], pkt->pfx->comp[1]);
            prefix = ccnl_URItoPrefix(scratch, CCNL_SUITE_NDNTLV, NULL);

            from->flags |= CCNL_FACE_FLAGS_STATIC;
            int ret = ccnl_fib_add_entry(relay, ccnl_prefix_dup(prefix), from);
            if (ret != 0) {
                puts("FIB FULL");
            }
            ccnl_prefix_free(prefix);

            // manually craft an ACK data packet
            int len = 4;
            char buffer[len];
            snprintf(buffer, len, ACK);
            unsigned char *b = (unsigned char *)buffer;
            struct ccnl_content_s *c = ccnl_mkContentObject(pkt->pfx, b, len, NULL);
            c->last_used -= CCNL_CONTENT_TIMEOUT + 5;

#ifdef MODULE_OPENDSME
            // cap on
            netopt_enable_t set = NETOPT_ENABLE;
            if (gnrc_netapi_set(opendsme_get_netif()->pid, NETOPT_GTS_TX, 0, &set, sizeof(netopt_enable_t)) < 0) {
                puts("unable to set CAP");
            }
            else {
                puts("set CAP");
            }
#endif
            if (c) {
                ccnl_content_add2cache(relay, c);
            }
#ifdef MODULE_OPENDSME
            // cap on
            set = NETOPT_DISABLE;
            if (gnrc_netapi_set(opendsme_get_netif()->pid, NETOPT_GTS_TX, 0, &set, sizeof(netopt_enable_t)) < 0) {
                puts("unable to unset CAP");
            }
            else {
                puts("unset CAP");
            }
#endif
        }
    }

    return 0;
}


int local_producer_on_interest_cb(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   struct ccnl_pkt_s *pkt)
{
    (void)from;

    if(pkt->pfx->compcnt == 2) {
        // if my id
        if (!memcmp(pkt->pfx->comp[0], my_id, pkt->pfx->complen[0])) { // todo
            char buffer[4];
            uint8_t rand;
            random_bytes(&rand, 1);
            snprintf(buffer, sizeof(buffer), "%03d", rand);
            unsigned char *b = (unsigned char *)buffer;
            struct ccnl_content_s *c = ccnl_mkContentObject(pkt->pfx, b, sizeof(buffer), NULL);
            c->last_used -= CCNL_CONTENT_TIMEOUT + 5;
            if (c) {
                ccnl_content_add2cache(relay, c);
            }
        }
        else{
            printf("request not for me. my_id is: %s\n", my_id);
        }
    }
    return 0;
}

extern struct ccnl_face_s *loopback_face;

// return 1: stops propagation
// return 0: interest will be handed to propagate funcition
int gw_on_propagate_cb(struct ccnl_relay_s *relay,
                       struct ccnl_face_s *from,
                       struct ccnl_interest_s *i)
{
    (void)relay;
    char buffer[12];
    if(i->pkt->pfx->compcnt == 2) {
    // is prefix in fib?
        struct ccnl_forward_s *fwd;
        for (fwd = ccnl_relay.fib; fwd; fwd = fwd->next) {
            if (!fwd->prefix) {
                continue;
            }
            int rc = ccnl_prefix_cmp(fwd->prefix, NULL, i->pkt->pfx, CMP_LONGEST);
            if(rc > 0) {
                //return  to regular fwd operation. if PIT there, will not fwd
                printf("FOUND PRX IN FIB. send WAIT\n");
                snprintf(buffer, sizeof(buffer), "%s+%i", WAIT, WAITTIME); // TODO: make dynamic?
                unsigned char *b = (unsigned char *)buffer;
                struct ccnl_content_s *c = ccnl_mkContentObject(i->pkt->pfx, b, sizeof(buffer), NULL);
                ccnl_send_pkt(&ccnl_relay, from, c->pkt);

                // now set interest loopback face local to terminate the request on GW
                memcpy((&from->peer), (&loopback_face->peer), sizeof(sockunion));
                from->ifndx = loopback_face->ifndx;
                return 0;

            }
        }

        printf("NO registered node. send back NACK\n");
        snprintf(buffer, sizeof(buffer), NACK);
        unsigned char *b = (unsigned char *)buffer;
        struct ccnl_content_s *c = ccnl_mkContentObject(i->pkt->pfx, b, sizeof(buffer), NULL);
        ccnl_send_pkt(&ccnl_relay, from, c->pkt);
        // didn't find an entry for that prefix, no endnode available. send NACK
        // manually craft an NACK data packet
        return 1;
    }
    return 0;
}

// if registered, called before static setup of retrans timer. called on interest creation and retransmission.
// return 0 -> do nothing
// return -1 -> abort retrans
// return > 0 -> time in [ms] until next retrans. CCNL_MAX_INTEREST_RETRANSMIT is still maximum number of retransmissions.
int _on_retrans_cb(struct ccnl_interest_s *ccnl_int)
{
    (void)ccnl_int;
    printf("_on_retrans_cb; retries: %i\n", ccnl_int->retries);
    // TODO!
    return 2;
}

int register_node(char *regpfx, char *testid, char *addr_str)
{
    // set callback to process ACK (same as set_node)
    ccnl_set_cb_rx_on_data(register_on_data_cb);

    // add static fib entry for registration at GW
    _intern_fib_add(regpfx, addr_str);

    // send registering interest
    char buf[64];

    snprintf(buf, sizeof(buf), "/%s/%s", regpfx, testid);
    struct ccnl_prefix_s *prefix = ccnl_URItoPrefix(buf, CCNL_SUITE_NDNTLV, NULL);

#ifdef MODULE_OPENDSME
    // cap on
    netopt_enable_t set = NETOPT_ENABLE;
    if (gnrc_netapi_set(opendsme_get_netif()->pid, NETOPT_GTS_TX, 0, &set, sizeof(netopt_enable_t)) < 0) {
        puts("unable to set CAP");
    }
    else {
        puts("set CAP");
    }
#endif

    memset(buf, '\0', sizeof(buf)); // why?
    int res = ccnl_send_interest(prefix, (unsigned char *)&buf, sizeof(buf), NULL);
    ccnl_prefix_free(prefix);

#ifdef MODULE_OPENDSME
    // cap of
    set = NETOPT_DISABLE;
    if (gnrc_netapi_set(opendsme_get_netif()->pid, NETOPT_GTS_TX, 0, &set, sizeof(netopt_enable_t)) < 0) {
        puts("unable to unset CAP");
    }
    else {
        puts("unset CAP");
    }
#endif
    return res;
}

int send_data_push(char *name, char *data)
{
    struct ccnl_content_s *c;
    struct ccnl_prefix_s *prefix;

    if (from_gw_global) {
        prefix = ccnl_URItoPrefix(name, CCNL_SUITE_NDNTLV, NULL);
        c = ccnl_mkContentObject(prefix, (unsigned char *)data, sizeof(data), NULL);

        ccnl_send_pkt(&ccnl_relay, from_gw_global, c->pkt);
        ccnl_prefix_free(prefix);
    }
    else {
        puts("from_gw_global NOT SET");
    }

    return 0;
}



static void log_name(uint8_t *payload, unsigned len)
{
    unsigned i = 0;
    while (i < len) {
        if (payload[i] == 0x08) {
            unsigned complen = payload[i+1];
            printf("/%.*s", complen, (char *)&payload[i+2]);
            i += complen + 2;
        }
        else {
            /* this should not happen, actually */
            break;
        }
    }
    printf(";");
    return;
}

void print_l3info_l2(uint8_t *payload, unsigned len, bool direction)
{
    (void)len;

    unsigned pkttype = payload[0];
    unsigned pktlen = payload[1];

    printf("[info];");

    if (direction) {
        printf("TX;");
    }
    else {
        printf("RX;");
    }

    if(pkttype == 0x05) { // interest type
        printf("I;");
    }
    if (pkttype == 0x06) { // data type
        printf("D;");
    }

    unsigned i = 2;
    while (i < pktlen) {
        unsigned tlvtype = payload[i];
        unsigned tlvlen = payload[i+1];

        /* Name TLV */
        if (tlvtype == 0x7) { // name tlv
            log_name(payload + i + 2, tlvlen);
            i += tlvlen + 2;
            break;
        }
    }

    // for interest packets, print nonce at last position
    // nonce type is 0x0a
    if (pkttype == 0x05 && payload[i] == 0x0a) {
        unsigned complen = payload[i+1];
        for (unsigned j=0;j<complen;j++) {
            printf("%x", payload[j+i+2]);
        }
    }

    // for data packets, print data al last potition
    if (pkttype == 0x06 && payload[i+2] == 0x15) { // +2 to skip "meta info"?
        unsigned complen = payload[i+3];
        printf("%.*s", complen, (char *)&payload[i+4]);
    }
    printf("\n");

}

void print_l3info(int type, struct ccnl_prefix_s *pfx)
{
    ccnl_prefix_to_str(pfx,(char *)_buf,CCNL_MAX_PREFIX_SIZE);
    printf("[info];");
    switch(type) {
        case ADD_CONTENT:
            printf("NEW;D;%s;XXX", _buf);
            break;
        case ADD_PIT:
            printf("NEW;I;%s;XXX", _buf);
            break;
        case CS_TIMEOUT:
            printf("TO;D;%s;XXX", _buf);
            break;
        case PIT_TIMEOUT:
            printf("TO;I;%s;XXX", _buf);
            break;
        default:
            break;
    }
    printf("\n");
}