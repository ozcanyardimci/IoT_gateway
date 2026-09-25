#pragma once
#include <stddef.h>
#include <stdint.h>

class UpdateClass {
public:
    UpdateClass() {}

    bool begin(size_t size) {
        fake_begin_called_ = true;
        fake_size_ = size;
        return fake_begin_return_;
    }

    size_t write(uint8_t *data, size_t len) {
        fake_write_called_ = true;
        fake_bytes_written_ += len;
        return fake_write_return_sz_ != (size_t)-1 ? fake_write_return_sz_ : len;
    }

    bool end(bool evenIfRemaining = false) {
        fake_end_called_ = true;
        fake_end_arg_ = evenIfRemaining;
        return fake_end_return_;
    }

    void abort() {
        fake_abort_called_ = true;
    }

    size_t progress() const {
        return fake_bytes_written_;
    }

    size_t size() const {
        return fake_size_;
    }

    // --- Fake instrumentation ---
    bool fake_begin_called_ = false;
    size_t fake_size_ = 0;
    bool fake_begin_return_ = true;

    bool fake_write_called_ = false;
    size_t fake_bytes_written_ = 0;
    size_t fake_write_return_sz_ = (size_t)-1; // -1 means return `len`

    bool fake_end_called_ = false;
    bool fake_end_arg_ = false;
    bool fake_end_return_ = true;

    bool fake_abort_called_ = false;

    void fake_reset() {
        fake_begin_called_ = false;
        fake_size_ = 0;
        fake_begin_return_ = true;
        
        fake_write_called_ = false;
        fake_bytes_written_ = 0;
        fake_write_return_sz_ = (size_t)-1;

        fake_end_called_ = false;
        fake_end_arg_ = false;
        fake_end_return_ = true;

        fake_abort_called_ = false;
    }
};

extern UpdateClass Update;
