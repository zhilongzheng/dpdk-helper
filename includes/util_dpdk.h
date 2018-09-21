/*
* Created by Zhilong Zheng
*/

#ifndef GOLF_UTIL_DPDK_H
#define GOLF_UTIL_DPDK_H


struct rte_mbuf;
struct tcp_hdr;
struct udp_hdr;
struct ipv4_hdr;

#define IP_PROTOCOL_TCP 6
#define IP_PROTOCOL_UDP 17

/**
 * Flip the source and destination mac address of a packet
 */
int
onvm_pkt_mac_addr_swap(struct rte_mbuf* pkt, unsigned dst_port);

/**
 * Return a pointer to the tcp/udp/ip header in the packet, or NULL if not a TCP packet
 */
struct tcp_hdr*
onvm_pkt_tcp_hdr(struct rte_mbuf* pkt);

struct ether_hdr*
onvm_pkt_ether_hdr(struct rte_mbuf* pkt);

struct udp_hdr*
onvm_pkt_udp_hdr(struct rte_mbuf* pkt);

struct ipv4_hdr*
onvm_pkt_ipv4_hdr(struct rte_mbuf* pkt);

void *
onvm_pkt_payload(struct rte_mbuf *pkt, int *payload_size);

/**
 * Check the type of a packet. Return 1 if packet is of the specified type, else 0
 */
int
onvm_pkt_is_tcp(struct rte_mbuf* pkt);

int
onvm_pkt_is_udp(struct rte_mbuf* pkt);

int
onvm_pkt_is_ipv4(struct rte_mbuf* pkt);

/**
 * Print out a packet or header.  Check to be sure DPDK doesn't already do any of these
 */
void
onvm_pkt_print(struct rte_mbuf* pkt);  // calls the funcs below

void
onvm_pkt_print_tcp(struct tcp_hdr* hdr);

void
onvm_pkt_print_udp(struct udp_hdr* hdr);

void
onvm_pkt_print_ipv4(struct ipv4_hdr* hdr);

void
onvm_pkt_print_ether(struct ether_hdr* hdr);

#endif //GOLF_UTIL_DPDK_H
