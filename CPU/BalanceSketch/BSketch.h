#ifndef _BSKETCH_H_
#define _BSKETCH_H_


// #define USING_SIMD

#include "param.h"
#include <immintrin.h>

#include "murmur3.h"

class BSketch{
public:
    Bucket* buckets;
    double flowlet_thld;
    double clear_thld;
    uint32_t bucket_num;
    uint32_t seed;

    BSketch(uint32_t mem_size, double flowlet_thld_, double clear_thld_){
        bucket_num = (mem_size + sizeof(Bucket) - 1) / sizeof(Bucket);
        printf("bucket num: %d\n", bucket_num);
        flowlet_thld = flowlet_thld_;
        clear_thld = clear_thld_;
        buckets = new(std::align_val_t{64}) Bucket[bucket_num];
        memset(buckets, 0, bucket_num * sizeof(Bucket));
        seed = rand() % MAX_PRIME32;
    }

    ~BSketch(){
        delete [] buckets;
    }

    uint16_t query(uint32_t key, double nowtime){
        int pos = CalculatePos(key);
        for(int i = 0; i < CELL_PER_BUCKET; ++i){
            if(buckets[pos].keys[i] == key){
                if(nowtime - buckets[pos].timestamps[i] >= flowlet_thld)
                    return 0;
                else
                    return buckets[pos].curf[i];
            }
        }
        return 0;
    }

    bool insert(uint32_t key, double nowtime){
        int pos = CalculatePos(key);
        int empty_i = -1;
        int min_freq = __INT32_MAX__;
        int min_i = -1;
        for(int i = 0; i < CELL_PER_BUCKET; ++i){
            if(buckets[pos].keys[i] == key){
                if(nowtime - buckets[pos].timestamps[i] >= flowlet_thld)
                    buckets[pos].curf[i] = 1;
                else
                    buckets[pos].curf[i]++;
                buckets[pos].freq[i]++;
                buckets[pos].timestamps[i] = nowtime;
                return true;
            }
            else if(buckets[pos].keys[i] == 0 || (nowtime - buckets[pos].timestamps[i] >= clear_thld)){
                if(empty_i == -1)
                    empty_i = i;
            }
            else if(buckets[pos].freq[i] < min_freq){
                min_freq = buckets[pos].freq[i];
                min_i = i;
            }
        }
        if(empty_i != -1){
            buckets[pos].keys[empty_i] = key;
            buckets[pos].freq[empty_i] = 1;
            buckets[pos].curf[empty_i] = 1;
            buckets[pos].timestamps[empty_i] = nowtime;
            return true;
        }
        if(--buckets[pos].freq[min_i] == 0){
            buckets[pos].keys[min_i] = key;
            buckets[pos].freq[min_i] = 1;
            buckets[pos].curf[min_i] = 1;
            buckets[pos].timestamps[min_i] = nowtime;
            return true;
        }
        return false;
    }

private:
    inline int CalculatePos(uint32_t key){
        // return CalculateBucketPos(key) % bucket_num;
        return MurmurHash3_x86_32((const char*)&key, sizeof(uint32_t), seed) % bucket_num;
    }
};



#endif // _BSKETCH_H_
