// Native-test fake for SystemWatchdog - modbus_master.cpp links against
// the real class interface (system_watchdog.h) but this test always
// passes watchdog=nullptr to start(), so addCurrentTask()/feed() are
// declared here only to satisfy the linker for code paths the compiler
// still emits (it can't know at compile time the pointer is always
// null) - they are never actually called at runtime by this test. Not
// a functional stand-in for the real ESP-IDF TWDT wrapper, and not
// meant to be - system_watchdog.cpp's own real behavior is untested
// natively for that reason (see docs/roadmap.md's test-coverage notes).
#include "system_watchdog.h"

bool SystemWatchdog::begin(uint32_t timeout_ms) { (void)timeout_ms; return true; }
bool SystemWatchdog::addCurrentTask() { return true; }
void SystemWatchdog::feed() {}
