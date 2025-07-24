# Game Boy Video Tap & Output Hardware Guide

This document walks you through the **non-destructive** method of extracting the LCD bus signals from an original Game Boy (DMG-01) or Game Boy Pocket (MGB-001) and feeding them to the Raspberry Pi Pico video board.

> ⚠️  All work is at your own risk. These consoles are 30-year-old hardware – use a temperature-controlled iron, ESD protection, and patience.

---
## 1. Bill of Materials  
*(standard battery-powered build for DMG or Pocket)*

| Qty | Ref | Part | Notes |
|-----|-----|------|-------|
| 1 | U1 | Raspberry Pi Pico | or Pico W |
| 1 | U2 | 74LVC245 | Level-shifter, SOIC-20 / TSSOP-20 |
| – |  | 4″ CRT board | AliExpress/EBay driver w/ composite input |
| 3 | BT1-3 | 18650 Li-ion cell | 2600–3500 mAh, matched set |
| 1 | BMS | 3-S Li-ion protection board | 4 A continuous |
| 1 | PSU | Smart Li-ion charger | 12.6 V, CC-CV, 1–2 A |
| 1 | U3 | MP1584EN buck module | 12 V→5 V logic rail |
| 1 | F1 | 2 A resettable polyfuse | Between pack and BMS |
| 1 | J1 | 2.1 mm DC jack | Charge / dev power |
| 8 | R1-R8 | 560 Ω ±1 % | R ladder ("R") |
| 8 | R9-R16 | 1.1 kΩ ±1 % | R ladder ("2R") |
| 1 | R17 | 75 Ω ±1 % | Video series resistor |
| 1 | Q1 | 2N3904 | Emitter follower |
| 1 | R18 | 1 kΩ | Q1 bias |
| 1 | R19 | 10 kΩ | Q1 emitter bias |
| 1 | C1 | 47 µF tantalum | Main 3.3 V rail |
| 3 | C2-4 | 100 nF ceramic | Decoupling (U1, U2, DAC) |
| 1 | L1 | 33 MHz ferrite bead | DAC noise filter (optional) |
| 1 | SW1 | SPST on/off switch | 5 A rated |
| 1 | J2 | 0.5 mm 21-pin FFC breakout | Pocket LCD interposer |
| – | – | 30 AWG silicone wire | Various colors, ~2 meters total |
| – | – | RCA jack | Composite output (or direct solder) |
| – | – | 3 × 18650 battery holder | With balance leads |
| 1 | – | Proto/perf board | 70 × 90 mm minimum |
| – | – | M2 × 6 mm screws + standoffs | Board mounting |
| – | – | Heat-shrink tubing | Various sizes |

---
## 2. Required Signals

| Purpose | Console net | Pico pin | Notes |
|---------|-------------|----------|-------|
| Pixel bit 0 | **DATA0** | GPIO 0 | 2-bit grayscale LSB |
| Pixel bit 1 | **DATA1** | GPIO 1 | 2-bit grayscale MSB |
| Pixel clock | **CLOCK** | GPIO 4 | ≈ 4.19 MHz |
| Line start | **HSYNC** | GPIO 20 | 1 → 0 at each 160-pixel line |
| Frame start | **VSYNC** | GPIO 21 | 1 → 0 at the top of each frame |

---
## 3. Test Point Locations

### 3.1 Original Game Boy (DMG-01)

| Signal | Test pad / IC pin | Location on PCB |
|--------|-------------------|-----------------|
| DATA0 | **TP5** (Pin 33 of LCD driver U2) | Right of CPU | 
| DATA1 | **TP6** (Pin 32 of LCD driver U2) | Right of CPU |
| CLOCK | **TP4** (Pin 35 of LCD driver U2) | Right of CPU |
| HSYNC | **TP2** (Pin 40 of LCD driver U2) | Right of CPU |
| VSYNC | **TP3** (Pin 39 of LCD driver U2) | Right of CPU |

> Note: Test points are available on most DMG board revisions. Early boards may require tapping the LCD driver IC pins directly.

### 3.2 Game Boy Pocket (MGB-001)

The Pocket puts all five signals on the **21-pin LCD flex cable (0.5 mm pitch)**:

| Signal | Flex pad # | Counting from… |
|--------|------------|----------------|
| DATA0 | 11 | Left (pin 1 = leftmost) |
| DATA1 | 12 | " |
| CLOCK | 13 | " |
| VSYNC | 14 | " |
| HSYNC | 15 | " |

---
## 4. Level Shifting (5 V → 3.3 V)

Game Boy logic runs at 5 V; the Pico GPIO is 3.3 V only. Use a **74LVC245** octal buffer:

```
Game Boy 5 V signals → 74LVC245 (VCC = 3.3 V, inputs 5 V-tolerant) → Pico GPIO
```
*(If you prefer true dual-rail operation, substitute a 74LVC8T245 and keep VCCA=5 V, VCCB=3.3 V.)*

### Schematic

```
         74LVC245
      ┌─────────────┐
 DATA0 ──│A1      B1│── GPIO 0
 DATA1 ──│A2      B2│── GPIO 1
 CLOCK ──│A3      B3│── GPIO 4
   (NC)──│A4      B4│── (NC)
   (NC)──│A5      B5│── (NC)
   (NC)──│A6      B6│── (NC)
 HSYNC ──│A7      B7│── GPIO 20
 VSYNC ──│A8      B8│── GPIO 21
         │          │
  +3.3V──│VCC       │
         │          │
    GND──│GND     OE│── GND (always enabled)
  +3.3V──│DIR       │   (A→B direction)
         └──────────┘
```

> Alternative: 74AHC245 (cheaper but slightly higher propagation delay).

---

## 5. 8-Bit R-2R DAC Circuit

The PIO outputs grayscale values on GPIO 8–15. An R-2R ladder converts this to 0–0.714 V analog:

```
GPIO 8  (LSB) ──[1.1k]──┬──[1.1k]──┬──[1.1k]──┬──...
                       [560]       [560]       [560]
                        │           │           │
GPIO 9  ───────[1.1k]──┴──[1.1k]──┴──[1.1k]──┴──...
                              [560]       [560]
                               │           │
GPIO 10 ──────────────[1.1k]──┴──[1.1k]──┴──...
                                     [560]
                                      │
                                     ...
GPIO 15 (MSB) ───────────────────────[1.1k]──┬── DAC_OUT
                                             [560]
                                              │
                                             GND

DAC_OUT → Q1 base (emitter follower) → 75Ω → Composite output
```

The emitter follower (2N3904 + 1 kΩ base + 10 kΩ emitter) provides current drive for 75 Ω loads.

### R-2R Design Notes

- **Resistor values**: 560 Ω ≈ R and 1.1 kΩ ≈ 2R are standard E24 values that maintain the 1:2 ratio within 1% tolerance
- **Output voltage**: Vout = (Digital Value ÷ 256) × 0.714 V
- **Precision option**: Use 0.1% or 0.5% resistors for more even grayscale steps
- **Noise filtering**: Add 100 Ω + 47 pF RC after DAC_OUT to reduce high-frequency ringing if you see dot crawl

### 5.1 IC DAC Alternative

For a more compact and precise solution, consider these parallel-input DAC ICs:

| Part | Resolution | Speed | Interface | Supply | Notes |
|------|------------|-------|-----------|--------|-------|
| **DAC0808** | 8-bit | 150 ns | Parallel | 5 V | Classic, needs op-amp I-to-V |
| **AD7524** | 8-bit | 100 ns | Parallel | 5 V | Buffered voltage out |
| **TLC7524** | 8-bit | 100 ns | Parallel | 5 V | Pin-compatible with AD7524 |
| **MAX507** | 12-bit | 2 µs | Parallel | 5 V | Use 8 MSBs only |

**Example circuit with DAC0808:**
```
GPIO 8-15 ──→ DAC0808 D0-D7
              │
              ├─ VREF+ → 1.43 V (for 0.714 V FS)
              ├─ VREF- → GND
              ├─ VCC → +5 V
              └─ IOUT → Op-amp I-to-V → 75 Ω → Video
```

The DAC0808 settles in 150 ns, easily meeting the 124 ns/pixel requirement at 8.06 MHz. Use an LM358 op-amp configured as current-to-voltage converter with gain set for 0.714 V full scale.

> Note: While IC DACs offer better linearity and matching, the R-2R ladder is perfectly adequate for composite video and uses more readily available parts.

### 5.2 R-2R Ladder Perfboard Assembly Guide

Here's a practical layout for building the R-2R ladder on perfboard:

```
Perfboard Layout (0.1" spacing):
      GPIO Side                    Output Side
      
    [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]
15→ [●]━━━━━━━━━━━━━━━━━━━━━━[1.1k]━━[●] DAC_OUT
    [ ]                               [560]
14→ [●]━━━━━━━━━━━━━━━━[1.1k]━━[●]━━━━[●] GND
    [ ]                   [560]
13→ [●]━━━━━━━━━━[1.1k]━━[●]━━━━┘
    [ ]             [560]
12→ [●]━━━━━[1.1k]━━[●]━━━━┘
    [ ]       [560]
11→ [●]━[1.1k]━[●]━━━━┘
    [ ]   [560]
10→ [●]━━━[●]━━━━┘      (Continue pattern)
    [ ]   [560]
 9→ [●]━━━[●]━━━━┘
    [ ]   [560]
 8→ [●]━━━[●]━━━━━━━━━━━━━━━━━━━━━━━━━[●] GND
    [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]
```

**Assembly steps:**

1. **Component prep** (makes assembly much easier):
   - Pre-form all 1.1k resistors with 0.4" lead spacing
   - Pre-form all 560Ω resistors with 0.3" lead spacing (vertical mount)
   - Cut hookup wire: 8 pieces at 1.5" for GPIO connections

2. **Build order** (start from GPIO 8, work up):
   - Solder the bottom ground rail first
   - Install 560Ω "rungs" vertically, starting from GPIO 8 position
   - Add 1.1k "rails" horizontally, connecting the ladder
   - Connect GPIO wires last

3. **Wiring tips**:
   - Keep GPIO wires under 2" to minimize noise
   - Use different colors: red=1.1k, yellow=560Ω, blue=GPIO
   - Add a 100nF ceramic cap between DAC_OUT and GND
   - Consider socketing the output connection for easy mods

4. **Testing before connecting**:
   - Measure resistance from each GPIO pad to DAC_OUT
   - Expected values (±5%):
     - GPIO 15 → OUT: 366Ω
     - GPIO 14 → OUT: 733Ω  
     - GPIO 13 → OUT: 1.47kΩ
     - GPIO 8 → OUT: 23.5kΩ
   - Check isolation: >1MΩ between any GPIO and ground

5. **Output buffer mounting**:
   - Place 2N3904 near DAC_OUT (within 0.5")
   - 1kΩ from DAC_OUT to base, 10kΩ from emitter to ground
   - 75Ω from emitter to RCA center pin

**Pro tip**: Build on a 18×12 hole section of perfboard. This gives room for the Pico pins on the left and the video output circuit on the right, with the ladder in between.

### 5.3 Breadboard Prototyping Note

The R-2R ladder works great on breadboard for testing. If using a **Jumperless V5**, you can leverage its unique features:

- **Software routing**: Define the R-2R connections in the Wokwi app, eliminating physical jumper clutter
- **Built-in measurement**: Use the Jumperless's ADC/DAC to verify ladder voltages at each node
- **Cleaner layout**: No physical wires means less parasitic inductance and capacitance
- **Rapid iteration**: Test different resistor values by updating the netlist instead of moving components

**Jumperless V5 considerations:**
- The crosspoint switches add ~100Ω series resistance per connection
- This is negligible compared to the 560Ω/1.1kΩ ladder values
- For final video output, use the breadboard rails or direct connections to minimize switch resistance
- The onboard RP2040 could potentially generate test patterns for DAC verification

Example netlist snippet for Jumperless:
```
# R-2R connections
8-9:1100    # GPIO8 to node via 1.1k
9-10:560    # Node to ground via 560Ω  
9-11:1100   # Node to next stage via 1.1k
```

### 5.4 Jumperless V5 DAC Testing Procedure

Here's how to leverage the Jumperless's built-in RP2040 to test your R-2R ladder before connecting the main Pico:

**1. Full R-2R netlist for Jumperless:**
```python
# Jumperless netlist for 8-bit R-2R DAC
# Row assignments: 1-8 = GPIO inputs, 20-27 = ladder nodes

connections = [
    # GPIO to first resistor
    "1-20:1100",   # GPIO8 to node via 1.1k
    "2-21:1100",   # GPIO9 to node via 1.1k
    "3-22:1100",   # GPIO10 to node via 1.1k
    "4-23:1100",   # GPIO11 to node via 1.1k
    "5-24:1100",   # GPIO12 to node via 1.1k
    "6-25:1100",   # GPIO13 to node via 1.1k
    "7-26:1100",   # GPIO14 to node via 1.1k
    "8-27:1100",   # GPIO15 to node via 1.1k
    
    # Ladder connections (2R to ground)
    "20-GND:560",  # Node 8 to ground
    "21-GND:560",  # Node 9 to ground
    "22-GND:560",  # Node 10 to ground
    "23-GND:560",  # Node 11 to ground
    "24-GND:560",  # Node 12 to ground
    "25-GND:560",  # Node 13 to ground
    "26-GND:560",  # Node 14 to ground
    "27-GND:560",  # Node 15 to ground
    
    # Ladder chain (R between nodes)
    "20-21:1100",  # Chain nodes
    "21-22:1100",
    "22-23:1100",
    "23-24:1100",
    "24-25:1100",
    "25-26:1100",
    "26-27:1100",
    
    # Output
    "27-30:DIRECT",  # Final node to output row
]
```

**2. Jumperless RP2040 test pattern generator:**
```python
# MicroPython code for Jumperless's RP2040
from machine import Pin
import time

# Set up GPIO 8-15 as outputs
dac_pins = [Pin(i, Pin.OUT) for i in range(8, 16)]

def set_dac_value(value):
    """Set 8-bit value on the DAC pins"""
    for i in range(8):
        dac_pins[i].value((value >> i) & 1)

# Test patterns
def test_ramp():
    """Generate a ramp from 0 to 255"""
    print("Ramp test - measure row 30 with multimeter")
    for i in range(256):
        set_dac_value(i)
        time.sleep_ms(10)
        if i % 32 == 0:
            expected = (i / 255) * 0.714
            print(f"Value: {i:3d} | Expected: {expected:.3f}V")

def test_bits():
    """Test each bit individually"""
    print("Bit test - verify 2:1 voltage ratios")
    for bit in range(8):
        set_dac_value(1 << bit)
        expected = ((1 << bit) / 255) * 0.714
        print(f"Bit {bit} high | Expected: {expected:.3f}V")
        time.sleep(1)

def test_linearity():
    """Check key points for linearity"""
    test_values = [0, 64, 128, 192, 255]
    print("Linearity test:")
    for val in test_values:
        set_dac_value(val)
        expected = (val / 255) * 0.714
        print(f"Value: {val:3d} | Expected: {expected:.3f}V")
        time.sleep(2)

# Run tests
test_linearity()  # Quick verification
# test_ramp()     # Full ramp
# test_bits()     # Individual bits
```

**3. Measurement verification:**
- Connect multimeter/scope to row 30 (DAC output)
- Run `test_linearity()` first - should see:
  - 0 → 0.000V
  - 64 → 0.179V  
  - 128 → 0.357V
  - 192 → 0.536V
  - 255 → 0.714V
- Tolerance: ±10mV is fine for video work
- If values are way off, check resistor connections in netlist

**3. Automated measurement with Jumperless V5's built-in ADC:**

Since the Jumperless V5 has 7 built-in 12-bit ADCs that can read ±8V, you can automate the entire test process:

```python
# Auto-measure DAC output using Jumperless's built-in ADC
def test_dac_auto():
    """Fully automated DAC test using Jumperless measurement"""
    # Route ADC channel 0 to row 30 (DAC output)
    jumperless.connect("ADC0", "30")
    
    print("Automated DAC Test")
    print("-" * 40)
    
    errors = []
    for val in range(0, 256, 16):  # Test every 16 values
        set_dac_value(val)
        time.sleep_ms(10)  # Allow settling
        
        # Read voltage using Jumperless ADC
        measured = jumperless.measure("ADC0")
        expected = (val / 255) * 0.714
        error = abs(measured - expected)
        
        print(f"DAC: {val:3d} | Expected: {expected:.3f}V | "
              f"Measured: {measured:.3f}V | Error: {error:.3f}V")
        
        if error > 0.010:  # Flag errors > 10mV
            errors.append((val, expected, measured))
    
    # Report results
    if errors:
        print(f"\n⚠️  Found {len(errors)} values out of spec:")
        for val, exp, meas in errors:
            print(f"   Value {val}: expected {exp:.3f}V, got {meas:.3f}V")
    else:
        print("\n✅ All measurements within 10mV tolerance!")
    
    # Bonus: Check linearity
    print("\nLinearity check (should see ~2.8mV per step):")
    for i in range(1, 5):
        val1, val2 = i * 50, (i + 1) * 50
        set_dac_value(val1)
        v1 = jumperless.measure("ADC0")
        set_dac_value(val2)
        v2 = jumperless.measure("ADC0")
        step_size = (v2 - v1) / 50 * 1000  # mV per bit
        print(f"Steps {val1}-{val2}: {step_size:.2f} mV/bit")

# Run the automated test
test_dac_auto()
```

**Using the probe for spot checks:**
- Set Jumperless to multimeter mode
- Touch the probe to row 30 while running test patterns
- The breadboard LEDs will display the voltage in real-time
- You can also probe individual ladder nodes to debug issues

**Real-time oscilloscope mode:**
```python
# Use Jumperless as oscilloscope to view DAC waveform
jumperless.scope_mode("ADC0", "row_30")
jumperless.scope_trigger(0.350, "rising")  # Trigger at mid-scale

# Generate test pattern
while True:
    # Sawtooth for linearity check
    for i in range(256):
        set_dac_value(i)
    # Step response for settling time
    set_dac_value(0)
    time.sleep_us(100)
    set_dac_value(255)
    time.sleep_us(100)
```

The Jumperless will display the waveform on the breadboard LEDs, letting you visually inspect:
- Linearity (should be a perfect ramp)
- Settling time (watch for overshoot/ringing)
- Glitch energy at major bit transitions

### 5.5 Complete Idiot's Guide: Testing Your R-2R DAC with Jumperless V5

This guide assumes you've never used a Jumperless before. We'll build and test the entire R-2R DAC step-by-step.

#### What You'll Need:
- Jumperless V5 breadboard
- 8× 1.1kΩ resistors (1% tolerance)
- 8× 560Ω resistors (1% tolerance)
- USB-C cable
- Computer with Chrome/Edge browser

#### Step 1: Initial Jumperless Setup

1. **Connect Jumperless to your computer** via USB-C
2. **Open the Wokwi web interface**:
   - Go to [wokwi.com](https://wokwi.com)
   - Click "New Project" → "Jumperless"
   - Your Jumperless should connect automatically
3. **Test the connection**:
   - Type in the terminal: `jumperless.connect("1", "2")`
   - You should see row 1 and 2 light up and connect

#### Step 2: Build the R-2R Ladder

1. **Place resistors on the Jumperless** (physical components):
   ```
   Row   Component    Placement
   ----  ----------   ---------
   1-8   GPIO inputs  (virtual - we'll route these)
   20    560Ω #1      Between holes a20 and b20
   21    560Ω #2      Between holes a21 and b21
   ...   (continue for all 8)
   27    560Ω #8      Between holes a27 and b27
   
   20-21 1.1kΩ #1     Between holes e20 and e21
   21-22 1.1kΩ #2     Between holes e21 and e22
   ...   (continue for all 7)
   26-27 1.1kΩ #7     Between holes e26 and e27
   ```

2. **Create the netlist in Wokwi terminal**:
   ```python
   # Copy and paste this entire block:
   netlist = """
   1-20:1100, 2-21:1100, 3-22:1100, 4-23:1100,
   5-24:1100, 6-25:1100, 7-26:1100, 8-27:1100,
   20-GND:560, 21-GND:560, 22-GND:560, 23-GND:560,
   24-GND:560, 25-GND:560, 26-GND:560, 27-GND:560,
   20-21:1100, 21-22:1100, 22-23:1100, 23-24:1100,
   24-25:1100, 25-26:1100, 26-27:1100, 27-30:DIRECT
   """
   jumperless.netlist(netlist)
   ```

3. **Verify connections visually**:
   - Rows should light up showing connections
   - Row 30 is your DAC output
   - Ground connections show as blue

#### Step 3: Load Test Code

1. **Copy this test code** into a file called `dac_test.py`:
   ```python
   from machine import Pin
   import time
   
   # GPIO 8-15 as DAC outputs
   pins = [Pin(i, Pin.OUT) for i in range(8, 16)]
   
   def set_dac(value):
       for i in range(8):
           pins[i].value((value >> i) & 1)
   
   def test_basic():
       print("Basic DAC Test - Watch row 30")
       print("=" * 40)
       
       # Test key values
       test_vals = [0, 64, 128, 192, 255]
       for val in test_vals:
           set_dac(val)
           expected = (val / 255) * 0.714
           print(f"Setting DAC to {val:3d} → {expected:.3f}V")
           
           # Route ADC to measure
           jumperless.connect("ADC0", "30")
           time.sleep_ms(50)
           
           measured = jumperless.measure("ADC0")
           error = abs(measured - expected)
           
           status = "✅" if error < 0.010 else "❌"
           print(f"  Measured: {measured:.3f}V {status}")
           print()
           time.sleep(2)
   
   # Run the test
   test_basic()
   ```

2. **Upload to Jumperless**:
   - In Wokwi, click "Files" → "Upload"
   - Select your `dac_test.py`
   - In terminal: `exec(open('dac_test.py').read())`

#### Step 4: Run Tests & Interpret Results

**Good result looks like:**
```
Basic DAC Test - Watch row 30
========================================
Setting DAC to   0 → 0.000V
  Measured: 0.002V ✅

Setting DAC to  64 → 0.179V
  Measured: 0.181V ✅

Setting DAC to 128 → 0.357V
  Measured: 0.356V ✅
```

**If you see ❌ errors:**

| Problem | Likely Cause | Fix |
|---------|--------------|-----|
| All values read 0V | No power to ladder | Check row 1-8 connections |
| All values too high | Wrong resistor values | Verify 560Ω vs 1.1kΩ |
| One value way off | Bad connection | Check that specific bit's path |
| Random/unstable | Loose component | Reseat resistors |

#### Step 5: Visual Oscilloscope Test

1. **Run this for a visual check**:
   ```python
   # Continuous ramp for scope view
   while True:
       for i in range(256):
           set_dac(i)
       # Press Ctrl+C to stop
   ```

2. **What to look for**:
   - Jumperless shows the waveform on the breadboard
   - Should see a smooth ramp/sawtooth
   - Any "steps" or jumps indicate bad connections

#### Step 6: Using the Probe

1. **Physical probe measurements**:
   - Set probe to voltage mode (green LED)
   - Touch probe to row 30
   - Breadboard displays voltage in real-time
   - Try probing rows 20-27 to check ladder nodes

2. **Expected node voltages** (with DAC set to 255):
   - Row 27 (MSB): ~0.357V
   - Row 26: ~0.536V  
   - Row 25: ~0.625V
   - Row 20 (LSB): ~0.710V

#### Step 7: Final Verification

Run this comprehensive test:
```python
# Automated full test
errors = 0
for val in range(0, 256, 8):
    set_dac(val)
    time.sleep_ms(20)
    measured = jumperless.measure("ADC0")
    expected = (val / 255) * 0.714
    if abs(measured - expected) > 0.015:
        errors += 1
        print(f"FAIL at {val}: {measured:.3f}V vs {expected:.3f}V")

if errors == 0:
    print("🎉 PERFECT! Your DAC is ready!")
    print("You can now move resistors to perfboard")
else:
    print(f"😞 Found {errors} errors - check connections")
```

#### Step 8: Moving to Perfboard

Once everything passes:
1. **Document your working layout** (take a photo!)
2. **Transfer resistors one by one** to perfboard
3. **Follow the perfboard layout** from section 5.2
4. **Test with your Pico** using the same DAC values

#### Troubleshooting Tips

- **Nothing works?** Start with just 2 bits (GPIO 8-9) first
- **Voltage too low?** Check ground connections
- **Noisy readings?** Add 100nF cap between row 30 and GND
- **Still stuck?** The Jumperless Discord is super helpful!

#### Bonus: Save Your Setup

In Wokwi terminal:
```python
# Save your netlist for later
jumperless.save_project("r2r_dac_test")
# Reload anytime with:
# jumperless.load_project("r2r_dac_test")
```

That's it! You've now built, tested, and verified an 8-bit R-2R DAC without touching a multimeter. The Jumperless did all the measuring for you! 🚀

#### Pro Tip: Testing with Your Actual Pico

Since the Jumperless V5 works with any dev board, you can test with your **actual Raspberry Pi Pico**:

1. **Place your Pico directly on the Jumperless breadboard**
2. **Route the connections**:
   ```python
   # Connect your Pico's GPIO 8-15 to the R-2R ladder
   jumperless.netlist("""
   PICO_GP8-20:DIRECT, PICO_GP9-21:DIRECT, 
   PICO_GP10-22:DIRECT, PICO_GP11-23:DIRECT,
   PICO_GP12-24:DIRECT, PICO_GP13-25:DIRECT,
   PICO_GP14-26:DIRECT, PICO_GP15-27:DIRECT
   """)
   ```

3. **Run your actual video output code** on the Pico:
   - The same PIO program from your project
   - The same timing and clock settings
   - The same grayscale generation

4. **Jumperless measures your real video signal**:
   - See the composite waveform on the LED display
   - Verify sync levels (should dip to ~0V)
   - Check blanking level (~0.34V)
   - Confirm white level (~0.714V)

This way you're testing with the **exact hardware and software** that will go in your final build. No surprises when you move to perfboard - you've already proven it works with your actual Pico running your actual code!

**Bonus**: You can even connect a small monitor to row 30 (through proper coupling) to see if your Game Boy video displays correctly while measuring signals simultaneously.

---

## 6. Assembly Procedures

### 6.1 DMG-01 Wiring

1. Open the console, remove the front PCB.
2. Locate test pads TP2–TP6 near the CPU.
3. Clean pads with IPA; tin with fresh leaded solder.
4. Solder 32 AWG enamel wires to each pad:
   - Strip 2 mm, pre-tin, tack perpendicular to the pad
   - Keep wires < 10 cm to minimize noise
5. Route the 5-wire bundle along the PCB edge to the cartridge slot area.
6. Mount the 74LVC245 on perfboard with the Pico nearby.
7. Connect:
   - Test pad wires → LVC245 A-side inputs
   - LVC245 B-side outputs → Pico GPIO pins
   - +3.3 V from Pico 3V3 pin → VCC + DIR pins (DIR held high for A→B direction)
   - GB ground → Pico ground (single point near DC jack)
8. Build R-2R ladder between GPIO 8–15 and composite output.
9. Before powering, continuity-test every connection and check for shorts.

### 6.2 Game Boy Pocket (MGB-001) Wiring

The Pocket routes all LCD signals through a 0.5 mm-pitch 21-pin flex cable:

1. Disassemble the console; remove the front PCB and release the LCD flex from its ZIF socket.
2. Insert a 0.5 mm FFC-to-2.54 mm breakout board into the motherboard's ZIF connector.
3. Re-insert the original LCD flex into the breakout's top socket.
4. On the breakout, solder 32 AWG wires to pads 11–15 (DATA0/1, CLOCK, VSYNC, HSYNC).
5. Route wires to the 74LVC245 and continue as with the DMG procedure above.

> Alternative (advanced): Carefully solder directly to the 0.5 mm flex pads using flux and a fine-tip iron.

---

## 7. Power Architecture

### Standard Battery Supply (3× 18650 Li-ion)

The battery pack **feeds the CRT first** at 11.1 V nominal; all other rails step down from this parent supply:

| Rail | Voltage | Current (typical) | Part | Notes |
|------|---------|-------------------|------|-------|
| **VBAT** | 9–12.6 V | 350–600 mA | 3S pack + BMS | CRT board draws most power |
| **+5 V** | 5 V ±5 % | ≤ 500 mA | MP1584EN buck | Pico, Game Boy, shifters |
| **+3.3 V** | 3.3 V | ≤ 100 mA | Pico's onboard LDO | From Pico 3V3 pin |

**Runtime calculation:**  
- Total draw ≈ 3.5 W → 315 mA @ 11.1 V  
- 2600 mAh cells → ~8 hours  
- 3500 mAh cells → ~11 hours

### Power Distribution

1. **Pack assembly**: Series-connect three 18650s with spot welds or spring holders
2. **Protection**: Wire through 3S BMS (4 A continuous rating minimum)
3. **Fusing**: Add 2 A polyfuse between pack positive and BMS B+
4. **Switching**: SPDT rocker between BMS P+ and downstream loads
5. **Buck converter**: MP1584EN module set to 5.0 V output
6. **CRT connection**: 12 V from BMS P+ → CRT board VIN (can handle 9–15 V)
7. **Game Boy power**:
   - DMG: 5 V → DC jack center pin (bypasses internal regulator)
   - Pocket: 5 V → boost module → 3 V to battery terminals

### Grounding & Filtering

- Star ground at the battery BMS negative terminal
- 100 nF ceramic at each IC's power pins
- 47 µF tantalum on 3.3 V rail near level shifter
- 220 µF electrolytic on 5 V rail after buck converter
- Optional: 33 MHz ferrite bead on 3.3 V feeding the DAC

---

## 8. Testing & Verification

1. **Power rails**: Check 5 V and 3.3 V before connecting logic
2. **Signal integrity**: Scope the Pico inputs; should see clean 3.3 V square waves
3. **DAC output**: Verify 0–0.714 V swing with Pico outputting test patterns
4. **Sync levels**: Composite output should show ~0.3 V sync tips
5. **First image**: Expect horizontal rolling until fine-tuning PIO timing

---

## 9. Troubleshooting

| Problem | Likely cause | Solution |
|---------|--------------|----------|
| No video | Power issue | Check all rails, verify CRT board is receiving 11+ V |
| Rolling image | Sync timing | Adjust PIO hsync/vsync delays |
| Vertical lines | Ground loop | Single-point ground, add ferrite to DAC supply |
| Dim image | DAC loading | Check emitter follower bias, verify 75 Ω termination |
| Intermittent | Poor solder | Reflow all connections, especially 0.5 mm flex |

---

## 10. Optional Upgrades

- **74LVC8T245** or **CD74VHCT540** for auto-direction or latching level-shift
- **AOZ1282/TPS22929** load-switch for automatic 3.3 V disconnect on low GB supply
- 2S pack + boost/SEPIC converter (less efficient but more compact than 3S)
- Flex interposer PCB for completely solder-free Pocket installation

---

### Optional: Bench Power Supply

If you need to power everything from an external brick for development:

| Rail | Voltage / Current | Conversion | Part |
|------|-------------------|------------|------|
| **+12 V** | 12 V, 1 A | Input brick | Any regulated 12 V/2 A adapter |
| **+5 V** | 5 V, 500 mA | Buck from 12 V | MP1584EN module |
| **+3.3 V** | 3.3 V, 100 mA | Pico's onboard LDO | From Pico 3V3 pin |

Connect through the charge jack with a Schottky diode (1N5819) to prevent back-feeding the battery pack.