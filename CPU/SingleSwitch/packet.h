#ifndef SRC_CODE_PACKET_H
#define SRC_CODE_PACKET_H

#include "common.h"

#define UINT16_2INT(x) (((x >> 8) | (x << 8)) & 0xffff)

#define BUF_SIZE 10240
#define STR_SIZE 1024


struct time_val
{
    unsigned int tv_sec;
    unsigned int tv_usec;
};

typedef struct pcap_pkthdr_t
{
    struct time_val ts;
    uint32_t caplen;
    uint32_t len;
}pcap_pkthdr;

typedef struct tag_ip_header_t
{
    uint8_t ver_headerlen;
    uint8_t tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flag_segment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t srcip;
    uint32_t dstip;
} ipheader_t;

typedef struct tag_tcpheader_t
{
    uint16_t srcport;
    uint16_t dstport;
    uint32_t seq;
    uint32_t ack;
    uint8_t headerlen;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_pointer;
}tcpheader_t;

typedef struct pcap_info_tag
{
    FILE *fp;
    pcap_pkthdr *pkt_header;
    ipheader_t *ip_header;
    tcpheader_t *tcp_header;

    unsigned long long pkt_offset;
    int pkt_num;
    int ip_len, http_len, ip_proto;
    int src_port, dst_port, tcp_flags;
    char buf[BUF_SIZE], my_time[STR_SIZE];
    char src_ip[STR_SIZE], dst_ip[STR_SIZE];
    char  host[STR_SIZE], uri[BUF_SIZE];

    uint32_t tuple[5];
    unsigned long long ts0;
    uint32_t ts_cur;
}pcap_info_t;

pcap_info_t* init_pcap_file(char file_name[128]);
int pcap_get_next_pkt(pcap_info_t *p_info);


#endif // SRC_CODE_PACKET_H
