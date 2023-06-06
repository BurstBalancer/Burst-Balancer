#include "common/param.h"
#include "common/trace.h"
#include "algos/BSketch.h"
#include "algos/CBSketch.h"
#include "algos/strawman.h"
#include <cmath>
#include <random>
#include <cassert>
using namespace std;

double loss_rate;

vector<pair<uint32_t, double> > pkts[2];
map<uint32_t, uint32_t> burst_size;
map<uint32_t, uint32_t> burst_drop_size;

vector<pair<uint32_t, double> > randomDrop(const vector<pair<uint32_t, double> > &vec) {
    int n = vec.size();

    vector<int> pos;
    for (int i = 0; i < n; i++)
        pos.push_back(i);
    std::random_shuffle(pos.begin(), pos.end());

    set<int> drop;
    int m = n * loss_rate;
    for (int i = 0; i < m; i++) {
        drop.insert(pos[i]);
        if (burst_size.count(vec[i].first))
            burst_drop_size[vec[i].first]++;
    }

    vector<pair<uint32_t, double> > ans;
    for (int i = 0; i < n; i++)
        if (drop.count(i) == false)
            ans.push_back(vec[i]);
    return ans;
}

void test_bsketch(int sketch_size, double flowlet_thld, double clear_thld, ofstream &outFile){
    BSketch bsketch[2] = {BSketch(sketch_size * 1000, flowlet_thld, clear_thld), BSketch(sketch_size * 1000, flowlet_thld, clear_thld)};

    int pkt_num[2];
    for (int i = 0; i < 2; i++) {
        pkt_num[i] = pkts[i].size();
        for (int j = 0; j < pkt_num[i]; j++)
            bsketch[i].insert(pkts[i][j].first, pkts[i][j].second);
    }

    double nowtime = pkts[0][pkt_num[0] - 1].second;

    int value0, value1;
    uint32_t all = 0, hit = 0, size = 0;
    double aae = 0, are = 0, rr = 0, pr = 0;
    int cnt = 0;
    for(auto it = burst_drop_size.begin(); it != burst_drop_size.end(); ++it){
        value0 = bsketch[0].query(it->first, nowtime);
        value1 = bsketch[1].query(it->first, nowtime);
        all++;
        if(value0 - value1 >= 1 && value1 > 0) {
            int value = value0 - value1;
            hit++;
            aae += fabs((double)it->second - value);
            are += fabs((double)it->second - value) / (double) it->second;
        }
        if (value0 > 0 && value1 > 0)
            size++;
        cnt++;
    }

    aae /= hit;
    are /= hit;
    rr = hit / (double)all;
    pr = hit / (double)size;

    printf("RR: %lf\n", rr);
    outFile << sketch_size << "," << rr << endl;
}


void test_all(double speed_thld, double flowlet_delta, double flow_timeout){
    ofstream outFile;

    printf("---------------------------------------------------------------\n\n");

    string filename = "../results/loss_" + to_string(loss_rate) + ".csv";
    outFile.open(filename.c_str(), ios::out);

    outFile << "memory(KB)" << "," << "RR" << endl;
    for(int i = 10; i <= 300; i += 30)
        test_bsketch(i, flowlet_delta, flow_timeout, outFile);

    printf("---------------------------------------------------------------\n\n");
}

int main(int argc, char **argv){
    loss_rate = atof(argv[1]);

    std::cerr << loss_rate << '\n';

    double flowlet_delta = 50;
    double flow_timeout = 1000;

    readCAIDA18(pkts[0], "../datasets/caida.dat", 400000);
    double speed_thld = getTopKFlowBurst(pkts[0], burst_size, 0.09, 0.45, 200, 0.5);
    pkts[1] = randomDrop(pkts[0]);
    test_all(speed_thld, 0.09, 0.45);
    return 0;
}
