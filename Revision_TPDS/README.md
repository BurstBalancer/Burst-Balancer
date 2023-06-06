# Codes for Accuracy in Finding FlowBursts

We conducted CPU experiments to evaluate the accuracy of BalanceSketch in finding FlowLets, detecting inflated queuing delay, detecting packet drops, and estimating the timespan of FlowBurst. We also added two more CPU experiments to show the ratios of manipulated flows/packets and recorded packets.

## About the codes

- `algos`: Source code for our algorithms and strawman.
    - `BSketch.h`: Source code for basic BalanceSketch, including its multi-cell version. 
    - `BSketchD.h`: Source code for basic BalanceSketch with a new property for calculating the duration of a stream, including its multi-cell version. 
    - `BSketchT.h`: Source code for basic BalanceSketch with a new property for calculating the duration of a stream and checking whether this stream is manipulated, including its multi-cell version. 
    - `CBSketch.h`: Source code for optimized BalanceSketch, including its multi-cell version. The optimized BalanceSketch uses the techniques of _Flow Fingerprint_, _Field Combination_, and _Compact Timestamp_, described in Section 3.4. 
    - `strawman.h`: Source code of the strawman solution for finding FlowBursts, described in Section 3.1.
- `common`: Source code of some basic functions and structures.
    - `murmur3.h`: Source code of 32-bit Murmur hash functions. 
    - `param.h`: Source code for the bucket structure in BalanceSketch and its parameters.  
    - `trace.h`: Source code for reading and parsing the datasets. 
- `active.cpp`: Source code for per-hop heavy hitter detection. 
- `delay.cpp`: Source code for detecting inflated queuing delay. 
- `duration.cpp`: Source code for estimating the timespan of FlowBurst. 
- `loss.cpp`: Source code for detecting packet drops. 
- `thld.cpp`: Source code for testing the ratio of manipulated flows/pkts. 
- `percent.cpp`: Source code for testing the ratio of recorded pkts. 

## Implementation

In this part of the experiments, our aim is only to detect FlowLet without manipulating them, so we do not need to implement the **next_hop** field in BalanceSketch. BalanceSketch evicts a recorded flow once its vote field is decremented to zero. 

We use 32-bit Murmur hash functions obtained [here](https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp). We implement all the codes with C++ and build them with g++ 7.5.0 (Ubuntu 7.5.0-6ubuntu2) and the -O2 option. 

### per-hop heavy hitter detection

To estimate the sizes of the FlowLet, we add an extra **flowlet_size** field to each bucket, which is cleared to zero once a new flow enters the bucket or the start of a FlowLet is detected.

We sequentially insert each packet in the trace into BalanceSketch and stop at a randomly chosen time point to test the performance of BalanceSketch. We find the ground-truth FlowLet at this timepoint according to the predefined parameters above, and store them as golden labels in a large hash table. Then we query the ground-truth FlowLet in our BalanceSketch and calculate the statistics.

### Estimating FlowBurst timespan

To estimate the timespan of a FlowBurst, we add an extra ***flowlet_starttime*** field to each bucket, which is set to the current timestamp once a new flow enters the bucket or the start of a flowlet is detected. 

We sequentially insert each packet in the trace into BalanceSketch and stop at a randomly chosen timepoint to test the performance of BalanceSketch. We find the ground-truth FlowBurst at this timepoint according to the predefined parameters above, and store them as golden labels in a large hash table. Then we query the ground-truth FlowBurst in our BalanceSketch and calculate the statistics.

### Packet drops detection

We simulated a packet-drop scenario with two BalanceSketches. The upstream BalanceSketch records all packets, while the downstream BalanceSketch randomly drops packets based on a fixed loss rate. We then compare the number of packets recorded by both the upstream and downstream BalanceSketches to detect any difference, which indicates packet loss. To estimate the sizes of a FlowBurst, we use the same extra **flowlet_size** field as in per-hop heavy hitter detection.

We used the FlowBurst label values obtained from the upstream to calculate the ground truth, and then compared it with the values obtained from the downstream.

### Inflated queuing delay detection

We simulated a delay scenario with two BalanceSketches. The upstream BalanceSketch records all packets. Before inserting packets into the downstream, some time points were randomly selected according to the delay rate, and a random delay was added to all packets after each time point. The processed packets were then inserted into the second BalanceSketch. Subsequently, the difference in FlowLet duration detected by comparing the two BalanceSketches was used to determine if a delay had occurred. To estimate the duration of a FlowLet, we use the same extra ***flowlet_starttime*** field for each bucket as in estimating FlowBurst timespan.

We used the FlowBurst label values obtained from the upstream to calculate the ground truth, and then compared them with the values obtained from the downstream.

### Ratio of manipulated flows/pkts

We simulate the current network load by assuming the current network bandwidth. We use the speed threshold calculation function given in the paper to calculate the corresponding manipulate speed threshold and scan the dataset using this threshold to obtain the theoretical proportion of packets that should be manipulated. Meanwhile, this threshold is used as the threshold in the BalanceSketch to obtain the actual proportion of packets that are manipulated.

### Ratio of recorded pkts

We pause at specific time intervals at corresponding time points in the dataset and calculate the number of packets in the Sketch and the number of actual FlowLet packets after each pause to calculate the corresponding ratio.

## Dataset

We use the CAIDA datasets to evaluate the accuracy of finding FlowBursts. The CAIDA Anonymized Internet Trace is a datastream of anonymized IP trace collected in 2018. Each packet is identified by its source IP address (4 bytes), source port number (2 bytes), destination IP address (4 bytes), destination port number (2 bytes), and protocol type (1 byte). Each packet is associated with an 8-byte timestamp of double type. 

Here, we provide a small sample extracted from the real-world CAIDA dataset (`caida.dat`), containing 400,000 packets of about 25,000 flows. For the full datasets, please register with [CAIDA](http://www.caida.org/home/) and apply for the traces. 

**Note**: The data file in this folder is only used for testing the performance of BalanceSketch and the related algorithms in this project. Please do not use these traces for any other purpose.

## How to run

To build and run our tests, perform the following steps:

1. Create the "build" and "results" directories: 

    ```bash
    $ mkdir build results 
    ```

2. Change to the "build" directory: 

    ```bash
    $ cd build
    ```

3. Run Cmake to generate makefiles: 

    ```bash
    $ cmake ..
    ```

4. Build the executables by running make:

    ```bash
    $ make
    ```

5. Run each experiment by running the corresponding executable:

    ```bash
    $ ./active
    $ ./duration
    $ ./delay [delay_rate]
    $ ./loss [loss_rate]
    $ ./percent
    $ ./thld
    ```

The results for each experiment are stored in the "results" folder. 
