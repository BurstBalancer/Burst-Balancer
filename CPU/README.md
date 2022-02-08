# Codes on CPU Platform 

We conduct CPU experiments to evaluate the accuracy of BalanceSketch in finding FlowBursts under small memory usage, and the load balance performance of BurstBalancer on a single switch. Our experiments are conducted on an 18-core CPU server (Intel i9-10980XE) with 128GB DDR4 memory and 24.75MB L3 cache. We set the CPU frequency to 4.2GHz, and set the memory frequency to 3200MHz. All codes are implemented using C++. 


## File Description 

- `BalanceSketch`: Source codes for evaluating the accuracy of finding FlowBursts, including codes of BalanceSketch, the optimized version of BalanceSketch, multi-cell BalanceSketch, and the strawman solution described in Section 3.1. 

- `SingleSwitch`: Source codes for evluating the load balancing performance on a single switch, including codes of BurstBalancer and LetFlow. 

- More details can be found in the folders.