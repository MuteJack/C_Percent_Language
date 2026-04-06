// cpct/data/dict.h
// cpct::Dict — untyped key-value dictionary (insertion-order preserved).
// Keys: string, int, char, bool (tagged hash to distinguish types).
// Values: any type (stored as std::any).
// Supports: has(), keys(), values(), remove(), len(), size(), is_empty(),
//           sortkey()/sortk(), sortval()/sortv(), sortvk().
#pragma once
#include <string>
#include <vector>
#include <any>
#include <unordered_map>
#include <numeric>
#include <stdexcept>
#include <iostream>
#include <algorithm>

namespace cpct {

class Dict {
public:
    Dict() = default;

    // C% built-in methods
    std::string type() const { return "dict"; }
    std::size_t len() const { return taggedKeys_.size(); }
    bool is_empty() const { return taggedKeys_.empty(); }

    // size() — sum of key bytes + value bytes (approximate)
    std::size_t size() const {
        std::size_t total = 0;
        for (auto& k : taggedKeys_) total += k.size();
        // Value sizes are not knowable from std::any, approximate
        total += values_.size() * sizeof(std::any);
        return total;
    }

    // has/set/get/remove — dispatches by key type using tag functions.
    // String key
    bool has(const std::string& key) const { return findIndex(tagStr(key)) != NOT_FOUND; }
    bool has(const char* key) const { return has(std::string(key)); }
    // Integer key (any integral except bool and char)
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool> && !std::is_same_v<T, char>>>
    bool has(T key) const { return findIndex(tagInt(static_cast<int64_t>(key))) != NOT_FOUND; }
    // Bool key
    bool has_bool(bool key) const { return findIndex(tagBool(key)) != NOT_FOUND; }
    // Char key
    bool has_char(char key) const { return findIndex(tagChar(key)) != NOT_FOUND; }

    // keys() — returns display key strings in insertion order
    std::vector<std::string> keys() const { return displayKeys_; }

    // values() — returns values in insertion order
    std::vector<std::any> values() const { return values_; }

    // set — string key
    template<typename V>
    void set(const std::string& key, const V& val) { setImpl(tagStr(key), key, val); }
    template<typename V>
    void set(const char* key, const V& val) { set(std::string(key), val); }
    // set — integer key
    template<typename V, typename T, typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool> && !std::is_same_v<T, char>>>
    void set(T key, const V& val) { setImpl(tagInt(static_cast<int64_t>(key)), std::to_string(key), val); }
    // set — bool key (named differently to avoid ambiguity)
    template<typename V>
    void set_bool(bool key, const V& val) { setImpl(tagBool(key), key ? "true" : "false", val); }
    // set — char key
    template<typename V>
    void set_char(char key, const V& val) { setImpl(tagChar(key), std::string(1, key), val); }

    // get<T> — string key
    template<typename V>
    V get(const std::string& key) const { return getImpl<V>(tagStr(key)); }
    template<typename V>
    V get(const char* key) const { return get<V>(std::string(key)); }
    // get<T> — integer key
    template<typename V, typename T, typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool> && !std::is_same_v<T, char>>>
    V get(T key) const { return getImpl<V>(tagInt(static_cast<int64_t>(key))); }
    // get<T> — bool key
    template<typename V>
    V get_bool(bool key) const { return getImpl<V>(tagBool(key)); }
    // get<T> — char key
    template<typename V>
    V get_char(char key) const { return getImpl<V>(tagChar(key)); }

    // remove — string key
    void remove(const std::string& key) { removeImpl(tagStr(key)); }
    void remove(const char* key) { remove(std::string(key)); }
    // remove — integer key
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool> && !std::is_same_v<T, char>>>
    void remove(T key) { removeImpl(tagInt(static_cast<int64_t>(key))); }
    // remove — bool key
    void remove_bool(bool key) { removeImpl(tagBool(key)); }
    // remove — char key
    void remove_char(char key) { removeImpl(tagChar(key)); }

    // sortkey(descending = false) / sortk() — sort by display key (string)
    void sortkey(bool descending = false) { sortk(descending); }
    void sortk(bool descending = false) {
        std::vector<std::size_t> order(displayKeys_.size());
        std::iota(order.begin(), order.end(), 0);
        if (descending)
            std::sort(order.begin(), order.end(),
                [this](std::size_t a, std::size_t b) { return displayKeys_[a] > displayKeys_[b]; });
        else
            std::sort(order.begin(), order.end(),
                [this](std::size_t a, std::size_t b) { return displayKeys_[a] < displayKeys_[b]; });
        applyOrder(order);
    }

    // sortval<T>(descending = false) / sortv<T>() — sort by value (must specify type)
    template<typename T>
    void sortval(bool descending = false) { sortv<T>(descending); }
    template<typename T>
    void sortv(bool descending = false) {
        std::vector<std::size_t> order(values_.size());
        std::iota(order.begin(), order.end(), 0);
        if (descending)
            std::stable_sort(order.begin(), order.end(),
                [this](std::size_t a, std::size_t b) {
                    return std::any_cast<T>(values_[a]) > std::any_cast<T>(values_[b]);
                });
        else
            std::stable_sort(order.begin(), order.end(),
                [this](std::size_t a, std::size_t b) {
                    return std::any_cast<T>(values_[a]) < std::any_cast<T>(values_[b]);
                });
        applyOrder(order);
    }

    // sortvk<T>(descending = false) — sort by value then key
    template<typename T>
    void sortvk(bool descending = false) {
        std::vector<std::size_t> order(values_.size());
        std::iota(order.begin(), order.end(), 0);
        if (descending)
            std::sort(order.begin(), order.end(),
                [this](std::size_t a, std::size_t b) {
                    T va = std::any_cast<T>(values_[a]);
                    T vb = std::any_cast<T>(values_[b]);
                    if (va > vb) return true;
                    if (vb > va) return false;
                    return displayKeys_[a] > displayKeys_[b];
                });
        else
            std::sort(order.begin(), order.end(),
                [this](std::size_t a, std::size_t b) {
                    T va = std::any_cast<T>(values_[a]);
                    T vb = std::any_cast<T>(values_[b]);
                    if (va < vb) return true;
                    if (vb < va) return false;
                    return displayKeys_[a] < displayKeys_[b];
                });
        applyOrder(order);
    }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const Dict& d) {
        os << "{";
        for (std::size_t i = 0; i < d.displayKeys_.size(); i++) {
            if (i > 0) os << ", ";
            os << d.displayKeys_[i] << ": ...";
        }
        return os << "}";
    }

    // Iterator support (iterates display keys in insertion order)
    auto begin() const { return displayKeys_.begin(); }
    auto end() const { return displayKeys_.end(); }

private:
    std::vector<std::any> values_;             // values in insertion order
    std::vector<std::string> taggedKeys_;      // tagged keys for lookup ("s:name", "i:42")
    std::vector<std::string> displayKeys_;     // display keys ("name", "42")
    std::unordered_map<std::string, std::vector<std::size_t>> index_; // tag → [positions]

    static constexpr std::size_t NOT_FOUND = static_cast<std::size_t>(-1);

    // Tagged hash keys — type prefix prevents "1" (string) vs 1 (int) collision
    static std::string tagStr(const std::string& k) { return "s:" + k; }
    static std::string tagInt(int64_t k) { return "i:" + std::to_string(k); }
    static std::string tagBool(bool k) { return k ? "b:true" : "b:false"; }
    static std::string tagChar(char k) { return "c:" + std::string(1, k); }

    // Find position by tagged key
    std::size_t findIndex(const std::string& tagKey) const {
        auto it = index_.find(tagKey);
        if (it == index_.end()) return NOT_FOUND;
        if (it->second.empty()) return NOT_FOUND;
        return it->second.front(); // tagged keys are unique, only one entry
    }

    // Set implementation
    template<typename V>
    void setImpl(const std::string& tagKey, const std::string& displayKey, const V& val) {
        std::size_t idx = findIndex(tagKey);
        if (idx != NOT_FOUND) {
            values_[idx] = val;
        } else {
            index_[tagKey].push_back(values_.size());
            taggedKeys_.push_back(tagKey);
            displayKeys_.push_back(displayKey);
            values_.push_back(val);
        }
    }

    // Get implementation
    template<typename V>
    V getImpl(const std::string& tagKey) const {
        std::size_t idx = findIndex(tagKey);
        if (idx == NOT_FOUND) {
            throw std::runtime_error("Key not found in dict");
        }
        return std::any_cast<V>(values_[idx]);
    }

    // Remove implementation — preserves insertion order (shift, not swap)
    void removeImpl(const std::string& tagKey) {
        std::size_t idx = findIndex(tagKey);
        if (idx == NOT_FOUND) return;

        // Erase from vectors (preserves order)
        taggedKeys_.erase(taggedKeys_.begin() + static_cast<std::ptrdiff_t>(idx));
        displayKeys_.erase(displayKeys_.begin() + static_cast<std::ptrdiff_t>(idx));
        values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(idx));

        // Rebuild index
        rebuildIndex();
    }

    void applyOrder(const std::vector<std::size_t>& order) {
        std::vector<std::any> newValues;
        std::vector<std::string> newTagged;
        std::vector<std::string> newDisplay;
        newValues.reserve(order.size());
        newTagged.reserve(order.size());
        newDisplay.reserve(order.size());
        for (auto i : order) {
            newValues.push_back(std::move(values_[i]));
            newTagged.push_back(std::move(taggedKeys_[i]));
            newDisplay.push_back(std::move(displayKeys_[i]));
        }
        values_ = std::move(newValues);
        taggedKeys_ = std::move(newTagged);
        displayKeys_ = std::move(newDisplay);
        rebuildIndex();
    }

    void rebuildIndex() {
        index_.clear();
        for (std::size_t i = 0; i < taggedKeys_.size(); i++) {
            index_[taggedKeys_[i]].push_back(i);
        }
    }
};

} // namespace cpct
