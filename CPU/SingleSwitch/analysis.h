#ifndef SRC_CODE_ANALYSIS_H
#define SRC_CODE_ANALYSIS_H


#include "packet.h"
#include "handle.h"
#include "data_set.h"


#define MAX_REDISP_NUM 0xfffff


void per_flowlet_lb(handle_info_t *handle);
void per_flow_lb(handle_info_t *handle);
void per_pkt_lb(handle_info_t *handle);

void check_disordered_pkt(handle_info_t *handle);

void display_analysis_result(handle_info_t *handle);
pcap_data_set_t *read_pcap_file(char file[60], uint32_t num);


#endif // SRC_CODE_ANALYSIS_H
