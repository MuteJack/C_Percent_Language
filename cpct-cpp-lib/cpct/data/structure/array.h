// cpct/data/array.h
// cpct::Array<T, N> — wraps std::array with C%-compatible methods.
// Supports: len(), type(), size(), negative indexing, slicing, front(), back().
#pragma once
#include <array>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <functional>
#include <cstdint>

namespace cpct {

template<typename T, std::size_t N>
class Array {
public:
    using native_type = std::array<T, N>;

    // Constructors
    Array() : data_{} {}
    Array(std::initializer_list<T> init) {
        std::size_t i = 0;
        for (auto& v : init) {
            if (i >= N) break;
            data_[i++] = v;
        }
    }
    Array(const native_type& arr) : data_(arr) {}

    // C% built-in methods
    std::string type() const {
        return "array"; // simplified; full would be "int[]" etc.
    }
    std::size_t size() const { return N * sizeof(T); }
    std::size_t len() const { return N; }
    bool is_empty() const { return N == 0; }

    // shape() — returns dimensions as vector
    std::vector<std::size_t> shape() const { return { N }; }

    T front() const { return data_[0]; }
    T back() const { return data_[N - 1]; }

    // Negative indexing
    T& operator[](int64_t idx) {
        return data_[resolveIndex(idx)];
    }
    const T& operator[](int64_t idx) const {
        return data_[resolveIndex(idx)];
    }

    // Slice: arr[start:end] → returns vector (size not known at compile time)
    std::vector<T> slice(int64_t start, int64_t end) const {
        int64_t n = static_cast<int64_t>(N);
        if (start < 0) start += n;
        if (end < 0) end += n;
        if (start < 0) start = 0;
        if (end > n) end = n;
        if (start >= end) return {};

        std::vector<T> result;
        for (int64_t i = start; i < end; i++) {
            result.push_back(data_[static_cast<std::size_t>(i)]);
        }
        return result;
    }

    // Iterators (for range-based for)
    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }

    // sort(descending = false)
    void sort(bool descending = false) {
        if (descending) std::sort(data_.begin(), data_.end(), std::greater<T>());
        else            std::sort(data_.begin(), data_.end());
    }

    // reverse()
    void reverse() {
        std::reverse(data_.begin(), data_.end());
    }

    // find(val) — returns index of first occurrence, -1 if not found
    int64_t find(const T& val) const {
        auto it = std::find(data_.begin(), data_.end(), val);
        if (it == data_.end()) return -1;
        return static_cast<int64_t>(it - data_.begin());
    }

    // is_contains(val)
    bool is_contains(const T& val) const {
        return std::find(data_.begin(), data_.end(), val) != data_.end();
    }

    // Access underlying std::array
    native_type& native() { return data_; }
    const native_type& native() const { return data_; }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const Array& arr) {
        os << "[";
        for (std::size_t i = 0; i < N; i++) {
            if (i > 0) os << ", ";
            os << arr.data_[i];
        }
        return os << "]";
    }

private:
    native_type data_;

    std::size_t resolveIndex(int64_t idx) const {
        int64_t n = static_cast<int64_t>(N);
        if (idx < 0) idx += n;
        if (idx < 0 || idx >= n) {
            throw std::out_of_range("Array index " + std::to_string(idx) +
                " out of range [0, " + std::to_string(N) + ")");
        }
        return static_cast<std::size_t>(idx);
    }
};

} // namespace cpct
