#ifndef NE2K_H
#define NE2K_H

#include "common.h"

void ne2k_Init(void);
void ne2k_rx_disable();
void ne2k_rx_enable();

size_t ne2k_Receive(u8* pkt, size_t max_len);
void ne2k_rx_status();
void ne2k_Transmit(u8* pkt, size_t length);
void ne2k_Transmit2(u8* pkt, size_t length);

void ne2k_Linkup_Main();
void ne2k_Linkup_Main2();
void ne2k_GetStats(u32* rx, u32* tx);

#endif
