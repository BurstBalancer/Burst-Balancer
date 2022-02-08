#ifndef SRC_CODE_COMMON_H
#define SRC_CODE_COMMON_H


#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include <math.h>


#ifdef WIN32
#include <ws2tcpip.h>
#include <winsock.h>

#include <windows.h>
#include <winsock2.h>

#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "ws2_32.lib")

#endif

#ifdef LINUX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


typedef int bpf_int32;

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;


#define POLY1 0x04c11db7
#define POLY2 0x741b8cd7
#define POLY3 0x45a5c105
#define POLY4 0x1edc6f41
#define POLY5 0x814141ab


#define RANDINT(a, b) (rand() % (b - a + 1) + a)

#define RANDFLOAT0_1 ((double)RANDINT(0, 99991)/99991.0)

#define RAND_EXP(a, b) ((int)(pow(2, 10 * RANDFLOAT0_1)/pow(2, 10)*(b-a)) + a)

#define RAND_FLOAT0_1(a, b) ((double)RANDINT((int)(99991*a), (int)(99991*b))/(double)99991.0)

#define MALLOC_AND_SET(ptr, size, type) {ptr = (type)malloc(size);memset(ptr, 0, size);}


#define MAX_SLOTS 16
#define MAX_ROWS 1024

typedef struct global_para_tag
{
    uint32_t srand_value;
    uint32_t gene_traff;        // 0: no generate traffic, 1: generate traffic
    char file_name[256];
    uint32_t traffic_type;

    uint32_t per_type;        // 0: per flow , 1: per pkt, 2: flowlet

    uint32_t eleflow_minsize;
    uint32_t max_flow_num;
    uint32_t packets_num;
    
    uint32_t flowlet_algo;
    uint32_t lb_case_num;

    char lb_burstbalancer_para[MAX_SLOTS][MAX_ROWS];


    uint32_t flow_let_rows;
    uint32_t flow_let_depth;

    uint32_t delta;

    uint32_t link_delay_range_majority;
    uint32_t link_delay_range_minority;
    double link_delay_majority_percentage;
}global_para_t;

extern global_para_t g_para;

#endif // SRC_CODE_COMMON_H
