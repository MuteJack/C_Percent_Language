// bigint.cpp
// BigInt arbitrary-precision integer implementation.
// Components: construction (int64/uint64/string), normalization (normalize), magnitude comparison (compareMagnitude),
//             arithmetic (+, -, *, / schoolbook long division, %), comparison operators.
// When small_ flag is true, uses fast int64_t arithmetic.
// Overflow is detected via __builtin_*_overflow and triggers promotion to big representation.
#include "bigint.h"
#include <sstream>
#include <cassert>
#include <cmath>

// ============== Construction ==============

BigInt::BigInt(int64_t val) : negative_(val < 0), small_(true), smallVal_(val) {}

BigInt::BigInt(uint64_t val) : negative_(false), small_(false), smallVal_(0) {
    if (val <= static_cast<uint64_t>(INT64_MAX)) {
        small_ = true;
        smallVal_ = static_cast<int64_t>(val);
    } else {
        // Value exceeds int64_t range — use string constructor path
        *this = BigInt(std::to_string(val));
    }
}

BigInt::BigInt(const std::string& str) : negative_(false), small_(false), smallVal_(0) {
    if (str.empty()) { small_ = true; smallVal_ = 0; return; }

    size_t start = 0;
    if (str[0] == '-') { negative_ = true; start = 1; }
    else if (str[0] == '+') { start = 1; }

    // Try to fit in int64_t first
    if (str.size() - start <= 18) {
        try {
            int64_t v = std::stoll(str);
            small_ = true;
            smallVal_ = v;
            negative_ = (v < 0);
            return;
        } catch (...) {}
    }

    // Parse into base-10^9 digits
    std::string numStr = str.substr(start);
    // Remove leading zeros
    size_t nzStart = numStr.find_first_not_of('0');
    if (nzStart == std::string::npos) {
        small_ = true; smallVal_ = 0; negative_ = false;
        return;
    }
    numStr = numStr.substr(nzStart);

    // Convert string to base-10^9 digits (least significant first)
    digits_.clear();
    int len = static_cast<int>(numStr.size());
    for (int i = len; i > 0; i -= 9) {
        int begin = std::max(0, i - 9);
        std::string chunk = numStr.substr(begin, i - begin);
        digits_.push_back(static_cast<uint32_t>(std::stoul(chunk)));
    }
    normalize();
    checkSize();
}

// ============== Helpers ==============

void BigInt::normalize() {
    if (small_) return;
    while (digits_.size() > 1 && digits_.back() == 0) {
        digits_.pop_back();
    }
    if (digits_.empty() || (digits_.size() == 1 && digits_[0] == 0)) {
        negative_ = false;
        small_ = true;
        smallVal_ = 0;
        digits_.clear();
        return;
    }
    // Try to demote to small if it fits in int64_t
    if (digits_.size() <= 3) { // max 3 base-10^9 digits = 27 decimal digits, int64 max is ~19
        // Check if it fits
        if (digits_.size() == 1) {
            int64_t v = static_cast<int64_t>(digits_[0]);
            small_ = true;
            smallVal_ = negative_ ? -v : v;
            digits_.clear();
        } else if (digits_.size() == 2) {
            int64_t v = static_cast<int64_t>(digits_[1]) * BASE + digits_[0];
            if (v >= 0 && ((!negative_ && v <= INT64_MAX) || (negative_ && v <= static_cast<uint64_t>(INT64_MAX) + 1))) {
                small_ = true;
                smallVal_ = negative_ ? -v : v;
                digits_.clear();
            }
        }
        // 3 digits could exceed int64 range, check carefully
        else if (digits_.size() == 3) {
            // Max int64 = 9223372036854775807 which is ~9.2 * 10^18
            // 3 base-10^9 digits can be up to ~10^27, way too big for int64
            // But small values like digits_[2] == 0 are already handled by normalize removing trailing zeros
            // Only demote if actually fits
            // Quick check: if top digit >= 10, definitely too big
            // Actually let's be precise
            if (digits_[2] == 0) {
                // Handled above (normalize strips trailing zeros)
            }
            // Otherwise too big for int64
        }
    }
}

void BigInt::promote() {
    if (!small_) return;
    int64_t v = smallVal_ < 0 ? -smallVal_ : smallVal_;
    negative_ = smallVal_ < 0;
    small_ = false;
    digits_.clear();
    if (v == 0) {
        digits_.push_back(0);
    } else {
        while (v > 0) {
            digits_.push_back(static_cast<uint32_t>(v % BASE));
            v /= BASE;
        }
    }
}

void BigInt::checkSize() const {
    if (!small_ && digits_.size() > MAX_DIGITS) {
        throw std::overflow_error("BigInt overflow: number exceeds maximum size limit (" +
                                  std::to_string(MAX_DIGITS * 9) + " decimal digits)");
    }
}

bool BigInt::isZero() const {
    if (small_) return smallVal_ == 0;
    return digits_.empty() || (digits_.size() == 1 && digits_[0] == 0);
}

bool BigInt::fitsInt64() const {
    return small_;
}

int64_t BigInt::toInt64() const {
    if (small_) return smallVal_;
    throw std::overflow_error("BigInt value too large for int64");
}

double BigInt::toDouble() const {
    if (small_) return static_cast<double>(smallVal_);
    double result = 0.0;
    double base = 1.0;
    for (size_t i = 0; i < digits_.size(); i++) {
        result += digits_[i] * base;
        base *= static_cast<double>(BASE);
    }
    return negative_ ? -result : result;
}

std::string BigInt::toString() const {
    if (small_) return std::to_string(smallVal_);

    std::string result;
    if (negative_) result = "-";
    // Most significant digit first (no leading zeros)
    result += std::to_string(digits_.back());
    // Remaining digits with zero-padding to 9 digits
    for (int i = static_cast<int>(digits_.size()) - 2; i >= 0; i--) {
        std::string chunk = std::to_string(digits_[i]);
        result += std::string(9 - chunk.size(), '0') + chunk;
    }
    return result;
}

// ============== Magnitude comparison ==============

int BigInt::compareMagnitude(const BigInt& a, const BigInt& b) {
    // Both should be in big form for this, or handle small
    if (a.small_ && b.small_) {
        int64_t av = a.smallVal_ < 0 ? -a.smallVal_ : a.smallVal_;
        int64_t bv = b.smallVal_ < 0 ? -b.smallVal_ : b.smallVal_;
        if (av < bv) return -1;
        if (av > bv) return 1;
        return 0;
    }
    // If one is small and other is big, the big one has more digits (since small demotes on normalize)
    // Actually not necessarily. Let's handle properly.
    // Create temporary promoted copies
    BigInt aa = a, bb = b;
    if (aa.small_) aa.promote();
    if (bb.small_) bb.promote();

    if (aa.digits_.size() != bb.digits_.size()) {
        return aa.digits_.size() < bb.digits_.size() ? -1 : 1;
    }
    for (int i = static_cast<int>(aa.digits_.size()) - 1; i >= 0; i--) {
        if (aa.digits_[i] < bb.digits_[i]) return -1;
        if (aa.digits_[i] > bb.digits_[i]) return 1;
    }
    return 0;
}

// ============== Magnitude arithmetic ==============

BigInt BigInt::addMagnitude(const BigInt& a, const BigInt& b) {
    BigInt aa = a, bb = b;
    if (aa.small_) aa.promote();
    if (bb.small_) bb.promote();

    size_t maxLen = std::max(aa.digits_.size(), bb.digits_.size());
    std::vector<uint32_t> result(maxLen + 1, 0);
    uint64_t carry = 0;
    for (size_t i = 0; i < maxLen || carry; i++) {
        uint64_t sum = carry;
        if (i < aa.digits_.size()) sum += aa.digits_[i];
        if (i < bb.digits_.size()) sum += bb.digits_[i];
        if (i < result.size()) {
            result[i] = static_cast<uint32_t>(sum % BASE);
        } else {
            result.push_back(static_cast<uint32_t>(sum % BASE));
        }
        carry = sum / BASE;
    }
    return BigInt(false, std::move(result));
}

BigInt BigInt::subMagnitude(const BigInt& a, const BigInt& b) {
    // Assumes |a| >= |b|
    BigInt aa = a, bb = b;
    if (aa.small_) aa.promote();
    if (bb.small_) bb.promote();

    std::vector<uint32_t> result(aa.digits_.size(), 0);
    int64_t borrow = 0;
    for (size_t i = 0; i < aa.digits_.size(); i++) {
        int64_t diff = static_cast<int64_t>(aa.digits_[i]) - borrow;
        if (i < bb.digits_.size()) diff -= bb.digits_[i];
        if (diff < 0) {
            diff += BASE;
            borrow = 1;
        } else {
            borrow = 0;
        }
        result[i] = static_cast<uint32_t>(diff);
    }
    return BigInt(false, std::move(result));
}

// ============== Arithmetic operators ==============

BigInt BigInt::operator+(const BigInt& rhs) const {
    // Fast path: both small, use __builtin_add_overflow
    if (small_ && rhs.small_) {
        int64_t result;
        if (!__builtin_add_overflow(smallVal_, rhs.smallVal_, &result)) {
            return BigInt(result);
        }
        // Overflow: fall through to big arithmetic
    }

    // Same sign: add magnitudes
    if (negative_ == rhs.negative_) {
        BigInt result = addMagnitude(*this, rhs);
        result.negative_ = negative_;
        result.normalize();
        result.checkSize();
        return result;
    }
    // Different signs: subtract magnitudes
    int cmp = compareMagnitude(*this, rhs);
    if (cmp == 0) return BigInt(int64_t(0));
    if (cmp > 0) {
        BigInt result = subMagnitude(*this, rhs);
        result.negative_ = negative_;
        result.normalize();
        return result;
    } else {
        BigInt result = subMagnitude(rhs, *this);
        result.negative_ = rhs.negative_;
        result.normalize();
        return result;
    }
}

BigInt BigInt::operator-(const BigInt& rhs) const {
    // Fast path: both small, use __builtin_sub_overflow
    if (small_ && rhs.small_) {
        int64_t result;
        if (!__builtin_sub_overflow(smallVal_, rhs.smallVal_, &result)) {
            return BigInt(result);
        }
        // Overflow: fall through to big arithmetic
    }

    BigInt negRhs = -rhs;
    return *this + negRhs;
}

BigInt BigInt::operator*(const BigInt& rhs) const {
    // Fast path
    if (small_ && rhs.small_) {
        int64_t result;
        if (!__builtin_mul_overflow(smallVal_, rhs.smallVal_, &result)) {
            return BigInt(result);
        }
    }

    if (isZero() || rhs.isZero()) return BigInt(int64_t(0));

    BigInt aa = *this, bb = rhs;
    if (aa.small_) aa.promote();
    if (bb.small_) bb.promote();

    size_t n = aa.digits_.size(), m = bb.digits_.size();
    std::vector<uint32_t> result(n + m, 0);
    for (size_t i = 0; i < n; i++) {
        uint64_t carry = 0;
        for (size_t j = 0; j < m || carry; j++) {
            uint64_t cur = static_cast<uint64_t>(result[i + j]) + carry;
            if (j < m) cur += static_cast<uint64_t>(aa.digits_[i]) * bb.digits_[j];
            result[i + j] = static_cast<uint32_t>(cur % BASE);
            carry = cur / BASE;
        }
    }

    BigInt res(negative_ != rhs.negative_, std::move(result));
    res.checkSize();
    return res;
}

BigInt BigInt::operator/(const BigInt& rhs) const {
    if (rhs.isZero()) {
        throw std::runtime_error("Division by zero");
    }

    // Fast path
    if (small_ && rhs.small_) {
        // int64 division doesn't overflow (except INT64_MIN / -1, handle that)
        if (smallVal_ == INT64_MIN && rhs.smallVal_ == -1) {
            // This would overflow int64, need big math
        } else {
            return BigInt(smallVal_ / rhs.smallVal_);
        }
    }

    // Simple long division
    // For now, use a simple approach: convert to positive, do division
    BigInt aa = *this, bb = rhs;
    bool resultNeg = (negative_ != rhs.negative_);
    aa.negative_ = false;
    bb.negative_ = false;

    if (aa.small_) aa.promote();
    if (bb.small_) bb.promote();

    // If dividend < divisor, result is 0
    if (compareMagnitude(aa, bb) < 0) return BigInt(int64_t(0));

    // Simple digit-by-digit division (schoolbook)
    // We'll work with the big representation
    std::vector<uint32_t> quotient;
    BigInt remainder(int64_t(0));
    remainder.promote();

    for (int i = static_cast<int>(aa.digits_.size()) - 1; i >= 0; i--) {
        // remainder = remainder * BASE + aa.digits_[i]
        if (!remainder.isZero()) {
            if (remainder.small_) remainder.promote();
            // Shift left by one base digit
            remainder.digits_.insert(remainder.digits_.begin(), aa.digits_[i]);
        } else {
            remainder = BigInt(static_cast<int64_t>(aa.digits_[i]));
            remainder.promote();
        }
        remainder.normalize();

        // Binary search for quotient digit
        uint32_t lo = 0, hi = static_cast<uint32_t>(BASE - 1);
        while (lo < hi) {
            uint32_t mid = lo + (hi - lo + 1) / 2;
            BigInt test = bb * BigInt(static_cast<int64_t>(mid));
            if (compareMagnitude(test, remainder) <= 0) {
                lo = mid;
            } else {
                hi = mid - 1;
            }
        }
        quotient.push_back(lo);
        if (lo > 0) {
            BigInt sub = bb * BigInt(static_cast<int64_t>(lo));
            remainder = remainder - sub;
        }
    }

    // Reverse quotient (we built it most-significant first)
    std::reverse(quotient.begin(), quotient.end());
    BigInt result(resultNeg, std::move(quotient));
    return result;
}

BigInt BigInt::operator%(const BigInt& rhs) const {
    if (rhs.isZero()) {
        throw std::runtime_error("Modulo by zero");
    }

    // Fast path
    if (small_ && rhs.small_) {
        return BigInt(smallVal_ % rhs.smallVal_);
    }

    // a % b = a - (a / b) * b
    BigInt q = *this / rhs;
    return *this - q * rhs;
}

BigInt BigInt::operator-() const {
    if (small_) {
        if (smallVal_ == INT64_MIN) {
            // -INT64_MIN overflows int64, need big
            BigInt result;
            result.small_ = false;
            result.negative_ = false;
            // INT64_MIN = -9223372036854775808
            // |INT64_MIN| = 9223372036854775808
            // In base 10^9: 9223372036 * 10^9 + 854775808
            result.digits_ = {854775808u, 223372036u, 9u};
            return result;
        }
        return BigInt(-smallVal_);
    }
    BigInt result = *this;
    if (!result.isZero()) result.negative_ = !result.negative_;
    return result;
}

// ============== Comparison operators ==============

bool BigInt::operator==(const BigInt& rhs) const {
    if (small_ && rhs.small_) return smallVal_ == rhs.smallVal_;
    if (negative_ != rhs.negative_) return false;
    return compareMagnitude(*this, rhs) == 0;
}

bool BigInt::operator!=(const BigInt& rhs) const { return !(*this == rhs); }

bool BigInt::operator<(const BigInt& rhs) const {
    if (small_ && rhs.small_) return smallVal_ < rhs.smallVal_;
    if (negative_ && !rhs.negative_) return true;
    if (!negative_ && rhs.negative_) return false;
    int cmp = compareMagnitude(*this, rhs);
    return negative_ ? (cmp > 0) : (cmp < 0);
}

bool BigInt::operator>(const BigInt& rhs) const { return rhs < *this; }
bool BigInt::operator<=(const BigInt& rhs) const { return !(rhs < *this); }
bool BigInt::operator>=(const BigInt& rhs) const { return !(*this < rhs); }

// ============== Bitwise operations ==============

// Helper: convert BigInt magnitude to binary chunks (base 2^32, little-endian)
static std::vector<uint32_t> toBinaryChunks(BigInt val) {
    if (val.isNegative()) val = -val;
    if (val.isZero()) return {0};

    BigInt base(uint64_t(0x100000000ULL)); // 2^32
    std::vector<uint32_t> chunks;
    while (!val.isZero()) {
        BigInt rem = val % base;
        chunks.push_back(static_cast<uint32_t>(rem.toInt64()));
        val = val / base;
    }
    return chunks;
}

// Helper: convert magnitude to two's complement binary representation
static std::vector<uint32_t> toTwosComp(const BigInt& val) {
    if (val.isZero()) return {0};

    auto chunks = toBinaryChunks(val);

    if (!val.isNegative()) {
        // Positive: ensure sign bit is 0
        if (chunks.back() & 0x80000000u)
            chunks.push_back(0);
    } else {
        // Negative: flip bits and add 1
        for (auto& c : chunks) c = ~c;
        uint64_t carry = 1;
        for (auto& c : chunks) {
            uint64_t sum = static_cast<uint64_t>(c) + carry;
            c = static_cast<uint32_t>(sum);
            carry = sum >> 32;
        }
        // Ensure sign bit is 1
        if (!(chunks.back() & 0x80000000u))
            chunks.push_back(0xFFFFFFFFu);
    }
    return chunks;
}

// Helper: convert two's complement chunks back to BigInt
static BigInt fromTwosComp(const std::vector<uint32_t>& chunks) {
    if (chunks.empty()) return BigInt(int64_t(0));

    bool negative = (chunks.back() & 0x80000000u) != 0;
    std::vector<uint32_t> mag = chunks;

    if (negative) {
        // Two's complement → magnitude: flip bits, add 1
        for (auto& c : mag) c = ~c;
        uint64_t carry = 1;
        for (auto& c : mag) {
            uint64_t sum = static_cast<uint64_t>(c) + carry;
            c = static_cast<uint32_t>(sum);
            carry = sum >> 32;
        }
    }

    // Remove trailing zeros
    while (mag.size() > 1 && mag.back() == 0) mag.pop_back();

    // Convert binary chunks (base 2^32) to BigInt
    BigInt result(int64_t(0));
    BigInt base(uint64_t(0x100000000ULL));
    BigInt multiplier(int64_t(1));

    for (size_t i = 0; i < mag.size(); i++) {
        result = result + BigInt(uint64_t(mag[i])) * multiplier;
        if (i + 1 < mag.size()) multiplier = multiplier * base;
    }

    if (negative) result = -result;
    return result;
}

// Bitwise AND
BigInt BigInt::operator&(const BigInt& rhs) const {
    // Fast path: both small
    if (small_ && rhs.small_)
        return BigInt(smallVal_ & rhs.smallVal_);

    auto a = toTwosComp(*this);
    auto b = toTwosComp(rhs);

    // Sign-extend to same length
    uint32_t aExt = (a.back() & 0x80000000u) ? 0xFFFFFFFFu : 0;
    uint32_t bExt = (b.back() & 0x80000000u) ? 0xFFFFFFFFu : 0;
    while (a.size() < b.size()) a.push_back(aExt);
    while (b.size() < a.size()) b.push_back(bExt);

    std::vector<uint32_t> result(a.size());
    for (size_t i = 0; i < a.size(); i++)
        result[i] = a[i] & b[i];

    return fromTwosComp(result);
}

// Bitwise OR
BigInt BigInt::operator|(const BigInt& rhs) const {
    if (small_ && rhs.small_)
        return BigInt(smallVal_ | rhs.smallVal_);

    auto a = toTwosComp(*this);
    auto b = toTwosComp(rhs);

    uint32_t aExt = (a.back() & 0x80000000u) ? 0xFFFFFFFFu : 0;
    uint32_t bExt = (b.back() & 0x80000000u) ? 0xFFFFFFFFu : 0;
    while (a.size() < b.size()) a.push_back(aExt);
    while (b.size() < a.size()) b.push_back(bExt);

    std::vector<uint32_t> result(a.size());
    for (size_t i = 0; i < a.size(); i++)
        result[i] = a[i] | b[i];

    return fromTwosComp(result);
}

// Bitwise XOR
BigInt BigInt::operator^(const BigInt& rhs) const {
    if (small_ && rhs.small_)
        return BigInt(smallVal_ ^ rhs.smallVal_);

    auto a = toTwosComp(*this);
    auto b = toTwosComp(rhs);

    uint32_t aExt = (a.back() & 0x80000000u) ? 0xFFFFFFFFu : 0;
    uint32_t bExt = (b.back() & 0x80000000u) ? 0xFFFFFFFFu : 0;
    while (a.size() < b.size()) a.push_back(aExt);
    while (b.size() < a.size()) b.push_back(bExt);

    std::vector<uint32_t> result(a.size());
    for (size_t i = 0; i < a.size(); i++)
        result[i] = a[i] ^ b[i];

    return fromTwosComp(result);
}

// Bitwise NOT: ~x = -(x + 1)
BigInt BigInt::operator~() const {
    return -(*this + BigInt(int64_t(1)));
}

// Left shift: x << n = x * 2^n
BigInt BigInt::operator<<(int64_t n) const {
    if (n == 0 || isZero()) return *this;
    if (n < 0) return *this >> (-n);

    // Build 2^n by repeated squaring
    BigInt power(int64_t(1));
    BigInt base(int64_t(2));
    int64_t exp = n;
    while (exp > 0) {
        if (exp & 1) power = power * base;
        base = base * base;
        exp >>= 1;
    }
    return *this * power;
}

// Right shift: x >> n = floor(x / 2^n)
BigInt BigInt::operator>>(int64_t n) const {
    if (n == 0 || isZero()) return *this;
    if (n < 0) return *this << (-n);

    // Build 2^n
    BigInt power(int64_t(1));
    BigInt base(int64_t(2));
    int64_t exp = n;
    while (exp > 0) {
        if (exp & 1) power = power * base;
        base = base * base;
        exp >>= 1;
    }

    BigInt result = *this / power;
    // Floor division: if negative and remainder != 0, subtract 1
    if (negative_ && !(*this % power).isZero())
        result = result - BigInt(int64_t(1));
    return result;
}
