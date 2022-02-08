/* -*- P4_16 -*- */
//need to handle ARP
#include <core.p4>
#include <tna.p4>

#define PRIME 2147483647
const bit<19>   ECN_MARK_THRESHOLD = 10;
/*************************************************************************
 ************* C O N S T A N T S    A N D   T Y P E S  *******************
*************************************************************************/
enum bit<16> ether_type_t {
    TPID       = 0x8100,
    IPV4       = 0x0800,
    ARP        = 0x0806
}

enum bit<8>  ip_proto_t {
    ICMP  = 1,
    IGMP  = 2,
    TCP   = 6,
    UDP   = 17
}
struct ports {
    bit<16>  sp;
    bit<16>  dp;
}


type bit<48> mac_addr_t;

/*************************************************************************
 ***********************  H E A D E R S  *********************************
 *************************************************************************/
/*  Define all the headers the program will recognize             */
/*  The actual sets of headers processed by each gress can differ */

/* Standard ethernet header */
header ethernet_h {
    mac_addr_t    dst_addr;
    mac_addr_t    src_addr;
    ether_type_t  ether_type;
}

header vlan_tag_h {
    bit<3>        pcp;
    bit<1>        cfi;
    bit<12>       vid;
    ether_type_t  ether_type;
}

header arp_h {
    bit<16>       htype;
    bit<16>       ptype;
    bit<8>        hlen;
    bit<8>        plen;
    bit<16>       opcode;
    mac_addr_t    hw_src_addr;
    bit<32>       proto_src_addr;
    mac_addr_t    hw_dst_addr;
    bit<32>       proto_dst_addr;
}

header ipv4_h {
    bit<4>       version;
    bit<4>       ihl;
    bit<6>       dscp;
    bit<2>       ecn;
    bit<16>      total_len;
    bit<16>      identification;
    bit<1>       res;
    bit<2>       flags;
    bit<13>      frag_offset;
    bit<8>       ttl;
    bit<8>       protocol;
    bit<16>      hdr_checksum;
    bit<32>      src_addr;
    bit<32>      dst_addr;
}

header icmp_h {
    bit<16>  type_code;
    bit<16>  checksum;
}

header igmp_h {
    bit<16>  type_code;
    bit<16>  checksum;
}

header tcp_h {
    bit<16>  src_port;
    bit<16>  dst_port;
    bit<32>  seq_no;
    bit<32>  ack_no;
    bit<4>   data_offset;
    bit<4>   res;
    bit<8>   flags;
    bit<16>  window;
    bit<16>  checksum;
    bit<16>  urgent_ptr;
}

header udp_h {
    bit<16>  src_port;
    bit<16>  dst_port;
    bit<16>  len;
    bit<16>  checksum;
}

/*************************************************************************
 **************  I N G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/
 
    /***********************  H E A D E R S  ************************/

struct my_ingress_headers_t{
    ethernet_h         ethernet;
    arp_h              arp;
    vlan_tag_h[2]      vlan_tag;
    ipv4_h             ipv4;
    icmp_h             icmp;
    igmp_h             igmp;
    tcp_h              tcp;
    udp_h              udp;
}


    /******  G L O B A L   I N G R E S S   M E T A D A T A  *********/


struct my_ingress_metadata_t {
    bit<32> ll;
}

    /***********************  P A R S E R  **************************/

parser IngressParser(packet_in        pkt,
    /* User */
    out my_ingress_headers_t          hdr,
    out my_ingress_metadata_t         meta,
    /* Intrinsic */
    out ingress_intrinsic_metadata_t  ig_intr_md)
{
    /* This is a mandatory state, required by Tofino Architecture */
    state start {
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);
        transition meta_init;
    }

    state meta_init {

        transition parse_ethernet;
    }
    
    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        /* 
         * The explicit cast allows us to use ternary matching on
         * serializable enum
         */        
        transition select((bit<16>)hdr.ethernet.ether_type) {
            (bit<16>)ether_type_t.TPID &&& 0xEFFF :  parse_vlan_tag;
            (bit<16>)ether_type_t.IPV4            :  parse_ipv4;
            (bit<16>)ether_type_t.ARP             :  parse_arp;
            default :  accept;
        }
    }

    state parse_arp {
        pkt.extract(hdr.arp);
        transition accept;
    }

    state parse_vlan_tag {
        pkt.extract(hdr.vlan_tag.next);
        transition select(hdr.vlan_tag.last.ether_type) {
            ether_type_t.TPID :  parse_vlan_tag;
            ether_type_t.IPV4 :  parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            1 : parse_icmp;
            2 : parse_igmp;
            6 : parse_tcp;
           17 : parse_udp;
            default : accept;
        }
    }


    state parse_icmp {
       meta.ll=pkt.lookahead<bit<32>>();//ll = src port +dst port
        pkt.extract(hdr.icmp);
        transition accept;
    }
    
    state parse_igmp {
      meta.ll=pkt.lookahead<bit<32>>();
        pkt.extract(hdr.igmp);
        transition accept;
    }
    
    state parse_tcp {
    meta.ll=pkt.lookahead<bit<32>>();
        pkt.extract(hdr.tcp);
        transition accept;
    }
    
    state parse_udp {
      meta.ll=pkt.lookahead<bit<32>>();
        pkt.extract(hdr.udp);
        transition accept;
    }


}
control Ingress(/* User */
    inout my_ingress_headers_t                       hdr,
    inout my_ingress_metadata_t                      meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_t               ig_intr_md,
    in    ingress_intrinsic_metadata_from_parser_t   ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t        ig_tm_md)
{

//bit<32> errorcode=0;
    CRCPolynomial<bit<32>>(0x04C11DB7,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32a;
    CRCPolynomial<bit<32>>(0x741B8CD7,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32b;
    CRCPolynomial<bit<32>>(0xDB710641,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32c;
    CRCPolynomial<bit<32>>(0x82608EDB,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32fp;

    Hash<bit<9>>(HashAlgorithm_t.CUSTOM,crc32a) hash_1;
    Hash<bit<9>>(HashAlgorithm_t.CUSTOM,crc32b) hash_2;
    Hash<bit<9>>(HashAlgorithm_t.CUSTOM,crc32c) hash_3;
    Hash<bit<9>>(HashAlgorithm_t.CUSTOM,crc32a) hash_i1;
    Hash<bit<9>>(HashAlgorithm_t.CUSTOM,crc32b) hash_i2;
    Hash<bit<9>>(HashAlgorithm_t.CUSTOM,crc32c) hash_i3;
    Hash<bit<1>>(HashAlgorithm_t.CUSTOM,crc32c) hash_ecmp;
    Hash<bit<14>>(HashAlgorithm_t.CUSTOM,crc32fp) hash_fp;
    Hash<bit<14>>(HashAlgorithm_t.CUSTOM,crc32fp) hash_fpi;


bit<1> ecmp=0;
action cal_ecmp()//计算index
    {
        ecmp=hash_ecmp.get({hdr.ipv4.src_addr,hdr.ipv4.dst_addr,meta.ll,hdr.ipv4.protocol});
    }
@stage(0)  table cal_ecmp_t
    {
        actions={cal_ecmp;}
        default_action=cal_ecmp;
    }

action ecmp_select(PortId_t port)//计算index
    {
        ig_tm_md.ucast_egress_port=port;
        hdr.ipv4.ttl=hdr.ipv4.ttl-1;
    }
@stage(1)  table ecmp_select_t
    {   
        key={hdr.ipv4.dst_addr:exact;ecmp:exact;}
        actions={ecmp_select;}
        default_action=ecmp_select(0);
        size=100;
    }

    /* arp packets processing */
    action unicast_send(PortId_t port) {
        ig_tm_md.ucast_egress_port = port;
        ig_tm_md.bypass_egress=1;
    }
    action drop() {
        ig_dprsr_md.drop_ctl = 1;
    }
    @stage(0) table arp_host {
        key = { hdr.arp.proto_dst_addr : exact; }
        actions = { unicast_send; drop; }
        default_action = drop();
    }

  

apply {
    if (hdr.arp.isValid())
    {
        arp_host.apply();
    }
    else if (hdr.ipv4.isValid())
    {
	    cal_ecmp_t.apply();
        ecmp_select_t.apply();
    }
}
}
control IngressDeparser(packet_out pkt,
    /* User */
    inout my_ingress_headers_t                       hdr,
    in    my_ingress_metadata_t                      meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md)
{
        // Checksum() ipv4_checksum;
    
    
     Checksum() ipv4_checksum;
    
    apply {
        if (hdr.ipv4.isValid()) {
            hdr.ipv4.hdr_checksum = ipv4_checksum.update({
                hdr.ipv4.version,
                hdr.ipv4.ihl,
                hdr.ipv4.dscp,
                hdr.ipv4.ecn,
                hdr.ipv4.total_len,
                hdr.ipv4.identification,
                hdr.ipv4.res,
                hdr.ipv4.flags,
                hdr.ipv4.frag_offset,
                hdr.ipv4.ttl,
                hdr.ipv4.protocol,
                hdr.ipv4.src_addr,
                hdr.ipv4.dst_addr
            });  
        }
        pkt.emit(hdr);
        
    }
}
/*************************************************************************
 ****************  E G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/

   /***********************  H E A D E R S  ************************/

    struct my_egress_headers_t {

    ethernet_h         ethernet;
    arp_h              arp;
    vlan_tag_h[2]      vlan_tag;
    ipv4_h             ipv4;

    }


    /********  G L O B A L   E G R E S S   M E T A D A T A  *********/

    struct my_egress_metadata_t {
        bit<16>     qdepth;
        bit<16>     ecn_thres;
    }

    /***********************  P A R S E R  **************************/

parser EgressParser(packet_in        pkt,
    /* User */
    out my_egress_headers_t          hdr,
    out my_egress_metadata_t         meta,
    /* Intrinsic */
    out egress_intrinsic_metadata_t  eg_intr_md)
{
    /* This is a mandatory state, required by Tofino Architecture */
    state start {
        pkt.extract(eg_intr_md);
        // pkt.advance(PORT_METADATA_SIZE);
        transition meta_init;
    }
    state meta_init{
        meta.qdepth = 0;
        meta.ecn_thres = 0;
        transition parse_ethernet;
    }

    state parse_ethernet{
        pkt.extract(hdr.ethernet);
        transition select((bit<16>)hdr.ethernet.ether_type) {
            (bit<16>)ether_type_t.TPID &&& 0xEFFF :  parse_vlan_tag;
            (bit<16>)ether_type_t.IPV4            :  parse_ipv4;
            (bit<16>)ether_type_t.ARP             :  parse_arp;
            default :  accept;
        }       
    }

    state parse_arp {
        pkt.extract(hdr.arp);
        transition  accept;
    }

    state parse_vlan_tag {
        pkt.extract(hdr.vlan_tag.next);
        transition select(hdr.vlan_tag.last.ether_type) {
            ether_type_t.TPID :  parse_vlan_tag;
            ether_type_t.IPV4 :  parse_ipv4;
            default: accept;
        }    
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition  accept;  
    }

}

    /***************** M A T C H - A C T I O N  *********************/

control Egress(
    /* User */
    inout my_egress_headers_t                          hdr,
    inout my_egress_metadata_t                         meta,
    /* Intrinsic */    
    in    egress_intrinsic_metadata_t                  eg_intr_md,
    in    egress_intrinsic_metadata_from_parser_t      eg_prsr_md,
    inout egress_intrinsic_metadata_for_deparser_t     eg_dprsr_md,
    inout egress_intrinsic_metadata_for_output_port_t  eg_oport_md)
{
    action mark_ecn(){
        hdr.ipv4.ecn = 3;
    }

    table mark_ecn_t{
        actions = {
            mark_ecn;
        }
        default_action = mark_ecn;
    }

    Register<bit<16>,bit<16>>(0x1) ecn_reg;
    RegisterAction<bit<16>,bit<16>,bit<1>>(ecn_reg) cmp_ecn_thres = 
    {
        void apply(inout bit<16> register_data, out bit<1> result){
            register_data = meta.qdepth;
            if(register_data > ECN_MARK_THRESHOLD){
                result = 1;
            }
            else result = 0;
        }
    };

bit<1> ecn_flag;
    action set_ecn_flag(){
        ecn_flag = cmp_ecn_thres.execute(0);
    }

    table set_ecn_flag_t{
        actions = {
            set_ecn_flag;
        }
        default_action = set_ecn_flag;
    }

    action ecn_qdepth(){
        meta.qdepth = eg_intr_md.enq_qdepth[15:0];
    }

    table ecn_qdepth_t{
        actions = {
            ecn_qdepth;
        }
        default_action = ecn_qdepth();
    }

    apply {
        if(hdr.ipv4.ecn == 1 || hdr.ipv4.ecn ==2){
            ecn_qdepth_t.apply();
            set_ecn_flag_t.apply();
            if(ecn_flag == 1){
                mark_ecn_t.apply();
            }
        }
    }

}



    /*********************  D E P A R S E R  ************************/

control EgressDeparser(packet_out pkt,
    /* User */
    inout my_egress_headers_t                       hdr,
    in    my_egress_metadata_t                      meta,
    /* Intrinsic */
    in    egress_intrinsic_metadata_for_deparser_t  eg_dprsr_md)
{
  
    Checksum() ipv4_checksum;
    
    apply {
        if (hdr.ipv4.isValid()) {
            hdr.ipv4.hdr_checksum = ipv4_checksum.update({
                hdr.ipv4.version,
                hdr.ipv4.ihl,
                hdr.ipv4.dscp,
                hdr.ipv4.ecn,
                hdr.ipv4.total_len,
                hdr.ipv4.identification,
                hdr.ipv4.res,
                hdr.ipv4.flags,
                hdr.ipv4.frag_offset,
                hdr.ipv4.ttl,
                hdr.ipv4.protocol,
                hdr.ipv4.src_addr,
                hdr.ipv4.dst_addr
            });  
        }
        pkt.emit(hdr);
        
    }
}


/************ F I N A L   P A C K A G E ******************************/
Pipeline(
    IngressParser(),
    Ingress(),
    IngressDeparser(),
    EgressParser(),
    Egress(),
    EgressDeparser()
) pipe;

Switch(pipe) main;
