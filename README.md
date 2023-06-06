# BurstBalancer


## Introduction

Layer-3 load balancing is a key topic in the networking field. It is well acknowledged that flowlet is the most promising solution because of its good trade-off between load balance and packet reordering. However, we find its another limitation: it makes the forwarding paths of flows unpredictable. To address this limitation, this paper presents BurstBalancer, a simple yet efficient load balancing system with a sketch, named BalanceSketch. Our design philosophy is doing less changes to keep the forwarding path of most flows fixed, which guides the design of BalanceSketch and balance operations. We have fully implemented BurstBalancer in a testbed and conducted both event-level and ESL (electronic system level) simulations. Our results show that BurstBalancer achieves 5%∼35% and up to $30\times$ smaller FCT in symmetric and asymmetric topologies, respectively, while 58 times less flows suffer from path changing. Related codes are open-sourced anonymously.


## About this repo

- `CPU` contains codes implemented on CPU platforms:
  - `BalanceSketch`: Source codes of BalanceSketch, including the optimized version of BalanceSketch and multi-cell BalanceSketch. 
  - `SingleSwitch`: Source codes of BurstBalancer and LetFlow on a single switch. 

- `NS2` contains codes implemented on NS-2.

- `Testbed` contains codes related to our testbed. We have implemented BalanceSketch on a Edgecore Wedge 100BF-32X switch (with Tofino ASIC), and build our BurstBalancer prototype on a small-scale testbed. 

- `Math` contains codes related to our numerical verification of the accuracy of BalanceSketch. 

- `Revision_TPDS` contains codes of the supplementary experiments after revision.

- More details can be found in the folders.

