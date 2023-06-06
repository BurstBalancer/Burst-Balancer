#include "common/param.h"
#include "common/trace.h"
#include "algos/BSketchD.h"
#include "algos/strawman.h"
#include <cmath>
#include <random>
#include <cassert>
using namespace std;

double delay_rate;

vector<pair<uint32_t, double> > pkts[2];
map<uint32_t, double> burst_duration;
map<uint32_t, double> burst_ts;
map<uint32_t, double> burst_extra_delay;

vector<pair<uint32_t, double> > randomDelay(const vector<pair<uint32_t, double> > &vec) {
    int n = vec.size();
    vector<pair<uint32_t, double> > ans = vec;

    vector<int> pos;
    for (int i = 0; i < n; i++)
        pos.push_back(i);
    random_shuffle(pos.begin(), pos.end());

    set<int> delayPos;
    int m = n * delay_rate;
    for (int i = 0; i < m; i++) {
        delayPos.insert(pos[i]);
    }

    random_device rd;
    default_random_engine eng(rd());
    uniform_real_distribution<double> distr(0, 0.00001);

    double delay = 0;
    for (int i = 0; i < n; i++) {
        if (delayPos.count(i)) {
            double extra_delay = distr(eng);
            delay += extra_delay;
        }
        if (burst_ts.count(ans[i].first) && ans[i].second >= burst_ts[ans[i].first]) {
            if (burst_extra_delay.count(ans[i].first) == 0)
                burst_extra_delay[ans[i].first] = delay;
        }
        ans[i].second += delay;
    }
    for (auto &x : burst_extra_delay)
        x.second = delay - x.second;
    return ans;
}

void test_bsketch(int sketch_size, double flowlet_thld, double clear_thld, ofstream &outFile){
    BSketchD bsketch[2] = {BSketchD(sketch_size * 1000, flowlet_thld, clear_thld), BSketchD(sketch_size * 1000, flowlet_thld * 2, clear_thld * 2)};

    int pkt_num[2];
    for (int i = 0; i < 2; i++) {
        pkt_num[i] = pkts[i].size();
        for (int j = 0; j < pkt_num[i]; j++)
            bsketch[i].insert(pkts[i][j].first, pkts[i][j].second);
    }

    double nowtime0 = pkts[0][pkt_num[0] - 1].second;
    double nowtime1 = pkts[1][pkt_num[1] - 1].second;

    double value0, value1;
    uint32_t all = 0, hit = 0, size = 0;
    double aae = 0, are = 0, rr = 0, pr = 0;
    int cnt = 0;
    for(auto it = burst_extra_delay.begin(); it != burst_extra_delay.end(); ++it) if (it->second > 0) {
        value0 = bsketch[0].queryDuration(it->first, nowtime0);
        value1 = bsketch[1].queryDuration(it->first, nowtime1);
        all++;
        if(value1 - value0 > 0 && value0 > 0) {
            double value = value1 - value0;
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

    string filename = "../results/delay_" + to_string(delay_rate) + ".csv";
    outFile.open(filename.c_str(), ios::out);

    outFile << "memory(KB), RR" << endl;
    for(int i = 10; i <= 300; i += 30)
        for (int j = 0; j < 100; j++)
            test_bsketch(i, flowlet_delta, flow_timeout, outFile);

    printf("---------------------------------------------------------------\n\n");
}

int main(int argc, char **argv){
    delay_rate = atof(argv[1]);

    std::cerr << delay_rate << '\n';

    readCAIDA18(pkts[0], "../datasets/caida.dat", 400000);
    double speed_thld = getTopKFlowBurstDuration(pkts[0], burst_ts, burst_duration, 0.09, 0.45, 200, 0.7);
    pkts[1] = randomDelay(pkts[0]);
    test_all(speed_thld, 0.09, 0.45);
    return 0;
}

