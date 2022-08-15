/*
 * Copyright (C) 2015 Inria
 *               2022 HAW Hamburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Basic ccn-lite relay example (produce and consumer via shell)
 *
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 * @author      Peter Kietzmann <peter.kietzmann@haw-hamburg.de>
 *
 * @}
 */

#include <stdio.h>

#include "fmt.h"
#include "msg.h"
#include "shell.h"
#include "ccn-lite-riot.h"
#include "ccnl-callbacks.h"
#include "ccnl-producer.h"
#include "ccnl-pkt-builder.h"
#include "net/gnrc/netif.h"
#include "ztimer.h"

#include "net/gnrc.h"
#include "net/gnrc/netreg.h"

#include "ccn-lite-helpers.h"
#ifdef MODULE_OPENDSME
#include "opendsme/opendsme.h"
#if !defined(BOARD_NATIVE) && !IS_ACTIVE(DISABLE_LED)
#include "board.h"
#include "event.h"
#include "event/periodic.h"
#include "event/thread.h"
#endif
#endif

#if !defined(BOARD_NATIVE) && !IS_ACTIVE(DISABLE_LED)
static event_periodic_t event_periodic;
static void _poll_status(event_t *event)
{
    (void) event;
    netopt_enable_t en;
    gnrc_netapi_get(opendsme_get_netif()->pid, NETOPT_LINK, 0, &en, sizeof(en));
    if (en == NETOPT_ENABLE) {
        LED0_ON;
    }
    else {
        LED0_OFF;
    }
}
static event_t ev_status = {.handler=_poll_status};
#endif

gnrc_netif_t *opendsme_get_netif(void);

/* main thread's message queue */
#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

uint8_t i_am_sinkfx = 0;
char my_id[4];

static unsigned char _buf[64];
extern struct ccnl_face_s *from_fwd_global;

uint16_t node_id;
uint16_t get_node_id(void)
{
    return node_id;
}

// prefix to identify a registration interest from nodes to gateway
#define TESTID      "xid"
#define TESTNAME    "nam"
#define TESTDATA    "dat"


#define NEVENTS (unsigned)(10)
static evtimer_msg_event_t events[NEVENTS];
static char resend_ints[NEVENTS][40];
static unsigned event_idx = 0;

static void _cb(uint16_t cmd, gnrc_pktsnip_t *pkt,
                                       void *ctx)
{
    (void) cmd;
    (void) ctx;

    // on WAIT instruction, extact time and re-send interest
    if(!memcmp(WAIT, pkt->data, 4)) {
        char dest[4];
        char *dat = (char *)pkt->data;
        strncpy(dest, &dat[sizeof(WAIT)], 2);
        int time = atoi(dest);
        printf("re-send interest: %.*s in %i seconds\n", (int)pkt->next->size, (char*)pkt->next->data, time);

        evtimer_del(&ccnl_evtimer, &events[event_idx].event);

        memset(resend_ints[event_idx], '\0', sizeof(resend_ints[event_idx]));
        memcpy(resend_ints[event_idx], pkt->next->data, pkt->next->size);

        events[event_idx].event.offset = time * 1000;
        events[event_idx].msg.type = CCNL_MSG_APP_INTEREST_RETRY;
        events[event_idx].msg.content.ptr = resend_ints[event_idx];
        evtimer_add_msg(&ccnl_evtimer, &events[event_idx], ccnl_event_loop_pid);
        event_idx++;
        if(event_idx > NEVENTS) {
            event_idx = 0;
        }
    }
    else if (!memcmp(ACK, pkt->data, 3)) {
        printf("got ACK\n");
    }
    else if (!memcmp(NACK, pkt->data, 3)) {
        printf("got NACK\n");
    }
    else {
        if (i_am_sinkfx)
        {
            // if sink, send back ack
            char buffer[4];
            memset(buffer, '\0', sizeof(buffer)); // why?
            snprintf(buffer, sizeof(buffer), ACK);
            // name
            struct ccnl_prefix_s *prefix_in = ccnl_URItoPrefix((char *)pkt->next->data, CCNL_SUITE_NDNTLV, NULL);

            snprintf((char *)&_buf, sizeof(_buf),
                "/%s/%.*s/%.*s", RFX_NAME_PFX, prefix_in->complen[1], prefix_in->comp[1], prefix_in->complen[2], prefix_in->comp[2]);

            struct ccnl_prefix_s *prefix = ccnl_URItoPrefix((char *)_buf, CCNL_SUITE_NDNTLV, NULL);
            struct ccnl_content_s *c = ccnl_mkContentObject(prefix, (unsigned char *)&buffer, sizeof(buffer), NULL);
            c->last_used -= CCNL_CONTENT_TIMEOUT + 5; // let it timeout in 5 sec?s

            // this is hacky. we only print if content is NOT in CS. this is the first reception of that content item. in that case, i will be put to the CS afterwards. hence, iterating through the CS will not show a match. only
            // on the > second try, the CS will hold the content.
            // if exist, we do not print BUT want to send the ACK anyway. why hacky? a duplicate received /RNP always
            // triggers an other interest /RPT. in the case that we already have the content, we call the duplicate /RPT
            // anyway BUT it doesn't hurt, since it is answered from the local CS. hence, we only disable the duplicate
            struct ccnl_content_s *c_iter;
            bool print_arx=1;
            for (c_iter = ccnl_relay.contents; c_iter; c_iter = c_iter->next) {
                int rc = ccnl_prefix_cmp(c_iter->pkt->pfx, NULL, prefix_in, CMP_EXACT);
                if(rc == 0) { // on exact mathc, 0 indicates full match
                    printf("CONTENT EXISTS %p\n", (void *) c_iter);
                    print_arx = 0;
                    break;
                }
            }
            if(print_arx) {
                // using prefix_in for printing here, because the uri2prefix operates on the pointer?!
                printf("[info];ARX;D;%s;%.*s\n", ccnl_prefix_to_str(prefix_in,(char *)&_buf,CCNL_MAX_PREFIX_SIZE), (int)pkt->size, (char*)pkt->data);
            }

            ccnl_prefix_free(prefix);
            ccnl_prefix_free(prefix_in);

            if (c) {
                if(from_fwd_global) {
                    ccnl_send_pkt(&ccnl_relay, from_fwd_global, c->pkt);
                    // ccnl_content_add2cache(&ccnl_relay, c);
                }
                else {
                    puts("from_fwd_global not set");
                }

            }
            else{
                puts("could create content");
            }
        }
        else {// other cases simply print
            printf("[info];ARX;D;%.*s;%.*s\n", (int)pkt->next->size, (char*)pkt->next->data, (int)pkt->size, (char*)pkt->data);
        }
    }

    // do i need to release both manually?
    gnrc_pktbuf_release(pkt);
}

static gnrc_netreg_entry_cbd_t _cbd = {.cb = _cb};
gnrc_netreg_entry_t payload_dump = GNRC_NETREG_ENTRY_INIT_CB(GNRC_NETREG_DEMUX_CTX_ALL, &_cbd);


int data_push(int argc, char **argv)
{
    int len = 4;
    char buffer[len];
    int len_pfx = 32;
    char buffer_pfx[len_pfx];

    if(argc == 3)
    {
        snprintf(buffer_pfx, len_pfx, "%s", argv[1]);
        snprintf(buffer, len, "%s", argv[2]);
    }
    else
    {
        snprintf(buffer, len, TESTDATA);
        // b = (char *)buffer;
        snprintf(buffer_pfx, len_pfx, "/%s/%s", TESTID, TESTNAME);
    }

    printf("[info];ATX;D;%s;%s\n", buffer_pfx, buffer);

    puts("SEND DATA PUSH");
    send_data_push(buffer_pfx, buffer);

    return 0;
}

int set_retrans(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    puts("SET UP ALTERNATIVE RETRANS FUNCTIONALITY");
    ccnl_set_cb_on_retrans(_on_retrans_cb);
    return 0;
}

int set_special_gw(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    puts("SET UP GW");
    gnrc_netreg_unregister(GNRC_NETTYPE_CCN_CHUNK, &payload_dump);
    ccnl_set_cb_on_propagate(gw_on_propagate_cb);
    ccnl_set_cache_strategy_cache(cache_strategy_static_cb);
    return 0;
}

static void _set_refx_case(void){
    ccnl_set_local_producer(rfx_on_interest_cb);
    ccnl_set_cache_strategy_cache(filter_ack_nack_wait_on_data_cb);
}

int set_sinkfx(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    puts("SET UP SINK FX");
    _set_refx_case();
    i_am_sinkfx = 1;
    return 0;
}

int set_nodefx(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    puts("SET UP NODE FX");
    _set_refx_case();
    return 0;
}

int set_filter_nackwait(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    puts("SET UP FAST NODE");
    ccnl_set_cache_strategy_cache(filter_ack_nack_wait_on_data_cb);
    return 0;
}

int set_ondata(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    puts("SET ON DATA CB");
    ccnl_set_cb_rx_on_data(on_data_cb);
    return 0;
}

int set_localproducer(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (argc == 2) {
        memcpy(my_id, argv[1], sizeof(my_id));
    }
    else {
        memcpy(my_id, TESTID, sizeof(my_id));
    }
    printf("SET LOCAL PRODUCER; my_id is: %s\n", my_id);

    ccnl_set_local_producer(local_producer_on_interest_cb);
    return 0;
}

int reg_node(int argc, char **argv)
{
    if (argc == 2) {
        return register_node(REGPFX, TESTID, argv[1]);
    }
    else if (argc == 3)
    {
        return register_node(REGPFX, argv[2], argv[1]);
    }
    else {
        printf("usage: reg <GW address> [pfx]\n");
        return -1;
    }
}
int call_ccnl_dump(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("Num interfaces: %i\n", ccnl_get_ifcount(&ccnl_relay));
    ccnl_dump(0, CCNL_RELAY, &ccnl_relay);
    return 0;
}
int getset_params(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (argc == 4)
    {
        global_ccnl_interest_timeout = atoi(argv[1]);
        global_ccnl_interest_retrans_timeout = atoi(argv[2]);
        global_ccnl_max_interest_retransmit =atoi(argv[3]);
    }
    else {
        printf("usage: params <interest t/o [s]> <retrans t/o [ms] <num retrans> \n");
        printf("compiled default is: %i [s] %i [ms] %i [#]\n\n", CCNL_INTEREST_TIMEOUT, CCNL_INTEREST_RETRANS_TIMEOUT, CCNL_MAX_INTEREST_RETRANSMIT);

        printf("global_ccnl_interest_timeout [s]: %i\n", (int)global_ccnl_interest_timeout);
        printf("global_ccnl_interest_retrans_timeout [ms]: %i\n", global_ccnl_interest_retrans_timeout);
        printf("global_ccnl_max_interest_retransmit [#]: %i\n", global_ccnl_max_interest_retransmit);
    }

    return 0;
}

static int id_cmd(int argc, char **argv)
{
    (void) argc;
    node_id = (uint16_t) ((argv[1][0] << 8) + (argv[1][1]));
    return 0;
}
#if !IS_ACTIVE(CONFIG_OPENDSME_USE_CAP) && IS_ACTIVE(CONFIG_DSME_PLATFORM_STATIC_GTS)
static int gts_cmd(int argc, char **argv)
{
    if (argc != 6) {
        printf("%s reguires 6 arguments\n", argv[0]);
        return 1;
    }
    dsme_alloc_t alloc;
    memset(&alloc, 0, sizeof(alloc));
    l2util_addr_from_str(argv[1], (uint8_t*) &alloc.addr);
    alloc.tx = scn_u32_dec(argv[2],1);
    alloc.superframe_id = scn_u32_dec(argv[3],1);
    alloc.slot_id = scn_u32_dec(argv[4],1);
    alloc.channel_id = scn_u32_dec(argv[5], 1);
    gnrc_netapi_set(opendsme_get_netif()->pid, NETOPT_GTS_ALLOC, 0, &alloc, sizeof(alloc));

    return 0;
}
#endif

SHELL_COMMAND(setretrans, "set up alternative retransmission functionality", set_retrans);
SHELL_COMMAND(setgw, "set up gateway functionality", set_special_gw);
SHELL_COMMAND(setondata, "enable on data cb", set_ondata);
SHELL_COMMAND(setsinkfx, "set up reflexive fwd gateway functionality", set_sinkfx);
SHELL_COMMAND(setnw, "set up nack wait data filter", set_filter_nackwait);
SHELL_COMMAND(setnodefx, "set up node functionality", set_nodefx);
SHELL_COMMAND(reg, "register node at gateway", reg_node);
SHELL_COMMAND(prod, "enable local producer", set_localproducer);
SHELL_COMMAND(txpush, "send data push message", data_push);
SHELL_COMMAND(dump, "dump ccnl internal states", call_ccnl_dump);
SHELL_COMMAND(params, "print or set timout and retrans params", getset_params);
#if IS_ACTIVE(CONFIG_DSME_PLATFORM_STATIC_GTS)
SHELL_COMMAND(gts, "add gts slot", gts_cmd);
#endif
SHELL_COMMAND(id, "set node id", id_cmd);

int main(void)
{
#ifdef USE_RONR
    printf("YOU ARE USING RONR\n");
#endif
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    ccnl_core_init();

    ccnl_start();

    /* get the default interface */
    gnrc_netif_t *netif = NULL;

    /* set the relay's PID, configure the interface to use CCN nettype */
    while ((netif = gnrc_netif_iter(netif)))
    {
        if (ccnl_open_netif(netif->pid, GNRC_NETTYPE_CCN) < 0)
        {
            printf("Error registering network interface: %i\n", netif->pid);
            return -1;
        }
        else {
            printf("main registered netif->pid: %i\n", netif->pid);
        }
    }

#ifdef MODULE_NETIF
    gnrc_netreg_register(GNRC_NETTYPE_CCN_CHUNK, &payload_dump);
#endif

#if !defined(BOARD_NATIVE) && !IS_ACTIVE(DISABLE_LED)
    /* Start poll for status */
    event_periodic_init(&event_periodic, ZTIMER_MSEC, EVENT_PRIO_MEDIUM, &ev_status);
    event_periodic_start(&event_periodic, 500);
#endif

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
    return 0;
}
