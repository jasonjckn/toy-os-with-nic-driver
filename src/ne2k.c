//
// Driver to fool Qemu into sending and receiving packets for us via it's ne2k_isa emulation
//
// This driver is unlikely to work with real hardware without substantial modifications
// and is purely for helping with the development of network stacks.
//
// Interrupts are not supported.
//


#include "ne2k.h"
#include "common.h"
#include "monitor.h"


// Make concrete class out of netdevice abstract one


// Port addresses
#define NE_BASE		0x300
#define NE_CMD          (NE_BASE + 0x00)


#define EN0_STARTPG      (NE_BASE + 0x01)
#define EN0_STOPPG       (NE_BASE + 0x02)


#define EN0_BOUNDARY    (NE_BASE + 0x03)
#define EN0_TPSR        (NE_BASE + 0x04)

#define EN0_TBCR_LO     (NE_BASE + 0x05)
#define EN0_TBCR_HI     (NE_BASE + 0x06)

#define EN0_ISR         (NE_BASE + 0x07)
#define NE_DATAPORT     (NE_BASE + 0x10)    // NatSemi-defined port window offset


#define EN1_CURR        (NE_BASE + 0x07)    // current page
#define EN1_PHYS        (NE_BASE + 0x11)    // physical address


// Where to DMA data to/from
#define EN0_REM_START_LO     (NE_BASE + 0x08)
#define EN0_REM_START_HI     (NE_BASE + 0x09)

// How much data to DMA
#define EN0_REM_CNT_LO     (NE_BASE + 0x0a)
#define EN0_REM_CNT_HI     (NE_BASE + 0x0b)


#define EN0_RSR		   (NE_BASE + 0x0c)

// Commands to select the different pages.
#define NE_PAGE0_STOP        0x21
#define NE_PAGE1_STOP        0x61



#define NE_PAGE0        0x20
#define NE_PAGE1        0x60

#define NE_REMOTE_WRITE        0x10
#define NE_REMOTE_READ        0x08
#define ISR_DMA_COMPLETE        0x40


#define NE_START        0x02
#define NE_TRANSMIT        0x04
#define NE_STOP         0x01


#define NE_PAR0         (NE_BASE + 0x01)
#define NE_PAR1         (NE_BASE + 0x02)
#define NE_PAR2         (NE_BASE + 0x03)
#define NE_PAR3         (NE_BASE + 0x04)
#define NE_PAR4         (NE_BASE + 0x05)
#define NE_PAR5         (NE_BASE + 0x06)


#define ENISR_RESET   0x80


#define TX_BUFFER_START ((16*1024)/256)
#define RX_BUFFER_START ((16*1024)/256+6)
#define RX_BUFFER_END ((32*1024)/256)    // could made this a lot higher


static u8 ETHER_MAC[] = {0xb0, 0xc4, 0x20, 0x00, 0x00, 0x00};


// statistics
static u32 g_RX_packets;
static u32 g_TX_packets;

void ne2k_GetStats(u32* rx, u32* tx)
{
  *rx = g_RX_packets;
  *tx = g_TX_packets;
}



// Applies to ne2000 version of the card.
#define NESM_START_PG   0x40    /* First page of TX buffer */
#define NESM_STOP_PG    0x80    /* Last page +1 of RX ring */


// setup descriptors, start packet reception
void ne2k_Init(void)
{
  // Clear stats.
  g_RX_packets = 0;
  g_TX_packets = 0;
  
  //ne2k_rx_disable();
}

static void SetTxCount(u32 val)
{
  // Set how many bytes we're going to send.
  outbr((u8)(val & 0xff), EN0_TBCR_LO);
  outbr((u8)((val & 0xff00) >> 8), EN0_TBCR_HI);  
}



static void SetRemAddress(u32 val)
{
  // Set how many bytes we're going to send.
  outbr((u8)(val & 0xff), EN0_REM_START_LO);
  outbr((u8)((val & 0xff00) >> 8), EN0_REM_START_HI);  
}


static void SetRemByteCount(u32 val)
{
  // Set how many bytes we're going to send.
  outbr((u8)(val & 0xff), EN0_REM_CNT_LO);
  outbr((u8)((val & 0xff00) >> 8), EN0_REM_CNT_HI);  
}


static void CopyDataToCard(u32 dest, u8* src, u32 length)
{
  outbr(NE_PAGE0 | NE_START, NE_CMD);
  outbr(ISR_DMA_COMPLETE, EN0_ISR);

  SetRemByteCount(length);
  SetRemAddress(dest);

  outbr(NE_REMOTE_WRITE | NE_START, NE_CMD);
  
  u32 i;
  for (i=0;i<length;i++)
  {
    outbr(*src, NE_BASE + 0x10);
    src++;
  }

  outbr(ISR_DMA_COMPLETE, EN0_ISR);
  outbr(NE_PAGE0 | NE_START, NE_CMD);
}


static void CopyDataFromCard(u32 src, u8* dest, u32 length)
{
  outbr(NE_PAGE0, NE_CMD);
  outbr(ISR_DMA_COMPLETE, EN0_ISR);

  SetRemAddress(src);
  SetRemByteCount(length);

  outbr(NE_REMOTE_READ | NE_START, NE_CMD);

  u32 i;
  for (i=0;i<length;i++)
  {
    *dest = inb(NE_BASE + 0x10);
    dest++;
  }

  outbr(ISR_DMA_COMPLETE, EN0_ISR);
}


// Copy data out of the receive buffer.
static size_t CopyPktFromCard(u8* dest, u32 max_len)
{
  // Find out where the next packet is in card memory
  u32 src = inb(EN0_BOUNDARY) * 256;
  
  u8 header[4];
  CopyDataFromCard(src, header, sizeof(header));
  
  u32 next = header[1];
  
  u32 total_length = header[3];
  total_length <<= 8;
  total_length |=  header[2];
  
  // Now copy it to buffer, if possible, skipping the info header.
  src += 4;
  total_length -= 4;
  CopyDataFromCard(src, dest, total_length);
  
  // Release the buffer by increasing the boundary pointer.
  outbr(next, EN0_BOUNDARY);
  
  return total_length;
}




// Returns size of pkt, or zero if none received.
// Returns size of pkt, or zero if none received.
size_t ne2k_Receive(u8* pkt, size_t max_len)
{
  size_t ret = 0;
  
  outbr(NE_PAGE1 | NE_START, NE_CMD);
  u32 current = inb(EN1_CURR);
  
  // Check if rsr fired.
  outbr(NE_PAGE0, NE_CMD);
  u32 boundary = inb(EN0_BOUNDARY);

  if (boundary != current)
  {
    ret = CopyPktFromCard(pkt, max_len);
    g_RX_packets++;
  }
  
  return ret;  
}

// Returns size of pkt, or zero if none received.
void ne2k_rx_status(u8* pkt, size_t max_len)
{
  size_t ret = 0;
  
  outbr(NE_PAGE1 | NE_START, NE_CMD);
  u32 current = inb(EN1_CURR);
  
  // Check if rsr fired.
  outbr(NE_PAGE0, NE_CMD);
  u32 boundary = inb(EN0_BOUNDARY);

  u8 rsr = inb(NE_BASE + 0x0c);
  u8 isr = inb(NE_BASE + 0x07); 

    monitor_write("\nboundary=");
    monitor_write_hex(boundary);
    monitor_write("\n current=");
    monitor_write_hex(current);
    monitor_write("\n packet rx no errors?=");
    monitor_write_hex(rsr & 0x01);
    monitor_write("\n packet rx with errors?=");
    monitor_write_hex(isr & 0x04);
    monitor_write("\n overflow rx buf?=");
    monitor_write_hex(isr & 0x10);
    monitor_write("\n reset state=");
    monitor_write_hex(isr & 0x80);
    monitor_write("\n");
  
  return ret;  
}



// queue packet for transmission
void ne2k_Transmit(u8* pkt, size_t length, int silent)
{
  CopyDataToCard(16*1024, pkt, length);
  
  // Set how many bytes to transmit
  SetTxCount(length);
  // set transmit page start address to where we copied data above.
  outbr((16*1024) >> 8, EN0_TPSR);

    // issue command to actually transmit a frame
  outbr(NE_PAGE0 | NE_TRANSMIT | NE_START, NE_CMD);  
  
  // Wait for transmission to complete.
  u32 counter = 0;
  while (inb(NE_CMD) & 0x04) { counter++; }

  if(silent) {
      monitor_write("KERNEL>> Transmitted 1 packet with ");
      monitor_write_dec(length);
      monitor_write(" bytes\n");
  }

  g_TX_packets++;
}

void set_physical_addr() {
  outbr(NE_PAGE1_STOP, NE_CMD);

  u8* mac = (u8*)ETHER_MAC;
  outbr(*mac++, NE_BASE + 0x01);
  outbr(*mac++, NE_BASE + 0x02);
  outbr(*mac++, NE_BASE + 0x03);
  outbr(*mac++, NE_BASE + 0x04);
  outbr(*mac++, NE_BASE + 0x05);
  outbr(*mac++, NE_BASE + 0x06);

}

void set_interrupt_mask() {
  outbr(NE_PAGE0_STOP, NE_CMD);
  //outbr(0, NE_BASE + 0x0f);  // no interrupts.
  outbr(0x1, NE_BASE + 0x0f);
}

void ne2k_Linkup_Main()
{
    ne2k_Init();

  set_physical_addr();
  set_interrupt_mask();
  
  outbr(NE_PAGE0_STOP, NE_CMD);
  // clear isr bits
  outbr(0xff, EN0_ISR);
 
  
  // set start/end pages of receive buffer ring.
  outbr(RX_BUFFER_START, EN0_STARTPG);
  outbr(RX_BUFFER_END, EN0_STOPPG);

  // set boundary and current page.
  outbr(RX_BUFFER_START, EN0_BOUNDARY);
  outbr(NE_PAGE1_STOP, NE_CMD);
  outbr(RX_BUFFER_START, EN1_CURR);

  // set receive configuration. (rsr)
  // liberal: 
  //    0xdb :: 11011011 :: 11 MON=0 PRO=1 AM=1 AB=0 AR=1 SEP=1
  // conservative: 
  //    0xc2 :: 11001011 :: 11 MON=0 PRO=0 AM=0 AB=0 AR=1 SEP=0
  outbr(NE_PAGE0_STOP, NE_CMD);
  outbr(0xc2, NE_BASE + 0x0b);

  // setup irq
  // 10010000
  // irqen=1, irqs=001, ios=0000
  //outbr(0x90, NE_BASE + 0x0c);
  
  outbr(NE_PAGE0 | NE_START, NE_CMD);
}
