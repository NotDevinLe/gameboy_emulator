#pragma once
#include <cstdint>

void dma_start(uint8_t start);
void dma_tick();
bool dma_transferring();
