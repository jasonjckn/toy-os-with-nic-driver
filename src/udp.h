
#ifndef UDP_H
#define UDP_H

#include "common.h"

typedef u8 phys_addr_t[6];
typedef u8 ip_addr_t[4];

typedef struct {
    phys_addr_t dst;
    phys_addr_t src;
    u16 type;
} eth_hdr_t;

typedef struct {
 unsigned char      ihl:4, ver:4;
 unsigned char      tos;
 unsigned short int len;
 unsigned short int ident;
 unsigned char      flag; //= 0x40
 unsigned char      offset;
 unsigned char      ttl;
 unsigned char      protocol;
 unsigned short int chksum;
 ip_addr_t       src_ip;
 ip_addr_t       dst_ip;
} ip_hdr_t ;

typedef struct {
    u16 src_port;
    u16 dst_port;
    u16 len;
    u16 chksum;
} udp_hdr_t;

typedef struct {
    eth_hdr_t eth;
    ip_hdr_t iph;
    udp_hdr_t udp;
} udp_pkt_t ;


int create_pkt(u8* buf, char * s);
int create_pkt2(u8* buf, char * s);

#endif
