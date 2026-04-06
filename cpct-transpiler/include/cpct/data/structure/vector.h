// cpct/data/vector.h
// cpct::Vector<T> — wraps std::vector with C%-compatible methods.
// Supports: push(), pop(), insert(), erase(), clear(), len(), is_empty(),
//           front(), back(), negative indexing, slicing, sort(), sortkey().
#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <functional>
#include <cstdint>

namespace cpct {

template<typename T>
class Vector {
public:
    using native_type = std::vector<T>;

    // Constructors
    Vector() = default;
    Vector(std::initializer_list<T> init) : data_(init) {}
    Vector(const native_type& v) : data_(v) {}
    Vector(native_type&& v) noexcept : data_(std::move(v)) {}

    // C% built-in methods
    std::string type() const { return "vector"; }
    std::size_t size() const { return data_.size() * sizeof(T); }
    std::size_t len() const { return data_.size(); }
    bool is_empty() const { return data_.empty(); }

    T front() const {
        if (data_.empty()) throw std::out_of_range("front() on empty vector");
        return data_.front();
    }
    T back() const {
        if (data_.empty()) throw std::out_of_range("back() on empty vector");
        return data_.back();
    }

    // push(val) — append to end
    void push(const T& val) { data_.push_back(val); }

    // pop() — remove last and return
    T pop() {
        if (data_.empty()) throw std::out_of_range("pop() on empty vector");
        T val = std::move(data_.back());
        data_.pop_back();
        return val;
    }

    // insert(index, val)
    void insert(int64_t idx, const T& val) {
        auto pos = resolveInsertIndex(idx);
        data_.insert(data_.begin() + static_cast<std::ptrdiff_t>(pos), val);
    }

    // erase(index)
    void erase(int64_t idx) {
        auto pos = resolveIndex(idx);
        data_.erase(data_.begin() + static_cast<std::ptrdiff_t>(pos));
    }

    // clear()
    void clear() { data_.clear(); }

    // remove(val) — remove first occurrence
    void remove(const T& val) {
        auto it = std::find(data_.begin(), data_.end(), val);
        if (it != data_.end()) data_.erase(it);
    }

    // sort(descending = false)
    void sort(bool descending = false) {
        if (descending) std::sort(data_.begin(), data_.end(), std::greater<T>());
        else            std::sort(data_.begin(), data_.end());
    }

    // sort(comparator)
    template<typename Comp>
    void sort(Comp comp) { std::sort(data_.begin(), data_.end(), comp); }

    // reverse()
    void reverse() { std::reverse(data_.begin(), data_.end()); }

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

    // Negative indexing
    T& operator[](int64_t idx) {
        return data_[resolveIndex(idx)];
    }
    const T& operator[](int64_t idx) const {
        return data_[resolveIndex(idx)];
    }

    // Slice: v[start:end]
    Vector slice(int64_t start, int64_t end) const {
        int64_t n = static_cast<int64_t>(data_.size());
        if (start < 0) start += n;
        if (end < 0) end += n;
        if (start < 0) start = 0;
        if (end > n) end = n;
        if (start >= end) return Vector();

        return Vector(native_type(
            data_.begin() + start,
            data_.begin() + end
        ));
    }

    // Iterators
    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }

    // Access underlying std::vector
    native_type& native() { return data_; }
    const native_type& native() const { return data_; }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const Vector& v) {
        os << "[";
        for (std::size_t i = 0; i < v.data_.size(); i++) {
            if (i > 0) os << ", ";
            os << v.data_[i];
        }
        return os << "]";
    }

private:
    native_type data_;

    std::size_t resolveIndex(int64_t idx) const {
        int64_t n = static_cast<int64_t>(data_.size());
        if (idx < 0) idx += n;
        if (idx < 0 || idx >= n) {
            throw std::out_of_range("Vector index " + std::to_string(idx) +
                " out of range [0, " + std::to_string(data_.size()) + ")");
        }
        return static_cast<std::size_t>(idx);
    }

    std::size_t resolveInsertIndex(int64_t idx) const {
        int64_t n = static_cast<int64_t>(data_.size());
        if (idx < 0) idx += n;
        if (idx < 0 || idx > n) {
            throw std::out_of_range("Vector insert index out of range");
        }
        return static_cast<std::size_t>(idx);
    }
};

} // namespace cpct
