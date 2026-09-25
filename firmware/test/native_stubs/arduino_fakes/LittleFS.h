#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <string>
#include <map>
#include <set>

// Minimal native fake for LittleFS.h - in-memory filesystem, scoped to
// exactly what event_log.cpp calls: begin(formatOnFail), exists(path),
// mkdir(path), open(path, mode[, create]), remove(path), rename(from,to),
// plus File's printf/print/read/size/close and its bool-conversion (used
// as `if (!f) return;`). Not a faithful LittleFS reimplementation -
// content just lives in a std::map<path, std::string> for the lifetime of
// the test process, which is all event_log.cpp's own tests need (nothing
// here tests real flash/power-loss/rotation-at-real-size behavior).
class File {
public:
    File() = default;
    File(std::string *content, bool append)
        : content_(content), valid_(content != nullptr), pos_(append && content ? content->size() : 0) {}

    explicit operator bool() const { return valid_; }

    size_t size() const { return content_ ? content_->size() : 0; }

    void printf(const char *fmt, ...) {
        if (!content_) return;
        char buf[512];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        content_->append(buf);
    }

    void print(char c) { if (content_) content_->push_back(c); }
    void print(const char *s) { if (content_) content_->append(s); }

    size_t read(uint8_t *buf, size_t len) {
        if (!content_) return 0;
        size_t avail = content_->size() > pos_ ? content_->size() - pos_ : 0;
        size_t n = len < avail ? len : avail;
        memcpy(buf, content_->data() + pos_, n);
        pos_ += n;
        return n;
    }

    void close() { valid_ = false; content_ = nullptr; }

private:
    std::string *content_ = nullptr;
    bool valid_ = false;
    size_t pos_ = 0;
};

class LittleFSClass {
public:
    bool begin(bool formatOnFail = false) {
        (void)formatOnFail;
        mounted_ = true;
        return true;
    }

    bool exists(const char *path) {
        return dirs_.count(path) > 0 || files_.count(path) > 0;
    }

    bool mkdir(const char *path) {
        dirs_.insert(path);
        return true;
    }

    // event_log.cpp's two call shapes: open(path, "a", true) to
    // append-or-create, open(path, "r") to read an existing file.
    File open(const char *path, const char *mode, bool create = false) {
        std::string m(mode);
        if (m == "a" || m == "w") {
            if (files_.find(path) == files_.end()) {
                if (!create && m == "w") return File();
                files_[path] = "";
            }
            return File(&files_[path], /*append=*/m == "a");
        }
        auto it = files_.find(path);
        if (it == files_.end()) return File();
        return File(&it->second, false);
    }

    bool remove(const char *path) {
        files_.erase(path);
        return true;
    }

    bool rename(const char *from, const char *to) {
        auto it = files_.find(from);
        if (it == files_.end()) return false;
        files_[to] = it->second;
        files_.erase(it);
        return true;
    }

private:
    bool mounted_ = false;
    std::map<std::string, std::string> files_;
    std::set<std::string> dirs_;
};

// One shared instance, same as the real Arduino core's global `LittleFS`
// object - `inline` (C++17) so this header can be included from multiple
// translation units in the same test binary without a multiple-definition
// link error.
inline LittleFSClass LittleFS;
