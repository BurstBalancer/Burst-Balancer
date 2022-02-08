#include "handle.h"
#include "data_set.h"

// init
void init_dataset(pcap_data_set_t *pcap_data)
{
    uint32_t i, j;


    for (j = 0; j < 2; j++) {
        data_dict_t *dict;

        MALLOC_AND_SET(dict, sizeof(data_dict_t), data_dict_t *)

        dict->max_num = MAX_DICT_NUM;
        dict->num = 0;

        MALLOC_AND_SET(dict->units, dict->max_num * sizeof(data_unit_t), data_unit_t *)

        dict->idxm_num = MAX_IDX_NUM;

        MALLOC_AND_SET(dict->idx_map, dict->idxm_num * sizeof(idx_map_t), idx_map_t *)

        for (i = 0; i < dict->idxm_num; i++) {
            dict->idx_map[i].max_num = MAX_IDX_NUM;
            dict->idx_map[i].num = 0;

            MALLOC_AND_SET(dict->idx_map[i].idx_list, dict->idx_map[i].max_num * sizeof(idx_t), idx_t *)
        }

        if (j == 0) {
            pcap_data->pkt_dict = dict;
        } else {
            pcap_data->flow_dict = dict;
        }
    }
}

// free memory
void free_dataset(pcap_data_set_t **pcap_data)
{
    uint32_t i, j;
    pcap_data_set_t *ps = *pcap_data;

    for (i = 0; i < 2; i++) {
        data_dict_t *d;

        if (i == 0) {
            d = ps->pkt_dict;
        }else {
            d = ps->flow_dict;
        }

        for (j = 0; j < d->num; j++) {
            free(d->units[j].data);
        }
        free(d->units);
        free(d);
    }
}

void insert_idx_map(data_dict_t *dict, void *data, uint32_t len, uint32_t index)
{
    uint32_t addr;
    unsigned long long ck = 0;
    idx_map_t *idx_map;
	
    ck = new_loop(data, len, POLY4) | ((unsigned long long)(new_loop(data, len, POLY5)) << 32) ;

    addr = HASH_FUN_G(data, len, POLY1, dict->idxm_num);

    idx_map = &dict->idx_map[addr];

    if (idx_map->num >= idx_map->max_num) {
        idx_t *tmp;

        tmp = idx_map->idx_list;

        MALLOC_AND_SET(idx_map->idx_list, (idx_map->max_num + INC_NUM) * sizeof(idx_t), idx_t *)

        memcpy(idx_map->idx_list, tmp, idx_map->max_num * sizeof(idx_t));
        idx_map->max_num += INC_NUM;
    }

    idx_map->idx_list[idx_map->num].idx = index;
    idx_map->idx_list[idx_map->num].ck = ck;
    idx_map->num++;

}

uint32_t search_idx_map(data_dict_t *dict, void *data, uint32_t len)
{
    uint32_t i;
    uint32_t addr;
    unsigned long long ck = 0;
    idx_map_t *idx_map;

    ck = new_loop(data, len, POLY4) | (((unsigned long long)new_loop(data, len, POLY5)) << 32) ;

    addr = HASH_FUN_G(data, len, POLY1, dict->idxm_num);

    idx_map = &dict->idx_map[addr];

    for (i = 0; i < idx_map->num; i++) {
        if (idx_map->idx_list[i].ck == ck) {
            return idx_map->idx_list[i].idx;
        }
    }
    return -1;
}

// insert
void insert_data(data_dict_t *dict, void *data, uint32_t len)
{
    if (dict->num >= dict->max_num) {
        data_unit_t *tmp;

        tmp = dict->units;

        MALLOC_AND_SET(dict->units, (dict->max_num + DICT_INC_NUM) * sizeof(data_unit_t), data_unit_t *)

        memcpy(dict->units, tmp, dict->max_num * sizeof(data_unit_t));

        dict->max_num += DICT_INC_NUM;
    }
    dict->units[dict->num].len = len;
    dict->units[dict->num].data = (void *)malloc(len);
    memcpy(dict->units[dict->num].data, data, len);
    dict->num++;
}


// search
// in : return idx
// not in : return -1
uint32_t search_data(data_dict_t *dict, void *data, uint32_t len)
{
    uint32_t i, addr, idx = -1;
    idx_map_t *idx_map;
    unsigned long long ck;

    ck = new_loop(data, len, POLY4) | (((unsigned long long)new_loop(data, len, POLY5)) << 32) ;

    addr = HASH_FUN_G(data, len, POLY1, dict->idxm_num);

    idx_map = &dict->idx_map[addr];

    for (i = 0; i < idx_map->num; i++) {
        if (idx_map->idx_list[i].ck == ck) {
            idx = idx_map->idx_list[i].idx;
            break;
        }
    }
    return idx;
}

void record_pkt_info(pcap_info_t *pinfo, pkt_info_t *pkt_info)
{
    if (pkt_info->pkt_num >= pkt_info->max_pkt_num) {
        uint32_t *tmp = pkt_info->pkt_ts_list;

        MALLOC_AND_SET(pkt_info->pkt_ts_list, (pkt_info->max_pkt_num + INC_NUM) * sizeof(uint32_t), uint32_t *)

        memcpy(pkt_info->pkt_ts_list, tmp, pkt_info->max_pkt_num * sizeof(uint32_t));
        free(tmp);
        pkt_info->max_pkt_num += INC_NUM;
    }
    pkt_info->pkt_ts_list[pkt_info->pkt_num++] = pinfo->ts_cur;
}
