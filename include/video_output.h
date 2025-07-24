#ifndef VIDEO_OUTPUT_H
#define VIDEO_OUTPUT_H

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "video_config.h"

/**
 * Initialize the horizontal and vertical sync generator
 * 
 * @param p PIO instance to use
 */
void hv_init(PIO p);

/**
 * Initialize the pixel stream generator
 * 
 * @param p PIO instance to use
 */
void stream_init(PIO p);

/**
 * Setup the DMA ring buffer for video output
 * 
 * @param fb_ptr Pointer to framebuffer
 * @param ring_buffer Pointer to ring buffer array
 */
void video_dma_ring(volatile uint8_t *fb_ptr, uint32_t *ring_buffer);

#endif // VIDEO_OUTPUT_H 