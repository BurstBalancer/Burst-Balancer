#ifndef SRC_CODE_DATA_SET_H
#define SRC_CODE_DATA_SET_H


#define MAX_DICT_NUM 1000000
#define MAX_IDX_NUM 10000
#define INC_NUM 1000
#define DICT_INC_NUM 100000
#define FLOW_INC_NUM 100
#define MAX_BUF_LEN 10240000
#define MAX_LIST_LEN 102400

typedef struct data_unit_tag
{
    uint32_t len;
    void *data;
}data_unit_t;

typedef struct idx_tag
{
    uint32_t tuple[5];
    uint32_t idx;
    unsigned long long ck;
}idx_t;

typedef struct idx_map_tag
{
    uint32_t max_num;
    uint32_t num;
    idx_t *idx_list;
}idx_map_t;

typedef struct data_dict_tag
{
    uint32_t max_num;
    uint32_t num;

    data_unit_t *units;

    uint32_t idxm_num;
    idx_map_t *idx_map;
}data_dict_t;

typedef struct pkt_unit_tag
{
    uint32_t tuple[5];
    uint32_t ts;
    uint32_t arrive_ts;
    uint32_t sn;
    uint32_t fid;

    uint32_t ele_flow_flag;
    uint32_t row_id;
}pkt_unit_t;

typedef struct flow_unit_tag
{
    uint32_t tuple[5];
    uint32_t fid;

    uint32_t cur_max_delay;
    uint32_t last_pkt_ts;
    uint32_t last_pkt_arrival;
    uint32_t disorder_pkt_num;

    uint32_t pkt_num;
    uint32_t pkt_max_num;
    uint32_t *pkt_sn;
}flow_unit_t;

typedef struct pcap_data_set_tag
{
    data_dict_t *pkt_dict;
    data_dict_t *flow_dict;

    uint32_t flow_num;
}pcap_data_set_t;

typedef struct pkt_info_tag
{
    uint32_t pkt_num;
    uint32_t max_pkt_num;
    uint32_t *pkt_ts_list;
}pkt_info_t;

typedef struct flow_info_tag
{
    uint32_t flow_id;
    uint32_t tuple[5];
    uint32_t record_pkt_num;

    pkt_info_t pkt_info; // pkt ts info
}flow_info_t;


void init_dataset(pcap_data_set_t *pcap_data);
void free_dataset(pcap_data_set_t **pcap_data);

void insert_data(data_dict_t *dict, void *data, uint32_t len);
void insert_idx_map(data_dict_t *dict, void *data, uint32_t len, uint32_t index);
uint32_t search_data(data_dict_t *dict, void *data, uint32_t len);
uint32_t search_idx_map(data_dict_t *dict, void *data, uint32_t len);


#endif // SRC_CODE_DATA_SET_H
