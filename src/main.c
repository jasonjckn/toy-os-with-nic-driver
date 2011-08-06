// main.c -- Defines the C-code kernel entry point, calls initialisation routines.
//           Made for JamesM's tutorials <www.jamesmolloy.co.uk>

#include "monitor.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "ne2k.h"
#include "udp.h"
#include "paging.h"
#include "isr.h"

void nic_int(registers_t r) {
    monitor_write("NIC interrupt\n");

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
            memset(line, 'X', 80);
            cursor = 0;
            monitor_write("Chat> "); 
        }
    }
}

void kbd_drv2(registers_t r) {
    unsigned char cx= 0;
    char c = kbd_to_ascii(cx = inb(0x60));

    if (c != 0) { 
    }
}

void send_pkt(char* s) {
    u8 buf[500] = {};
    int len = create_pkt(buf, s);
    ne2k_Transmit(buf, len);
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

    monitor_write("Chat> "); 
}

int main(struct multiboot *mboot_ptr)
{
    // Initialise all the ISRs and segmentation
    init_descriptor_tables();

    monitor_clear();
    register_interrupt_handler(IRQ3, nic_int);
    //register_interrupt_handler(IRQ1, kbd_drv2);
    ne2k_Linkup_Main2();
    asm volatile("sti");

    standard();

    /*
    char buf[400] = {};
    while(1) { 
        ne2k_Receive(buf, 400);
    }
    */


    return 0;
}
