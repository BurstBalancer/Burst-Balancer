source "tcp-common-opt.tcl"

set ns [new Simulator]
puts "Date: [clock format [clock seconds]]"
set sim_start [clock seconds]

if {$argc != 34} {
    puts "wrong number of arguments $argc"
    exit 0
}

set sim_end [lindex $argv 0]
set link_rate [lindex $argv 1]
set mean_link_delay [lindex $argv 2]
set host_delay [lindex $argv 3]
set queueSize [lindex $argv 4]
set load_a [lindex $argv 5]
set load_b [lindex $argv 6]
set connections_per_pair [lindex $argv 7]

set meanFlowSize_a [lindex $argv 8]
set flow_cdf_a [lindex $argv 9]
set meanFlowSize_b [lindex $argv 10]
set flow_cdf_b [lindex $argv 11]

#### Transport settings options
set initWindow [lindex $argv 12]
set ackRatio [lindex $argv 13]
set slowstartrestart [lindex $argv 14]
set DCTCP_g [lindex $argv 15] ; # DCTCP alpha estimation gain
set min_rto [lindex $argv 16]
set prob_cap_ [lindex $argv 17] ; # Threshold of consecutive timeouts to trigger probe mode

#### Switch side options
set DCTCP_K [lindex $argv 18]
set drop_prio_ [lindex $argv 19]
set prio_scheme_ [lindex $argv 20]
set deque_prio_ [lindex $argv 21]
set keep_order_ [lindex $argv 22]
set ECN_scheme_ [lindex $argv 23]

#### topology
set topology_spt [lindex $argv 24]
set topology_tors [lindex $argv 25]
set topology_spines [lindex $argv 26]
set topology_x [lindex $argv 27]

set memsize_ [lindex $argv 28]
set delta_flow_ [lindex $argv 29]
set delta_flet_ [lindex $argv 30]
set balanceAlg_ [lindex $argv 31]
set seed_ [lindex $argv 32]

### result file
set flowlog [open [lindex $argv 33] w]

#### Packet size is in bytes.
set pktSize 1460
#### trace frequency
set queueSamplingInterval 0.0001
#set queueSamplingInterval 1

puts "Simulation input:"
puts "topology: spines server per rack = $topology_spt, x = $topology_x"
puts "sim_end $sim_end"
puts "link_rate $link_rate Gbps"
puts "link_delay $mean_link_delay sec"
puts "host_delay $host_delay sec"
puts "queue size $queueSize pkts"
puts "load 1 $load_a"
puts "load 2 $load_b"
puts "connections_per_pair $connections_per_pair"
puts "TCP initial window: $initWindow"
puts "ackRatio $ackRatio"
puts "DCTCP_g $DCTCP_g"
puts "slow-start Restart $slowstartrestart"
puts "DCTCP_K_ $DCTCP_K"
puts "pktSize(payload) $pktSize Bytes"
puts "pktSize(include header) [expr $pktSize + 40] Bytes"

puts " "

################# Transport Options ####################
Agent/TCP set ecn_ 1
Agent/TCP set old_ecn_ 1
Agent/TCP set packetSize_ $pktSize
Agent/TCP/FullTcp set segsize_ $pktSize
Agent/TCP/FullTcp set spa_thresh_ 0
Agent/TCP set slow_start_restart_ $slowstartrestart
Agent/TCP set windowOption_ 0
Agent/TCP set minrto_ $min_rto
Agent/TCP set tcpTick_ 0.000001
Agent/TCP set maxrto_ 64
Agent/TCP set lldct_w_min_ 0.125
Agent/TCP set lldct_w_max_ 2.5
Agent/TCP set lldct_size_min_ 204800
Agent/TCP set lldct_size_max_ 1048576

Agent/TCP/FullTcp set nodelay_ true; # disable Nagle
Agent/TCP/FullTcp set segsperack_ $ackRatio;
Agent/TCP/FullTcp set interval_ 0.000006

if {$ackRatio > 2} {
    Agent/TCP/FullTcp set spa_thresh_ [expr ($ackRatio - 1) * $pktSize]
}

set sourceAlg DCTCP-Sack
#DCTCP-Sack
Agent/TCP set ecnhat_ true
Agent/TCPSink set ecnhat_ true
Agent/TCP set ecnhat_g_ $DCTCP_g
Agent/TCP set lldct_ false

#Shuang
Agent/TCP/FullTcp set prio_scheme_ $prio_scheme_;
Agent/TCP/FullTcp set dynamic_dupack_ 1000000; #disable dupack
Agent/TCP set window_ 1000000
Agent/TCP set windowInit_ $initWindow
Agent/TCP set rtxcur_init_ $min_rto;
Agent/TCP/FullTcp/Sack set clear_on_timeout_ false;
Agent/TCP/FullTcp/Sack set sack_rtx_threshmode_ 2;
Agent/TCP/FullTcp set prob_cap_ $prob_cap_;

set switchAlg Priority
#Priority
Agent/TCP/FullTcp set enable_pias_ true
Agent/TCP/FullTcp set pias_prio_num_ 1
Agent/TCP/FullTcp set pias_debug_ false
Agent/TCP/FullTcp set pias_thresh_0 0
Agent/TCP/FullTcp set pias_thresh_1 0
Agent/TCP/FullTcp set pias_thresh_2 0
Agent/TCP/FullTcp set pias_thresh_3 0
Agent/TCP/FullTcp set pias_thresh_4 0
Agent/TCP/FullTcp set pias_thresh_5 0
Agent/TCP/FullTcp set pias_thresh_6 0


if {$queueSize > $initWindow } {
    Agent/TCP set maxcwnd_ [expr $queueSize - 1];
} else {
    Agent/TCP set maxcwnd_ $initWindow
}

# set myAgent "Agent/TCP/FullTcp/Sack/MinTCP";
set myAgent "Agent/TCP/FullTcp";

################# Switch Options ######################
Queue set limit_ $queueSize

Queue/Priority set queue_num_ 1
Queue/Priority set thresh_ $DCTCP_K
Queue/Priority set mean_pktsize_ [expr $pktSize+40]
Queue/Priority set marking_scheme_ $ECN_scheme_

############## Multipathing ###########################
$ns rtproto DV
Agent/rtProto/DV set advertInterval	[expr 2*$sim_end]
Node set multiPath_ 1
Classifier/MultiPath set perflow_ 1
Agent/TCP/FullTcp set dynamic_dupack_ 0; # enable duplicate ACK

#LoadBalance
Classifier set memsize_ $memsize_
Classifier/MultiPath set balanceAlg_ $balanceAlg_
Classifier/MultiPath set delta_flow_ $delta_flow_
Classifier/MultiPath set delta_flet_ $delta_flet_
Classifier/MultiPath set seed_ $seed_

############# Topoplgy #########################
set S [expr $topology_spt * $topology_tors] ; #number of servers
set UCap [expr $link_rate * $topology_spt / $topology_spines / $topology_x] ; #uplink rate

puts "UCap: $UCap"

for {set i 0} {$i < $S} {incr i} {
    set s($i) [$ns node]
}

for {set i 0} {$i < $topology_tors} {incr i} {
    set n($i) [$ns node]
}

for {set i 0} {$i < $topology_spines} {incr i} {
    set a($i) [$ns node]
}

############ Edge links ##############
for {set i 0} {$i < $S} {incr i} {
    set j [expr $i/$topology_spt]
    $ns duplex-link $s($i) $n($j) [set link_rate]Gb [expr $host_delay + $mean_link_delay] $switchAlg
}

############ Core links ##############
for {set i 0} {$i < $topology_tors} {incr i} {
    for {set j 0} {$j < $topology_spines} {incr j} {
				puts "$i $j"
				# if {(($i == 0) && ($j == 0))} {
				# 		$ns duplex-link $n($i) $a($j) [expr $UCap/2.0]Gb $mean_link_delay $switchAlg
				# } else {
				# 		$ns duplex-link $n($i) $a($j) [set UCap]Gb $mean_link_delay $switchAlg
				# }
				$ns duplex-link $n($i) $a($j) [set UCap]Gb $mean_link_delay $switchAlg
    }
}

#############  Agents ################
set lambda_a [expr ($link_rate*$load_a*1000000000)/($meanFlowSize_a*8.0/1460*1500)/($topology_x)]
set lambda_b [expr ($link_rate*$load_b*1000000000)/($meanFlowSize_b*8.0/1460*1500)/($topology_x)]
puts "Arrival 1: Poisson with inter-arrival [expr 1/$lambda_a * 1000] ms"
puts "Arrival 2: Poisson with inter-arrival [expr 1/$lambda_b * 1000] ms"

puts "Setting up connections ..."; flush stdout

set flow_gen 0
set flow_fin 0

set init_fid 0
for {set j 0} {$j < $S } {incr j} {
    for {set i 0} {$i < $S } {incr i} {
			  set tor_i [expr $i / $topology_spt]
			  set tor_j [expr $j / $topology_spt]
        # if {($tor_i == $tor_j) && ($i != $j)} {
        #         set agtagr($i,$j) [new Agent_Aggr_pair]
        #         $agtagr($i,$j) setup $s($i) $s($j) "$i $j" $connections_per_pair $init_fid "TCP_pair"
        #         $agtagr($i,$j) attach-logfile $flowlog

        #         puts -nonewline "($i,$j) "
        #         #For Poisson/Pareto
        #         $agtagr($i,$j) set_PCarrival_process [expr $lambda/($S - $topology_spt - 1)] $flow_cdf [expr 17*$i+1244*$j] [expr 33*$i+4369*$j]

        #         $ns at 0.1 "$agtagr($i,$j) warmup 0.5 5"
        #         $ns at 1 "$agtagr($i,$j) init_schedule"

        #         set init_fid [expr $init_fid + $connections_per_pair];
				# }
        # if {$i != $j} {
        #         set agtagr($i,$j) [new Agent_Aggr_pair]
        #         $agtagr($i,$j) setup $s($i) $s($j) "$i $j" $connections_per_pair $init_fid "TCP_pair"
        #         $agtagr($i,$j) attach-logfile $flowlog

        #         puts -nonewline "($i,$j) "
        #         #For Poisson/Pareto
        #         $agtagr($i,$j) set_PCarrival_process [expr $lambda/($S - 1)] $flow_cdf [expr 17*$i+1244*$j] [expr 33*$i+4369*$j]

        #         $ns at 0.1 "$agtagr($i,$j) warmup 0.5 5"
        #         $ns at 1 "$agtagr($i,$j) init_schedule"

        #         set init_fid [expr $init_fid + $connections_per_pair];
				# }
        if {$tor_i != $tor_j} {
                set agtagr_a($i,$j) [new Agent_Aggr_pair]
                $agtagr_a($i,$j) setup $s($i) $s($j) "$i $j (a)" $connections_per_pair $init_fid "TCP_pair"
                $agtagr_a($i,$j) attach-logfile $flowlog

                puts -nonewline "($i,$j) "
                #For Poisson/Pareto
                $agtagr_a($i,$j) set_PCarrival_process [expr $lambda_a/($S - $topology_spt)] $flow_cdf_a [expr 17*$i+1244*$j] [expr 33*$i+4369*$j]

                $ns at 0.1 "$agtagr_a($i,$j) warmup 0.5 5"
                $ns at 1 "$agtagr_a($i,$j) init_schedule"

                set init_fid [expr $init_fid + $connections_per_pair];

                set agtagr_b($i,$j) [new Agent_Aggr_pair]
                $agtagr_b($i,$j) setup $s($i) $s($j) "$i $j (b)" $connections_per_pair $init_fid "TCP_pair"
                $agtagr_b($i,$j) attach-logfile $flowlog

                puts -nonewline "($i,$j) "
                #For Poisson/Pareto
                $agtagr_b($i,$j) set_PCarrival_process [expr $lambda_b/($S - $topology_spt)] $flow_cdf_b [expr 17*$i+1244*$j] [expr 33*$i+4369*$j]

                $ns at 0.1 "$agtagr_b($i,$j) warmup 0.5 5"
                $ns at 1 "$agtagr_b($i,$j) init_schedule"

                set init_fid [expr $init_fid + $connections_per_pair];
				}
		}
}

puts "Initial agent creation done";flush stdout
puts "Simulation started!"

$ns run
