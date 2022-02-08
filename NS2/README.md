# Codes for NS-2 Simulations

We evaluate BurstBalancer using an event-level network simulator, Network Simulator 2 (NS-2), in large-scale symmetric topologies, where we compare BurstBalancer against ECMP, DRILL, and LetFlow under different network loads. We also evaluate the performance of BurstBalancer and LetFlow using tables of different sizes, validating the memory efficiency of BurstBalancer. The NS-2 simulator codes of this paper are modified from [PIAS-NS2](https://github.com/HKUST-SING/PIAS-NS2).


## File Description

- `ns-2.34`: Modified NS-2 codes for evluating BurstBalancer and the compared schemes in large-scale simulations.

- `scripts`: Scripts for running the simulations and analyzing the results, and configuration files of the workloads, topologies, and connections. 
  - `run.py`: Script to run the simulations. 
  - `analysis.py`: Script to analyze the results.  
  - `CDF_dctcp.tcl`: CDF of the web search workload in our paper. 
  - `CDF_msg.tcl`: CDF of the RPC workload in our paper. 
  - `spine_empirical.tcl`: Configuration file for constructing topologies. 
  - `tcp-common-opt.tcl`: Configuration file for mataining TCP connections. 


## Installation

First, install ns-2 according to the instructions of [PIAS-NS2](https://github.com/HKUST-SING/PIAS-NS2).

Replace the original folder `ns-allinone-2.34/ns-2.34` with `ns-2.34` in this repository.

Run `configure.sh` in `ns-allinone-2.34/ns-2.34`.

Add `OBJ_CC`: add `queue/priority.o` and `classifier/murmur3.o` to `ns-allinone-2.34/ns-2.34/Makefile`.

Add `CCOPT`: add `-O2`, `-std=c++11`, and `-Wno-narrowing`  to `ns-allinone-2.34/ns-2.34/Makefile`.

Run `make` in `ns-allinone-2.34/ns-2.34`.

## Running Large-scale Simulations

Run `scripts/run.py` for large-scale simulations.

You should replace `ns_path` with the correct path.

The parameter configurations used in different experiments have been indicated in `scripts/run.py` by comments.

Run `scripts/analysis.py` to analyze the simulation results.

## Modified Files

* `ns-2.34/classifier.h`, `ns-2.34/classifier.cc`, and `ns-2.34/classifier-mpath.cc`: We modify them to add the support for BurstBalancer, LetFlow, and DRILL.
* `scripts/run.py`, `scripts/spine_empirical.tcl`, `scripts/analysis.py`: We modify them to add the support for generating background traffic.
* `scripts/tcp-common-opt.tcl`: We fix some bugs. 