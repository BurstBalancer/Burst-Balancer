#ifndef _TRACE_H_
#define _TRACE_H_

#include "param.h"
#include <cassert>

struct NodeSpeed{
    uint32_t fid;
    double speed;
    NodeSpeed (uint32_t fid_, double speed_): fid(fid_), speed(speed_) {}
    bool operator< (const NodeSpeed& nd) const{
        return speed > nd.speed || (speed == nd.speed && fid > nd.fid);
    }
};

struct NodeFreq{
    uint32_t fid, freq;
    NodeFreq (uint32_t fid_, uint32_t freq_): fid(fid_), freq(freq_) {}
    bool operator< (const NodeFreq& nd) const{
        return freq > nd.freq || (freq == nd.freq && fid > nd.fid);
    }
};

struct FlowInfo{
    set<uint32_t> flow_set;
    map<uint32_t, double> flowlet_tt;
    map<uint32_t, double> flowlet_ts;
    map<uint32_t, uint32_t> flow_size;
    map<uint32_t, uint32_t> flowlet_size;
};

void readCAIDA18(vector<pair<uint32_t, double> > &pkts, const char* filename, int read_num = 0) {
    // printf("open %s \n", filename);
    
    ifstream fin(filename, ios::binary);

    pkts.clear();
    char trace[21];
    uint32_t pkt_cnt = 0;

    uint32_t key;
    double ttime;
    while(fin.read(trace, 21)){
        key = *(uint32_t*) (trace);
        ttime = *(double*) (trace+13);
        pkts.push_back(pair<uint32_t, double>(key, ttime));
        if(++pkt_cnt == read_num)
            break;
    }

    fin.close();
}

double getActiveFlowLet(vector<pair<uint32_t, double> > &pkts, FlowInfo &info, double flowlet_thld, double nowtime = -1) {
    set<uint32_t>           &flow_set     = info.flow_set;
    map<uint32_t, double>   &flowlet_tt   = info.flowlet_tt;
    map<uint32_t, double>   &flowlet_ts   = info.flowlet_ts;
    map<uint32_t, uint32_t> &flow_size    = info.flow_size;
    map<uint32_t, uint32_t> &flowlet_size = info.flowlet_size;

    int pkt_num = pkts.size();
    double stime = -1, lasttime = 0;
    for (int i = 0; i < pkt_num; i++) {
        uint32_t key = pkts[i].first;
        double ttime = pkts[i].second;
        if (stime < 0)
            stime = ttime;
        flow_set.insert(key);
        if(flowlet_tt.count(key) == 0){
            flowlet_tt[key] = ttime;
            flowlet_ts[key] = ttime;
            flowlet_size[key] = 1;
        }
        else{
            if(ttime - flowlet_tt[key] >= flowlet_thld){
                flowlet_size[key] = 1;
                flowlet_ts[key] = ttime;
                flowlet_tt[key] = ttime;
            }
            else{
                flowlet_size[key]++;
                flowlet_tt[key] = ttime;
            }
        }
        if(flow_size.count(key) == 0)
            flow_size[key] = 1;
        else
            flow_size[key]++;
        lasttime = ttime;
    }

    if (nowtime == -1)
        nowtime = lasttime;
    
    for (auto x: flow_set)
        if (nowtime - flowlet_tt[x] > flowlet_thld)
            flowlet_size.erase(x);
    return lasttime;
}

void getTopKFlowLet(vector<pair<uint32_t, double> > &pkts, map<uint32_t, uint32_t> &flowlet_size,
        double flowlet_thld, double clear_thld, int k = 200) {
    FlowInfo info;
    int pkt_num = pkts.size();
    
    double nowtime = getActiveFlowLet(pkts, info, flowlet_thld);

    set<uint32_t>           &flow_set     = info.flow_set;
    map<uint32_t, double>   &flowlet_tt   = info.flowlet_tt;
                             flowlet_size = info.flowlet_size;
    
    set<NodeFreq> flowlet_freq_set;
    for(auto x: flowlet_size){
        uint32_t fid = x.first;
        uint32_t freq = x.second;
        assert(nowtime - flowlet_tt[fid] <= flowlet_thld);
        if(freq > 3)
            flowlet_freq_set.insert(NodeFreq(fid, freq));
    }

    flowlet_size.clear();
    int cnt = 0;
    uint32_t freq_thld;
    for(auto x: flowlet_freq_set){
        auto fid = x.fid;
        auto freq = x.freq;
        flowlet_size[fid] = freq;
        if(++cnt == k){
            freq_thld = freq;
            break;
        }
    }

    printf("load %u packets of %lu flow. \n", pkt_num, flow_set.size());
    printf("(freq top-%d, freq_thld: %u)\n", k, freq_thld);
}

double getTopKFlowBurst(vector<pair<uint32_t, double> > &pkts, map<uint32_t, uint32_t> &burst_size,
        double flowlet_thld, double clear_thld, int k = 200, double speed_percentile = 0.7) {
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

    set<NodeFreq> burst_freq_set;
    double speed_thld;
    int cnt = 0;
    for(auto x: burst_speed_set){
        auto fid = x.fid;
        auto speed = x.speed;
        burst_freq_set.insert(NodeFreq(fid, flowlet_size[fid]));
        if((double)(cnt++) / burst_speed_set.size() > speed_percentile){
            speed_thld = speed;
            break;
        }
    }

    uint32_t freq_thld;
    cnt = 0;
    for(auto x: burst_freq_set){
        auto fid = x.fid;
        auto freq = x.freq;
        burst_size[fid] = flowlet_size[fid];
        if(++cnt == k){
            freq_thld = flowlet_size[fid];
            break;
        }
    }

    printf("load %u packets of %lu flow. \n", pkt_num, flow_set.size());
    printf("flowlet number: %lu \n", burst_speed_set.size());
    printf("FlowBurst number: %lu \n", burst_size.size());
    printf("(freq top-%d, speed percentile %lf, speed_thld: %lf pkts/s, freq_thld: %u)\n", k, speed_percentile, speed_thld, freq_thld);

    return speed_thld;
}

double getTopKFlowBurstDuration(vector<pair<uint32_t, double> > &pkts, map<uint32_t, double> &burst_ts,
        map<uint32_t, double> &burst_duration, double flowlet_thld, double clear_thld, int k = 200, double speed_percentile = 0.7) {
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

    set<NodeFreq> burst_freq_set;
    double speed_thld;
    int cnt = 0;
    for(auto x: burst_speed_set){
        auto fid = x.fid;
        auto speed = x.speed;
        burst_freq_set.insert(NodeFreq(fid, flowlet_size[fid]));
        if((double)(cnt++) / burst_speed_set.size() > speed_percentile){
            speed_thld = speed;
            break;
        }
    }

    uint32_t freq_thld;
    cnt = 0;
    for(auto x: burst_freq_set){
        auto fid = x.fid;
        auto freq = x.freq;
        burst_ts[fid] = flowlet_ts[fid];
        burst_duration[fid] = nowtime - flowlet_ts[fid];
        if(++cnt == k){
            freq_thld = flowlet_size[fid];
            break;
        }
    }

    printf("load %u packets of %lu flow. \n", pkt_num, flow_set.size());
    printf("flowlet number: %lu \n", burst_speed_set.size());
    printf("FlowBurst number: %lu \n", burst_duration.size());
    printf("(freq top-%d, speed percentile %lf, speed_thld: %lf pkts/s, freq_thld: %u)\n", k, speed_percentile, speed_thld, freq_thld);

    return speed_thld;
}

#endif // _TRACE_H_
