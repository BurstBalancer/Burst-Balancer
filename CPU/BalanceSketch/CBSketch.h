#ifndef _CBSKETCH_H_
#define _CBSKETCH_H_



#include "param.h"
#include <immintrin.h>

#include "murmur3.h"


class CBSketch{
public:
    CBucket* buckets;
    double flowlet_thld;
    double clear_thld;
    double unit;
    int flowlet_delta;
    int clear_delta;
    uint32_t bucket_num;
    uint32_t bp_seed;  // bucket position seed
    uint32_t fp_seed;  // fingerprint seed

    CBSketch(uint32_t mem_size, double flowlet_thld_, double clear_thld_){
        bucket_num = (mem_size + sizeof(CBucket) - 1) / sizeof(CBucket);
        printf("bucket num: %d\n", bucket_num);

        flowlet_thld = flowlet_thld_;
        clear_thld = clear_thld_;

        unit = flowlet_thld ;
        flowlet_delta = (int)(flowlet_thld / unit);
        clear_delta = (int)(clear_thld / unit);
        printf("flowlet delta: %d; clear delta: %d\n", flowlet_delta, clear_delta);

        buckets = new(std::align_val_t{64}) CBucket[bucket_num];
        memset(buckets, 0, bucket_num * sizeof(CBucket));
        bp_seed = rand() % MAX_PRIME32;
        fp_seed = rand() % MAX_PRIME32;
    }

    ~CBSketch(){
        delete [] buckets;
    }

    uint32_t query(uint32_t key, double nowtime){
        int ts = (uint64_t)(nowtime / unit) % 256;
        uint16_t fp = CalculateFP(key);

        int pos = CalculatePos(key);
        for(int i = 0; i < CELL_PER_BUCKET; ++i){
            if(buckets[pos].fps[i] == fp){
                if(abs(ts - (int)buckets[pos].timestamps[i]) > flowlet_delta)
                // if(nowtime - buckets[pos].timestamps[i] >= flowlet_thld)
                    return 0;
                else
                    return buckets[pos].curf[i];
            }
        }
        return 0;
    }

    bool insert(uint32_t key, double nowtime){
        int ts = (uint64_t)(nowtime / unit) % 256;

        uint16_t fp = CalculateFP(key);

        int pos = CalculatePos(key);
        int empty_i = -1;
        int min_freq = __INT32_MAX__;
        int min_i = -1;
        for(int i = 0; i < CELL_PER_BUCKET; ++i){
            if(buckets[pos].fps[i] == fp){
                if(abs(ts - (int)buckets[pos].timestamps[i]) > flowlet_delta)
                // if(nowtime - buckets[pos].timestamps[i] >= flowlet_thld)
                    buckets[pos].curf[i] = 1;
                else
                    buckets[pos].curf[i]++;
                if(buckets[pos].freq[i] < __UINT16_MAX__)
                    buckets[pos].freq[i]++;
                buckets[pos].timestamps[i] = ts;
                // buckets[pos].timestamps[i] = nowtime;
                return true;
            }

           else if(buckets[pos].fps[i] == 0 || abs(ts - (int)buckets[pos].timestamps[i]) > clear_delta){
           // else if(buckets[pos].fps[i] == 0 || nowtime - buckets[pos].timestamps[i] >= clear_thld){
                if(empty_i == -1)
                    empty_i = i;
            }
            else if(buckets[pos].freq[i] < min_freq){
                min_freq = buckets[pos].freq[i];
                min_i = i;
            }
        }
        if(empty_i != -1){
            buckets[pos].fps[empty_i] = fp;
            buckets[pos].freq[empty_i] = 1;
            buckets[pos].curf[empty_i] = 1;
            buckets[pos].timestamps[empty_i] = ts;
            // buckets[pos].timestamps[empty_i] = nowtime;
            return true;
        }
        if(--buckets[pos].freq[min_i] == 0){
            buckets[pos].fps[min_i] = fp;
            buckets[pos].freq[min_i] = 1;
            buckets[pos].curf[min_i] = 1;
            buckets[pos].timestamps[min_i] = ts;
            // buckets[pos].timestamps[min_i] = nowtime;
            return true;
        }
        return false;
    }

private:
    inline int CalculatePos(uint32_t key){
        // return CalculateBucketPos(key) % bucket_num;
        return MurmurHash3_x86_32((const char*)&key, sizeof(uint32_t), bp_seed) % bucket_num;
    }
    inline uint8_t CalculateFP(uint32_t key){
        return MurmurHash3_x86_32((const char*)&key, sizeof(uint32_t), fp_seed) & ((1 << 16) - 1);
    }
};



#endif // _CBSKETCH_H_
