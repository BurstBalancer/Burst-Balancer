# Codes for Accuracy in Finding FlowBursts

We conduct CPU experiments to evaluate the accuracy of BalanceSketch in finding FlowBursts under small memory usage. 
The results show that BalanceSketch achieves >90% Recall Rate in finding FlowBursts. BalanceSketch well achieves our design goal of accurately identifying FlowBursts using small memory. And the optimized/multi-cell BalanceSketch is more efficient.

## About the codes

- `BSketch.h`: Source codes for basic BalanceSketch, including its multi-cell version. 
- `CBSketch.h`: Source codes for optimized BalanceSketch, including its multi-cell version. The optimized BalanceSketch uses the techniques of _Flow Fingerprint_, _Field Combination_, and _Compact Timestamp_ described in Section 3.4. 
- `strawman.h`: Source codes of the strawman solution in finding FlowBursts described in Section 3.1.
- `murmur3.h`: Source codes of 32-bit Murmur hash functions. 
- `param.h`: Source codes of the bucket structure in BalanceSketch and its parameters.  
- `trace.h`: Source codes for reading and parsing the datasets. 
- `main.cpp`: Source codes for testing the performance of the algorithms. 

## Implementation

Since our aim in this part of experiments is to only detect FlowBursts and do not manipulate them, we do not implement the ***next_hop*** field in BalanceSketch. BalanceSketch evicts a recorded flow once its vote field is decremented to zero. To estimate the sizes of FlowBursts, we add an extra ***flowburst_size*** field to each bucket, and this field is cleared to zero once a new flow enters the bucket or the start of a flowlet is detected. 

In the experiments, we sequentially insert each packet in the trace into BalanceSketch, and stop at a randomly chosen timepoint to test the performance of BalanceSketch. We find the ground-truth FlowBursts at this timepoint according to the predefined parameters above, and store them as golden labels in a large hash table. Then we query the ground-truth FlowBursts in our BalanceSketch, and calculate the statistics.

The hash functions we use are 32-bit Murmur hash functions, obtained [here](https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp).
We implement all the codes with C++ and build them with g++ 7.5.0 (Ubuntu 7.5.0-6ubuntu2) and -O2 option. 


## Dataset

We use the CAIDA datsets to evaluate the accuracy of finding FlowBursts. CAIDA Anonymized Internet Trace is a data stream of anonymized IP trace collected in 2018. Each packet is identified by its source IP address (4 bytes), source port number (2 bytes), destination IP address (4 bytes), destination port number (2 bytes), and protocol type (1 bytes). Each packet is associated with a 8-byte timestamps of double type. 

Here, we provide a small sample extracted from the real-world CAIDA dataset (`caida.dat`), which contains 400,000 packets of about 25,000 flows. For the full datasets, please register in [CAIDA](http://www.caida.org/home/) first, and then apply for the traces. 

**Notification:** The data file in this folder is only used for testing the performance of BalanceSketch and the related algorithms in this project. Please do not use these traces for other purpose. 




## How to run

To build and run our tests, you just need: 

```bash
$ make
$ ./bsketch
```

This will output the performance of BalanceSketch, optimized BalanceSketch, and the strawman solution to the command line, and will generate three `.csv` files containing their specific results. 

You can use `make clean` to delete the exe file and the results. 
