#include "common/param.h"
#include "common/trace.h"
#include "algos/BSketch.h"
#include "algos/CBSketch.h"
#include "algos/strawman.h"
#include <cmath>
#include <cassert>
using namespace std;

int cur;
vector<pair<uint32_t, double> > pkts;
vector<pair<uint32_t, double> > pkts_window;

bool readWindow(double stime, double duration) {
    if (cur >= pkts.size())
        return false;

    pkts_window.clear();
    while (cur < pkts.size() && pkts[cur].second - stime <= duration)
        pkts_window.push_back(pkts[cur++]);
    return true;
}

void test_bsketch(int sketch_size, double flowlet_thld, double clear_thld, ofstream &outFile){
    BSketch bsketch(sketch_size * 1000, flowlet_thld, clear_thld);

    cur = 0;
    double nowtime = pkts[0].second;
    FlowInfo info;
    while (readWindow(nowtime, 1)) {
        nowtime += 1;
        getActiveFlowLet(pkts_window, info, flowlet_thld, nowtime);

        for (int i = 0; i < pkts_window.size(); i++)
            bsketch.insert(pkts_window[i].first, pkts_window[i].second);
        
        uint32_t flowletPktCount = 0;
        for (auto x : info.flowlet_size)
            flowletPktCount += x.second;
        uint32_t estFlowletPktCount = bsketch.flowletPktCount(nowtime);

        uint32_t flowCount = info.flow_set.size();
        uint32_t estFlowCount = bsketch.flowCount();

        std::cerr << nowtime - pkts[0].second << '\n';
        outFile << nowtime - pkts[0].second << ',' <<
                   flowletPktCount << ',' << estFlowletPktCount << ',' << (double)estFlowletPktCount / flowletPktCount << ',' << 
                   flowCount << ',' << estFlowCount << ',' << (double)estFlowCount / flowCount << '\n';
    }
}

void test_cbsketch(int sketch_size, double flowlet_thld, double clear_thld, ofstream &outFile){
    CBSketch cbsketch(sketch_size * 1000, flowlet_thld, clear_thld);

    cur = 0;
    double nowtime = pkts[0].second;
    FlowInfo info;
    while (readWindow(nowtime, 1)) {
        nowtime += 1;
        getActiveFlowLet(pkts_window, info, flowlet_thld, nowtime);

        for (int i = 0; i < pkts_window.size(); i++)
            cbsketch.insert(pkts_window[i].first, pkts_window[i].second);
        
        uint32_t flowletPktCount = 0;
        for (auto x : info.flowlet_size)
            flowletPktCount += x.second;
        uint32_t estFlowletPktCount = cbsketch.flowletPktCount(nowtime);

        uint32_t flowCount = info.flow_set.size();
        uint32_t estFlowCount = cbsketch.flowCount();

        std::cerr << nowtime - pkts[0].second << '\n';
        outFile << nowtime - pkts[0].second << ',' <<
                   flowletPktCount << ',' << estFlowletPktCount << ',' << (double)estFlowletPktCount / flowletPktCount << ',' << 
                   flowCount << ',' << estFlowCount << ',' << (double)estFlowCount / flowCount << '\n';
    }
}

void test_all(double flowlet_delta, double flow_timeout){
    ofstream outFile;

    string filename = "../results/bsketch_percent.csv";
    outFile.open(filename.c_str(), ios::out);
    outFile << "Time" << "," <<
               "flowletPktCount" << "," << "estFlowletPktCount" << "," << "flowletPktPercent" << "," << 
               "flowCount" << "," << "estFlowCount" << "," << "flowPercent" << endl;

    test_bsketch(300, flowlet_delta, flow_timeout, outFile);

    outFile.close();

    filename = "../results/cbsketch_percent.csv";
    outFile.open(filename.c_str(), ios::out);
    outFile << "Time" << "," <<
               "flowletPktCount" << "," << "estFlowletPktCount" << "," << "flowletPktPercent" << "," << 
               "flowCount" << "," << "estFlowCount" << "," << "flowPercent" << endl;


    test_cbsketch(300, flowlet_delta, flow_timeout, outFile);

    outFile.close();
}

int main(){
    double flowlet_delta = 0.5;
    double flow_timeout = 10;

    readCAIDA18(pkts, "../datasets/130000.dat", 1e9);
    test_all(flowlet_delta, flow_timeout);
    return 0;
}
