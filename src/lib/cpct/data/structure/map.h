// cpct/data/map.h
// cpct::Map<K, V> — typed key-value map (insertion-order preserved).
// Hash-based with collision handling via chaining.
// Supports: has(), keys(), values(), remove(), len(), size(), is_empty(),
//           sortkey(), sortval(), sortvk().
// Type coercion: values are static_cast'd to V on insertion.
#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <iostream>
#include <functional>
#include <algorithm>
#include <numeric>
#include <type_traits>

namespace cpct {

template<typename K, typename V>
class Map {
public:
    Map() = default;
    Map(std::initializer_list<std::pair<K, V>> init) {
        for (auto& [k, v] : init) {
            set(k, v);
        }
    }

    // C% built-in methods
    std::string type() const { return "map"; }
    std::size_t len() const { return keys_.size(); }
    bool is_empty() const { return keys_.empty(); }

    // size() — sum of key bytes + value bytes
    std::size_t size() const {
        return keys_.size() * (sizeOfKey() + sizeof(V));
    }

    // has(key)
    bool has(const K& key) const {
        return findIndex(key) != NOT_FOUND;
    }

    // keys() — returns keys in insertion order
    std::vector<K> keys() const { return keys_; }

    // values() — returns values in insertion order
    std::vector<V> values() const { return values_; }

    // set(key, val) — with type coercion
    template<typename U>
    void set(const K& key, const U& val) {
        V coerced = coerce(val);
        std::size_t idx = findIndex(key);
        if (idx != NOT_FOUND) {
            values_[idx] = coerced;
        } else {
            auto hash = makeHash(key);
            index_[hash].push_back(keys_.size());
            keys_.push_back(key);
            values_.push_back(coerced);
        }
    }

    V& operator[](const K& key) {
        std::size_t idx = findIndex(key);
        if (idx == NOT_FOUND) {
            auto hash = makeHash(key);
            index_[hash].push_back(keys_.size());
            keys_.push_back(key);
            values_.push_back(V{});
            return values_.back();
        }
        return values_[idx];
    }

    const V& operator[](const K& key) const {
        std::size_t idx = findIndex(key);
        if (idx == NOT_FOUND) {
            throw std::runtime_error("Key not found in map");
        }
        return values_[idx];
    }

    // get(key) — throws if not found
    V& get(const K& key) {
        std::size_t idx = findIndex(key);
        if (idx == NOT_FOUND) {
            throw std::runtime_error("Key not found in map");
        }
        return values_[idx];
    }

    const V& get(const K& key) const {
        std::size_t idx = findIndex(key);
        if (idx == NOT_FOUND) {
            throw std::runtime_error("Key not found in map");
        }
        return values_[idx];
    }

    // remove(key) — preserves insertion order (shift, not swap)
    void remove(const K& key) {
        std::size_t idx = findIndex(key);
        if (idx == NOT_FOUND) return;

        keys_.erase(keys_.begin() + static_cast<std::ptrdiff_t>(idx));
        values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(idx));
        rebuildIndex();
    }

    // sortkey(descending = false)
    void sortkey(bool descending = false) {
        reorder(descending,
            [this](std::size_t a, std::size_t b) { return keys_[a] < keys_[b]; },
            [this](std::size_t a, std::size_t b) { return keys_[a] > keys_[b]; });
    }

    // sortval(descending = false) — stable sort by value
    void sortval(bool descending = false) {
        reorder_stable(descending,
            [this](std::size_t a, std::size_t b) { return values_[a] < values_[b]; },
            [this](std::size_t a, std::size_t b) { return values_[a] > values_[b]; });
    }

    // sortvk(descending = false) — sort by value, then key
    void sortvk(bool descending = false) {
        reorder(descending,
            [this](std::size_t a, std::size_t b) {
                if (values_[a] < values_[b]) return true;
                if (values_[b] < values_[a]) return false;
                return keys_[a] < keys_[b];
            },
            [this](std::size_t a, std::size_t b) {
                if (values_[a] > values_[b]) return true;
                if (values_[b] > values_[a]) return false;
                return keys_[a] > keys_[b];
            });
    }

    // Iterator support (iterates keys in insertion order)
    auto begin() const { return keys_.begin(); }
    auto end() const { return keys_.end(); }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const Map& m) {
        os << "{";
        for (std::size_t i = 0; i < m.keys_.size(); i++) {
            if (i > 0) os << ", ";
            os << m.keys_[i] << ": " << m.values_[i];
        }
        return os << "}";
    }

private:
    std::vector<K> keys_;
    std::vector<V> values_;
    std::unordered_map<std::size_t, std::vector<std::size_t>> index_; // hash → [positions]

    static constexpr std::size_t NOT_FOUND = static_cast<std::size_t>(-1);

    // Type coercion — cast value to V (mirrors C%'s implicit type conversion)
    template<typename U>
    static V coerce(const U& val) {
        if constexpr (std::is_same_v<std::decay_t<U>, V>) {
            return val;
        } else if constexpr (requires { typename V::native_type; }) {
            // cpct wrapper: convert via native type
            using N = typename V::native_type;
            return V(static_cast<N>(val));
        } else if constexpr (std::is_convertible_v<U, V>) {
            return static_cast<V>(val);
        } else {
            return V(val);
        }
    }

    // Byte size of key type
    static std::size_t sizeOfKey() {
        if constexpr (requires { typename K::native_type; }) {
            return sizeof(typename K::native_type);
        } else {
            return sizeof(K);
        }
    }

    // Hash function — converts cpct wrapper types to native before hashing.
    static std::size_t makeHash(const K& key) {
        if constexpr (requires { typename K::native_type; }) {
            using N = typename K::native_type;
            return std::hash<N>{}(static_cast<N>(key));
        } else {
            return std::hash<K>{}(key);
        }
    }

    // Compare keys — handles cpct wrapper types via implicit conversion.
    static bool keysEqual(const K& a, const K& b) {
        if constexpr (requires { typename K::native_type; }) {
            using N = typename K::native_type;
            return static_cast<N>(a) == static_cast<N>(b);
        } else {
            return a == b;
        }
    }

    // Find position of key, returns NOT_FOUND if absent.
    std::size_t findIndex(const K& key) const {
        auto hash = makeHash(key);
        auto it = index_.find(hash);
        if (it == index_.end()) return NOT_FOUND;
        for (auto pos : it->second) {
            if (keysEqual(keys_[pos], key)) return pos;
        }
        return NOT_FOUND;
    }

    // Reorder keys_ and values_ by sorted index order, rebuild index_.
    template<typename CompAsc, typename CompDesc>
    void reorder(bool descending, CompAsc asc, CompDesc desc) {
        std::vector<std::size_t> order(keys_.size());
        std::iota(order.begin(), order.end(), 0);
        if (descending) std::sort(order.begin(), order.end(), desc);
        else            std::sort(order.begin(), order.end(), asc);
        applyOrder(order);
    }

    template<typename CompAsc, typename CompDesc>
    void reorder_stable(bool descending, CompAsc asc, CompDesc desc) {
        std::vector<std::size_t> order(keys_.size());
        std::iota(order.begin(), order.end(), 0);
        if (descending) std::stable_sort(order.begin(), order.end(), desc);
        else            std::stable_sort(order.begin(), order.end(), asc);
        applyOrder(order);
    }

    void applyOrder(const std::vector<std::size_t>& order) {
        std::vector<K> newKeys;
        std::vector<V> newValues;
        newKeys.reserve(order.size());
        newValues.reserve(order.size());
        for (auto i : order) {
            newKeys.push_back(std::move(keys_[i]));
            newValues.push_back(std::move(values_[i]));
        }
        keys_ = std::move(newKeys);
        values_ = std::move(newValues);
        rebuildIndex();
    }

    void rebuildIndex() {
        index_.clear();
        for (std::size_t i = 0; i < keys_.size(); i++) {
            index_[makeHash(keys_[i])].push_back(i);
        }
    }
};

} // namespace cpct
