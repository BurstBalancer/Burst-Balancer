from ipaddress import ip_address

p4 = bfrt.flowlet_switching.pipe

path="../table/"

'''ecmp_select'''
ecmp_select_conf=open(path+"ecmp_select_conf.txt")

table=p4.Ingress.ecmp_select_t
for t in ecmp_select_conf:
    d=t.split()
    table.add_with_ecmp_select(dst_addr=ip_address(d[0]), ecmp=int(d[1]),port=int(d[2]))


'''if_ecmp'''
if_ecmp_conf=open(path+"if_ecmp_conf.txt")

table=p4.Ingress.if_ecmp_t
for t in if_ecmp_conf:
    d=t.split()
    table.add_with_if_ecmp(dst_addr=ip_address(d[0]), sign=int(d[1]))

'''arp_host'''
arp_host_conf=open(path+"arp_host_conf.txt")

table=p4.Ingress.arp_host
for t in arp_host_conf:
    d=t.split()
    table.add_with_unicast_send(proto_dst_addr=ip_address(d[0]), port=int(d[1]))
