# P4 Codes of BalanceSketch

## P4 Implementation

This folder contains the P4 codes of our BalanceSketch. To process packets at line rate, Tofino switch requires the algorithms running on it to comply with many constraints. And thus the workflow of BalanceSketch on hardware platform is different from that on software platform. For the detailed workflow, please refer to Section 4.2. Here, `BlanceSketch.p4` is the data plane codes, and `tableinit.py` is the control plane codes.

## Requirements

- Please compile and run these codes on a Tofino ASIC.

## Usage

- To run the control plane codes, you must compile the P4 program `BlanceSketch.p4` with compiling option `--with-thrift`.

- Configure `ecmp_select_conf.txt`, ` arp_host_conf.txt`, and ` if_ecmp.txt` as you need.

