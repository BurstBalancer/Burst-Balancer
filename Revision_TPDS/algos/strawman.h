#ifndef _STRAWMAN_H_
#define _STRAWMAN_H_



#include "common/param.h"
#include <immintrin.h>

#include "common/murmur3.h"

struct SBucket{
public:
    uint32_t key;
    uint32_t freq;
    double stime;
};



class Strawman{
public:
    SBucket* buckets;
    double * ftable;

    double flowlet_thld;
    double clear_thld;
    double speed_thld;

    uint32_t ftable_size;
    uint32_t bucket_num;
    uint32_t seedb;  // flowlet table seed
    uint32_t seedf;  // bucket position seed

    Strawman(uint32_t mem_size, double flowlet_thld_, double clear_thld_, double speed_thld_){
        bucket_num = (mem_size / 2 + sizeof(SBucket) - 1) / sizeof(SBucket);
        ftable_size = mem_size / 2 / sizeof(double);

        // printf("flowlet table size: %d, bucket num: %d\n", ftable_size, bucket_num);

        flowlet_thld = flowlet_thld_;
        clear_thld = clear_thld_;
        speed_thld = speed_thld_;

        ftable = new double[ftable_size];
        buckets = new(std::align_val_t{64}) SBucket[bucket_num];
        memset(ftable, 0, ftable_size * sizeof(double));
        memset(buckets, 0, bucket_num * sizeof(SBucket));

        seedb = rand() % MAX_PRIME32;
        seedf = rand() % MAX_PRIME32;
    }

    ~Strawman(){
        delete [] ftable;
        delete [] buckets;
    }

    uint32_t query(uint32_t key, double nowtime){
        int bpos = CalculateBPos(key);
        if(buckets[bpos].key == key)
            return buckets[bpos].freq;
        return 0;
    }

    bool insert(uint32_t key, double nowtime){
        bool flowlet_start = false;
        int fpos = CalculateFPos(key);
        if(abs(nowtime - ftable[fpos]) > flowlet_thld)
            flowlet_start = true;
        ftable[fpos] = nowtime;

        int bpos = CalculateBPos(key);
        if(buckets[bpos].key == key){
            buckets[bpos].freq ++;
            return true;
        }
        else if(flowlet_start == true){
            double speed = (double) buckets[bpos].freq / (nowtime - buckets[bpos].stime);
            if(buckets[bpos].key == 0 || (buckets[bpos].freq > 1 && speed < speed_thld)){
                buckets[bpos].key = key;
                buckets[bpos].freq = 1;
                buckets[bpos].stime = nowtime;
                return true;
            }
        }
        return false;
    }

private:
    inline int CalculateFPos(uint32_t key){
        return MurmurHash3_x86_32((const char*)&key, sizeof(uint32_t), seedf) % ftable_size;
    }
    inline int CalculateBPos(uint32_t key){
        return MurmurHash3_x86_32((const char*)&key, sizeof(uint32_t), seedb) % bucket_num;
    }
};



#endif // _STRAWMAN_H_
