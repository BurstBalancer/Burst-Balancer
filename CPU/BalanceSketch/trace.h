#ifndef _TRACE_H_
#define _TRACE_H_

#include "param.h"


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



double loadCAIDA18(vector<pair<uint32_t, double> > &pkts, map<uint32_t, uint32_t> &burst_size, const char* filename, 
        double flowlet_thld, double clear_thld, int read_num = 0, int k = 200, double speed_percentile = 0.7){
    set<uint32_t> flow_set;
    map<uint32_t, double> flowlet_tt; // time of the last packet of flowlet
    map<uint32_t, double> flowlet_ts; // time of the first packet of flowlet
    map<uint32_t, uint32_t> flow_size;
    map<uint32_t, uint32_t> flowlet_size;

    printf("open %s \n", filename);
    
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
        if(++pkt_cnt == read_num)
            break;
        // if(pkt_cnt % 1000000 == 1){
        //     printf("%u : %lf\n", key, ttime);
        // }
    }

    fin.close();

    set<NodeSpeed> burst_speed_set;
    for(auto x: flowlet_size){
        uint32_t fid = x.first;
        uint32_t freq = x.second;
        if(ttime - flowlet_tt[key] < flowlet_thld && freq > 3){
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
        // cout << x.fid << " "  << flowlet_size[fid] << " " << flow_size[fid] << endl;
        if(++cnt == k){
            freq_thld = flowlet_size[fid];
            break;
        }
    }

    printf("load %u packets of %lu flow. \n", pkt_cnt, flow_set.size());
    printf("flowlet number: %lu \n", burst_speed_set.size());
    printf("FlowBurst number: %lu \n", burst_size.size());
    printf("(freq top-%d, speed percentile %lf, speed_thld: %lf pkts/s, freq_thld: %u)\n", k, speed_percentile, speed_thld, freq_thld);

    return speed_thld;
}




#endif // _TRACE_H_
