#!/bin/python
import threading
import os
import Queue


def worker():
    while True:
        try:
            j = q.get(block=0)
        except Queue.Empty:
            return
        # Make directory to save results
        os.system('mkdir '+j[1])
        os.system(j[0])


q = Queue.Queue()

sim_end = 50000000
link_rate = 10
mean_link_delay = 0.000000200
host_delay = 0.000020
queueSize = 240

# exp - load - (ECMP, DRILL, LetFlow, BB)
# load_arr_1 = [0.50, 0.55, 0.60, 0.65, 0.70, 0.75, 0.80, 0.85, 0.90, 0.95, 1.00]
# load_arr_2 = [0.10, 0.10, 0.10, 0.10, 0.10, 0.10, 0.10, 0.10, 0.10, 0.10, 0.10]

# exp - bucket - (LetFlow, BB)
load_arr_1 = [0.90]
load_arr_2 = [0.10]

connections_per_pair = 1
meanFlowSize_1 = 2510778
flow_cdf_1 = 'CDF_dctcp.tcl'
meanFlowSize_2 = 185
flow_cdf_2 = 'CDF_msg.tcl'

initWindow = 70
ackRatio = 1
slowstartrestart = 'true'
DCTCP_g = 0.0625
min_rto = 0.002
prob_cap_ = 5

DCTCP_K = 65.0
drop_prio_ = 'true'
prio_scheme_ = 2
deque_prio_ = 'true'
keep_order_ = 'true'
ECN_scheme_ = 2  # Per-port ECN marking

topology_spt = 16
topology_tors = 8
topology_spines = 8
topology_x = 2

# exp - load - ECMP
# memsize_arr = [250]
# delta_flow_arr = [5e-2]
# delta_flet_arr = [2e-4]
# balanceAlg_ = 0

# exp - load - BB
memsize_arr = [250]
delta_flow_arr = [5e-2]
delta_flet_arr = [2e-4]
balanceAlg_ = 1

# exp - load - LetFlow
# memsize_arr = [250]
# delta_flow_arr = [5e-2]
# delta_flet_arr = [2e-4]
# balanceAlg_ = 1

# exp -load - DRILL
# memsize_arr = [1]
# delta_flow_arr = [0]
# delta_flet_arr = [0]
# balanceAlg_ = 3

seed_arr = [528]

ns_path = '/root/Hotlet/opensource/ns-hotlet/ns-2.34/ns'
sim_script = 'spine_empirical.tcl'

for i in range(len(load_arr_1)):
    for j in range(len(delta_flow_arr)):
        for k in range(len(memsize_arr)):
            for l in range(len(seed_arr)):
                scheme = 'dctcp'

                # Directory name: workload_scheme_load_[load]
                directory_name = 'load_%d_%s_%d_%d_%d_%d_%d' % (
                    balanceAlg_, scheme, int(load_arr_1[i]*100), int(load_arr_2[i]*100), int(delta_flet_arr[j]*1e9), memsize_arr[k], seed_arr[l])
                directory_name = directory_name.lower()
                # Simulation command
                cmd = ns_path+' '+sim_script+' '\
                    + str(sim_end)+' '\
                    + str(link_rate)+' '\
                    + str(mean_link_delay)+' '\
                    + str(host_delay)+' '\
                    + str(queueSize)+' '\
                    + str(load_arr_1[i])+' '\
                    + str(load_arr_2[i])+' '\
                    + str(connections_per_pair)+' '\
                    + str(meanFlowSize_1)+' '\
                    + str(flow_cdf_1)+' '\
                    + str(meanFlowSize_2)+' '\
                    + str(flow_cdf_2)+' '\
                    + str(initWindow)+' '\
                    + str(ackRatio)+' '\
                    + str(slowstartrestart)+' '\
                    + str(DCTCP_g)+' '\
                    + str(min_rto)+' '\
                    + str(prob_cap_)+' '\
                    + str(DCTCP_K)+' '\
                    + str(drop_prio_)+' '\
                    + str(prio_scheme_)+' '\
                    + str(deque_prio_)+' '\
                    + str(keep_order_)+' '\
                    + str(ECN_scheme_)+' '\
                    + str(topology_spt)+' '\
                    + str(topology_tors)+' '\
                    + str(topology_spines)+' '\
                    + str(topology_x)+' '\
                    + str(memsize_arr[k])+' '\
                    + str(delta_flow_arr[j])+' '\
                    + str(delta_flet_arr[j])+' '\
                    + str(balanceAlg_)+' '\
                    + str(seed_arr[l])+' '\
                    + str('./'+directory_name+'/flow.tr')+'  >'\
                    + str('./'+directory_name+'/logFile.tr')+' 2>'\
                    + str('./'+directory_name+'/err.tr')

                print(cmd)
                q.put([cmd, directory_name])

# Create all worker threads
threads = []
number_worker_threads = 36

# Start threads to process jobs
for i in range(number_worker_threads):
    t = threading.Thread(target=worker)
    threads.append(t)
    t.start()

# Join all completed threads
for t in threads:
    t.join()
