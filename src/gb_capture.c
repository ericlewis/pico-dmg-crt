#include "gb_capture.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "hardware/sync.h"
#include "gb_capture.pio.h"
#include "video_config.h"
#include <assert.h>

// original 2-bit -> 8-bit grayscale map
static const uint8_t lut[4] = {210, 180, 145, 110};

// 32-bit expansion LUT: 0x00-0xFF -> four 8-bit grayscale pixels
static uint32_t __attribute__((aligned(256))) expand_lut[256];

// Raw buffer: 160 pixels × 4 bits per sample = 80 bytes = 20 words
static volatile uint32_t __attribute__((aligned(32))) raw[20];  // 20 32-bit words

// Global pointer to framebuffer
static volatile uint8_t *g_fb_ptr;

static void build_expand_lut(void) {
    for (uint16_t b = 0; b < 256; ++b) {
        uint32_t word = 0;
        for (int p = 0; p < 4; ++p) {
            uint8_t val = (b >> (6 - (p * 2))) & 0x3; // extract 2-bit pixel
            ((uint8_t *)&word)[p] = lut[val];
        }
        expand_lut[b] = word;
    }
}

// PIO program init functions
static inline void gb_capture_program_init(PIO pio, uint sm, uint offset, uint pin_base) {
    pio_sm_config c = gb_capture_program_get_default_config(offset);
    sm_config_set_in_pins(&c, pin_base);
    sm_config_set_jmp_pin(&c, pin_base + 3);  // HSYNC on pin 3 for conditional jump
    sm_config_set_in_shift(&c, false, true, 32);  // autopush at 32 bits
    
    // Initialize all 5 pins
    for (int i = 0; i < 5; i++) {
        pio_gpio_init(pio, pin_base + i);
    }
    pio_sm_set_consecutive_pindirs(pio, sm, pin_base, 5, false);
    pio_sm_init(pio, sm, offset, &c);
}

void __isr __time_critical_func(dma0_isr)(void){
    dma_hw->ints0 = 1u << CAP_DMA_CH;
    static uint row = 0;

    // Direct expansion to framebuffer
    uint8_t *dst = (uint8_t*)g_fb_ptr + (PAD_T + row * YS) * LINE_BYTES + PAD_L;
    
    // Extract pixel data from 4-bit samples
    // Each 32-bit word contains 8 samples, each sample is 4 bits: [HSYNC|VSYNC|D1|D0]
    uint8_t packed_pixels[GB_W/4];
    int pixel_idx = 0;
    
    for (int word = 0; word < 20; word++) {
        uint32_t data = raw[word];
        // Process 8 samples per word
        for (int sample = 0; sample < 8 && pixel_idx < GB_W; sample++) {
            uint8_t pixel_data = data & 0x3;  // Extract D1:D0
            
            // Pack 4 pixels into each byte
            int byte_idx = pixel_idx / 4;
            int bit_pos = 6 - ((pixel_idx % 4) * 2);
            
            if (pixel_idx % 4 == 0) {
                packed_pixels[byte_idx] = 0;  // Clear byte on first pixel
            }
            packed_pixels[byte_idx] |= (pixel_data << bit_pos);
            
            data >>= 4;  // Move to next 4-bit sample
            pixel_idx++;
        }
    }
    
    // Expand 2-bit packed pixels to framebuffer using LUT
    for (int j = 0; j < GB_W / 4; ++j) {
        *(uint32_t *)(dst + (j * 4)) = expand_lut[packed_pixels[j]];
    }
    
    // Trigger DMA chains for vertical replication (lines 2 and 3)
    dma_channel_set_read_addr(VSCALE_DMA_CH0, dst, false);
    dma_channel_set_write_addr(VSCALE_DMA_CH0, dst + LINE_BYTES, false);
    dma_channel_set_read_addr(VSCALE_DMA_CH1, dst, false);
    dma_channel_set_write_addr(VSCALE_DMA_CH1, dst + 2 * LINE_BYTES, true); // Trigger chain

    if (++row >= GB_H) {
        row = 0;
    }
}

void cap_init(PIO p, volatile uint8_t *fb_ptr) {
    // Store pointer to framebuffer
    g_fb_ptr = fb_ptr;
    
    // Initialize the expansion lookup table
    build_expand_lut();
    
    int off = pio_add_program(p, &gb_capture_program);
    assert(off >= 0);
    gb_capture_program_init(p, 0, off, GB_PIN_BASE);
    pio_sm_set_enabled(p, 0, true);

    dma_channel_config cfg = dma_channel_get_default_config(CAP_DMA_CH);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_dreq(&cfg, pio_get_dreq(p, 0, false));
    dma_channel_configure(CAP_DMA_CH, &cfg, raw, &p->rxf[0], 20, true);  // 20 words for 160 pixels
    dma_channel_set_irq0_enabled(CAP_DMA_CH, true);
    
    // Configure DMA for vertical scaling (3x)
    dma_channel_config vs0 = dma_channel_get_default_config(VSCALE_DMA_CH0);
    channel_config_set_transfer_data_size(&vs0, DMA_SIZE_32);
    channel_config_set_read_increment(&vs0, true);
    channel_config_set_write_increment(&vs0, true);
    channel_config_set_chain_to(&vs0, VSCALE_DMA_CH1);
    dma_channel_configure(VSCALE_DMA_CH0, &vs0,
        NULL, NULL,  // addresses set dynamically in ISR
        GB_W / 4, false);
    
    dma_channel_config vs1 = dma_channel_get_default_config(VSCALE_DMA_CH1);
    channel_config_set_transfer_data_size(&vs1, DMA_SIZE_32);
    channel_config_set_read_increment(&vs1, true);
    channel_config_set_write_increment(&vs1, true);
    dma_channel_configure(VSCALE_DMA_CH1, &vs1,
        NULL, NULL,  // addresses set dynamically in ISR
        GB_W / 4, false);
    
    // Setup interrupt handler (will run on core 1)
    irq_set_exclusive_handler(DMA_IRQ_0, dma0_isr);
    irq_set_enabled(DMA_IRQ_0, true);
} 