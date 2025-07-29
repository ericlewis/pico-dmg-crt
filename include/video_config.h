#ifndef VIDEO_CONFIG_H
#define VIDEO_CONFIG_H

// Pin assignments
#define GB_PIN_BASE 0
#define HV_PIN_BASE 20
#define DAC_PIN_BASE 8

// DMA channel assignments
#define CAP_DMA_CH 0
#define VID_DMA_CH 1
#define CTRL_DMA_CH 2
#define VSCALE_DMA_CH0 3
#define VSCALE_DMA_CH1 4

// Game Boy resolution
#define GB_W 160
#define GB_H 144

// Scaling factors
#define XS 3
#define YS 3

// Output resolution
#define OUT_W (GB_W*XS)
#define OUT_H (GB_H*YS)

// Padding for centering - MAXIMIZED for full display
#define PAD_L 4    // Ultra-minimal left padding for maximum display
#define PAD_T 8    // Ultra-minimal top padding for maximum display
#define PAD_R 0
#define PAD_B 0

// Line width in bytes (must be power of 2 for DMA alignment)
#define LINE_BYTES 512

#endif // VIDEO_CONFIG_H