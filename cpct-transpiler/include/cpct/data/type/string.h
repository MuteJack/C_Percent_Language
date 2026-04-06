// cpct/string.h
// cpct::String — wraps std::string with C%-compatible methods.
// Provides: len(), type(), size(), upper(), lower(), trim(), find(), replace(),
//           split(), substr(), reverse(), is_contains(), is_starts_with(), etc.
#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace cpct {

class String {
public:
    using native_type = std::string;

    // Constructors
    String() = default;
    String(const std::string& s) : val_(s) {}
    String(std::string&& s) noexcept : val_(std::move(s)) {}
    String(const char* s) : val_(s) {}
    String(char c) : val_(1, c) {}

    // Implicit conversion
    operator const std::string&() const { return val_; }
    operator std::string&() { return val_; }

    // C% built-in methods
    std::string type() const { return "string"; }
    std::size_t size() const { return val_.size(); }
    std::size_t len() const { return val_.size(); }

    // String methods
    String upper() const {
        std::string r = val_;
        std::transform(r.begin(), r.end(), r.begin(), ::toupper);
        return r;
    }

    String lower() const {
        std::string r = val_;
        std::transform(r.begin(), r.end(), r.begin(), ::tolower);
        return r;
    }

    String trim() const {
        auto start = val_.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return String("");
        auto end = val_.find_last_not_of(" \t\n\r");
        return String(val_.substr(start, end - start + 1));
    }

    int64_t find(const String& sub) const {
        auto pos = val_.find(sub.val_);
        return (pos == std::string::npos) ? -1 : static_cast<int64_t>(pos);
    }

    String replace(const String& old_s, const String& new_s) const {
        std::string r = val_;
        size_t pos = 0;
        while ((pos = r.find(old_s.val_, pos)) != std::string::npos) {
            r.replace(pos, old_s.val_.size(), new_s.val_);
            pos += new_s.val_.size();
        }
        return r;
    }

    String substr(int64_t start, int64_t length) const {
        if (start < 0) start += static_cast<int64_t>(val_.size());
        if (start < 0) start = 0;
        return String(val_.substr(static_cast<size_t>(start), static_cast<size_t>(length)));
    }

    String reverse() const {
        std::string r = val_;
        std::reverse(r.begin(), r.end());
        return r;
    }

    std::vector<String> split(const String& sep) const {
        std::vector<String> result;
        size_t start = 0, end;
        while ((end = val_.find(sep.val_, start)) != std::string::npos) {
            result.push_back(String(val_.substr(start, end - start)));
            start = end + sep.val_.size();
        }
        result.push_back(String(val_.substr(start)));
        return result;
    }

    // Boolean checks
    bool is_contains(const String& sub) const { return val_.find(sub.val_) != std::string::npos; }
    bool is_starts_with(const String& prefix) const { return val_.rfind(prefix.val_, 0) == 0; }
    bool is_ends_with(const String& suffix) const {
        if (suffix.val_.size() > val_.size()) return false;
        return val_.compare(val_.size() - suffix.val_.size(), suffix.val_.size(), suffix.val_) == 0;
    }
    bool is_empty() const { return val_.empty(); }

    bool is_digit() const {
        if (val_.empty()) return false;
        return std::all_of(val_.begin(), val_.end(), ::isdigit);
    }
    bool is_alpha() const {
        if (val_.empty()) return false;
        return std::all_of(val_.begin(), val_.end(), ::isalpha);
    }
    bool is_upper() const {
        if (val_.empty()) return false;
        return std::all_of(val_.begin(), val_.end(), ::isupper);
    }
    bool is_lower() const {
        if (val_.empty()) return false;
        return std::all_of(val_.begin(), val_.end(), ::islower);
    }

    // Element access
    char operator[](int64_t idx) const {
        if (idx < 0) idx += static_cast<int64_t>(val_.size());
        return val_[static_cast<size_t>(idx)];
    }

    // Concatenation
    String operator+(const String& rhs) const { return String(val_ + rhs.val_); }
    String& operator+=(const String& rhs) { val_ += rhs.val_; return *this; }

    // Comparison
    bool operator==(const String& rhs) const { return val_ == rhs.val_; }
    bool operator!=(const String& rhs) const { return val_ != rhs.val_; }
    bool operator<(const String& rhs) const { return val_ < rhs.val_; }
    bool operator>(const String& rhs) const { return val_ > rhs.val_; }
    bool operator<=(const String& rhs) const { return val_ <= rhs.val_; }
    bool operator>=(const String& rhs) const { return val_ >= rhs.val_; }

    // Truthy: non-empty = true (C% rule)
    explicit operator bool() const { return !val_.empty(); }

    // Stream
    friend std::ostream& operator<<(std::ostream& os, const String& s) {
        return os << s.val_;
    }
    friend std::istream& operator>>(std::istream& is, String& s) {
        return is >> s.val_;
    }

    // Access underlying std::string
    const std::string& str() const { return val_; }
    std::string& str() { return val_; }

private:
    std::string val_;
};

// Free function: join
inline String join(const String& sep, const std::vector<String>& parts) {
    std::string result;
    for (size_t i = 0; i < parts.size(); i++) {
        if (i > 0) result += sep.str();
        result += parts[i].str();
    }
    return String(result);
}

} // namespace cpct
