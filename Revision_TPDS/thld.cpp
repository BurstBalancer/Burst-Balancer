#include "common/param.h"
#include "common/trace.h"
#include "algos/BSketchT.h"
#include "algos/CBSketch.h"
#include "algos/strawman.h"
#include <cmath>
using namespace std;

vector<pair<uint32_t, double> > pkts;

int main(){
    double flowlet_thld = 0.9;

    readCAIDA18(pkts, "../datasets/caida.dat", INT32_MAX);

    FlowInfo info;
    int pkt_num = pkts.size();
    
    double nowtime = getActiveFlowLet(pkts, info, flowlet_thld);
    
    set<uint32_t>           &flow_set     = info.flow_set;
    map<uint32_t, double>   &flowlet_tt   = info.flowlet_tt;
    map<uint32_t, double>   &flowlet_ts   = info.flowlet_ts;
    map<uint32_t, uint32_t> &flowlet_size = info.flowlet_size;

    set<NodeSpeed> burst_speed_set;
    for(auto x: flowlet_size){
        uint32_t fid = x.first;
        uint32_t freq = x.second;
        assert(nowtime - flowlet_tt[fid] <= flowlet_thld);
        if(freq > 3){
            double speed = (double)freq / (flowlet_tt[fid] - flowlet_ts[fid]) ;
            burst_speed_set.insert(NodeSpeed(fid, speed));
        }
    }
    string filename = "../results/thld.csv";
    ofstream outFile;
    outFile.open(filename.c_str(), ios::out);
    outFile << "theoretical flow percent,theoretical packet percent,flow percent,packet percent" << '\n';
    for (double percent = 0.01; percent <= 1; percent += 0.01) {
        double threshold = pkt_num * exp(-4 / std::max(0.6, 1 - percent));
        int cnt = 0;
        for (auto x : burst_speed_set)
            if (x.speed > threshold)
                ++cnt;
        outFile << (double)cnt / burst_speed_set.size() << ',';
        cnt = 0;
        for (auto x : burst_speed_set)
            if (x.speed > threshold)
                cnt += flowlet_size[x.fid];
        int pkt_cnt = 0;
        for (auto x : flowlet_size)
            pkt_cnt += x.second;
        outFile << (double)cnt / pkt_cnt << ',';
        BSketchT sketch(100000, flowlet_thld, flowlet_thld * 5, threshold);
        for (int i = 0; i < pkt_num; i++)
            sketch.insert(pkts[i].first, pkts[i].second);
        outFile << (double)sketch.scheduledCount() / burst_speed_set.size() << ',';
        outFile << (double)sketch.scheduledPktCount() / pkt_cnt << '\n';
    }
    return 0;
}
