// main.c -- Defines the C-code kernel entry point, calls initialisation routines.
//           Made for JamesM's tutorials <www.jamesmolloy.co.uk>

#include "monitor.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "ne2k.h"
#include "udp.h"
#include "paging.h"
#include "isr.h"

void print_pkt(u8* buf, size_t len );
void send_arp_reply();

void nic_int(registers_t r) {
    //monitor_write("KERNEL>> NIC interrupt\n");

    u8 buf[500] = {};
    size_t len = ne2k_Receive(buf, 400);
    //print_pkt(buf, len);
    udp_pkt_t* pkt = (udp_pkt_t*)buf;
    //size_t msg_len = (pkt->udp.len - sizeof(udp_hdr_t))
    monitor_write("Clojure says> ");
    monitor_write(buf+sizeof(udp_pkt_t));

    monitor_write("You say> "); 

    outbr(0x01, 0x307);
}

char kbd_to_ascii(unsigned char cx) {
    char* map = "qwertyuiop[]\nxasdfghjkl;'xxxzxcvbnm,./xxx ";
    int len = strlen(map);

    char c = cx - 0x10;
    if(c >= 0 && c < len) {
        return map[c];
    } else {
        return 0;
    }
}

char line[100] = {};


void kbd_drv(registers_t r) {
    static int cursor = 0;
    unsigned char cx= 0;
    char c = kbd_to_ascii(cx = inb(0x60));

    if (c != 0) { 
        monitor_put(c); 
        
        if(c != '\n' ) {
            line[cursor++] = c;
        }

        if(c=='\n') {
            line[cursor] = '\n';
            line[cursor+1] = '\0';
            send_pkt(line);
            send_arp_reply();
            memset(line, 'X', 80);
            cursor = 0;
            monitor_write("You say> "); 
        }
    }
}
void print_pkt(u8* buf, size_t len ) {
        if(len > 0 ) {
            monitor_write("packet len= ");
            monitor_write_dec(len);
            monitor_write("\n");

            int i;
            for(i = 0; i  < len; i++ ) {
                if(i%10==0) {
                    monitor_write("\n");
                }
                monitor_write_hex(buf[i]);
                monitor_write(" ");
            }
            monitor_write("\n");
        } else {
            monitor_write("no packets received\n");
        }
}

void kbd_drv2(registers_t r) {
    unsigned char cx= 0;
    char c = kbd_to_ascii(cx = inb(0x60));

    if (c != 0) { 
        u8 buf[500] = {};
        size_t len = ne2k_Receive(buf, 400);
        print_pkt(buf, len);
    }
}

void send_arp_reply() {
    u8 pkt[] = {0x00, 0x50, 0x56, 0xc0, 0x00, 0x08, 0xb0, 0xc4, 0x20, 0x00, 0x00, 0x00, 0x08, 0x06, 0x00, 0x01,
0x08, 0x00, 0x06, 0x04, 0x00, 0x02, 0xb0, 0xc4, 0x20, 0x00, 0x00, 0x00, 0xac, 0x10, 0x85, 0x9f,
0x00, 0x50, 0x56, 0xc0, 0x00, 0x08, 0xac, 0x10, 0x85, 0x01};

    ne2k_Transmit(pkt, 42, 1);
}

void send_pkt(char* s) {
    u8 buf[500] = {};
    int len = create_pkt(buf, s);
    ne2k_Transmit(buf, len, 0);
}

void standard() {
    register_interrupt_handler(IRQ1, kbd_drv);
    monitor_write("KERNEL>> Setting up GDT and ISRs\n");
    monitor_write("KERNEL>> Loading Video Driver\n");
    monitor_write("KERNEL>> Loading Virtual Memory\n");
    monitor_write("KERNEL>> Loading Keyboard Driver\n");


    monitor_write("KERNEL>> Loading NIC Driver\n");
    monitor_write("KERNEL>> MAC Addr = b0:c4:20:00:00:00\n");

    monitor_write("KERNEL>> Kernel Finished Loading\n\n");

    monitor_write("You say> "); 
}

int main(struct multiboot *mboot_ptr)
{
    // Initialise all the ISRs and segmentation
    init_descriptor_tables();

    monitor_clear();
    register_interrupt_handler(IRQ3, nic_int);
    register_interrupt_handler(IRQ1, kbd_drv2);
    ne2k_Linkup_Main2();
    asm volatile("sti");

    standard();



    return 0;
}
