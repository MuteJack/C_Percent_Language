#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include <iostream>

// BigInt: arbitrary precision integer using base 10^9 digits
// Small values (fits in int64_t) use a fast path
class BigInt {
public:
    static constexpr int64_t BASE = 1000000000LL; // 10^9
    static constexpr size_t MAX_DIGITS = 10000;   // max number of base-10^9 digits (~90,000 decimal digits)

    BigInt() : negative_(false), small_(true), smallVal_(0) {}
    BigInt(int64_t val);
    BigInt(const std::string& str);

    // Copy/move
    BigInt(const BigInt&) = default;
    BigInt(BigInt&&) = default;
    BigInt& operator=(const BigInt&) = default;
    BigInt& operator=(BigInt&&) = default;

    // Arithmetic
    BigInt operator+(const BigInt& rhs) const;
    BigInt operator-(const BigInt& rhs) const;
    BigInt operator*(const BigInt& rhs) const;
    BigInt operator/(const BigInt& rhs) const;
    BigInt operator%(const BigInt& rhs) const;
    BigInt operator-() const;

    // Comparison
    bool operator==(const BigInt& rhs) const;
    bool operator!=(const BigInt& rhs) const;
    bool operator<(const BigInt& rhs) const;
    bool operator>(const BigInt& rhs) const;
    bool operator<=(const BigInt& rhs) const;
    bool operator>=(const BigInt& rhs) const;

    // Conversion
    std::string toString() const;
    bool fitsInt64() const;
    int64_t toInt64() const; // throws if doesn't fit
    double toDouble() const;
    bool isZero() const;
    bool isNegative() const { return negative_; }
    bool isSmall() const { return small_; }
    bool toBool() const { return !isZero(); }
    size_t byteCount() const { return small_ ? 8 : digits_.size() * 4; }

private:
    bool negative_;
    bool small_;        // true = use smallVal_, false = use digits_
    int64_t smallVal_;  // used when small_ == true (stores absolute value concept handled by negative_)
    // Actually for simplicity, smallVal_ stores the signed value directly when small_
    std::vector<uint32_t> digits_; // base 10^9 digits, least significant first

    // Internal helpers
    void normalize();
    void promote(); // convert from small to big representation
    static BigInt addMagnitude(const BigInt& a, const BigInt& b);
    static BigInt subMagnitude(const BigInt& a, const BigInt& b); // |a| >= |b|
    static int compareMagnitude(const BigInt& a, const BigInt& b);
    void checkSize() const;

    // For big representation: create from digits
    BigInt(bool neg, std::vector<uint32_t> digits)
        : negative_(neg), small_(false), smallVal_(0), digits_(std::move(digits)) {
        normalize();
    }
};
