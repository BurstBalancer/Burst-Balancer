# P4 Codes of the Flowlet Table 

## P4 Implementation

This folder contains the P4 codes of the Flowlet Table in LetFlow or other flowlet-level load balancing schemes. Here, `flowlet_switching.p4` is the data plane codes, and `tableinit.py` is the control plane codes. 

## Requirements

- Please compile and run these codes on a Tofino ASIC.

## Usage

- To run the control plane, you must compile the P4 program `flowlet_switching.p4` with compiling option `--with-thrift`. 

- Configure `ecmp_select_conf.txt`, `arp_host_conf.txt`, and`if_ecmp.txt` as you need. 







