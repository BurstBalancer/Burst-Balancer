/* -*- P4_16 -*- */
//need to handle ARP
#include <core.p4>
#include <tna.p4>

#define ts_thresh  200000
#define sketch_size    65536
#define freq_thresh 30
#define overtime_thresh   20000000
#define  ECN_MARK_THRESHOLD  10

/*************************************************************************
 ************* C O N S T A N T S    A N D   T Y P E S  *******************
*************************************************************************/
enum bit<16> ether_type_t {
    TPID       = 0x8100,
    IPV4       = 0x0800,
    ARP        = 0x0806,
    CLEAR      = 0x3333
}

enum bit<8>  ip_proto_t {
    ICMP  = 1,
    IGMP  = 2,
    TCP   = 6,
    UDP   = 17
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

header mirror_h {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
    bit<16> index;
}

header clear_h {
    bit<16> index;
}

/*************************************************************************
 **************  I N G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/
  
    /***********************  H E A D E R S  ************************/

struct my_ingress_headers_t {
    ethernet_h         ethernet;
    clear_h            clear;
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
    bit<32>     s_d_port;
    bit<32>     timestamp_now;
    bit<32>     timestamp_last;
    bit<16>     index;
    bit<32>     flow_id_fp;
    bit<8>      flag;//use 3 bit [first come][delta time > thres][freq > thres]
    bit<16>     outport;
    bit<16>     port;
    bit<16>     static_port;
    bit<1>      same_id;
    bit<1>      over_flag;
    bit<32>     freq_int;
    bit<8>      port_select;
    mirror_h    mirror;
    MirrorId_t  session_id;
}

struct id_pair{
    bit<32>     counter;
    bit<32>     id;
}
 
struct fp_pair{
    bit<32>     freq;
    bit<32>     fp;
}

struct id_port{
    bit<32>     port;
    bit<32>     id;
}



    /***********************  P A R S E R  **************************/

parser IngressParser(packet_in        pkt,
    /* User */
    out my_ingress_headers_t          hdr,
    out my_ingress_metadata_t         meta,
    /* Intrinsic */
    out ingress_intrinsic_metadata_t  ig_intr_md)
{
    state start {
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);
        transition meta_init;
    }

    state meta_init {
        meta.timestamp_now = ig_intr_md.ingress_mac_tstamp[47:16];

        meta.s_d_port = 0;
        meta.freq_int = 0;
        meta.flow_id_fp = 0;
        meta.timestamp_last = 0;
        
        meta.index = 0;
        meta.port = 0;
        meta.static_port = 0;
        meta.outport = 0;
        meta.flag = 0;
        meta.same_id = 0;
        meta.over_flag = 0;
        meta.port_select = 0;
        transition parse_ethernet; 
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select((bit<16>)hdr.ethernet.ether_type) {
            (bit<16>)ether_type_t.TPID &&& 0xEFFF :  parse_vlan_tag;
            (bit<16>)ether_type_t.IPV4            :  parse_ipv4;
            (bit<16>)ether_type_t.ARP             :  parse_arp;
            (bit<16>)ether_type_t.CLEAR           :  parse_clear;
            default :  accept;
        }
    }

    state parse_clear{
        pkt.extract(hdr.clear);
        transition accept;
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
        transition select(hdr.ipv4.protocol) {
            1 : parse_icmp;
            2 : parse_igmp;
            6 : parse_tcp;
           17 : parse_udp;
            default : accept;
        }    
    }

    state parse_icmp {
        meta.s_d_port = pkt.lookahead<bit<32>>();
        pkt.extract(hdr.icmp);
        transition accept;
    }

    state parse_igmp {
        meta.s_d_port = pkt.lookahead<bit<32>>();
        pkt.extract(hdr.igmp);
        transition accept;  
    }

    state parse_tcp {
        meta.s_d_port = pkt.lookahead<bit<32>>();
        pkt.extract(hdr.tcp);
        transition accept;
    }

    state parse_udp {
        meta.s_d_port = pkt.lookahead<bit<32>>();
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
    CRCPolynomial<bit<32>>(0x04C11DB7,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32a;
    CRCPolynomial<bit<32>>(0x741B8CD7,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32b;
    CRCPolynomial<bit<32>>(0xDB710641,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32c;
    CRCPolynomial<bit<32>>(0x82608EDB,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32fp;

    Hash<bit<16>>(HashAlgorithm_t.CUSTOM,crc32a) hash_1;
    Hash<bit<9>>(HashAlgorithm_t.CUSTOM,crc32b) hash_st_ecmp;
    Hash<bit<9>>(HashAlgorithm_t.CUSTOM,crc32c) hash_3;
    Hash<bit<9>>(HashAlgorithm_t.CUSTOM,crc32a) hash_i1;
    Hash<bit<9>>(HashAlgorithm_t.CUSTOM,crc32b) hash_i2;
    Hash<bit<9>>(HashAlgorithm_t.CUSTOM,crc32c) hash_i3;
    Hash<bit<9>>(HashAlgorithm_t.CUSTOM,crc32c) hash_ecmp;
    Hash<bit<32>>(HashAlgorithm_t.CUSTOM,crc32fp) hash_fp;
    Hash<bit<14>>(HashAlgorithm_t.CUSTOM,crc32fp) hash_fpi;
    bit<9> ecmp;
    bit<9> static_ecmp;
    bit<1> sch_flag = 0;
    bit<1> recir_flag = 0;
    bit<8> clear_count = 0;
    bit<32> ts_to_port = 0;
    bit<2> tmp = 0;
    bit<1> tt = 0;

    Register<id_pair,bit<16>>(sketch_size) flow_id_reg;
    RegisterAction<id_pair, bit<16>, bit<32>>(flow_id_reg) sketch_id = 
    {
        void apply(inout id_pair register_data, out bit<32> result){

            if(register_data.id != meta.flow_id_fp && register_data.counter != 0){
                register_data.counter = register_data.counter -1;
            }
            else{
                register_data.counter = register_data.counter +1;
                if(register_data.id == meta.flow_id_fp){
                    result = register_data.counter;
                }
                register_data.id = meta.flow_id_fp;
            }

        }

    };

    RegisterAction<id_pair, bit<16>, bit<1>>(flow_id_reg) sketch_id_clear =
    {
        void apply(inout id_pair register_data, out bit<1> result){
            register_data.counter = 0;
            register_data.id = 0;
        }
    };


    action id_insert(){
        meta.freq_int = sketch_id.execute(meta.index);
    }


    table sketch_id_t{
        actions = {
            id_insert;
        }
        default_action = id_insert;
    }

    Register<bit<32>,bit<16>>(sketch_size) ts_reg;
    RegisterAction<bit<32>,bit<16>,bit<1>>(ts_reg) ts_cmp = 
    {
        void apply(inout bit<32> register_data, out bit<1>result){

            if(meta.timestamp_now - register_data > ts_thresh){
                result = 1;
                
            }
            register_data = meta.timestamp_now;
        }
    };
    
    action ts_flag1_set(){
        meta.flag[1:1] = ts_cmp.execute(meta.index);
    }

    table ts_flag1_set_t{
        actions = {
            ts_flag1_set;
        }
        default_action = ts_flag1_set;
    }

    RegisterAction<bit<32>,bit<16>,bit<1>>(ts_reg) over_check = 
    {
        void apply(inout bit<32> register_data, out bit<1> result){
            if(register_data == 0){
                register_data = meta.timestamp_now;
                result = 0;
            }
            else if(meta.timestamp_now - register_data > overtime_thresh){
                result = 1;
                register_data =0;
            }
        }
    };

    action overtime_check(){
        meta.over_flag = over_check.execute(meta.index);
    }

    table overtime_check_t{
        actions = {
            overtime_check;
        }
        default_action = overtime_check;
    }

    Register<id_pair,bit<16>>(sketch_size)id_ts_reg;
    RegisterAction<id_pair,bit<16>,bit<1>>(id_ts_reg) id_ts_a = 
    {
        void apply(inout id_pair register_data, out bit<1> result){
            if(register_data.counter == 0){
                register_data.id = meta.flow_id_fp;
                register_data.counter = meta.timestamp_now;
                result = 1;
            }
            else if(register_data.id == meta.flow_id_fp){
                result = 1;
                register_data.counter = meta.timestamp_now;
            }
        }
    };

    action sch_1_action(){
        meta.port_select[1:1] = id_ts_a.execute(meta.index);
    }
    
    table sch_1_t{
        actions = {
            sch_1_action;
        }
        default_action = sch_1_action;
    }

    RegisterAction<id_pair,bit<16>,bit<1>>(id_ts_reg) id_ts_b = 
    {
        void apply(inout id_pair register_data, out bit<1> result){
            if(meta.timestamp_now - register_data.counter > overtime_thresh){
                register_data.counter = 0;
                register_data.id = 0;
            }//clear the register 

            else if(register_data.id == meta.flow_id_fp){
                result = 1;
                register_data.counter = meta.timestamp_now;
            }
            
        }
    };



    action sch_0_action(){
        meta.port_select[0:0] = id_ts_b.execute(meta.index);
    }

    table sch_0_t{
        actions = {
            sch_0_action;
        }
        default_action = sch_0_action;
    }


    //Register to maintain egress port
    Register<bit<16>,bit<16>>(sketch_size) port_reg;
    RegisterAction<bit<16>, bit<16>, bit<16>>(port_reg) output_port = 
    {
        void apply(inout bit<16> register_data, out bit<16> result){

            if(meta.port_select == 2) {
                register_data = meta.port;
            }
            result = register_data;
        }
    };

    //Register to compare freq and threshold
    Register<bit<32>,bit<16>>(0x1) cmp_freq_thres_reg;
    RegisterAction<bit<32>,bit<16>,bit<1>>(cmp_freq_thres_reg) cmp_freq = 
    {
        void apply(inout bit<32> register_data, out bit<1> result){
            register_data = meta.freq_int;
            if(meta.freq_int > freq_thresh) result = 1;
            else result = 0;
        }
    };


    action out_port(){
        meta.outport = output_port.execute(meta.index);
    }

    table output_port_t{
        actions = {
            out_port;
        }
        default_action = out_port;
    }

    
    action freq_falg0_set(){
        meta.flag[0:0] = cmp_freq.execute(0);
    }

    table freq_flag0_set_t{
        actions = {
            freq_falg0_set;
        }
        default_action = freq_falg0_set;
    }

    // send packet
    action send_packet(){
        ig_tm_md.ucast_egress_port = (PortId_t)meta.outport;
    }
    
    table send_packet_t{
        actions = {
            send_packet;
        }
        default_action = send_packet;
    }


    
    action cal_hash_ecmp(){
        ecmp = hash_ecmp.get({hdr.ipv4.src_addr, hdr.ipv4.dst_addr, hdr.ipv4.protocol, meta.s_d_port,ig_intr_md.ingress_mac_tstamp});
      }

    table cal_hash_ecmp_t{
        actions = {
            cal_hash_ecmp;
        }
        default_action = cal_hash_ecmp;
    }
    action cal_hash_static_ecmp(){
        static_ecmp = hash_st_ecmp.get({hdr.ipv4.src_addr, hdr.ipv4.dst_addr, hdr.ipv4.protocol, meta.s_d_port});
    }

    table cal_hash_static_ecmp_t{
        actions = {
            cal_hash_static_ecmp;
        }
        default_action = cal_hash_static_ecmp;
    }

    action cal_hash_index(){
       meta.index = hash_1.get({hdr.ipv4.src_addr, hdr.ipv4.dst_addr, hdr.ipv4.protocol, meta.s_d_port});      

       recir_flag = hdr.ipv4.res;
    }

    table cal_hash_index_t{
        actions = {
            cal_hash_index;
        }
        default_action = cal_hash_index;
    }

   action cal_hash_fp(){
        meta.flow_id_fp = hash_fp.get({hdr.ipv4.src_addr, hdr.ipv4.dst_addr, hdr.ipv4.protocol, meta.s_d_port});       
    }

    table cal_hash_fp_t{
        actions = {
            cal_hash_fp;
        }
        default_action = cal_hash_fp;
    }

   action ecmp_select_port(bit<16> port){
        meta.port = port;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
    }

    table ecmp_select_port_t{
        actions = {
            ecmp_select_port;
        }
        key = {
            hdr.ipv4.dst_addr: exact;
            ecmp:   exact;
        }
        size = 65536;
        default_action = ecmp_select_port(0);
    }

    action ecmp_static_select_port(bit<16> port){
        meta.outport = port;
    }

    table ecmp_static_select_port_t{
        actions = {
            ecmp_static_select_port;
        }
        key = {
            hdr.ipv4.dst_addr: exact;
            static_ecmp:   exact;
        }
        size = 65536;
        default_action = ecmp_static_select_port(0);
    }

    action unicast_send(PortId_t port){
        ig_tm_md.ucast_egress_port = port;
        ig_tm_md.bypass_egress = 1;
    }
    action drop(){
        ig_dprsr_md.drop_ctl = 1;
    }

    table arp_host{
        actions = {
            unicast_send;
            drop;
        }
        key = {
            hdr.arp.proto_dst_addr:   exact;
        }
        default_action = drop;
        size = 100;
    }

bit<1> ecmp_able=0;
    action if_ecmp(bit<1> sign)
        {
            ecmp_able=sign;
        }
   table if_ecmp_t
        {
            key={hdr.ipv4.dst_addr:exact;}
            actions={if_ecmp;}
            default_action=if_ecmp(0);
        }
    

    apply{
        
        if(hdr.arp.isValid()){
            arp_host.apply();
        }
        else if(hdr.ipv4.isValid()){
            
                cal_hash_ecmp_t.apply(); 
                cal_hash_static_ecmp_t.apply();
                cal_hash_index_t.apply();
                if_ecmp_t.apply();

                cal_hash_fp_t.apply();
                ecmp_select_port_t.apply();
                ecmp_static_select_port_t.apply();

            if(ecmp_able == 1){

                sketch_id_t.apply();
                if(meta.freq_int != 0) meta.same_id = 1;
            
                if(meta.same_id == 0){

                    overtime_check_t.apply();
                    debug_count_4.execute(0);
                }
                else{
                    ts_flag1_set_t.apply();
                }

                freq_flag0_set_t.apply();

                
                if(meta.flag == 3){
                    sch_1_t.apply();
                }
                else {
                    sch_0_t.apply();
                }

                if(meta.over_flag == 1){
                    meta.session_id = 1;
                    meta.mirror.index = meta.index;
                    meta.mirror.ether_type = 0x3333;
                    ig_dprsr_md.mirror_type = 1;

                }

                if(meta.port_select > 0){
                    output_port_t.apply();
                }
            }

            send_packet_t.apply();


        }
        else if(hdr.clear.isValid()){
            sketch_id_clear.execute(hdr.clear.index);
            debug_count_1.execute(0);
            
        }

    }

}

/**************************D E P A R S E R*******************/

control IngressDeparser(packet_out pkt,
    /* User */
    inout my_ingress_headers_t                       hdr,
    in    my_ingress_metadata_t                      meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md)
{
        // Checksum() ipv4_checksum;
    
    
     Checksum() ipv4_checksum;
    Mirror() mirror;

    apply {
        if(meta.over_flag == 1){
            mirror.emit<mirror_h>(meta.session_id, meta.mirror);
        }
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
