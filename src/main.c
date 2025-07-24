#include <string.h>
#include "pico/stdlib.h"
#include "pico/platform.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "pico/multicore.h"
#include "boards/pico.h"
#include "video_config.h"
#include "gb_capture.h"
#include "video_output.h"
#include <assert.h>

// Build-time option: retain UART only for debug builds
#ifndef NDEBUG
#define USE_STDIO 1
#else
#define USE_STDIO 0
#endif

// Global buffers and state variables
static volatile uint8_t  __attribute__((aligned(64))) fb[LINE_BYTES*OUT_H];  // Single framebuffer

// Global ring buffer for DMA control
static uint32_t __attribute__((aligned(32))) ring_buffer[OUT_H];

static void core1_entry(void) {
    cap_init(pio0, fb);
    // Core 1 handles all interrupts for better cache locality
    while (1) {
        __asm__("wfi");
    }
}

int main(void){
    // Optimize voltage for 133 MHz operation
    vreg_set_voltage(VREG_VOLTAGE_1_10);
    sleep_ms(1);
    set_sys_clock_khz(133000, true);
    
#if USE_STDIO
    stdio_init_all();
#endif
    
    // Set DMA high priority for capture and video channels
    hw_set_bits(&dma_hw->ch[CAP_DMA_CH].ctrl_trig, DMA_CH0_CTRL_TRIG_HIGH_PRIORITY_BITS);
    hw_set_bits(&dma_hw->ch[VID_DMA_CH].ctrl_trig, DMA_CH1_CTRL_TRIG_HIGH_PRIORITY_BITS);
    hw_set_bits(&dma_hw->ch[CTRL_DMA_CH].ctrl_trig, DMA_CH2_CTRL_TRIG_HIGH_PRIORITY_BITS);
    hw_set_bits(&dma_hw->ch[VSCALE_DMA_CH0].ctrl_trig, DMA_CH0_CTRL_TRIG_HIGH_PRIORITY_BITS);
    hw_set_bits(&dma_hw->ch[VSCALE_DMA_CH1].ctrl_trig, DMA_CH1_CTRL_TRIG_HIGH_PRIORITY_BITS);

    hv_init(pio1);
    stream_init(pio1);
    video_dma_ring(fb, ring_buffer);

    // Aggressive clock gating for unused peripherals
    clock_stop(clk_adc);
    clock_stop(clk_rtc);
#ifndef USE_STDIO
    clock_stop(clk_peri);  // No USB/UART/SPI/I2C needed after init
#endif
    
    // Define pins that are actually used by the application
    const uint8_t used_pins[] = {
        // GB capture pins
        GB_PIN_BASE, GB_PIN_BASE + 1, GB_PIN_BASE + 4,
        // HV sync pins
        HV_PIN_BASE, HV_PIN_BASE + 1,
        // DAC output pins (8 pins)
        DAC_PIN_BASE, DAC_PIN_BASE + 1, DAC_PIN_BASE + 2, DAC_PIN_BASE + 3,
        DAC_PIN_BASE + 4, DAC_PIN_BASE + 5, DAC_PIN_BASE + 6, DAC_PIN_BASE + 7,
        // SWD debug pins - always preserve these
        PICO_DEFAULT_SWCLK_PIN, PICO_DEFAULT_SWDIO_PIN
    };
    
    // Disable pull-ups/pull-downs on all pins except those used
    for (uint i = 0; i < 30; i++) {
        bool is_used = false;
        for (size_t j = 0; j < sizeof(used_pins) / sizeof(used_pins[0]); j++) {
            if (i == used_pins[j]) {
                is_used = true;
                break;
            }
        }
        
        if (!is_used) {
            gpio_set_pulls(i, false, false);
        }
    }
    
    // Move time-critical interrupts to core 1
    multicore_launch_core1(core1_entry);
    
    // Core 0 sleeps
    while(1) __asm__("wfi");
}

// Custom panic handler for Release builds that doesn't use stdio
__attribute__((used)) __attribute__((noreturn)) 
void custom_panic(const char *fmt, ...) {
    (void)fmt;
    // In release mode, just halt the system
    // Could add LED blink pattern here if desired
    __asm__("bkpt #0");
    while(1) {
        __asm__("wfi");
    }
}

