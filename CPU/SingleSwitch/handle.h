#ifndef SRC_CODE_HANDLE_H
#define SRC_CODE_HANDLE_H


#include "common.h"
#include "data_set.h"
#include "packet.h"


#define MAX_PORT_NUM 128

#define HASH_FUN_G(tuple, len, poly, size) new_loop((uint8_t *)(tuple), len, poly) % size

enum DETECT_TYPE {BurstBalaner = 1001, DEF_LETFLOW};

typedef struct group_info_tag
{
    uint32_t group_id;
    uint32_t delta;

    uint32_t linked_op_num;
    uint32_t linked_outport_id[MAX_PORT_NUM];
}group_info_t;

typedef struct outport_info_tag
{
    uint32_t outport_id;
    uint32_t link_delay;
    uint32_t pkt_num;
    uint32_t flow_num;
    uint32_t max_flow_num;
    flow_info_t *flow_list;

    pkt_info_t pkt_info;

    unsigned long long buf_out_ts;
    uint32_t buf_head;
    uint32_t buf_tail;
    uint32_t buf_len;
    uint32_t *buf_list;
}outport_info_t;

typedef struct gslot_tag
{
    uint32_t ck;
    uint32_t cnt;
    uint32_t cnt_max;
    uint32_t lts;
    uint32_t laddr;
    uint32_t pnum;
}gslot_t;

typedef struct nxm_grid_tag
{
    uint32_t size;
    uint32_t depth;
    gslot_t **gslot;
}nxm_grid_t;

typedef struct flt_grid_tag
{
    uint32_t size;
    uint32_t depth;
    gslot_t **gslot;
}flt_grid_t;

typedef struct measurc_info_tag
{
    uint32_t total_flow_num;
    uint32_t total_pkt_num;

    uint32_t total_fl_num;
    uint32_t total_fl_size;

    uint32_t total_disorder_num;

}measure_info_t;

typedef struct flowlet_stat_tag
{
    uint32_t max_fl_n;
    flow_info_t *flowlet_array;    // record flowlet info

    uint32_t flowlet_fid_n;        // statistic flowlet relevant flowid
    uint32_t **flowlet_fid_list;

}flowlet_stat_t;

typedef struct para_heavyflow_detect_tag
{
    uint32_t fl_type;
    uint32_t burstblc_rows;
    uint32_t num;
}para_heavyflow_detect_t;

typedef struct para_fl_table_tag
{
    uint32_t size;
    uint32_t depth;
}para_fl_table_t;

typedef struct para_integrate_tag
{
    void *heavy_detect_para;    // heavy flow detect para point

    void *flowlet_table_para;    // flowlet table para point

}para_integrate_t;

typedef struct handle_info_tag
{
    pcap_data_set_t *pcap_data;    // pcap_data point

    para_integrate_t para;         // parameter for multi module, user configure

    flt_grid_t flt_grid;           // flowlet table

    void *heavy_detect_grid;        // heavy flow detect module grid

    void (*init_hfd_grid)(void *, void *);        // init heavy detect grid

    void (*heavy_detect_func)(void *, void *);        // heavy detect func module point
    uint32_t (*route_by_flowlet)(void *, void *);     // route by flowlet	

    uint32_t group_num;
    group_info_t group[MAX_PORT_NUM];

    uint32_t outport_num;
    outport_info_t outport[MAX_PORT_NUM];

    measure_info_t measure_data;

    flowlet_stat_t fl_stat;        // record flowlet distribution

    FILE *flog;
}handle_info_t;

handle_info_t* init_handle(pcap_data_set_t *pcap_data);

void free_handle(handle_info_t **handle);

uint32_t new_loop(uint8_t *key, size_t length, uint32_t poly);

uint32_t flowlet_table_cnt(handle_info_t *handle, pkt_unit_t *pinfo);

void flowburst_detect(void *para, void *info);

void dummy_flowburst_by_letflow(void *para, void *info);

uint32_t get_group_id(handle_info_t *handle, pkt_unit_t *pinfo);

void configue_parameter(handle_info_t *handle, para_integrate_t *para);

void *init_heavy_detect_para();


#endif // SRC_CODE_HANDLE_H
