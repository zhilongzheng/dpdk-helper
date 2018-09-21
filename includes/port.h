/*
* Created by Zhilong Zheng
*/

#ifndef GOLF_PORT_H_H
#define GOLF_PORT_H_H

#include <stdint.h>
#include <rte_mempool.h>

#define MAX_PORTS_NB  8
#define MAX_WORKERS_NB  16

struct port_info {
    uint8_t port_id[MAX_PORTS_NB];
    uint32_t port_nb;
    //other info should be added.
    struct {
        uint64_t rx_pkts;
        uint64_t drop_pkts;
        uint64_t enqueue_pkts;
    } rx_stats[MAX_PORTS_NB] __rte_cache_aligned;

    struct {
        uint64_t dequeue_pkts;
        uint64_t enqueue_pkts;
        uint64_t enqueue_failed_pkts;
    } wkr_stats[MAX_WORKERS_NB] __rte_cache_aligned;

    struct {
        uint64_t tx_pkts;
        uint64_t drop_pkts;
        uint64_t dequeue_pkts;
    } tx_stats[MAX_PORTS_NB] __rte_cache_aligned;
};

uint8_t get_nb_ports(void);

struct port_info init_ports(uint32_t nb_ports, uint32_t portmask, struct rte_mempool *mbuf_pool);

void display_ports(unsigned difftime, struct port_info *portinfo);

#endif //GOLF_PORT_H_H 