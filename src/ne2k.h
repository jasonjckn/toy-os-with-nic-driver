#ifndef NE2K_H
#define NE2K_H

#include "common.h"

void ne2k_Init(void);

size_t ne2k_Receive(u8* pkt, size_t max_len);
void ne2k_rx_status();
void ne2k_Transmit(u8* pkt, size_t length, int silent);

void ne2k_Linkup_Main();
void ne2k_GetStats(u32* rx, u32* tx);

#endif
