#ifndef _PARAM_H_
#define _PARAM_H_

#include <iostream>
#include <string>
#include <cstring>
#include <cmath>
#include <vector>
#include <fstream>
#include <set>
#include <map>
#include <cstdlib>
#include <algorithm>

using namespace std;


#define CONSTANT_NUMBER 2654435761u
#define CalculateBucketPos(fp) (((fp) * CONSTANT_NUMBER) >> 15)

#define CELL_PER_BUCKET 1


struct Bucket{
public:
    uint32_t keys[CELL_PER_BUCKET];
    uint32_t freq[CELL_PER_BUCKET];
    uint16_t curf[CELL_PER_BUCKET];
    double   timestamps[CELL_PER_BUCKET];
};

struct BucketD{
public:
    uint32_t keys[CELL_PER_BUCKET];
    uint32_t freq[CELL_PER_BUCKET];
    uint16_t curf[CELL_PER_BUCKET];
    double   firstTimestamps[CELL_PER_BUCKET];
    double   lastTimestamps[CELL_PER_BUCKET];
};

struct BucketT{
public:
    uint32_t keys[CELL_PER_BUCKET];
    uint32_t freq[CELL_PER_BUCKET];
    uint16_t curf[CELL_PER_BUCKET];
    double   firstTimestamps[CELL_PER_BUCKET];
    double   lastTimestamps[CELL_PER_BUCKET];
    bool scheduled[CELL_PER_BUCKET];
};

struct CBucket{
public:
    uint16_t freq[CELL_PER_BUCKET];
    uint16_t curf[CELL_PER_BUCKET];
    uint16_t fps[CELL_PER_BUCKET];
    // double timestamps[CELL_PER_BUCKET];
    uint8_t timestamps[CELL_PER_BUCKET];
};




#endif // _PARAM_H_
