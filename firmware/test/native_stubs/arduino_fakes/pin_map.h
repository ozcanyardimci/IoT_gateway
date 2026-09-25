#pragma once
// Single source of truth for GPIO/bus assignment on core-compute.kicad_sch.
// Mirrors docs/architecture.md's "Core-compute pin assignment" table
// (verified 2026-09-23) - if that table changes, update here too, and
// vice versa. Do not duplicate these numbers elsewhere in firmware.

// --- Shared I2C bus (ADS1115 ADC, MCP4725 DAC, PCA9535PW expander) ---
#define PIN_I2C_SDA        8
#define PIN_I2C_SCL        9

// I2C addresses (confirmed against docs/subsystems/analog-io.md and
// status-indication.md - address-pin strapping on the real schematic,
// not assumed).
#define I2C_ADDR_ADS1115   0x48  // ADDR -> GND
#define I2C_ADDR_MCP4725   0x60  // A0 -> GND
#define I2C_ADDR_PCA9535   0x20  // A0/A1/A2 -> GND_LOGIC

// --- Ethernet (WIZnet W5500, SPI) ---
#define PIN_ETH_CS         10
#define PIN_ETH_MOSI       11
#define PIN_ETH_SCLK       12
#define PIN_ETH_MISO       13
#define PIN_ETH_INT        14

// --- LTE modem (Quectel EG915U-EU, UART + control lines) ---
#define PIN_LTE_TXD        17  // MCU -> modem
#define PIN_LTE_RXD        18  // modem -> MCU
#define PIN_LTE_PWRKEY     15
#define PIN_LTE_RESET      16

// --- Relay outputs (4x, NPN transistor driver stage) ---
#define PIN_RELAY1         7
#define PIN_RELAY2         38
#define PIN_RELAY3         43  // shares module's default UART0 TX - fine, UART0 unused
#define PIN_RELAY4         44  // shares module's default UART0 RX - fine, UART0 unused

// --- RS232 (MAX3232EIPWR) ---
#define PIN_RS232_TXD      5
#define PIN_RS232_RXD      6

// --- RS485 (isolated transceiver) ---
#define PIN_RS485_TXD      1
#define PIN_RS485_RXD      2

// --- Digital inputs (8x, opto-isolated, active-low - see di_driver.h) ---
#define PIN_DI1            40
#define PIN_DI2            41
#define PIN_DI3            48
#define PIN_DI4            3   // strapping pin (JTAG src select) - safe, see architecture.md
#define PIN_DI5            39
#define PIN_DI6            42
#define PIN_DI7            46  // strapping pin (boot mode) - safe in normal operation
#define PIN_DI8            47

// Note: analog input (ADS1115) and analog output (MCP4725) have no
// dedicated MCU GPIO - both ride the shared I2C bus above (see
// architecture.md: "analog_io only exposes I2C pins today").
