#include "ota_version_check.h"

bool isOtaVersionAcceptable(uint32_t declared_version, uint32_t current_version) {
    return declared_version > current_version;
}
