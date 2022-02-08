#include "analysis.h"

void per_flow_analysis(pcap_data_set_t *pcap_data)
{
    handle_info_t *handle;

    printf("\nPer flow:\n\n");

    //init handle
    handle = init_handle(pcap_data);

    per_flow_lb(handle);


    //check disordered packets
    check_disordered_pkt(handle);

    //print analysis result
    display_analysis_result(handle);

    //free memory
    free_handle(&handle);

}

void per_pkt_analysis(pcap_data_set_t *pcap_data)
{
    handle_info_t *handle;

    printf("\nPer pkt:\n\n");

    //init handle
    handle = init_handle(pcap_data);

    per_pkt_lb(handle);

    //check disordered packets
    check_disordered_pkt(handle);

    //print analysis result
    display_analysis_result(handle);

    //free memory
    free_handle(&handle);

}

void read_flowburst_detect_para(para_integrate_t *para, uint32_t type, uint32_t case_idx)
{
    para_heavyflow_detect_t *para_hf;

    para_hf = (para_heavyflow_detect_t *)malloc(sizeof(para_heavyflow_detect_t));
    para->flowlet_table_para = (para_fl_table_t *)malloc(sizeof(para_fl_table_t));

    // flowburst detect

    para_hf->fl_type = type;

    if (type == BurstBalaner) {
        para_hf->burstblc_rows = g_para.flow_let_depth * g_para.flow_let_rows;
    } 

    para_hf->num = 1;
    para->heavy_detect_para = para_hf;
}

void read_flowlet_para(para_integrate_t *para)
{
    para_fl_table_t *para_fl = (para_fl_table_t *)(para->flowlet_table_para);

    para_fl->size = g_para.flow_let_rows;
    para_fl->depth = g_para.flow_let_depth;

}

void print_parameter(pcap_data_set_t *pcap_data, para_integrate_t para)
{
    para_heavyflow_detect_t *para_hf = (para_heavyflow_detect_t *)para.heavy_detect_para;

    if (para_hf->fl_type == BurstBalaner) {
        printf("\n Algorithm : %s , tables = %d , \n ",
                "BurstBalaner", para_hf->burstblc_rows);
    } else if (para_hf->fl_type == DEF_LETFLOW) {
        printf("\n Default LetFlow ,\n");
    }

}

void per_flowlet_analysis(pcap_data_set_t *pcap_data)
{
    uint32_t i, n;
    uint32_t type, case_idx;

    n = g_para.flowlet_algo == DEF_LETFLOW ? 1 : g_para.lb_case_num;

    for (i = 0; i < n; i++) {
        handle_info_t *handle;
        para_integrate_t para = {0};
        
        type = g_para.flowlet_algo;
        case_idx = i;
        
        printf("\n\n\t####\tPer flowlet:\t####\n");

        // flowburst detect size, depth, type
        read_flowburst_detect_para(&para, type, case_idx);

        // flowlet table para : size, depth
        read_flowlet_para(&para);

        print_parameter(pcap_data, para);

        //init handle
        handle = init_handle(pcap_data);

        //configure flowburst detect and flowlet parameter
        configue_parameter(handle, &para);

        //route by flowlet
        per_flowlet_lb(handle);

        //check disordered packets
        check_disordered_pkt(handle);

        //print analysis result
        display_analysis_result(handle);

        //free memory
        free_handle(&handle);
    }

}

void analysis_traffic()
{
    pcap_data_set_t *pcap_data;
    char file_name[256];

    FILE *log;

    log = fopen("data//output.txt", "a+");
    fprintf(log, "\n per %s :\n", g_para.per_type == 2 ? "flowlet" : "flow/pkt");

    fclose(log);

    snprintf(file_name, sizeof(file_name), "%s", g_para.file_name);
    int i = 0;
    for (i = 0; i < strlen(file_name); i++) {
        if (file_name[i] == '\0' || file_name[i] == '\r' || file_name[i] == '\n') {
            file_name[i] = '\0';
            break;
        }
    }

    printf("\t Read pcap file\n");
    pcap_data = read_pcap_file(file_name, g_para.packets_num);


    switch (g_para.per_type) {
        case 0:
            // per flow analysis
            per_flow_analysis(pcap_data);
            break;
        case 1:
            // per pkt analysis
            per_pkt_analysis(pcap_data);
            break;

        case 2:
            // flowlet analysis
            per_flowlet_analysis(pcap_data);
            break;
    }

    free_dataset(&pcap_data);

    printf("\nAnalysis finished!\n");

}

void load_parameter()
{
    FILE *para_file;
    char buf[1024];

    para_file = fopen("data//parameter_configure.txt", "r");


    while (fgets(buf, 1024, para_file)) {
        if (buf[0] == '#' || buf[0] == ' ' || buf[0] == '\r') {
            continue;
        }

        char *catag, *value;

        catag = strtok(buf, "=");

        value = strtok(NULL, "&");


        if (strcmp(catag, "srand") == 0) {
            g_para.srand_value = atoi(value);
        } else if (strcmp(catag, "pcap_file_name") == 0) {
            memcpy(g_para.file_name, value, strlen(value) * sizeof(char));
        } else if (strcmp(catag, "lb_scheme") == 0) {
            g_para.per_type = atoi(value);
        } else if (strcmp(catag, "pkt_num") == 0) {
            g_para.packets_num = atoi(value);
        }


        if (strcmp(catag, "flowlet_algo") == 0) {
            g_para.flowlet_algo = atoi(value);
        } else if ((strcmp(catag, "hash_table_rows") == 0)) {
            g_para.flow_let_rows = atoi(value);
        } else if ((strcmp(catag, "hash_table_depth") == 0)) {
            g_para.flow_let_depth = atoi(value);
        } else if ((strcmp(catag, "delta") == 0)) {
            g_para.delta = atoi(value);
        } else if ((strcmp(catag, "link_delay_range_majority") == 0)) {
            g_para.link_delay_range_majority = atoi(value);
        } else if ((strcmp(catag, "link_delay_range_minority") == 0)) {
            g_para.link_delay_range_minority = atoi(value);
        } else if ((strcmp(catag, "link_delay_majority_percentage") == 0)) {
            g_para.link_delay_majority_percentage = atof(value);
        }

    }
    g_para.lb_case_num = 1;

    fclose(para_file);
}

int main(void)
{
    // load parameter
    load_parameter();

    // analysis pcap file
    analysis_traffic();

    return 0;
}
