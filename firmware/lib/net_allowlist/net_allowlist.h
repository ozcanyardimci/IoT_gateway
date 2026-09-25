#pragma once

// NOT a real firewall - said plainly, not glossed over. ESP32 Arduino
// has no packet-filter/netfilter framework to wrap: doing real
// interface-level packet filtering needs raw access to ESP-IDF's lwIP
// internals, which `framework = arduino` doesn't expose without real
// IDF-level integration work - the same class of gap wireguard_link's
// build integration already has (see its own header). This project
// hasn't done that work, so a real firewall isn't buildable here right
// now.
//
// What this actually is: an application-layer outbound-connection
// allowlist - a pure function the device's own connection code (MQTT,
// NTP, LTE data session, etc.) can consult before dialing out, so a
// config bug or a tampered setting can't make this device start
// talking to arbitrary hosts. It does nothing to inbound traffic,
// nothing at the packet/interface level, and provides no protection
// against any code path that doesn't call it. Deliberately named
// NetAllowlist rather than "Firewall" for that reason - the roadmap's
// item is called "firewall", this is what's honestly achievable toward
// it without a real IDF-level rewrite.
//
// Pure logic, no hardware/Arduino dependency - gcc-unit-testable the
// same way as F1's protocol modules (see firmware/test/test_net_allowlist).
class NetAllowlist {
public:
    static const int MAX_ENTRIES = 8;
    static const int MAX_HOST_LEN = 64;

    // Clears the list.
    void clear();

    // Adds a host to the allowlist (exact string match, case-sensitive
    // - caller's job to normalize case/trailing dots/etc consistently).
    // Returns false if the list is already full, or host is
    // null/empty/too long for MAX_HOST_LEN.
    bool add(const char *host);

    // True if host exactly matches an entry added via add(). Fail-
    // closed: false for a null/empty host, and false for ANY host when
    // the list itself is empty (an empty allowlist permits nothing, not
    // everything - the safer default for something a config bug could
    // leave unpopulated).
    bool isAllowed(const char *host) const;

    int count() const { return count_; }

private:
    char entries_[MAX_ENTRIES][MAX_HOST_LEN];
    int count_ = 0;
};
