#include "video_output.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/sync.h"
#include "hv_gen.pio.h"
#include "mono_stream3x.pio.h"
#include "video_config.h"
#include <assert.h>

// Video timing constants
#define LINES_PER_FRAME 262u
#define CYCLES_PER_LINE 1524u
#define GB_VISIBLE_PIXELS 160u

// DMA ring buffer size constants
#define RING_ON_PIO_TX_FIFO_BITS   4  // 16-byte wrap
#define RING_ON_RING_LIST_BITS    11  // 2048-byte wrap

static inline void hv_gen_program_init(PIO pio, uint sm, uint offset, uint pin_base) {
    pio_sm_config c = hv_gen_program_get_default_config(offset);
    sm_config_set_out_pins(&c, pin_base, 2);
    sm_config_set_sideset_pins(&c, pin_base);
    pio_gpio_init(pio, pin_base);
    pio_gpio_init(pio, pin_base + 1);
    pio_sm_set_consecutive_pindirs(pio, sm, pin_base, 2, true);
    pio_sm_init(pio, sm, offset, &c);
}

static inline void mono_stream3x_program_init(PIO pio, uint sm, uint offset, uint pin_base, uint pin_count) {
    pio_sm_config c = mono_stream3x_program_get_default_config(offset);
    sm_config_set_out_pins(&c, pin_base, pin_count);
    sm_config_set_out_shift(&c, true, false, 32);  // shift right, no autopull
    for(uint i = 0; i < pin_count; i++) {
        pio_gpio_init(pio, pin_base + i);
    }
    pio_sm_set_consecutive_pindirs(pio, sm, pin_base, pin_count, true);
    // Pre-load the pixel count (GB_VISIBLE_PIXELS-1) into the TX FIFO
    pio->txf[sm] = GB_VISIBLE_PIXELS-1;
    pio_sm_init(pio, sm, offset, &c);
}

void hv_init(PIO p) {
    int off = pio_add_program(p, &hv_gen_program);
    assert(off >= 0);
    hv_gen_program_init(p, 0, off, HV_PIN_BASE);
    pio_sm_put_blocking(p, 0, ((LINES_PER_FRAME-1)<<16)|(CYCLES_PER_LINE-1));
    pio_sm_set_clkdiv(p, 0, (float)clock_get_hz(clk_sys)/24e6f);
    pio_sm_set_enabled(p, 0, true);
}

void stream_init(PIO p) {
    int off = pio_add_program(p, &mono_stream3x_program);
    assert(off >= 0);
    mono_stream3x_program_init(p, 1, off, DAC_PIN_BASE, 8);
    pio_sm_set_clkdiv(p, 1, (float)clock_get_hz(clk_sys)/8.06e6f);
    pio_sm_set_enabled(p, 1, true);
}

void video_dma_ring(volatile uint8_t *fb_ptr, uint32_t *ring_buffer) {
    // Initialize ring buffer
    for(int i = 0; i < OUT_H; i++) {
        ring_buffer[i] = (uint32_t)(fb_ptr + i * LINE_BYTES);
    }

    dma_channel_config vc = dma_channel_get_default_config(VID_DMA_CH);
    channel_config_set_read_increment(&vc, true);
    channel_config_set_write_increment(&vc, false);
    channel_config_set_transfer_data_size(&vc, DMA_SIZE_8);
    channel_config_set_dreq(&vc, pio_get_dreq(pio1, 1, true));
    channel_config_set_chain_to(&vc, CTRL_DMA_CH);
    // Ring the write address of the data channel to the PIO TX FIFO
    channel_config_set_ring(&vc, false, RING_ON_PIO_TX_FIFO_BITS);
    dma_channel_configure(VID_DMA_CH, &vc, &pio1->txf[1], fb_ptr, LINE_BYTES, false);

    dma_channel_config cc = dma_channel_get_default_config(CTRL_DMA_CH);
    channel_config_set_read_increment(&cc, true);
    channel_config_set_write_increment(&cc, false);
    channel_config_set_transfer_data_size(&cc, DMA_SIZE_32);
    channel_config_set_chain_to(&cc, VID_DMA_CH);
    channel_config_set_ring(&cc, true, RING_ON_RING_LIST_BITS);
    dma_channel_configure(CTRL_DMA_CH, &cc,
        &dma_hw->ch[VID_DMA_CH].read_addr, ring_buffer, OUT_H, true);

    dma_channel_start(CTRL_DMA_CH);
} 