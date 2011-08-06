#include "common.h"
#include "udp.h"

u16 ip_sum_calc(u16 len_ip_header, u8 buff[])
{
u16 word16;
u32 sum=0;
u16 i;
    
	// make 16 bit words out of every two adjacent 8 bit words in the packet
	// and add them up
	for (i=0;i<len_ip_header;i=i+2){
		word16 =(((u16)buff[i]<<8)&0xFF00)+((u16)buff[i+1]&0xFF);
		sum = sum + (u32) word16;	
	}
	
	// take only 16 bits out of the 32 bit sum and add up the carries
	while (sum>>16)
	  sum = (sum & 0xFFFF)+(sum >> 16);

	// one's complement the result
	sum = ~sum;
	
return ((u16) sum);
}

u16 udp_sum_calc(u16 len_udp, u8 src_addr[],
                 u8 dest_addr[], BOOL padding, u8 buff[])
{
        u16 prot_udp=17;
        u16 padd=0;
        u16 word16;
        u32 sum;	
	
	// Find out if the length of data is even or odd number. If odd,
	// add a padding byte = 0 at the end of packet
	if (padding&1==1){
		padd=1;
		buff[len_udp]=0;
	}
	
	//initialize sum to zero
	sum=0;
	
	// make 16 bit words out of every two adjacent 8 bit words and 
	// calculate the sum of all 16 vit words
        u32 i;
	for (i=0;i<len_udp+padd;i=i+2){
		word16 =(((u16)buff[i]<<8)&0xFF00)+((u16)buff[i+1]&0xFF);
		sum = sum + (unsigned long)word16;
	}	
	// add the UDP pseudo header which contains the IP source and destinationn addresses
	for (i=0;i<4;i=i+2){
		word16 =(((u16)src_addr[i]<<8)&0xFF00)+((u16)src_addr[i+1]&0xFF);
		sum=sum+word16;	
	}
	for (i=0;i<4;i=i+2){
		word16 =(((u16)dest_addr[i]<<8)&0xFF00)+((u16)dest_addr[i+1]&0xFF);
		sum=sum+word16; 	
	}
	// the protocol number and the length of the UDP packet
	sum = sum + prot_udp + len_udp;

	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
    	while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);
		
	// Take the one's complement of sum
	sum = ~sum;

        return ((u16) sum);
}

u16 htos(u16 i) {
    return (i>>8)&0xFF | ((i&0xFF)<<8);
}

void set_udp_pkt(u8*buf, char*s, int s_len) {
    udp_pkt_t* pkt = (udp_pkt_t*)buf;

    pkt->iph.protocol = 17;

    int udp_len = s_len + sizeof(udp_hdr_t);

    memcpy(buf + sizeof(udp_pkt_t), s, s_len);
    pkt->udp.src_port = htos(255);
    pkt->udp.dst_port = htos(9002);
    pkt->udp.len = htos(udp_len);
    pkt->udp.chksum = htos(udp_sum_calc(udp_len, pkt->iph.src_ip, 
                                    pkt->iph.dst_ip,
                                   udp_len%2==1, (u8*)&pkt->udp));

}

int set_ip_hdr(udp_pkt_t*pkt, char* s) {
    int s_len = strlen(s);
    int pkt_len = s_len + sizeof(udp_pkt_t);

    pkt->iph.ihl = 5;
    pkt->iph.ver = 4;
    pkt->iph.tos = 0;
    pkt->iph.len = htos(pkt_len - sizeof(eth_hdr_t));
    pkt->iph.chksum = 0x0000;
    pkt->iph.ident = 0x3412;
    pkt->iph.flag = 0x40;
    pkt->iph.offset = 0x00;
    pkt->iph.ttl = 0xff;

    u8 src_ip[] = { 0xac, 0x10, 0x85, 0x9f };
    u8 dst_ip[] = { 0xac, 0x10, 0x85, 0x01 };

    memcpy(pkt->iph.src_ip, src_ip, 4);
    memcpy(pkt->iph.dst_ip, dst_ip, 4);

    set_udp_pkt((u8*)pkt, s, s_len);

    pkt->iph.chksum = htos(ip_sum_calc(sizeof(ip_hdr_t), &pkt->iph));
    return pkt_len;
}


void set_eth_hdr(udp_pkt_t*pkt) {
    u8 dst[] = {0x00, 0x50, 0x56, 0xc0, 0x00, 0x08};
    u8 src[] = {0xb0, 0xc4, 0x20, 0x00, 0x00, 0x00};
    memcpy(pkt->eth.src, src, 6);
    memcpy(pkt->eth.dst, dst, 6);
    pkt->eth.type = 0x0008;
}

int create_pkt(u8* buf, char * s) {
    udp_pkt_t* pkt = (udp_pkt_t*)buf;

    set_eth_hdr(pkt);
    return set_ip_hdr(pkt, s);
}

