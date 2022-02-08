# P4 Codes of ECMP

## P4 Implementation

This folder contains the P4 codes of ECMP. Here, `simpleecmp.p4` is the data plane codes, and `tableinit.py` is the control plane codes. 

## Requirements

- Please compile and run these codes on a Tofino ASIC.

## Usage

- To run the control plane codes, you must compile the P4 program `simpleecmp.p4` with compiling option `--with-thrift`. 

- Configure `ecmp_select_conf.txt` and `arp_host_conf`.txt as you need. 









