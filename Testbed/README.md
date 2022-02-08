# Codes for Testbed Experiments

We have fully implemented our BurstBalancer prototype on a testbed with 4 Edgecore Wedge 100BF-32X switches (with Tofino ASIC) and 16 end-hosts in a Leaf-Spine topology. On each switch, we develop BalanceSketch using P4 language. The results show that in asymmetric topologies, BurstBalancer achieves up to $30\times$ smaller FCT than LetFlow and up to $6.4\times$ smaller FCT than WCMP.


## File Description

- `BurstBalancer`: Source codes of our BalanceSketch implemented on Tofino platform using P4 language.

- `ECMP`: Source codes of ECMP implemented on Tofino platform using P4 language. 

- `LetFlow`: Source codes of the Flowlet Table in LetFlow implemented on Tofino platform using P4 language. 

- More details can be found in the folders.