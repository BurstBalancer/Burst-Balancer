#include "param.h"
#include "trace.h"
#include "BSketch.h"
#include "CBSketch.h"
#include "strawman.h"
#include <cmath>
using namespace std;


vector<pair<uint32_t, double> > pkts;
map<uint32_t, uint32_t> burst_size;


void test_bsketch(int sketch_size, double flowlet_thld, double clear_thld, ofstream &outFile){
    BSketch bsketch(sketch_size * 1000, flowlet_thld, clear_thld);

    timespec time1, time2;
    uint64_t resns = 0;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    int pkt_num = pkts.size();
    for(int i = 0; i < pkt_num; ++i)
        bsketch.insert(pkts[i].first, pkts[i].second);
    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    double insert_mops = (double)1000.0 * (pkt_num) / resns;
    printf("# packets: %d, Insertion Mops: %lf\n", pkt_num, insert_mops);


    uint32_t ans = 0;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    for(int i = 0; i < pkt_num; ++i)
        ans += bsketch.query(pkts[i].first, pkts[i].second);
    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    double query_mops = (double)1000.0 * (pkt_num) / resns;
    printf("# packets: %d, Query Mops: %lf\n", pkt_num, query_mops);
    printf("ans: %d (print to avoid O3)\n", ans);

    uint32_t value = 0, all = 0, hit = 0, size = 0;
    double aae = 0, are = 0, rr = 0, pr = 0;
    int cnt = 0;
    for(auto it = burst_size.begin(); it != burst_size.end(); ++it){
        value = bsketch.query(it->first, pkts[pkt_num-1].second);
        all++;
        if(value > 0){
            hit++;
            aae += fabs((double)it->second - value);
            are += fabs((double)it->second - value) / (double) it->second;
        }
        if(value > 0)
            size++;
        cnt++;
    }

    aae /= hit;
    are /= hit;
    rr = hit / (double)all;
    pr = hit / (double)size;

    printf("bsketch memory: %d KB\n", sketch_size);
    printf("AAE:%lf, ARE:%lf, RR: %lf, PR: %lf\n", aae, are, rr, pr);
    printf("cnt:%d, size:%d, all:%d, hit: %d\n", cnt, size, all, hit);
    outFile << sketch_size << "," << aae << "," << are << "," << rr << "," << pr << "," << insert_mops << "," << query_mops << endl;
}


void test_cbsketch(int sketch_size, double flowlet_thld, double clear_thld, ofstream &outFile){
    CBSketch cbsketch(sketch_size * 1000, flowlet_thld, clear_thld);

    timespec time1, time2;
    uint64_t resns = 0;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    int pkt_num = pkts.size();
    for(int i = 0; i < pkt_num; ++i)
        cbsketch.insert(pkts[i].first, pkts[i].second);
    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    double insert_mops = (double)1000.0 * (pkt_num) / resns;
    printf("# packets: %d, Insertion Mops: %lf\n", pkt_num, insert_mops);


    uint32_t ans = 0;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    for(int i = 0; i < pkt_num; ++i)
        ans += cbsketch.query(pkts[i].first, pkts[i].second);
    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    double query_mops = (double)1000.0 * (pkt_num) / resns;
    printf("# packets: %d, Query Mops: %lf\n", pkt_num, query_mops);
    printf("ans: %d (print to avoid O3)\n", ans);

    uint32_t value = 0, all = 0, hit = 0, size = 0;
    double aae = 0, are = 0, rr = 0, pr = 0;
    int cnt = 0;
    for(auto it = burst_size.begin(); it != burst_size.end(); ++it){
        value = cbsketch.query(it->first, pkts[pkt_num-1].second);
        all++;
        if(value > 0){
            hit++;
            aae += fabs((double)it->second - value);
            are += fabs((double)it->second - value) / (double) it->second;
        }
        if(value > 0)
            size++;
        cnt++;
    }

    aae /= hit;
    are /= hit;
    rr = hit / (double)all;
    pr = hit / (double)size;

    printf("cbsketch memory: %d KB\n", sketch_size);
    printf("AAE:%lf, ARE:%lf, RR: %lf, PR: %lf\n", aae, are, rr, pr);
    printf("cnt:%d, size:%d, all:%d, hit: %d\n", cnt, size, all, hit);
    outFile << sketch_size << "," << aae << "," << are << "," << rr << "," << pr << "," << insert_mops << "," << query_mops << endl;
}

void test_strawman(int sketch_size, double flowlet_thld, double clear_thld, double speed_thld, ofstream &outFile){
    Strawman strawman(sketch_size * 1000, flowlet_thld, clear_thld, speed_thld);

    timespec time1, time2;
    uint64_t resns = 0;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    int pkt_num = pkts.size();
    for(int i = 0; i < pkt_num; ++i)
        strawman.insert(pkts[i].first, pkts[i].second);
    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    double insert_mops = (double)1000.0 * (pkt_num) / resns;
    printf("# packets: %d, Insertion Mops: %lf\n", pkt_num, insert_mops);


    uint32_t ans = 0;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    for(int i = 0; i < pkt_num; ++i)
        ans += strawman.query(pkts[i].first, pkts[i].second);
    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    double query_mops = (double)1000.0 * (pkt_num) / resns;
    printf("# packets: %d, Query Mops: %lf\n", pkt_num, query_mops);
    printf("ans: %d (print to avoid O3)\n", ans);

    uint32_t value = 0, all = 0, hit = 0, size = 0;
    double aae = 0, are = 0, rr = 0, pr = 0;
    int cnt = 0;
    for(auto it = burst_size.begin(); it != burst_size.end(); ++it){
        value = strawman.query(it->first, pkts[pkt_num-1].second);
        all++;
        if(value > 0){
            hit++;
            aae += fabs((double)it->second - value);
            are += fabs((double)it->second - value) / (double) it->second;
        }
        if(value > 0)
            size++;
        cnt++;
    }

    aae /= hit;
    are /= hit;
    rr = hit / (double)all;
    pr = hit / (double)size;

    printf("strawman memory: %d KB\n", sketch_size);
    printf("AAE:%lf, ARE:%lf, RR: %lf, PR: %lf\n", aae, are, rr, pr);
    printf("cnt:%d, size:%d, all:%d, hit: %d\n", cnt, size, all, hit);
    outFile << sketch_size << "," << aae << "," << are << "," << rr << "," << pr << "," << insert_mops << "," << query_mops << endl;
}



void test_all(double speed_thld, double flowlet_delta, double flow_timeout){
    ofstream outFile;

    printf("---------------------------------------------------------------\n\n");

    string filename = "data_bsketch_bucketsize" + to_string(CELL_PER_BUCKET) + ".csv";
    outFile.open(filename.c_str(), ios::out);
    outFile << "memory(KB)" << "," << "AAE" << "," << "ARE" << "," << "RR" << "," << "PR" << "," << "insertionMops" << "," << "queryMops" << endl;
    for(int i = 10; i <= 50; i += 10)
        test_bsketch(i, flowlet_delta, flow_timeout, outFile);
    outFile.close();

    printf("---------------------------------------------------------------\n\n");

    filename = "data_cbsketch_bucketsize" + to_string(CELL_PER_BUCKET) + ".csv";
    outFile.open(filename.c_str(), ios::out);
    outFile << "memory(KB)" << "," << "AAE" << "," << "ARE" << "," << "RR" << "," << "PR" << "," << "insertionMops" << "," << "queryMops" << endl;
    for(int i = 10; i <= 50; i += 10)
        test_cbsketch(i, flowlet_delta, flow_timeout, outFile);
    outFile.close();

    printf("---------------------------------------------------------------\n\n");

    filename = "data_strawman_bucketsize" + to_string(CELL_PER_BUCKET) + ".csv";
    outFile.open(filename.c_str(), ios::out);
    outFile << "memory(KB)" << "," << "AAE" << "," << "ARE" << "," << "RR" << "," << "PR" << "," << "insertionMops" << "," << "queryMops" << endl;
    for(int i = 10; i <= 50; i += 10)
        test_strawman(i, flowlet_delta, flow_timeout, speed_thld, outFile);
    outFile.close();

}


int main(){
    double flowlet_delta = 50;
    double flow_timeout = 1000;

    double speed_thld = loadCAIDA18(pkts, burst_size, "./caida.dat", 0.09, 0.45, 400000);
    test_all(speed_thld, 0.09, 0.45);
    return 0;
}
