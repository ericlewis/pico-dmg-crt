# pico-dmg-crt

A highly optimized project that uses a Raspberry Pi Pico to directly translate LCD signals from an original Game Boy (DMG) to composite video with minimal processor usage. Designed as a portable CRT-based Game Boy with 8+ hour battery life.

## Features

- Real-time Game Boy LCD to NTSC composite video conversion
- 3x horizontal and vertical scaling (160x144 → 480x432)
- Maximized display area with ultra-minimal borders (4 pixels left, 28 pixels right)
- Hardware-accelerated capture and output using PIO state machines
- DMA-driven video pipeline with zero CPU involvement during active video
- Dual-core architecture with dedicated interrupt handling
- Battery powered (3× 18650 Li-ion cells, 8-10 hour runtime)
- Power-optimized with aggressive clock gating
- 4-shade grayscale mapping (authentic Game Boy look)
- 133 MHz system clock for optimal performance [[memory:4297501]]

## Hardware Requirements

- Raspberry Pi Pico
- Game Boy (DMG-01 or Pocket MGB-001) for LCD signal tapping
- 74LVC245 level shifter (5V → 3.3V)
- 8-bit R2R DAC (560Ω/1.1kΩ) for composite video output
- 4" CRT display with composite input
- 3× 18650 Li-ion battery pack with BMS
- MP1584 buck converter (12V → 5V)
- See complete BOM and wiring in [docs/hardware.md](docs/hardware.md)

## Pin Connections

| Signal | Pico GPIO | Description |
|--------|-----------|-------------|
| DATA0 | GPIO 0 | Game Boy pixel data bit 0 |
| DATA1 | GPIO 1 | Game Boy pixel data bit 1 |
| CLOCK | GPIO 4 | Game Boy pixel clock (~4.19 MHz) |
| DAC0-7 | GPIO 8-15 | 8-bit R2R DAC output |
| HSYNC | GPIO 20 | Game Boy horizontal sync input |
| VSYNC | GPIO 21 | Game Boy vertical sync input |

Note: SWD debug pins (GPIO 24/25) are preserved for debugging.

## Building

```bash
# Clone with submodules
git clone --recursive https://github.com/yourusername/pico-dmg-crt.git
cd pico-dmg-crt

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Flash to Pico
cp build/pico_dmg_crt.uf2 /path/to/RPI-RP2
```

## Technical Details

### Performance Optimizations

- **32-bit LUT expansion**: 2-bit packed pixels expanded to 8-bit grayscale in 1/4 the operations
- **DMA vertical scaling**: Hardware-based line triplication eliminates memcpy overhead  
- **Dual-core architecture**: Core 0 handles setup, Core 1 dedicated to interrupts
- **Time-critical functions**: ISR placed in RAM for deterministic timing
- **High-priority DMA**: Capture and video channels prioritized for glitch-free output
- **Aggressive clock gating**: Unused peripherals disabled to minimize power

### Video Timing

- NTSC standard: 262 lines, 15.734 kHz horizontal, 59.94 Hz vertical
- Active video: 480x432 with minimal 4-pixel left border for maximum display coverage
- Pixel clock: 8.06 MHz (3x Game Boy rate)
- Line timing: 63.5 µs (4.7µs sync + 4.7µs back porch + 52.6µs active + 1.5µs front porch)

### Resource Usage

- PIO0: Game Boy capture (1 SM)
- PIO1: H/V sync generation (1 SM) + pixel streaming (1 SM)  
- DMA channels: 5 (capture, video, control, 2x vertical scaling)
- CPU: <5% during active video
- RAM: ~250KB framebuffer + minimal stack/heap

## License

MIT - See LICENSE file for details