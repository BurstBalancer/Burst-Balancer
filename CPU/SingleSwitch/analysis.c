#include "analysis.h"

global_para_t g_para;

void display_analysis_result(handle_info_t *handle)
{
    // display
    uint32_t i, j;
    FILE *log;
    int k;
    uint32_t bid_sort[256] = {0}, sorted_n = 0;
    double var_per = 0, mean_per = 0;
	outport_info_t *cur_port_0, *cur_port_z;
	measure_info_t *cur_measure = &handle->measure_data;

    log = fopen("data//output.txt", "a+");

    // print group num sorted by pkt num of each group
    for (i = 0; i < handle->outport_num; i++) {
        if (sorted_n == 0) {
            bid_sort[sorted_n++] = i;
        } else {
            for (j = 0; j < sorted_n; j++) {
                if (bid_sort[j] == i) {
                    continue;
                }
                if (handle->outport[i].pkt_num >= handle->outport[bid_sort[j]].pkt_num) {
                    for (k = sorted_n; k > j; k--) {
                        bid_sort[k] = bid_sort[k - 1];
                    }
                    bid_sort[j] = i;
                    sorted_n++;
                    break;
                }
            }
            if (j == sorted_n) {
                bid_sort[sorted_n++] = i;
            }
        }
    }


    for (i = 0; i < handle->outport_num; i++) {
        uint32_t idx = bid_sort[i];
        outport_info_t *outport = &handle->outport[idx];

        mean_per += (double)outport->pkt_num / (double)cur_measure->total_pkt_num*100.0;
    }
    mean_per = mean_per / (double)handle->outport_num;

    for (i = 0; i < handle->outport_num; i++) {
        uint32_t idx = bid_sort[i];
        outport_info_t *outport = &handle->outport[idx];

        var_per += (mean_per - (double)outport->pkt_num / (double)cur_measure->total_pkt_num*100.0)
                   * (mean_per - (double)outport->pkt_num / (double)cur_measure->total_pkt_num*100.0);
    }

    var_per = var_per / handle->outport_num;

	cur_port_0 = &handle->outport[bid_sort[0]];
	cur_port_z = &handle->outport[bid_sort[handle->outport_num - 1]];

    fprintf(log, "%.4lf%% %.4lf%% %.2lf %d %d %.4lf %d \n",
            (double)cur_port_0->pkt_num / cur_measure->total_pkt_num * 100,
            (double)cur_port_z->pkt_num / cur_measure->total_pkt_num * 100,
            (double)cur_port_0->pkt_num / (double)cur_port_z->pkt_num,
            cur_measure->total_fl_num, cur_measure->total_fl_size,
            (double)cur_measure->total_fl_size / (double)cur_measure->total_pkt_num,
            handle->fl_stat.flowlet_fid_n);

    fprintf(log, "   percent max, min, rate, new flowlet num, total flowlet size, percent, flowlet fid num, disorder num \n");
    fprintf(log, "\n group percent %.4lf%%  %.4lf%%  %.2lf  %d  %d  %.4lf  %d  %d \n\n",
            (double)cur_port_0->pkt_num / cur_measure->total_pkt_num * 100,
            (double)cur_port_z->pkt_num / cur_measure->total_pkt_num * 100,
            (double)cur_port_0->pkt_num / (double)cur_port_z->pkt_num,
            cur_measure->total_fl_num, cur_measure->total_fl_size,
            (double)cur_measure->total_fl_size / (double)cur_measure->total_pkt_num,
            handle->fl_stat.flowlet_fid_n,
            cur_measure->total_disorder_num);

    printf("\n new flowlet num             = %d\n", cur_measure->total_fl_num);
    printf(" flowlet volume in percent   = %.4lf%%\n", (double)cur_measure->total_fl_size / (double)cur_measure->total_pkt_num * 100.0);
    printf(" flowlet enabled flow number = %d\n", handle->fl_stat.flowlet_fid_n);
    printf(" unbalancing-ratio           = %.2lf\n", (double)cur_port_0->pkt_num / (double)cur_port_z->pkt_num);
    printf(" unbalancing-variance        = %.4lf\n", var_per);
    printf(" disorder percent            = %.4lf%%\n", (double)cur_measure->total_disorder_num / (double)handle->pcap_data->pkt_dict->num*100.0);

    for (i = 0; i < handle->outport_num; i++) {
        fprintf(log,"port %d , pkt num = %d\n", i, handle->outport[bid_sort[i]].pkt_num);
    }

    fclose(log);
}

void check_disordered_pkt(handle_info_t *handle)
{
    uint32_t i, j;
    outport_info_t *outport;
    pcap_data_set_t *pcap_data = handle->pcap_data;

    // out of outport buffer
    for (i = 0; i < handle->outport_num; i++) {
        uint32_t len, idx, pkt_ts, old_delay;

        outport = &handle->outport[i];

        old_delay = handle->outport[i].link_delay;

        len = outport->buf_head >= outport->buf_tail ? (outport->buf_head - outport->buf_tail) :
              (outport->buf_len - outport->buf_tail + outport->buf_head);

        for (j = 0; j < len; j++) {
            uint32_t pkt_id;
            unsigned long long arrive_ts;

            idx = j + outport->buf_tail;
            idx = idx >= outport->buf_len ? idx - outport->buf_len : idx;

            pkt_id = outport->buf_list[idx];

            pkt_ts = ((pkt_unit_t *)(pcap_data->pkt_dict->units[pkt_id].data))->ts;

            if (pkt_ts + old_delay <= outport->buf_out_ts) {
                outport->buf_out_ts += 1;
                arrive_ts = outport->buf_out_ts;
            } else {
                arrive_ts = pkt_ts + old_delay;
                outport->buf_out_ts = arrive_ts;
            }

            ((pkt_unit_t *)(pcap_data->pkt_dict->units[pkt_id].data))->arrive_ts = arrive_ts;

        }

    }

    // check disordered packets

    for (i = 0; i < pcap_data->flow_num; i++) {
        flow_unit_t *flow;
        uint32_t cur_max_ts = 0;

        flow = (flow_unit_t *)(pcap_data->flow_dict->units[i].data);

        for (j = 0; j < flow->pkt_num; j++) {
            pkt_unit_t *pkt;

            pkt = (pkt_unit_t *)(pcap_data->pkt_dict->units[flow->pkt_sn[j]].data);

            if (cur_max_ts > pkt->arrive_ts) {
                handle->measure_data.total_disorder_num++;
            } else {
                cur_max_ts = pkt->arrive_ts;
            }

        }
    }

}

void update_link_delay(outport_info_t *outport, pcap_data_set_t *pcap_data, pkt_unit_t *pkt_info)
{
    uint32_t i, flag = 0;
    uint32_t cur_ts = pkt_info->ts;
    uint32_t old_delay, new_delay;

    if (RANDFLOAT0_1 < 0.001) {
        flag = 1;
    }

    // pkt into outport buffer
    outport->buf_list[outport->buf_head] = pkt_info->sn;
    outport->buf_head = outport->buf_head < outport->buf_len - 1 ? outport->buf_head + 1 : 0;

    old_delay = outport->link_delay;
    new_delay = old_delay;

    if (flag == 1){
        if (RANDFLOAT0_1 < g_para.link_delay_majority_percentage) {
            new_delay = g_para.link_delay_range_majority;
        } else {
            new_delay = g_para.link_delay_range_minority;
        }
    }

    uint32_t len, idx, pkt_ts, tmp_tail = outport->buf_tail;

    len = outport->buf_head >= outport->buf_tail ? (outport->buf_head - outport->buf_tail) :
          (outport->buf_len - outport->buf_tail + outport->buf_head);

    for (i = 0; i < len; i++) {
        uint32_t pkt_id, outflag = 0;
        unsigned long long arrive_ts;

        idx = i + outport->buf_tail;
        idx = idx >= outport->buf_len ? idx - outport->buf_len : idx;

        pkt_id = outport->buf_list[idx];

        pkt_ts = ((pkt_unit_t *)(pcap_data->pkt_dict->units[pkt_id].data))->ts;


        if (pkt_ts + old_delay <= cur_ts) {
            if (pkt_ts + old_delay >= outport->buf_out_ts) {
                outport->buf_out_ts = pkt_ts + old_delay;
                arrive_ts = pkt_ts + old_delay;
                outflag = 1;
            } else if (pkt_ts + old_delay <= outport->buf_out_ts && outport->buf_out_ts <= cur_ts) {
                outport->buf_out_ts += 1;
                arrive_ts = outport->buf_out_ts;
                outflag = 1;
            }
        } else if (pkt_ts + new_delay <= cur_ts) {
            if (pkt_ts + new_delay >= outport->buf_out_ts) {
                outport->buf_out_ts = pkt_ts + new_delay;
                arrive_ts = pkt_ts + new_delay;
                outflag = 1;
            } else if (pkt_ts + new_delay <= outport->buf_out_ts && outport->buf_out_ts <= cur_ts) {
                outport->buf_out_ts += 1;
                arrive_ts = outport->buf_out_ts;
                outflag = 1;
            }
        }

        if (outflag == 1) {
            ((pkt_unit_t *)(pcap_data->pkt_dict->units[pkt_id].data))->arrive_ts = arrive_ts;

            // out of outport buffer
            tmp_tail = tmp_tail < outport->buf_len - 1 ? tmp_tail + 1 : 0;
        } else {
            break;
        }
    }
    outport->buf_tail = tmp_tail;
}

void get_pkt_info(pkt_unit_t *pkt_info, pcap_data_set_t *pcap_data, uint32_t i)
{

    // transform pkt info
    pkt_info->ts = ((pkt_unit_t *)(pcap_data->pkt_dict->units[i].data))->ts;
    pkt_info->tuple[0] = ((pkt_unit_t *)(pcap_data->pkt_dict->units[i].data))->tuple[0];
    pkt_info->tuple[1] = ((pkt_unit_t *)(pcap_data->pkt_dict->units[i].data))->tuple[1];
    pkt_info->sn = i;

}

void record_measure_value(handle_info_t *handle, uint32_t outport_id)
{
    outport_info_t *outport;

    outport = &handle->outport[outport_id];
    outport->pkt_num++;

    handle->measure_data.total_pkt_num++;
}

void record_pcap_into_dataset(pcap_data_set_t *pcap_data, pcap_info_t *pinfo, uint32_t sn)
{
    pkt_unit_t pkt = {0};

    pkt.sn = sn;
    pkt.ts = pinfo->ts_cur;
    pkt.arrive_ts = 0;
    memcpy(pkt.tuple , pinfo->tuple, sizeof(pkt.tuple));


    // insert packet
    insert_data(pcap_data->pkt_dict, &pkt, sizeof(pkt_unit_t));

    // record flow
    uint32_t tuple[5] = {0}, fid;

    tuple[0] = pinfo->tuple[0];
    tuple[1] = pinfo->tuple[1];

    uint32_t idx = -1;

    idx = search_idx_map(pcap_data->flow_dict, tuple, sizeof(tuple));

    if (idx == -1) {
        flow_unit_t flow = {0};
        flow.fid = pcap_data->flow_dict->num;
        fid = flow.fid;

        flow.tuple[0] = pinfo->tuple[0];
        flow.tuple[1] = pinfo->tuple[1];

        flow.pkt_max_num = FLOW_INC_NUM;
        MALLOC_AND_SET(flow.pkt_sn, flow.pkt_max_num * sizeof(uint32_t), uint32_t *);

        flow.pkt_sn[0] = sn;
        flow.pkt_num = 1;

        flow.disorder_pkt_num = 0;
        flow.cur_max_delay = 0;
        flow.last_pkt_ts = 0;
        flow.last_pkt_arrival = 0;

        insert_idx_map(pcap_data->flow_dict, tuple, sizeof(tuple), flow.fid);

        insert_data(pcap_data->flow_dict, &flow, sizeof(flow_unit_t));

    } else {
        flow_unit_t *flow = (flow_unit_t *)(pcap_data->flow_dict->units[idx].data);

        if (flow->pkt_num >= flow->pkt_max_num) {
            uint32_t *tmp = flow->pkt_sn;
            MALLOC_AND_SET(flow->pkt_sn, (flow->pkt_max_num + INC_NUM) * sizeof(uint32_t), uint32_t *)

            memcpy(flow->pkt_sn, tmp, flow->pkt_max_num * sizeof(uint32_t));
            free(tmp);
            flow->pkt_max_num += INC_NUM;
        }
        flow->pkt_sn[flow->pkt_num] = sn;
        flow->pkt_num++;
        fid = flow->fid;
    }

    pkt_unit_t *tmppkt;

    tmppkt = ((pkt_unit_t *)(pcap_data->pkt_dict->units[pcap_data->pkt_dict->num - 1].data));

    tmppkt->fid = fid;
}

pcap_data_set_t *read_pcap_file(char file[60], uint32_t num)
{
    int ret, sn = 0;

    pcap_info_t *pinfo;
    pcap_data_set_t *pcap_data;


    pinfo = init_pcap_file(file);

    MALLOC_AND_SET(pcap_data, sizeof(pcap_data_set_t), pcap_data_set_t *)

    init_dataset(pcap_data);


    while (pinfo->pkt_num < num) {
        ret = pcap_get_next_pkt(pinfo);
        if (ret == -1) {
            break;
        }
        if (pinfo->pkt_num % (num/10) == 0) {
            printf(" read pcap pkt num = %d , %.2lf%%\n",
                   pinfo->pkt_num, (double)(pinfo->pkt_num)/(double)num*100);
        }
        record_pcap_into_dataset(pcap_data, pinfo, sn);

        sn++;
    }

    pcap_data->flow_num = pcap_data->flow_dict->num;

    return pcap_data;
}

void per_flow_lb(handle_info_t *handle)
{
    pcap_data_set_t *pcap_data = handle->pcap_data;
    uint32_t i, total_pkt_num = pcap_data->pkt_dict->num;

    // process each packet
    for (i = 0; i < total_pkt_num; i++) {
        uint32_t gid, outport_id;
        pkt_unit_t pkt_info = {0};

        // transform pkt info
        get_pkt_info(&pkt_info, pcap_data, i);


        // choose a group based on per flow
        // gid = wrand(handle, pkt_info.tuple);
        outport_id = gid = get_group_id(handle, &pkt_info);

        // record group's pkt num
        record_measure_value(handle, outport_id);

        // change link delay info
        update_link_delay(&handle->outport[outport_id], pcap_data, &pkt_info);
    }

}

void per_pkt_lb(handle_info_t *handle)
{
    pcap_data_set_t * pcap_data = handle->pcap_data;
    uint32_t i, total_pkt_num = pcap_data->pkt_dict->num;

    // process each packet
    for (i = 0; i < total_pkt_num; i++) {
        uint32_t gid, outport_id;
        pkt_unit_t pkt_info = {0};

        // transform pkt info
        get_pkt_info(&pkt_info, pcap_data, i);

        // choose a group based on per pkt
        pkt_info.tuple[2] = i;
        outport_id = gid = get_group_id(handle, &pkt_info);

        // record group's pkt num
        record_measure_value(handle, outport_id);

        // change link delay info
        update_link_delay(&handle->outport[outport_id], pcap_data, &pkt_info);

    }
}

void per_flowlet_lb(handle_info_t *handle)
{
    pcap_data_set_t * pcap_data = handle->pcap_data;
    uint32_t i, total_pkt_num = pcap_data->pkt_dict->num;
    uint32_t new_flowlet_pktnum = 0, redisp_flow_num = 0;

    uint32_t *redispatch_flow_array;
    
    MALLOC_AND_SET(redispatch_flow_array, sizeof(uint32_t) * (MAX_REDISP_NUM), uint32_t *)

    // process each packet
    for (i = 0; i < total_pkt_num; i++) {
        uint32_t outport_id;
        pkt_unit_t pkt_info = {0};


        // transform pkt info
        get_pkt_info(&pkt_info, pcap_data, i);


        // flowburst detect module
        handle->heavy_detect_func(handle, &pkt_info);

        // route by flowlet
        outport_id = handle->route_by_flowlet(handle, &pkt_info);

        // new flowlet pktnum percent in all pkt
        uint32_t ecmp_id = get_group_id(handle, &pkt_info);

        if (outport_id != ecmp_id) {
            new_flowlet_pktnum++;

            uint32_t tmp_id;

            tmp_id = HASH_FUN_G(pkt_info.tuple, sizeof(pkt_info.tuple), POLY1, MAX_REDISP_NUM);

            redispatch_flow_array[tmp_id] = 1;
        }

        // record group's pkt num
        record_measure_value(handle, outport_id);

        // change link delay info
        update_link_delay(&handle->outport[outport_id], pcap_data, &pkt_info);

    }

    for (i = 0; i < MAX_REDISP_NUM; i++) {
        if (redispatch_flow_array[i] == 1) {
            redisp_flow_num++;
        }
    }

    printf("\n redispatch pkt num = %d , %.2lf %% , redispatch flow num = %d, %.2lf %%\n",
           new_flowlet_pktnum, (double)new_flowlet_pktnum/(double)total_pkt_num * 100.0,
           redisp_flow_num, (double)redisp_flow_num/(double)handle->pcap_data->flow_num * 100.0);

    free(redispatch_flow_array);

}

