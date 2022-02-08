from ipaddress import ip_address

p4 = bfrt.BalanceSketch.pipe

path = "。。/table/"

'''ecmp_select'''
ecmp_select_conf = open(path+"ecmp_select_conf.txt")

table = p4.Ingress.ecmp_select_port_t
table.clear()
for t in ecmp_select_conf:
    d = t.split()
    table.add_with_ecmp_select_port(dst_addr=ip_address(d[0]), ecmp=int(d[1]), port=int(d[2]))

ecmp_static_select_conf = open(path+"ecmp_select_conf.txt")
table = p4.Ingress.ecmp_static_select_port_t
table.clear()
for t in ecmp_static_select_conf:
    d = t.split()
    table.add_with_ecmp_static_select_port(dst_addr=ip_address(d[0]), static_ecmp=int(d[1]), port=int(d[2]))

arp_host_conf=open(path+"arp_host_conf.txt")

table=p4.Ingress.arp_host
table.clear()
for t in arp_host_conf:
    d=t.split()
    table.add_with_unicast_send(proto_dst_addr=ip_address(d[0]), port=int(d[1]))

if_ecmp_conf = open(path+"if_ecmp_conf.txt")

table = p4.Ingress.if_ecmp_t
table.clear()
for t in if_ecmp_conf:
    d = t.split()
    table.add_with_if_ecmp(dst_addr = ip_address(d[0]), sign = int(d[1]))

table = bfrt.mirror.cfg
table.add_with_normal(sid = 1, session_enable = True, ucast_egress_port = 68, ucast_egress_port_valid=True,direction = "INGRESS",max_pkt_len = 64)

