#include "common/param.h"
#include "common/trace.h"
#include "algos/BSketchD.h"
#include "algos/strawman.h"
#include <cmath>
#include <random>
#include <cassert>
using namespace std;

double delay_rate;

vector<pair<uint32_t, double> > pkts;
map<uint32_t, double> burst_ts;
map<uint32_t, double> burst_duration;

void test_bsketch(int sketch_size, double flowlet_thld, double clear_thld, ofstream &outFile){
    BSketchD bsketch(sketch_size * 1000, flowlet_thld, clear_thld);

    int pkt_num = pkts.size();
    double nowtime = pkts[pkt_num - 1].second;

    for (int i = 0; i < pkt_num; i++)
        bsketch.insert(pkts[i].first, pkts[i].second);

    double value;
    uint32_t all = 0, hit = 0, size = 0;
    double aae = 0, are = 0, rr = 0, pr = 0;
    int cnt = 0;
    for(auto it = burst_duration.begin(); it != burst_duration.end(); ++it) if (it->second > 0) {
        value = bsketch.queryDuration(it->first, nowtime);
        all++;
        if (value > 0) {
            hit++;
            aae += fabs((double)it->second - value);
            are += fabs((double)it->second - value) / (double) it->second;
        }
        if (value > 0)
            size++;
        cnt++;
    }

    aae /= hit;
    are /= hit;
    rr = hit / (double)all;
    pr = hit / (double)size;

    printf("AAE:%lf, ARE:%lf, RR: %lf, PR: %lf\n", aae, are, rr, pr);
    printf("cnt:%d, size:%d, all:%d, hit: %d\n", cnt, size, all, hit);
    outFile << sketch_size << "," << aae << "," << are << "," << rr << "," << pr << endl;
}


void test_all(double speed_thld, double flowlet_delta, double flow_timeout){
    ofstream outFile;

    printf("---------------------------------------------------------------\n\n");

    string filename = "../results/duration_" + std::to_string(CELL_PER_BUCKET) + ".csv";
    outFile.open(filename.c_str(), ios::out);

    outFile << "memory(KB)" << "," << "AAE" << "," << "ARE" << "," << "RR" << "," << "PR" << endl;
    for(int i = 10; i <= 300; i += 30)
        for (int j = 0; j < 100; j++)
            test_bsketch(i, flowlet_delta, flow_timeout, outFile);

    printf("---------------------------------------------------------------\n\n");
}

int main(int argc, char **argv){

    readCAIDA18(pkts, "../datasets/caida.dat", 400000);
    double speed_thld = getTopKFlowBurstDuration(pkts, burst_ts, burst_duration, 0.09, 0.45, 200, 0.7);
    test_all(speed_thld, 0.09, 0.45);
    return 0;
}

