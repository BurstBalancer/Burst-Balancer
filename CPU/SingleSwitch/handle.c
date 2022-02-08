#include "handle.h"

uint32_t new_loop(uint8_t *key, size_t length, uint32_t poly)
{
    size_t i = 0;
    uint32_t hash = poly;

    while (i != length) {
        hash += key[i++];
        hash += (((hash)<<(5)) | ((hash)>>(32-(5))));
        hash += hash << 11;
        hash ^= hash >> 7;
    }
    return hash;
}

handle_info_t* init_handle(pcap_data_set_t *pcap_data)
{
    int i, j;
    handle_info_t *handle;

    handle = (handle_info_t *)malloc(sizeof(handle_info_t));

    handle->pcap_data = pcap_data;

    handle->measure_data.total_disorder_num = 0;
    handle->measure_data.total_pkt_num = 0;
    handle->measure_data.total_flow_num = 0;
    handle->measure_data.total_fl_num = 0;
    handle->measure_data.total_fl_size = 0;

    handle->fl_stat.max_fl_n = INC_NUM;

    MALLOC_AND_SET(handle->fl_stat.flowlet_array, handle->fl_stat.max_fl_n * sizeof(flow_info_t), flow_info_t *)

    handle->fl_stat.flowlet_fid_n = 0;

    handle->fl_stat.flowlet_fid_list = (uint32_t **)malloc(MAX_LIST_LEN * sizeof(uint32_t *));
    for (i = 0; i < MAX_LIST_LEN; i++) {
        MALLOC_AND_SET(handle->fl_stat.flowlet_fid_list[i], FLOW_INC_NUM * sizeof(uint32_t), uint32_t *)
    }

    // init group
    handle->group_num = MAX_PORT_NUM;
    for (i = 0; i < handle->group_num; i++) {
        handle->group[i].group_id = i;
        handle->group[i].delta = g_para.delta;
        handle->group[i].linked_op_num = MAX_PORT_NUM;
        for (j = 0; j < handle->group[i].linked_op_num; j++) {
            handle->group[i].linked_outport_id[j] = j;
        }
    }

    // init outport
    handle->outport_num = MAX_PORT_NUM;

    for (i = 0; i < handle->outport_num; i++) {
        handle->outport[i].outport_id = i;

        handle->outport[i].pkt_num = 0;
        handle->outport[i].flow_num = 0;
        handle->outport[i].max_flow_num = FLOW_INC_NUM;

        MALLOC_AND_SET(handle->outport[i].flow_list, handle->outport[i].max_flow_num * sizeof(flow_info_t), flow_info_t *)

        handle->outport[i].pkt_info.pkt_num = 0;
        handle->outport[i].pkt_info.max_pkt_num = INC_NUM;
        
        MALLOC_AND_SET(handle->outport[i].pkt_info.pkt_ts_list, handle->outport[i].pkt_info.max_pkt_num * sizeof(uint32_t), uint32_t *)

        if (RANDFLOAT0_1 < g_para.link_delay_majority_percentage) {
            handle->outport[i].link_delay = g_para.link_delay_range_majority;
        } else {
            handle->outport[i].link_delay = g_para.link_delay_range_minority;
        }

        handle->outport[i].buf_out_ts = 0;
        handle->outport[i].buf_head = 0;
        handle->outport[i].buf_tail = 0;
        handle->outport[i].buf_len = MAX_BUF_LEN;
        handle->outport[i].buf_list = (uint32_t *)malloc(handle->outport[i].buf_len * sizeof(uint32_t));

    }

    srand(g_para.srand_value);

    handle->flog = fopen("data//output.txt", "w");

    return handle;
}

void free_handle(handle_info_t **handle)
{
    uint32_t i;
    handle_info_t *p;

    p = *handle;
    free(p->fl_stat.flowlet_array);
    for (i = 0; i < MAX_LIST_LEN; i++) {
        free(p->fl_stat.flowlet_fid_list[i]);
    }
    free(p->fl_stat.flowlet_fid_list);

    for (i = 0; i < p->outport_num; i++) {
        free(p->outport[i].flow_list);
        free(p->outport[i].pkt_info.pkt_ts_list);
    }

    fclose(p->flog);

    free(p);
}

uint32_t wrand(handle_info_t *handle, uint32_t tuple[5])
{
    uint32_t i, ret = 0;
    uint32_t max = 0, arr_n = 0, rdm;
    uint32_t arr[128] = {0};


    for (i = 0; i < handle->outport_num; i++) {
        if (handle->outport[i].pkt_num > max) {
            max = handle->outport[i].pkt_num;
        }
    }

    for (i = 0; i < handle->outport_num; i++) {
        arr_n += max + 1 - handle->outport[i].pkt_num;
        arr[i] = arr_n;
    }

    rdm = HASH_FUN_G(tuple, 5 * sizeof(uint32_t), POLY1, arr_n);

    for (i = 0; i < handle->outport_num; i++) {
        if (rdm < arr[i]) {
            return i;
        }
    }
    return ret;
}

void record_flowlet_distribution(handle_info_t *handle, gslot_t *gslot, uint32_t ck)
{
    if (handle->measure_data.total_fl_num < handle->fl_stat.max_fl_n) {
        handle->fl_stat.flowlet_array[handle->measure_data.total_fl_num].record_pkt_num = gslot->pnum;
        handle->fl_stat.flowlet_array[handle->measure_data.total_fl_num].flow_id = ck;
    } else {
        flow_info_t *tmpf;
        tmpf = handle->fl_stat.flowlet_array;
        handle->fl_stat.flowlet_array = (flow_info_t *)malloc((handle->fl_stat.max_fl_n + 1000) * sizeof(flow_info_t));
        memcpy(handle->fl_stat.flowlet_array, tmpf, handle->fl_stat.max_fl_n * sizeof(flow_info_t));
        handle->fl_stat.max_fl_n += 1000;
        handle->fl_stat.flowlet_array[handle->measure_data.total_fl_num].record_pkt_num = gslot->pnum;
        handle->fl_stat.flowlet_array[handle->measure_data.total_fl_num].flow_id = ck;
        free(tmpf);
    }

    handle->measure_data.total_fl_size += gslot->pnum;

    handle->measure_data.total_fl_num++;

    // statistic flowlet fid num
    uint32_t *fid_list = handle->fl_stat.flowlet_fid_list[ck % MAX_LIST_LEN];

    for (uint32_t j = 0; j < 100; j++) {
        if (fid_list[j] == ck) {
            break;
        }
        if (fid_list[j] == 0) {
            fid_list[j] = ck;
            handle->fl_stat.flowlet_fid_n++;
            break;
        }
    }

}

uint32_t get_group_delta(handle_info_t *handle, pkt_unit_t *pinfo)
{
    uint32_t gid, delta;

    gid = HASH_FUN_G(pinfo->tuple, sizeof(pinfo->tuple), POLY1, handle->group_num);

    delta = handle->group[gid].delta;

    return delta;
}

uint32_t get_group_id(handle_info_t *handle, pkt_unit_t *pinfo)
{
    uint32_t gid;

    gid = HASH_FUN_G(pinfo->tuple, sizeof(pinfo->tuple), POLY1, handle->group_num);

    return gid;
}

uint32_t do_letflow(void *handle_ptr, void *pinfo_ptr)
{
	handle_info_t *handle = (handle_info_t *)handle_ptr;
	pkt_unit_t *pinfo = (pkt_unit_t *)pinfo_ptr;

    uint32_t row_id, slot_id, ck, ret, default_portid, gid, type;
    gslot_t *gslot;

    uint32_t size, depth, delta, table_size;
    uint32_t ele_flow_flag = pinfo->ele_flow_flag;

    delta = get_group_delta(handle, pinfo);

    size = handle->flt_grid.size;
    depth = handle->flt_grid.depth;

    type = ((para_heavyflow_detect_t *)(handle->para.heavy_detect_para))->fl_type;

    if (type == BurstBalaner) {
        table_size = ((nxm_grid_t *)handle->heavy_detect_grid)->size;

        if (size <= table_size) {
            row_id = pinfo->row_id * size / table_size;
        } else {
            row_id = pinfo->row_id * size / table_size +
                     HASH_FUN_G(pinfo->tuple, sizeof(pinfo->tuple), POLY3, (int)(size/table_size));
        }
    } else {
        row_id = HASH_FUN_G(pinfo->tuple, sizeof(pinfo->tuple), POLY1, size);
    }
    slot_id = HASH_FUN_G(pinfo->tuple, sizeof(pinfo->tuple), POLY2, depth);

    if (type != DEF_LETFLOW) {
        ck = HASH_FUN_G(pinfo->tuple, sizeof(pinfo->tuple), POLY2, 0xfffffffd) + 1;
    } else {
        ck = 1;
    }

    ck = HASH_FUN_G(pinfo->tuple, sizeof(pinfo->tuple), POLY2, 0xfffffffd) + 1;

    default_portid = gid = get_group_id(handle, pinfo);

    gslot = handle->flt_grid.gslot[row_id];

    if (gslot[slot_id].ck == ck) {
        if (gslot[slot_id].lts + delta <= pinfo->ts) {

            // a new flowlet
            ret = wrand(handle, pinfo->tuple);

            // record flowlet size distribution
            if (gslot[slot_id].pnum > 0){
                uint32_t tck = HASH_FUN_G(pinfo->tuple, sizeof(pinfo->tuple), POLY2, 0xffffffff) + 1;
                record_flowlet_distribution(handle, &gslot[slot_id], tck);
            }

            gslot[slot_id].lts = pinfo->ts;
            gslot[slot_id].laddr = ret;
            gslot[slot_id].pnum = 1;

            return ret;
        } else {
            // still in the last flowlet

            gslot[slot_id].pnum++;
            gslot[slot_id].lts = pinfo->ts;
            ret = gslot[slot_id].laddr;
            return ret;
        }
    }

    if (gslot[slot_id].ck != 0) {
        default_portid = gslot[slot_id].laddr;
    } else {
        gslot[slot_id].laddr = wrand(handle, pinfo->tuple);
        default_portid = gslot[slot_id].laddr;
    }

    if (ele_flow_flag == 0) {
        return default_portid;
    }

    // find empty
    if (gslot[slot_id].ck == 0) {
        gslot[slot_id].ck = ck;
        gslot[slot_id].lts = pinfo->ts;
        ret = default_portid;


        gslot[slot_id].laddr = ret;
        gslot[slot_id].pnum = 0;
        return ret;
    }
    
    // replace
    if (pinfo->ts - gslot[slot_id].lts >= delta) {
        gslot[slot_id].ck = ck;
        gslot[slot_id].lts = pinfo->ts;
        ret = default_portid;

        gslot[slot_id].laddr = ret;
        gslot[slot_id].pnum = 0;
        return ret;
    }

    ret = default_portid;
    return ret;

}

uint32_t do_burstbalancer(void *handle_ptr, void *pinfo_ptr)
{
	handle_info_t *handle = (handle_info_t *)handle_ptr;
	pkt_unit_t *pinfo = (pkt_unit_t *)pinfo_ptr;

    uint32_t addr, ck, ret, default_portid, gid;
    uint32_t i;
    gslot_t *gslot;

    uint32_t size, depth, delta;

    uint32_t ele_flow_flag = pinfo->ele_flow_flag;

    delta = get_group_delta(handle, pinfo);

    size = handle->flt_grid.size;
    depth = handle->flt_grid.depth;

    addr = HASH_FUN_G(pinfo->tuple, sizeof(pinfo->tuple), POLY1, size);

    ck = HASH_FUN_G(pinfo->tuple, sizeof(pinfo->tuple), POLY2, 0xfffffffd) + 1;

    default_portid = gid = get_group_id(handle, pinfo);

    gslot = handle->flt_grid.gslot[addr];

    for (i = 0; i <depth; i++) {
        if (gslot[i].ck == ck) {
            if (pinfo->ts - gslot[i].lts >= delta) {
                // a new flowlet
                ret = wrand(handle, pinfo->tuple);

                // record flowlet size distribution
                if (gslot[i].pnum > 0){
                    record_flowlet_distribution(handle, &gslot[i], ck);
                }

                gslot[i].lts = pinfo->ts;
                gslot[i].laddr = ret;
                gslot[i].pnum = 1;

                return ret;
            } else {
                // belongs to the existing (old) flowlet

                gslot[i].pnum++;
                gslot[i].lts = pinfo->ts;
                ret = gslot[i].laddr;
                return ret;
            }
        }
    }

    // it's not in flowlet table, if is not a flowburst , then follow ECMP
    if (ele_flow_flag == 0) {
        return default_portid;
    }

    // find empty item
    for (i = 0; i < depth; i++) {
        if (gslot[i].ck == 0) {
            gslot[i].ck = ck;
            gslot[i].lts = pinfo->ts;
            ret = default_portid;


            gslot[i].laddr = ret;
            gslot[i].pnum = 0;
            return ret;
        }
    }

    // replace
    for (i = 0; i < depth; i++) {
        if (pinfo->ts - gslot[i].lts >= delta) {
            gslot[i].ck = ck;
            gslot[i].lts = pinfo->ts;
            ret = default_portid;

            gslot[i].laddr = ret;
            gslot[i].pnum = 0;
            return ret;
        }
    }

    ret = default_portid;
    return ret;

}

void configue_burstbalancer(void *handle_ptr, void *para)
{
    para_heavyflow_detect_t *para_hf = (para_heavyflow_detect_t *)para;
	handle_info_t *handle = (handle_info_t *)handle_ptr;

    nxm_grid_t *nxm_grid;

    nxm_grid = (nxm_grid_t *)malloc(sizeof(nxm_grid_t));
    nxm_grid->size = para_hf->burstblc_rows;
    nxm_grid->depth = 1;

    nxm_grid->gslot = (gslot_t **)malloc(nxm_grid->size * sizeof(gslot_t *));

    for (uint32_t i = 0; i < nxm_grid->size; i++) {
        MALLOC_AND_SET(nxm_grid->gslot[i], nxm_grid->depth * sizeof(gslot_t), gslot_t *)
    }

    handle->heavy_detect_grid = nxm_grid;
}

void configue_letflow_table(handle_info_t *handle, void *para)
{
    para_fl_table_t *para_fl = (para_fl_table_t *)para;

    handle->flt_grid.size = para_fl->size;
    handle->flt_grid.depth = para_fl->depth;

    handle->flt_grid.gslot = (gslot_t **)malloc(para_fl->size * sizeof(gslot_t *));

    for (uint32_t i = 0; i < para_fl->size; i++) {
        MALLOC_AND_SET(handle->flt_grid.gslot[i], para_fl->depth * sizeof(gslot_t), gslot_t *)
    }
}

void configue_parameter(handle_info_t *handle, para_integrate_t *para)
{
    para_heavyflow_detect_t *para_hf = (para_heavyflow_detect_t *)(para->heavy_detect_para);
    para_fl_table_t *para_fl = (para_fl_table_t *)(para->flowlet_table_para);

    handle->para.heavy_detect_para = para->heavy_detect_para;
    handle->para.flowlet_table_para = para->flowlet_table_para;

    // configue heave flow detect module function
    uint32_t type = para_hf->fl_type;

    if (type == DEF_LETFLOW) {
        handle->heavy_detect_func = dummy_flowburst_by_letflow;
    } else {
        handle->init_hfd_grid = configue_burstbalancer;
        handle->heavy_detect_func = flowburst_detect;

        handle->init_hfd_grid(handle, para_hf);
    }

    configue_letflow_table(handle, para_fl);

    if (type == DEF_LETFLOW) {
        handle->route_by_flowlet = do_letflow;
    } else {
        handle->route_by_flowlet = do_burstbalancer;
    }

}

// each hash bucket (row) has only one slot
void flowburst_detect(void *para, void *info)
{
    handle_info_t *handle = (handle_info_t *)para;
    pkt_unit_t *pkt_info = (pkt_unit_t *)info;

    nxm_grid_t *grid = (nxm_grid_t *)handle->heavy_detect_grid;

    uint32_t addr, ck, cnt_max = 31, k = 10240;
    gslot_t *slot;

    addr = HASH_FUN_G(pkt_info->tuple, sizeof(pkt_info->tuple), POLY1, grid->size);

    ck = HASH_FUN_G(pkt_info->tuple, sizeof(pkt_info->tuple), POLY2, 0xffffffff) + 1;

    slot = &grid->gslot[addr][0];

    pkt_info->row_id = addr;

    if (slot->ck == ck) {
        if (slot->cnt < cnt_max) {
            slot->cnt++;
        }
        if (slot->cnt >= cnt_max) {
            pkt_info->ele_flow_flag = 1;
            slot->pnum = 1;
            slot->cnt = 0;
        } else {
            pkt_info->ele_flow_flag = 1;
        }
        return ;
    }

    if ((pkt_info->ts - slot->lts > k) || (slot->cnt == 0 && slot->pnum == 0)) {
        slot->ck = ck;
        slot->cnt = 1;
        pkt_info->ele_flow_flag = 0;
        return;
    }

    // aging
    for (uint32_t i = 0; i < grid->size; i++) {
        slot = &grid->gslot[i][0];
        if (slot->pnum == 1) {
            continue;
        }
        if (slot->cnt > 0) {
            slot->cnt--;
        }
    }
    pkt_info->ele_flow_flag = 0;
}

void dummy_flowburst_by_letflow(void *para, void *info)
{
    pkt_unit_t *pkt_info = (pkt_unit_t *)info;

    pkt_info->ele_flow_flag = 1;
}

