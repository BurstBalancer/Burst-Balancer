#ifndef _BSKETCHD_H_
#define _BSKETCHD_H_


// #define USING_SIMD

#include "common/param.h"
#include <immintrin.h>

#include "common/murmur3.h"

class BSketchD{
public:
    BucketD* buckets;
    double flowlet_thld;
    double clear_thld;
    uint32_t bucket_num;
    uint32_t seed;

    BSketchD(uint32_t mem_size, double flowlet_thld_, double clear_thld_){
        bucket_num = (mem_size + sizeof(BucketD) - 1) / sizeof(BucketD);
        // printf("bucket num: %d\n", bucket_num);
        flowlet_thld = flowlet_thld_;
        clear_thld = clear_thld_;
        buckets = new(std::align_val_t{64}) BucketD[bucket_num];
        memset(buckets, 0, bucket_num * sizeof(BucketD));
        seed = rand() % MAX_PRIME32;
    }

    ~BSketchD(){
        delete [] buckets;
    }

    uint32_t flowletPktCount(double nowtime) {
        uint32_t cnt = 0;
        for (int i = 0; i < bucket_num; i++)
            for (int j = 0; j < CELL_PER_BUCKET; j++)
                if (buckets[i].keys[j] != 0 && nowtime - buckets[i].lastTimestamps[j] < flowlet_thld)
                    cnt += buckets[i].curf[j];
        return cnt;
    }

    uint32_t flowCount() {
        uint32_t cnt = 0;
        for (int i = 0; i < bucket_num; i++)
            for (int j = 0; j < CELL_PER_BUCKET; j++)
                if (buckets[i].keys[j] != 0)
                    cnt++;
        return cnt;
    }

    uint16_t query(uint32_t key, double nowtime){
        int pos = CalculatePos(key);
        for(int i = 0; i < CELL_PER_BUCKET; ++i){
            if(buckets[pos].keys[i] == key){
                if(nowtime - buckets[pos].lastTimestamps[i] >= flowlet_thld)
                    return 0;
                else
                    return buckets[pos].curf[i];
            }
        }
        return 0;
    }

    double queryDuration(uint32_t key, double nowtime){
        int pos = CalculatePos(key);
        for(int i = 0; i < CELL_PER_BUCKET; ++i){
            if(buckets[pos].keys[i] == key){
                if(nowtime - buckets[pos].lastTimestamps[i] >= flowlet_thld)
                    return 0;
                else
                    return nowtime - buckets[pos].firstTimestamps[i];
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
                if(nowtime - buckets[pos].lastTimestamps[i] >= flowlet_thld) {
                    buckets[pos].firstTimestamps[i] = nowtime;
                    buckets[pos].curf[i] = 1;
                }else
                    buckets[pos].curf[i]++;
                buckets[pos].freq[i]++;
                buckets[pos].lastTimestamps[i] = nowtime;
                return true;
            }
            else if(buckets[pos].keys[i] == 0 || (nowtime - buckets[pos].lastTimestamps[i] >= clear_thld)){
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
            buckets[pos].firstTimestamps[empty_i] = nowtime;
            buckets[pos].lastTimestamps[empty_i] = nowtime;
            return true;
        }
        if(--buckets[pos].freq[min_i] == 0){
            buckets[pos].keys[min_i] = key;
            buckets[pos].freq[min_i] = 1;
            buckets[pos].curf[min_i] = 1;
            buckets[pos].firstTimestamps[min_i] = nowtime;
            buckets[pos].lastTimestamps[min_i] = nowtime;
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



#endif // _BSKETCHD_H_
