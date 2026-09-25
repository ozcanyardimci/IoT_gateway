#include "reset_reason.h"
#include <esp_system.h>

ResetCause getResetCause() {
    esp_reset_reason_t r = esp_reset_reason();
    switch (r) {
        case ESP_RST_POWERON:  return ResetCause::POWER_ON;
        case ESP_RST_BROWNOUT: return ResetCause::BROWNOUT;
        case ESP_RST_SW:       return ResetCause::SOFTWARE;
        case ESP_RST_TASK_WDT:
        case ESP_RST_INT_WDT:
        case ESP_RST_WDT:      return ResetCause::WATCHDOG;
        default:                return ResetCause::OTHER;
    }
}

const char *resetCauseToString(ResetCause cause) {
    switch (cause) {
        case ResetCause::POWER_ON: return "power_on";
        case ResetCause::BROWNOUT: return "brownout";
        case ResetCause::SOFTWARE: return "software";
        case ResetCause::WATCHDOG: return "watchdog";
        default:                   return "other";
    }
}
