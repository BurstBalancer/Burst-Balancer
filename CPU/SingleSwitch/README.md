# Codes of BurstBalancer on a Single Switch

We evaluate the load balance performance of BurstBalancer on single switch and compare it against ECMP and LetFlow. We use C to implement the load balancing module of a 128-port switch, on which we deploy the Flowlet Tables (LetFlow) and the BalanceSketchs with different sizes (2K/4K # entries/buckets). In our experiments, we generate a synthetic workload that is of heavy-tailed distribution, where the average flow size is about 30 packets. We present a sample of our workload in `./data/sample.pcap`. 


## About the codes

 - `analysis.h` and `analysis.c`:
 Codes analyzing the load balance performance of BurstBalancer, ECMP, and LetFlow. 

 - `data_set.h` and `data_set.c`:
Data structures and basic operations, e.g., initialization, insertion, lookup, and release.

 - `handle.h` and `handle.c`:
Implementations of load balancing algorithms, including BurstBalancer, ECMP, and LetFlow.
 
 - `packet.h` and `packet.c`: Parser for `.pcap` formatted workloads. 

 - `common.h`:
Common header files.

 - `main.c`: 
Driver of our program. It also reads and parses configuration files.



## Configuration 

Before running, you should modify the configuration file in `./data/parameter_configure` to specify the algorithm to run and its parameters. Skip the lines start with '#', and configure the following variables: 

- `srand`: A random seed.
- `pcap_file_name`: Path to the workload (in `pcap` format).
- `pkt_num`: Number of packets to be read in the workload. 
- `lb_scheme`: Choose a load balance scheme (per flow / per packet / flowlet). 
- `flowlet_algo`: If `lb_scheme=2` (flowlet), choose a load balance algorithm to schedule flowlets (LetFlow / BurstBalancer). 
- `delta`: Flowlet threshold that spaces two adjacent flowlets. 
- `link_delay_range_majority`: The maximum link delay. 
- `link_delay_range_minority`: The minimum link delay. 
- `link_delay_majority_percentage`: Percent of links with the maximum delay. 


## How to run

You can use the following commands to build and run our experiments.

```
cd path/to/this
make
./demo
```



## About the output
Our program saves the results in `./data/output.txt`, and prints the following information to the command line: 
- `Algorithm` : The chosen load balancing algorithm and its table size. 
- `redispatch pkt num` : The number and ratio of reordering packets. 
- `redispatch flow num` : The number and ratio of manipulated flows. 
- `unbalancing-ratio`: Ratio of number of packets at the most loaded port to the least loaded port. 
- `unbalancing-variance`: Variance of number of packets across all ports. 
