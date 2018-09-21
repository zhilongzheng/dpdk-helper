/*
* Created by Zhilong Zheng
*/

#include "util_dpdk.h"

#include <inttypes.h>

#include <rte_branch_prediction.h>
#include <rte_mbuf.h>

#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>

int
onvm_pkt_mac_addr_swap(struct rte_mbuf* pkt, unsigned dst_port) {
    struct ether_hdr *eth;
    struct ether_addr addr;

    if (unlikely(pkt == NULL)) { // We do not expect to swap macs for empty packets
        return -1;
    }

    /*
     * Get the ethernet header from the pkt
     */
    eth = onvm_pkt_ether_hdr(pkt);

    /*
     * Copy the source mac address to the destination field.
     */
    ether_addr_copy(&eth->s_addr, &eth->d_addr);

    /*
     * Get the mac address of the current port to send out of
     * and set the source field to it.
     */
    rte_eth_macaddr_get(dst_port, &eth->s_addr);
    ether_addr_copy(&addr, &eth->s_addr);

    return 0;
}

struct ether_hdr*
onvm_pkt_ether_hdr(struct rte_mbuf* pkt) {
    if (unlikely(pkt == NULL)) {
        return NULL;
    }
    return rte_pktmbuf_mtod(pkt, struct ether_hdr *);
}

struct tcp_hdr*
onvm_pkt_tcp_hdr(struct rte_mbuf* pkt) {
    struct ipv4_hdr* ipv4 = onvm_pkt_ipv4_hdr(pkt);

    if (unlikely(ipv4 == NULL)) {  // Since we aren't dealing with IPv6 packets for now, we can ignore anything that isn't IPv4
        return NULL;
    }

    if (ipv4->next_proto_id != IP_PROTOCOL_TCP) {
        return NULL;
    }

    uint8_t* pkt_data = rte_pktmbuf_mtod(pkt, uint8_t*) + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr);
    return (struct tcp_hdr*)pkt_data;
}

struct udp_hdr*
onvm_pkt_udp_hdr(struct rte_mbuf* pkt) {
    struct ipv4_hdr* ipv4 = onvm_pkt_ipv4_hdr(pkt);

    if (unlikely(ipv4 == NULL)) {  // Since we aren't dealing with IPv6 packets for now, we can ignore anything that isn't IPv4
        return NULL;
    }

    if (ipv4->next_proto_id != IP_PROTOCOL_UDP) {
        return NULL;
    }

    uint8_t* pkt_data = rte_pktmbuf_mtod(pkt, uint8_t*) + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr);
    return (struct udp_hdr*)pkt_data;
}

struct ipv4_hdr*
onvm_pkt_ipv4_hdr(struct rte_mbuf* pkt) {
    struct ipv4_hdr* ipv4 = (struct ipv4_hdr*)(rte_pktmbuf_mtod(pkt, uint8_t*) + sizeof(struct ether_hdr));

    /* In an IP packet, the first 4 bits determine the version.
     * The next 4 bits are called the Internet Header Length, or IHL.
     * DPDK's ipv4_hdr struct combines both the version and the IHL into one uint8_t.
     */
    uint8_t version = (ipv4->version_ihl >> 4) & 0b1111;
    if (unlikely(version != 4)) {
        return NULL;
    }
    return ipv4;
}

void *
onvm_pkt_payload(struct rte_mbuf *pkt, int *payload_size) {
    uint8_t* pkt_data = rte_pktmbuf_mtod(pkt, uint8_t*) + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr);
    *payload_size = ((struct ipv4_hdr *)pkt_data)->total_length - sizeof(struct ipv4_hdr);

    if (((struct ipv4_hdr *)pkt_data)->next_proto_id == IP_PROTOCOL_TCP) {
        pkt_data = pkt_data + sizeof(struct tcp_hdr);
        *payload_size -= sizeof(struct tcp_hdr);
    } else if (((struct ipv4_hdr *)pkt_data)->next_proto_id == IP_PROTOCOL_UDP) {
        pkt_data = pkt_data + sizeof(struct udp_hdr);
        *payload_size -=sizeof(struct udp_hdr);
    } else {
        pkt_data = NULL;
    }

    return pkt_data;
}


int
onvm_pkt_is_tcp(struct rte_mbuf* pkt) {
    return onvm_pkt_tcp_hdr(pkt) != NULL;
}

int
onvm_pkt_is_udp(struct rte_mbuf* pkt) {
    return onvm_pkt_udp_hdr(pkt) != NULL;
}

int
onvm_pkt_is_ipv4(struct rte_mbuf* pkt) {
    return onvm_pkt_ipv4_hdr(pkt) != NULL;
}


void
onvm_pkt_print(struct rte_mbuf* pkt) {
    struct ipv4_hdr* ipv4 = onvm_pkt_ipv4_hdr(pkt);
    if (likely(ipv4 != NULL)) {
        onvm_pkt_print_ipv4(ipv4);
    }

    struct tcp_hdr* tcp = onvm_pkt_tcp_hdr(pkt);
    if (tcp != NULL) {
        onvm_pkt_print_tcp(tcp);
    }

    struct udp_hdr* udp = onvm_pkt_udp_hdr(pkt);
    if (udp != NULL) {
        onvm_pkt_print_udp(udp);
    }
}

void
onvm_pkt_print_tcp(struct tcp_hdr* hdr) {
    printf("Source Port: %" PRIu16 "\n", rte_be_to_cpu_16(hdr->src_port));
    printf("Destination Port: %" PRIu16 "\n", rte_be_to_cpu_16(hdr->dst_port));
    printf("Sequence number: %" PRIu32 "\n", rte_be_to_cpu_32(hdr->sent_seq));
    printf("Acknowledgement number: %" PRIu32 "\n", rte_be_to_cpu_32(hdr->recv_ack));
    printf("Data offset: %" PRIu8 "\n", hdr->data_off);

    /* TCP defines 9 different 1-bit flags, but DPDK's flags field only leaves room for 8.
     * I think DPDK's struct puts the first flag in the last bit of the data offset field.
     */
    uint16_t flags = ((hdr->data_off << 8) | hdr->tcp_flags) & 0b111111111;

    printf("Flags: %" PRIx16 "\n", flags);
    printf("\t(");
    if ((flags >> 8) & 0x1) printf("NS,");
    if ((flags >> 7) & 0x1) printf("CWR,");
    if ((flags >> 6) & 0x1) printf("ECE,");
    if ((flags >> 5) & 0x1) printf("URG,");
    if ((flags >> 4) & 0x1) printf("ACK,");
    if ((flags >> 3) & 0x1) printf("PSH,");
    if ((flags >> 2) & 0x1) printf("RST,");
    if ((flags >> 1) & 0x1) printf("SYN,");
    if (flags        & 0x1) printf("FIN,");
    printf(")\n");

    printf("Window Size: %" PRIu16 "\n", rte_be_to_cpu_16(hdr->rx_win));
    printf("Checksum: %" PRIu16 "\n", rte_be_to_cpu_16(hdr->cksum));
    printf("Urgent Pointer: %" PRIu16 "\n", rte_be_to_cpu_16(hdr->tcp_urp));
}

void
onvm_pkt_print_udp(struct udp_hdr* hdr) {
    printf("Source Port: %" PRIu16 "\n", hdr->src_port);
    printf("Destination Port: %" PRIu16 "\n", hdr->dst_port);
    printf("Length: %" PRIu16 "\n", hdr->dgram_len);
    printf("Checksum: %" PRIu16 "\n", hdr->dgram_cksum);
}

void
onvm_pkt_print_ipv4(struct ipv4_hdr* hdr) {
    printf("IHL: %" PRIu8 "\n", hdr->version_ihl & 0b1111);
    printf("DSCP: %" PRIu8 "\n", hdr->type_of_service & 0b111111);
    printf("ECN: %" PRIu8 "\n", (hdr->type_of_service >> 6) & 0b11);
    printf("Total Length: %" PRIu16 "\n", rte_be_to_cpu_16(hdr->total_length));
    printf("Identification: %" PRIu16 "\n", rte_be_to_cpu_16(hdr->packet_id));

    uint8_t flags = (hdr->fragment_offset >> 13) & 0b111;  // there are three 1-bit flags, but only 2 are used
    printf("Flags: %" PRIx8 "\n", flags);
    printf("\t(");
    if ((flags >> 1) & 0x1) printf("DF,");
    if ( flags       & 0x1) printf("MF,");
    printf("\n");

    printf("Fragment Offset: %" PRIu16 "\n", rte_be_to_cpu_16(hdr->fragment_offset) & 0b1111111111111);
    printf("TTL: %" PRIu8 "\n", hdr->time_to_live);
    printf("Protocol: %" PRIu8, hdr->next_proto_id);

    if (hdr->next_proto_id == IP_PROTOCOL_TCP) {
        printf(" (TCP)");
    }

    if (hdr->next_proto_id == IP_PROTOCOL_UDP) {
        printf(" (UDP)");
    }

    printf("\n");

    printf("Header Checksum: %" PRIu16 "\n", hdr->hdr_checksum);
    printf("Source IP: %" PRIu32 " (%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 ")\n", hdr->src_addr,
           hdr->src_addr & 0xFF, (hdr->src_addr >> 8) & 0xFF, (hdr->src_addr >> 16) & 0xFF, (hdr->src_addr >> 24) & 0xFF);
    printf("Destination IP: %" PRIu32 " (%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 ")\n", hdr->dst_addr,
           hdr->dst_addr & 0xFF, (hdr->dst_addr >> 8) & 0xFF, (hdr->dst_addr >> 16) & 0xFF, (hdr->dst_addr >> 24) & 0xFF);
}

void onvm_pkt_print_ether(struct ether_hdr* hdr) {
    const char *type = NULL;
    if (unlikely(hdr == NULL)) {
        return;
    }
    printf("Source MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           hdr->s_addr.addr_bytes[0], hdr->s_addr.addr_bytes[1],
           hdr->s_addr.addr_bytes[2], hdr->s_addr.addr_bytes[3],
           hdr->s_addr.addr_bytes[4], hdr->s_addr.addr_bytes[5]);
    printf("Dest MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           hdr->d_addr.addr_bytes[0], hdr->d_addr.addr_bytes[1],
           hdr->d_addr.addr_bytes[2], hdr->d_addr.addr_bytes[3],
           hdr->d_addr.addr_bytes[4], hdr->d_addr.addr_bytes[5]);
    switch(hdr->ether_type) {
        case ETHER_TYPE_IPv4:
            type = "IPv4";
            break;
        case ETHER_TYPE_IPv6:
            type = "IPv6";
            break;
        case ETHER_TYPE_ARP:
            type = "ARP";
            break;
        case ETHER_TYPE_RARP:
            type = "Reverse ARP";
            break;
        case ETHER_TYPE_VLAN:
            type = "VLAN";
            break;
        case ETHER_TYPE_1588:
            type = "1588 Precise Time";
            break;
        case ETHER_TYPE_SLOW:
            type = "Slow";
            break;
        case ETHER_TYPE_TEB:
            type = "Transparent Ethernet Bridging (TEP)";
            break;
        default:
            type = "unknown";
            break;
    }
    printf("Type: %s\n", type);
}
