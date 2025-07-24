#ifndef GB_CAPTURE_H
#define GB_CAPTURE_H

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "video_config.h"

/**
 * Initialize the Game Boy capture system
 * 
 * @param p PIO instance to use for capture
 * @param fb_ptr Pointer to framebuffer
 */
void cap_init(PIO p, volatile uint8_t *fb_ptr);

#endif // GB_CAPTURE_H 