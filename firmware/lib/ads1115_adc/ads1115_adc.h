#pragma once
#include <stdint.h>

// TI ADS1115 16-bit I2C ADC (docs/subsystems/analog-io.md), 2 channels
// used: AI1 = AIN0, AI2 = AIN1, single-ended, single-shot conversion.
// PGA fixed at +-2.048V full scale as a reasonable default for a signal
// already scaled by the board's own input-conditioning divider - the
// exact divider ratio isn't in the docs read so far, so this driver
// returns raw 16-bit signed ADC codes, not engineering units. Apply the
// real scale factor at the mapping layer once the divider ratio is known.
class AdsAnalogInput {
public:
    static const int CHANNEL_COUNT = 2;

    bool begin();

    // index 0 = AI1, index 1 = AI2. Blocks for one conversion (~8ms at the
    // default 128SPS). Returns false on I2C error or bad index.
    bool readChannel(int index, int16_t *out_raw);

private:
    bool startConversion(uint8_t mux_select);
    bool waitForConversion();
    bool readConversionResult(int16_t *out_raw);
};
