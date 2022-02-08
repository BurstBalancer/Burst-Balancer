#include "packet.h"

pcap_info_t* init_pcap_file(char file_name[128])
{
    pcap_info_t *p_info;

    MALLOC_AND_SET(p_info, sizeof(pcap_info_t), pcap_info_t *)

    p_info->pkt_header = (pcap_pkthdr *)malloc(sizeof(pcap_pkthdr));
    p_info->ip_header = (ipheader_t *)malloc(sizeof(ipheader_t));
    p_info->tcp_header = (tcpheader_t *)malloc(sizeof(tcpheader_t));

    p_info->fp = fopen(file_name, "rb");
    if ((p_info->fp = fopen(file_name, "rb")) == NULL) {
        printf("error : can not open file %s \n", file_name);
        exit(0);
    }
    
    p_info->pkt_offset = 20;

    return p_info;
}

int pcap_get_next_pkt(pcap_info_t *p_info)
{
    struct time_val ts;
    unsigned long long stamp;

    static uint32_t link_type;

    if (fseek(p_info->fp, p_info->pkt_offset, SEEK_SET) != 0){
        printf("read finish \n");
        return -1;
    }

    if (p_info->pkt_offset == 20) {
        fread(&link_type, 4, 1, p_info->fp);
        p_info->pkt_offset += 4;
    }

    if (fread(p_info->pkt_header, 16, 1, p_info->fp) != 1) {
        printf("end of pcap file");
        return -1;
    }
    p_info->pkt_num++;

    p_info->pkt_offset += 16 + p_info->pkt_header->caplen; // next pkt offset

    ts = p_info->pkt_header->ts;
    stamp = (unsigned long long)ts.tv_sec * 1000000 + ts.tv_usec;
    if (p_info->pkt_num == 1) {
        p_info->ts0 = stamp;
    }
    p_info->ts_cur = stamp - p_info->ts0;

    // Ethernet head 14B
    if (link_type == 1) {
        fseek(p_info->fp, 14, SEEK_CUR);
    }

    // IP head 20B
    if (fread(p_info->ip_header, sizeof(ipheader_t), 1, p_info->fp) != 1) {
        printf("pkt %d: can not read ip_header\n", p_info->pkt_num);
        return -1;
    }

    snprintf(p_info->src_ip, 16, "%d.%d.%d.%d",
             p_info->ip_header->srcip & 0xff, (p_info->ip_header->srcip >> 8) & 0xff,
             (p_info->ip_header->srcip >> 16) & 0xff, (p_info->ip_header->srcip >> 24) & 0xff);
    snprintf(p_info->dst_ip, 16, "%d.%d.%d.%d",
             p_info->ip_header->dstip & 0xff, (p_info->ip_header->dstip >> 8) & 0xff,
             (p_info->ip_header->dstip >> 16) & 0xff, (p_info->ip_header->dstip >> 24) & 0xff);


    p_info->tuple[0] = p_info->ip_header->srcip;
    p_info->tuple[1] = p_info->ip_header->dstip;

    p_info->ip_proto = p_info->ip_header->protocol;
    p_info->ip_len = p_info->ip_header->total_len;   // IP total len

    if (p_info->ip_proto != 0x06) {
        return 0;
    }
    // TCP head 20B
    if (fread(p_info->tcp_header, sizeof(tcpheader_t), 1, p_info->fp) != 1) {
        printf("pkt %d: can not read tcp_header\n", p_info->pkt_num);
        return 0;
    }

    p_info->src_port = UINT16_2INT(p_info->tcp_header->srcport);
    p_info->dst_port = UINT16_2INT(p_info->tcp_header->dstport);

    p_info->tcp_flags = p_info->tcp_header->flags;

    return 1;
}

