#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/in.h> // Include for IPPROTO_TCP and IPPROTO_UDP

#define ntohs(x)   ((((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8))

SEC("xdp")
int packet_monitor(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    // Parse Ethernet header
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end) 
        return XDP_PASS;

    // Print source MAC address
    bpf_printk("Src MAC: %02x:%02x:%02x",
               eth->h_source[0], eth->h_source[1], eth->h_source[2]);
    bpf_printk("Src MAC (cont.): %02x:%02x:%02x",
               eth->h_source[3], eth->h_source[4], eth->h_source[5]);

    // Print destination MAC address
    bpf_printk("Dest MAC: %02x:%02x:%02x",
               eth->h_dest[0], eth->h_dest[1], eth->h_dest[2]);
    bpf_printk("Dest MAC (cont.): %02x:%02x:%02x",
               eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);

    // Only process IPv4 packets (EtherType = 0x0800)
    if (eth->h_proto != __constant_htons(ETH_P_IP))
        return XDP_PASS;

    // Parse IP header
    struct iphdr *iph = (void *)(eth + 1);
    if ((void *)(iph + 1) > data_end) 
        return XDP_PASS;

    bpf_printk("IP Header: Src IP %x, Dest IP %x, Protocol %d",
               iph->saddr, iph->daddr, iph->protocol);

    // Check protocol for Layer 4 headers
    if (iph->protocol == IPPROTO_TCP) {
        struct tcphdr *tcph = (void *)(iph + 1);
        if ((void *)(tcph + 1) > data_end)
            return XDP_PASS;

        bpf_printk("TCP Header: Src Port %d, Dest Port %d",
                   ntohs(tcph->source), ntohs(tcph->dest));
    } else if (iph->protocol == IPPROTO_UDP) {
        struct udphdr *udph = (void *)(iph + 1);
        if ((void *)(udph + 1) > data_end)
            return XDP_PASS;

        bpf_printk("UDP Header: Src Port %d, Dest Port %d",
                   ntohs(udph->source), ntohs(udph->dest));
    }

    // Calculate packet length
    __u64 packet_len = (void *)data_end - (void *)data;
    bpf_printk("Packet Length: %llu bytes", packet_len);

    return XDP_PASS;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
