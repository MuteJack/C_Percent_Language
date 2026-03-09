#include "interpreter.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <algorithm>
#include <numeric>

// ============== CpctDict ==============

// Tagged hash for untyped dict — type prefix prevents "1" (string) vs 1 (int) collision
std::string CpctDict::taggedHash(const CpctValue& key) {
    if (key.isInt()) return "i:" + std::to_string(key.asInt());
    if (key.isString()) return "s:" + key.asString();
    if (key.isBool()) return key.asBool() ? "b:true" : "b:false";
    if (key.isBigInt()) return "B:" + key.asBigInt().toString();
    throw std::runtime_error("Unhashable dict key type");
}

// Int hash for typed map with int/char/bool keys — no string conversion
int64_t CpctDict::intHash(const CpctValue& key) {
    if (key.isInt()) return key.asInt();
    if (key.isBool()) return key.asBool() ? 1 : 0;
    throw std::runtime_error("Cannot use non-integer as int key");
}

bool CpctDict::has(const CpctValue& key) const {
    if (usesIntKey()) {
        return intIndex.find(intHash(key)) != intIndex.end();
    }
    std::string sk = keyType.empty() ? taggedHash(key) : key.asString();
    return strIndex.find(sk) != strIndex.end();
}

CpctValue& CpctDict::get(const CpctValue& key) {
    if (usesIntKey()) {
        auto it = intIndex.find(intHash(key));
        if (it == intIndex.end()) throw std::runtime_error("Key not found in map");
        return values[it->second];
    }
    std::string sk = keyType.empty() ? taggedHash(key) : key.asString();
    auto it = strIndex.find(sk);
    if (it == strIndex.end()) throw std::runtime_error("Key not found in dict");
    return values[it->second];
}

const CpctValue& CpctDict::get(const CpctValue& key) const {
    if (usesIntKey()) {
        auto it = intIndex.find(intHash(key));
        if (it == intIndex.end()) throw std::runtime_error("Key not found in map");
        return values[it->second];
    }
    std::string sk = keyType.empty() ? taggedHash(key) : key.asString();
    auto it = strIndex.find(sk);
    if (it == strIndex.end()) throw std::runtime_error("Key not found in dict");
    return values[it->second];
}

void CpctDict::set(const CpctValue& key, const CpctValue& val) {
    if (usesIntKey()) {
        int64_t ik = intHash(key);
        auto it = intIndex.find(ik);
        if (it != intIndex.end()) {
            values[it->second] = val;
        } else {
            intIndex[ik] = keys.size();
            keys.push_back(key);
            values.push_back(val);
        }
    } else {
        std::string sk = keyType.empty() ? taggedHash(key) : key.asString();
        auto it = strIndex.find(sk);
        if (it != strIndex.end()) {
            values[it->second] = val;
        } else {
            strIndex[sk] = keys.size();
            keys.push_back(key);
            values.push_back(val);
        }
    }
}

void CpctDict::remove(const CpctValue& key) {
    size_t pos;
    if (usesIntKey()) {
        int64_t ik = intHash(key);
        auto it = intIndex.find(ik);
        if (it == intIndex.end()) return;
        pos = it->second;
        intIndex.erase(it);
    } else {
        std::string sk = keyType.empty() ? taggedHash(key) : key.asString();
        auto it = strIndex.find(sk);
        if (it == strIndex.end()) return;
        pos = it->second;
        strIndex.erase(it);
    }
    // Swap-remove: move last element to deleted position
    size_t last = keys.size() - 1;
    if (pos != last) {
        keys[pos] = std::move(keys[last]);
        values[pos] = std::move(values[last]);
        // Update the moved element's index entry
        if (usesIntKey()) {
            intIndex[intHash(keys[pos])] = pos;
        } else {
            std::string sk = keyType.empty() ? taggedHash(keys[pos]) : keys[pos].asString();
            strIndex[sk] = pos;
        }
    }
    keys.pop_back();
    values.pop_back();
}

void CpctDict::rebuildIndex() {
    strIndex.clear();
    intIndex.clear();
    if (usesIntKey()) {
        for (size_t i = 0; i < keys.size(); i++) {
            intIndex[intHash(keys[i])] = i;
        }
    } else {
        for (size_t i = 0; i < keys.size(); i++) {
            std::string sk = keyType.empty() ? taggedHash(keys[i]) : keys[i].asString();
            strIndex[sk] = i;
        }
    }
}

// ============== TypedArray ==============

CpctValue TypedArray::get(size_t i) const {
    switch (elemType) {
        case ArrayElemType::Int8:    return CpctValue(static_cast<int64_t>(std::get<std::vector<int8_t>>(data)[i]));
        case ArrayElemType::Int16:   return CpctValue(static_cast<int64_t>(std::get<std::vector<int16_t>>(data)[i]));
        case ArrayElemType::Int32:   return CpctValue(static_cast<int64_t>(std::get<std::vector<int32_t>>(data)[i]));
        case ArrayElemType::Int64:   return CpctValue(std::get<std::vector<int64_t>>(data)[i]);
        case ArrayElemType::UInt8:   return CpctValue(static_cast<int64_t>(std::get<std::vector<uint8_t>>(data)[i]));
        case ArrayElemType::UInt16:  return CpctValue(static_cast<int64_t>(std::get<std::vector<uint16_t>>(data)[i]));
        case ArrayElemType::UInt32:  return CpctValue(static_cast<int64_t>(std::get<std::vector<uint32_t>>(data)[i]));
        case ArrayElemType::UInt64:  return CpctValue(static_cast<int64_t>(std::get<std::vector<uint64_t>>(data)[i]));
        case ArrayElemType::Float32: return CpctValue(static_cast<double>(std::get<std::vector<float>>(data)[i]));
        case ArrayElemType::Float64: return CpctValue(std::get<std::vector<double>>(data)[i]);
        case ArrayElemType::Bool:    return CpctValue(static_cast<bool>(std::get<std::vector<uint8_t>>(data)[i]));
    }
    return CpctValue();
}

void TypedArray::set(size_t i, const CpctValue& val) {
    int64_t iv = val.isInt() ? val.asInt() : static_cast<int64_t>(val.toNumber());
    switch (elemType) {
        case ArrayElemType::Int8:    std::get<std::vector<int8_t>>(data)[i] = static_cast<int8_t>(iv); break;
        case ArrayElemType::Int16:   std::get<std::vector<int16_t>>(data)[i] = static_cast<int16_t>(iv); break;
        case ArrayElemType::Int32:   std::get<std::vector<int32_t>>(data)[i] = static_cast<int32_t>(iv); break;
        case ArrayElemType::Int64:   std::get<std::vector<int64_t>>(data)[i] = iv; break;
        case ArrayElemType::UInt8:   std::get<std::vector<uint8_t>>(data)[i] = static_cast<uint8_t>(iv); break;
        case ArrayElemType::UInt16:  std::get<std::vector<uint16_t>>(data)[i] = static_cast<uint16_t>(iv); break;
        case ArrayElemType::UInt32:  std::get<std::vector<uint32_t>>(data)[i] = static_cast<uint32_t>(iv); break;
        case ArrayElemType::UInt64:  std::get<std::vector<uint64_t>>(data)[i] = static_cast<uint64_t>(iv); break;
        case ArrayElemType::Float32: std::get<std::vector<float>>(data)[i] = static_cast<float>(val.toNumber()); break;
        case ArrayElemType::Float64: std::get<std::vector<double>>(data)[i] = val.toNumber(); break;
        case ArrayElemType::Bool:    std::get<std::vector<uint8_t>>(data)[i] = val.toBool() ? 1 : 0; break;
    }
}

// ============== Sort helpers ==============

// Compare two CpctValues: returns true if a < b
static bool compareCpctValues(const CpctValue& a, const CpctValue& b) {
    // Same-type fast paths
    if (a.isInt() && b.isInt()) return a.asInt() < b.asInt();
    if (a.isFloat() && b.isFloat()) return a.asFloat() < b.asFloat();
    if (a.isString() && b.isString()) return a.asString() < b.asString();
    if (a.isBool() && b.isBool()) return !a.asBool() && b.asBool(); // false < true
    if (a.isBigInt() && b.isBigInt()) return a.asBigInt() < b.asBigInt();
    // BigInt vs int
    if (a.isBigInt() && b.isInt()) return a.asBigInt() < BigInt(b.asInt());
    if (a.isInt() && b.isBigInt()) return BigInt(a.asInt()) < b.asBigInt();
    // Mixed numeric → double
    if ((a.isInt() || a.isFloat() || a.isBigInt()) && (b.isInt() || b.isFloat() || b.isBigInt())) {
        return a.toNumber() < b.toNumber();
    }
    // Fallback: convert to string
    return a.toString() < b.toString();
}

// Sort a TypedArray in-place
static void sortTypedArray(TypedArray& ta, bool descending) {
    std::visit([descending](auto& vec) {
        if (descending)
            std::sort(vec.begin(), vec.end(), std::greater<>{});
        else
            std::sort(vec.begin(), vec.end());
    }, ta.data);
}

// Sort a CpctDict by permutation of indices
static void applyDictPermutation(CpctDict& dict, std::vector<size_t>& perm) {
    std::vector<CpctValue> newKeys(perm.size());
    std::vector<CpctValue> newValues(perm.size());
    for (size_t i = 0; i < perm.size(); i++) {
        newKeys[i] = std::move(dict.keys[perm[i]]);
        newValues[i] = std::move(dict.values[perm[i]]);
    }
    dict.keys = std::move(newKeys);
    dict.values = std::move(newValues);
    dict.rebuildIndex();
}

// ============== CpctValue ==============

double CpctValue::toNumber() const {
    if (isInt()) return static_cast<double>(asInt());  // int64_t → double
    if (isBigInt()) return asBigInt().toDouble();       // BigInt → double (may lose precision)
    if (isFloat()) return asFloat();
    if (isBool()) return asBool() ? 1.0 : 0.0;
    if (isString()) {
        try { return std::stod(asString()); }
        catch (...) { return 0.0; }
    }
    return 0.0;
}

bool CpctValue::toBool() const {
    if (isBool()) return asBool();
    if (isInt()) return asInt() != 0;
    if (isBigInt()) return asBigInt().toBool();
    if (isFloat()) return asFloat() != 0.0;
    if (isString()) return !asString().empty();
    if (isArray()) return !asArray().empty();
    if (isTypedArray()) return asTypedArray().size() > 0;
    if (isDict()) return asDict().size() > 0;
    return false;
}

std::string CpctValue::toString() const {
    if (isInt()) return std::to_string(asInt());
    if (isBigInt()) return asBigInt().toString();
    if (isFloat()) {
        double v = asFloat();
        // If it's a whole number, show with .0
        if (v == static_cast<double>(static_cast<int64_t>(v)) &&
            v >= -1e15 && v <= 1e15) {
            std::ostringstream oss;
            oss << std::setprecision(1) << std::fixed << v;
            return oss.str();
        }
        std::ostringstream oss;
        oss << std::setprecision(15) << v;
        std::string s = oss.str();
        // Remove trailing zeros after decimal point, but keep at least one decimal
        if (s.find('.') != std::string::npos) {
            size_t last = s.find_last_not_of('0');
            if (s[last] == '.') last++;
            s = s.substr(0, last + 1);
        }
        return s;
    }
    if (isBool()) return asBool() ? "true" : "false";
    if (isString()) return asString();
    if (isArray()) {
        std::string result = "[";
        auto& arr = asArray();
        for (size_t i = 0; i < arr.size(); i++) {
            if (i > 0) result += ", ";
            if (arr[i].isString()) {
                result += "\"" + arr[i].toString() + "\"";
            } else {
                result += arr[i].toString();
            }
        }
        result += "]";
        return result;
    }
    if (isTypedArray()) {
        auto& ta = asTypedArray();
        std::string result = "[";
        for (size_t i = 0; i < ta.size(); i++) {
            if (i > 0) result += ", ";
            CpctValue elem = ta.get(i);
            result += elem.toString();
        }
        result += "]";
        return result;
    }
    if (isDict()) {
        auto& d = asDict();
        std::string result = "{";
        for (size_t i = 0; i < d.size(); i++) {
            if (i > 0) result += ", ";
            if (d.keys[i].isString()) {
                result += "\"" + d.keys[i].toString() + "\"";
            } else {
                result += d.keys[i].toString();
            }
            result += ": ";
            if (d.values[i].isString()) {
                result += "\"" + d.values[i].toString() + "\"";
            } else {
                result += d.values[i].toString();
            }
        }
        result += "}";
        return result;
    }
    return "<unknown>";
}

// ============== Environment ==============

Environment::Environment(Environment* parent) : parent_(parent) {}

void Environment::define(const std::string& name, CpctValue value, const std::string& type) {
    vars_[name] = std::move(value);
    if (!type.empty()) {
        types_[name] = type;
    }
}

CpctValue& Environment::get(const std::string& name) {
    auto it = vars_.find(name);
    if (it != vars_.end()) return it->second;
    if (parent_) return parent_->get(name);
    throw RuntimeError("Undefined variable '" + name + "'");
}

void Environment::set(const std::string& name, CpctValue value) {
    auto it = vars_.find(name);
    if (it != vars_.end()) {
        it->second = std::move(value);
        return;
    }
    if (parent_) {
        parent_->set(name, std::move(value));
        return;
    }
    throw RuntimeError("Undefined variable '" + name + "'");
}

bool Environment::has(const std::string& name) const {
    if (vars_.count(name)) return true;
    if (parent_) return parent_->has(name);
    return false;
}

std::string Environment::getType(const std::string& name) const {
    auto it = types_.find(name);
    if (it != types_.end()) return it->second;
    if (parent_) return parent_->getType(name);
    return "";
}

// ============== Interpreter ==============

Interpreter::Interpreter() : currentEnv_(&globalEnv_) {}

void Interpreter::run(const std::vector<StmtPtr>& program) {
    for (auto& stmt : program) {
        execStmt(stmt.get());
    }
}

void Interpreter::execStmt(const Stmt* stmt) {
    switch (stmt->kind) {
        case StmtKind::VarDecl:   execVarDecl(stmt); break;
        case StmtKind::ExprStmt:  eval(stmt->expr.get()); break;
        case StmtKind::Block:     execBlock(stmt); break;
        case StmtKind::If:        execIf(stmt); break;
        case StmtKind::While:     execWhile(stmt); break;
        case StmtKind::DoWhile:   execDoWhile(stmt); break;
        case StmtKind::For:       execFor(stmt); break;
        case StmtKind::ForEach:   execForEach(stmt); break;
        case StmtKind::ForN:      execForN(stmt); break;
        case StmtKind::FuncDecl:  execFuncDecl(stmt); break;
        case StmtKind::Return:    execReturn(stmt); break;
        case StmtKind::Print:     execPrint(stmt); break;
        case StmtKind::Switch:    execSwitch(stmt); break;
        case StmtKind::Break:     throw BreakSignal{};
        case StmtKind::Continue:  throw ContinueSignal{};
    }
}

void Interpreter::execVarDecl(const Stmt* stmt) {
    // Dict declaration (untyped — any key/value)
    if (isDictType(stmt->varType)) {
        CpctDict dict;
        // keyType/valueType left empty → no coercion

        if (stmt->initExpr) {
            CpctValue initVal = eval(stmt->initExpr.get());
            if (!initVal.isDict())
                throw RuntimeError("Dict initializer must be a dict literal at line " + std::to_string(stmt->line));
            dict = std::move(initVal.asDict());
            dict.keyType = "";
            dict.valueType = "";
        }

        currentEnv_->define(stmt->varName, CpctValue(std::move(dict)), stmt->varType);
        return;
    }

    // Map declaration (typed — key/value type enforced)
    if (isMapType(stmt->varType)) {
        auto [kType, vType] = mapKeyValueTypes(stmt->varType);
        CpctDict dict;
        dict.keyType = kType;
        dict.valueType = vType;

        if (stmt->initExpr) {
            CpctValue initVal = eval(stmt->initExpr.get());
            if (!initVal.isDict())
                throw RuntimeError("Map initializer must be a dict literal at line " + std::to_string(stmt->line));
            dict = std::move(initVal.asDict());
            dict.keyType = kType;
            dict.valueType = vType;
            for (size_t i = 0; i < dict.keys.size(); i++) {
                dict.keys[i] = coerceDictKey(kType, std::move(dict.keys[i]), stmt->line);
                dict.values[i] = coerceDictValue(vType, std::move(dict.values[i]), stmt->line);
            }
            dict.rebuildIndex();
        }

        currentEnv_->define(stmt->varName, CpctValue(std::move(dict)), stmt->varType);
        return;
    }

    // Vector declaration (typed dynamic array)
    if (isVectorType(stmt->varType)) {
        std::string elemType = vectorElemType(stmt->varType);
        std::vector<CpctValue> elements;

        if (stmt->initExpr) {
            CpctValue initVal = eval(stmt->initExpr.get());
            if (!initVal.isArray())
                throw RuntimeError("Vector initializer must be an array literal at line " + std::to_string(stmt->line));
            elements = std::move(initVal.asArray());

            // Coerce each element to the declared type
            for (auto& elem : elements) {
                elem = coerceArrayElement(elemType, std::move(elem), stmt->line);
            }
        }

        currentEnv_->define(stmt->varName, CpctValue(std::move(elements)), stmt->varType);
        return;
    }

    // Array declaration
    if (isArrayType(stmt->varType)) {
        std::string baseType = arrayBaseType(stmt->varType);
        int dims = arrayDimCount(stmt->varType);

        if (dims == 1) {
            // 1D array
            bool hasDimSize = stmt->arrayDimSizes.size() > 0 && stmt->arrayDimSizes[0] != nullptr;
            int64_t explicitSize = -1;
            if (hasDimSize) {
                CpctValue sizeVal = eval(stmt->arrayDimSizes[0].get());
                if (!sizeVal.isInt() || sizeVal.asInt() <= 0)
                    throw RuntimeError("Array size must be a positive integer at line " + std::to_string(stmt->line));
                explicitSize = sizeVal.asInt();
            }

            std::vector<CpctValue> elements;

            if (stmt->initExpr) {
                CpctValue initVal = eval(stmt->initExpr.get());
                if (!initVal.isArray())
                    throw RuntimeError("Array initializer must be an array literal at line " + std::to_string(stmt->line));
                elements = std::move(initVal.asArray());

                // Coerce each element
                for (auto& elem : elements) {
                    elem = coerceArrayElement(baseType, std::move(elem), stmt->line);
                }

                if (explicitSize >= 0) {
                    if (static_cast<int64_t>(elements.size()) > explicitSize)
                        throw RuntimeError("Too many initializers for array of size " + std::to_string(explicitSize) +
                                          " at line " + std::to_string(stmt->line));
                    // Pad with defaults
                    while (static_cast<int64_t>(elements.size()) < explicitSize) {
                        elements.push_back(makeDefaultValue(baseType));
                    }
                } else if (elements.empty()) {
                    throw RuntimeError("Cannot declare array without size or initializer at line " + std::to_string(stmt->line));
                }
            } else if (explicitSize >= 0) {
                // No initializer, fill with defaults
                for (int64_t i = 0; i < explicitSize; i++) {
                    elements.push_back(makeDefaultValue(baseType));
                }
            } else {
                throw RuntimeError("Cannot declare array without size or initializer at line " + std::to_string(stmt->line));
            }

            // Convert to TypedArray for memory efficiency (1D only, non-string)
            if (supportsTypedArray(baseType)) {
                ArrayElemType aet = getArrayElemType(baseType);
                TypedArray ta = TypedArray::create(aet, elements.size());
                for (size_t i = 0; i < elements.size(); i++) {
                    ta.set(i, elements[i]);
                }
                currentEnv_->define(stmt->varName, CpctValue(std::move(ta)), stmt->varType);
            } else {
                currentEnv_->define(stmt->varName, CpctValue(std::move(elements)), stmt->varType);
            }
        } else {
            // Multi-dimensional array
            // Evaluate all dimension sizes
            std::vector<int64_t> dimSizes;
            bool allSizesExplicit = true;
            for (size_t d = 0; d < stmt->arrayDimSizes.size(); d++) {
                if (stmt->arrayDimSizes[d]) {
                    CpctValue sv = eval(stmt->arrayDimSizes[d].get());
                    if (!sv.isInt() || sv.asInt() <= 0)
                        throw RuntimeError("Array dimension size must be a positive integer at line " + std::to_string(stmt->line));
                    dimSizes.push_back(sv.asInt());
                } else {
                    allSizesExplicit = false;
                    dimSizes.push_back(-1);
                }
            }

            if (stmt->initExpr) {
                CpctValue initVal = eval(stmt->initExpr.get());
                if (!initVal.isArray())
                    throw RuntimeError("Array initializer must be an array literal at line " + std::to_string(stmt->line));

                // For multi-dim, validate and coerce the nested structure
                auto& rows = initVal.asArray();
                int64_t rowCount = static_cast<int64_t>(rows.size());

                if (dims == 2) {
                    // Determine column count
                    int64_t colCount = -1;
                    if (dimSizes.size() >= 2 && dimSizes[1] >= 0) colCount = dimSizes[1];

                    // Validate all rows are arrays
                    for (size_t r = 0; r < rows.size(); r++) {
                        if (!rows[r].isArray())
                            throw RuntimeError("Multi-dimensional array row must be an array at line " + std::to_string(stmt->line));
                        auto& row = rows[r].asArray();
                        if (colCount < 0) colCount = static_cast<int64_t>(row.size());
                        else if (static_cast<int64_t>(row.size()) > colCount && dimSizes.size() >= 2 && dimSizes[1] >= 0)
                            throw RuntimeError("Row initializer exceeds column size at line " + std::to_string(stmt->line));
                        else if (static_cast<int64_t>(row.size()) != colCount && (dimSizes.size() < 2 || dimSizes[1] < 0))
                            throw RuntimeError("Inconsistent row lengths in array at line " + std::to_string(stmt->line));
                    }

                    int64_t targetRows = (dimSizes.size() >= 1 && dimSizes[0] >= 0) ? dimSizes[0] : rowCount;
                    if (rowCount > targetRows)
                        throw RuntimeError("Too many row initializers at line " + std::to_string(stmt->line));

                    // Build the 2D array with coercion and padding
                    std::vector<CpctValue> result;
                    for (int64_t r = 0; r < targetRows; r++) {
                        std::vector<CpctValue> rowElems;
                        if (r < rowCount) {
                            auto& srcRow = rows[r].asArray();
                            for (auto& elem : srcRow) {
                                rowElems.push_back(coerceArrayElement(baseType, CpctValue(elem), stmt->line));
                            }
                        }
                        // Pad columns
                        while (static_cast<int64_t>(rowElems.size()) < colCount) {
                            rowElems.push_back(makeDefaultValue(baseType));
                        }
                        result.push_back(CpctValue(std::move(rowElems)));
                    }
                    currentEnv_->define(stmt->varName, CpctValue(std::move(result)), stmt->varType);
                } else {
                    // 3D+ — store as-is with coercion at leaf level (simplified)
                    currentEnv_->define(stmt->varName, std::move(initVal), stmt->varType);
                }
            } else if (allSizesExplicit) {
                // No initializer — create default-filled multi-dim array
                // Build from innermost dimension outward
                std::function<CpctValue(int)> buildArray = [&](int dim) -> CpctValue {
                    if (dim == dims - 1) {
                        // Innermost dimension
                        std::vector<CpctValue> arr;
                        for (int64_t i = 0; i < dimSizes[dim]; i++)
                            arr.push_back(makeDefaultValue(baseType));
                        return CpctValue(std::move(arr));
                    } else {
                        std::vector<CpctValue> arr;
                        for (int64_t i = 0; i < dimSizes[dim]; i++)
                            arr.push_back(buildArray(dim + 1));
                        return CpctValue(std::move(arr));
                    }
                };
                currentEnv_->define(stmt->varName, buildArray(0), stmt->varType);
            } else {
                throw RuntimeError("Cannot declare multi-dimensional array without all sizes or initializer at line " + std::to_string(stmt->line));
            }
        }
        return;
    }

    // Scalar declaration (unchanged)
    CpctValue val;
    if (stmt->initExpr) {
        val = eval(stmt->initExpr.get());
    } else {
        val = makeDefaultValue(stmt->varType);
    }
    // Range check for sized integer types (int/int8/16/32/64) and char, but NOT intbig/bigint (dynamic BigInt)
    if (isSizedIntegerType(stmt->varType) || isCharType(stmt->varType)) {
        val = checkIntRange(stmt->varType, std::move(val), stmt->line);
    }
    // Float type coercion
    if (isFloatType(stmt->varType)) {
        val = coerceFloat(stmt->varType, std::move(val), stmt->line);
    }
    currentEnv_->define(stmt->varName, std::move(val), stmt->varType);
}

void Interpreter::execBlock(const Stmt* stmt) {
    Environment blockEnv(currentEnv_);
    Environment* prev = currentEnv_;
    currentEnv_ = &blockEnv;
    try {
        for (auto& s : stmt->body) {
            execStmt(s.get());
        }
    } catch (...) {
        currentEnv_ = prev;
        throw;
    }
    currentEnv_ = prev;
}

void Interpreter::execBlockWithEnv(const std::vector<StmtPtr>& stmts, Environment& env) {
    Environment* prev = currentEnv_;
    currentEnv_ = &env;
    try {
        for (auto& s : stmts) {
            execStmt(s.get());
        }
    } catch (...) {
        currentEnv_ = prev;
        throw;
    }
    currentEnv_ = prev;
}

void Interpreter::execIf(const Stmt* stmt) {
    CpctValue cond = eval(stmt->condition.get());
    if (cond.toBool()) {
        execStmt(stmt->thenBranch.get());
    } else if (stmt->elseBranch) {
        execStmt(stmt->elseBranch.get());
    }
}

void Interpreter::execWhile(const Stmt* stmt) {
    while (true) {
        CpctValue cond = eval(stmt->condition.get());
        if (!cond.toBool()) break;
        try {
            execStmt(stmt->thenBranch.get());
        } catch (BreakSignal&) {
            break;
        } catch (ContinueSignal&) {
            continue;
        }
    }
}

void Interpreter::execDoWhile(const Stmt* stmt) {
    while (true) {
        try {
            execStmt(stmt->thenBranch.get());
        } catch (BreakSignal&) {
            break;
        } catch (ContinueSignal&) {
            // continue → evaluate condition
        }
        CpctValue cond = eval(stmt->condition.get());
        if (!cond.toBool()) break;
    }
}

void Interpreter::execFor(const Stmt* stmt) {
    Environment forEnv(currentEnv_);
    Environment* prev = currentEnv_;
    currentEnv_ = &forEnv;

    if (stmt->initStmt) execStmt(stmt->initStmt.get());

    while (true) {
        if (stmt->condition) {
            CpctValue cond = eval(stmt->condition.get());
            if (!cond.toBool()) break;
        }
        try {
            execStmt(stmt->thenBranch.get());
        } catch (BreakSignal&) {
            break;
        } catch (ContinueSignal&) {
            // fall through to increment
        }
        if (stmt->increment) eval(stmt->increment.get());
    }

    currentEnv_ = prev;
}

void Interpreter::execForEach(const Stmt* stmt) {
    CpctValue iterable = eval(stmt->expr.get());
    Environment forEnv(currentEnv_);
    Environment* prev = currentEnv_;
    currentEnv_ = &forEnv;

    if (iterable.isArray()) {
        const auto& arr = iterable.asArray();
        forEnv.define(stmt->varName, CpctValue(), stmt->varType);
        for (size_t i = 0; i < arr.size(); i++) {
            forEnv.set(stmt->varName, arr[i]);
            try {
                execStmt(stmt->thenBranch.get());
            } catch (BreakSignal&) { break; }
            catch (ContinueSignal&) { continue; }
        }
    } else if (iterable.isTypedArray()) {
        const auto& ta = iterable.asTypedArray();
        forEnv.define(stmt->varName, CpctValue(), stmt->varType);
        for (size_t i = 0; i < ta.size(); i++) {
            forEnv.set(stmt->varName, ta.get(i));
            try {
                execStmt(stmt->thenBranch.get());
            } catch (BreakSignal&) { break; }
            catch (ContinueSignal&) { continue; }
        }
    } else if (iterable.isString()) {
        const auto& str = iterable.asString();
        forEnv.define(stmt->varName, CpctValue(), stmt->varType);
        for (size_t i = 0; i < str.size(); i++) {
            forEnv.set(stmt->varName, CpctValue(std::string(1, str[i])));
            try {
                execStmt(stmt->thenBranch.get());
            } catch (BreakSignal&) { break; }
            catch (ContinueSignal&) { continue; }
        }
    } else if (iterable.isDict()) {
        const auto& dict = iterable.asDict();
        forEnv.define(stmt->varName, CpctValue(), stmt->varType);
        for (size_t i = 0; i < dict.size(); i++) {
            forEnv.set(stmt->varName, dict.keys[i]);
            try {
                execStmt(stmt->thenBranch.get());
            } catch (BreakSignal&) { break; }
            catch (ContinueSignal&) { continue; }
        }
    } else {
        throw RuntimeError("for-each requires array, string, or dict at line " + std::to_string(stmt->line));
    }

    currentEnv_ = prev;
}

void Interpreter::execForN(const Stmt* stmt) {
    CpctValue countVal = eval(stmt->expr.get());
    if (!countVal.isInt())
        throw RuntimeError("for(n) requires integer count at line " + std::to_string(stmt->line));
    int64_t n = countVal.asInt();

    for (int64_t i = 0; i < n; i++) {
        try {
            execStmt(stmt->thenBranch.get());
        } catch (BreakSignal&) { break; }
        catch (ContinueSignal&) { continue; }
    }
}

void Interpreter::execFuncDecl(const Stmt* stmt) {
    FuncDef def;
    def.returnType = stmt->funcReturnType;
    def.params = stmt->params;
    for (auto& s : stmt->body) {
        def.body.push_back(s.get());
    }
    functions_[stmt->funcName] = std::move(def);
}

void Interpreter::execReturn(const Stmt* stmt) {
    CpctValue val;
    if (stmt->expr) {
        val = eval(stmt->expr.get());
    }
    throw ReturnSignal{std::move(val)};
}

void Interpreter::execPrint(const Stmt* stmt) {
    for (size_t i = 0; i < stmt->printArgs.size(); i++) {
        if (i > 0) std::cout << " ";
        const Expr* argExpr = stmt->printArgs[i].get();
        CpctValue val = eval(argExpr);

        // Print char-typed variables and char literals as characters
        bool printAsChar = (argExpr->kind == ExprKind::CharLiteral);
        if (!printAsChar && argExpr->kind == ExprKind::Identifier) {
            std::string type = currentEnv_->getType(argExpr->name);
            if (type == "char") printAsChar = true;
        }

        if (printAsChar && val.isInt()) {
            std::cout << static_cast<char>(val.asInt());
        } else {
            std::cout << val.toString();
        }
    }
    if (stmt->printNewline) std::cout << std::endl;
    else std::cout << std::flush;
}

void Interpreter::execSwitch(const Stmt* stmt) {
    CpctValue switchVal = eval(stmt->condition.get());

    // Find matching case (or default)
    int matchIdx = -1;
    int defaultIdx = -1;
    for (size_t i = 0; i < stmt->caseValues.size(); i++) {
        if (!stmt->caseValues[i]) {
            defaultIdx = static_cast<int>(i);
            continue;
        }
        CpctValue caseVal = eval(stmt->caseValues[i].get());
        // Equality comparison
        bool equal = false;
        if (switchVal.isInt() && caseVal.isInt())
            equal = switchVal.asInt() == caseVal.asInt();
        else if (switchVal.isFloat() || caseVal.isFloat())
            equal = switchVal.toNumber() == caseVal.toNumber();
        else if (switchVal.isString() && caseVal.isString())
            equal = switchVal.asString() == caseVal.asString();
        else if (switchVal.isBool() && caseVal.isBool())
            equal = switchVal.asBool() == caseVal.asBool();

        if (equal) {
            matchIdx = static_cast<int>(i);
            break;
        }
    }

    int startIdx = (matchIdx >= 0) ? matchIdx : defaultIdx;
    if (startIdx < 0) return; // no match, no default

    // Execute from matched case onwards (fall-through), break exits
    try {
        for (size_t i = static_cast<size_t>(startIdx); i < stmt->caseBodies.size(); i++) {
            for (auto& s : stmt->caseBodies[i]) {
                execStmt(s.get());
            }
        }
    } catch (BreakSignal&) {
        // break exits switch
    }
    // ContinueSignal propagates to enclosing loop
}

// ============== Expression Evaluation ==============

CpctValue Interpreter::eval(const Expr* expr) {
    switch (expr->kind) {
        case ExprKind::IntLiteral:    return CpctValue(expr->intVal);
        case ExprKind::FloatLiteral:  return CpctValue(expr->floatVal);
        case ExprKind::StringLiteral: return CpctValue(expr->strVal);
        case ExprKind::CharLiteral:   return CpctValue(expr->intVal);
        case ExprKind::BoolLiteral:   return CpctValue(expr->boolVal);
        case ExprKind::Identifier:    return currentEnv_->get(expr->name);
        case ExprKind::BinaryOp:      return evalBinaryOp(expr);
        case ExprKind::UnaryOp:       return evalUnaryOp(expr);
        case ExprKind::Assign:        return evalAssign(expr);
        case ExprKind::CompoundAssign: return evalCompoundAssign(expr);
        case ExprKind::FunctionCall:  return evalFunctionCall(expr);
        case ExprKind::ArrayAccess:   return evalArrayAccess(expr);
        case ExprKind::ArrayAssign:   return evalArrayAssign(expr);
        case ExprKind::ArrayCompoundAssign: return evalArrayCompoundAssign(expr);
        case ExprKind::ArraySlice:    return evalArraySlice(expr);
        case ExprKind::Input: {
            std::string line;
            std::getline(std::cin, line);
            return CpctValue(line);
        }
        case ExprKind::FStringExpr: {
            std::string result;
            for (auto& part : expr->elements) {
                CpctValue v = eval(part.get());
                result += v.toString();
            }
            return CpctValue(result);
        }
        case ExprKind::ArrayLiteral: {
            std::vector<CpctValue> elements;
            for (auto& e : expr->elements) {
                elements.push_back(eval(e.get()));
            }
            return CpctValue(std::move(elements));
        }
        case ExprKind::DictLiteral: {
            CpctDict dict;
            for (size_t i = 0; i < expr->elements.size(); i++) {
                CpctValue key = eval(expr->elements[i].get());
                CpctValue val = eval(expr->dictValues[i].get());
                dict.set(key, val);
            }
            return CpctValue(std::move(dict));
        }
        case ExprKind::PreIncrement: {
            CpctValue& ref = currentEnv_->get(expr->operand->name);
            if (ref.isNumericInt()) {
                CpctValue nv = CpctValue(ref.toBigInt() + BigInt(int64_t(1)));
                std::string type = currentEnv_->getType(expr->operand->name);
                if (!type.empty() && (isSizedIntegerType(type) || isCharType(type)))
                    nv = checkIntRange(type, std::move(nv), expr->line);
                ref = nv;
            } else if (ref.isFloat()) ref = CpctValue(ref.asFloat() + 1.0);
            return ref;
        }
        case ExprKind::PreDecrement: {
            CpctValue& ref = currentEnv_->get(expr->operand->name);
            if (ref.isNumericInt()) {
                CpctValue nv = CpctValue(ref.toBigInt() - BigInt(int64_t(1)));
                std::string type = currentEnv_->getType(expr->operand->name);
                if (!type.empty() && (isSizedIntegerType(type) || isCharType(type)))
                    nv = checkIntRange(type, std::move(nv), expr->line);
                ref = nv;
            } else if (ref.isFloat()) ref = CpctValue(ref.asFloat() - 1.0);
            return ref;
        }
        case ExprKind::PostIncrement: {
            CpctValue& ref = currentEnv_->get(expr->operand->name);
            CpctValue old = ref;
            if (ref.isNumericInt()) {
                CpctValue nv = CpctValue(ref.toBigInt() + BigInt(int64_t(1)));
                std::string type = currentEnv_->getType(expr->operand->name);
                if (!type.empty() && (isSizedIntegerType(type) || isCharType(type)))
                    nv = checkIntRange(type, std::move(nv), expr->line);
                ref = nv;
            } else if (ref.isFloat()) ref = CpctValue(ref.asFloat() + 1.0);
            return old;
        }
        case ExprKind::PostDecrement: {
            CpctValue& ref = currentEnv_->get(expr->operand->name);
            CpctValue old = ref;
            if (ref.isNumericInt()) {
                CpctValue nv = CpctValue(ref.toBigInt() - BigInt(int64_t(1)));
                std::string type = currentEnv_->getType(expr->operand->name);
                if (!type.empty() && (isSizedIntegerType(type) || isCharType(type)))
                    nv = checkIntRange(type, std::move(nv), expr->line);
                ref = nv;
            } else if (ref.isFloat()) ref = CpctValue(ref.asFloat() - 1.0);
            return old;
        }
        case ExprKind::ChainedComparison: {
            // Evaluate each operand once, check consecutive pairs
            auto& operands = expr->chainOperands;
            auto& ops = expr->chainOps;
            CpctValue prev = eval(operands[0].get());
            for (size_t i = 0; i < ops.size(); i++) {
                CpctValue curr = eval(operands[i + 1].get());
                bool result;
                if (prev.isNumericInt() && curr.isNumericInt()) {
                    if (prev.isInt() && curr.isInt()) {
                        int64_t l = prev.asInt(), r = curr.asInt();
                        switch (ops[i]) {
                            case TokenType::LT:  result = l < r; break;
                            case TokenType::GT:  result = l > r; break;
                            case TokenType::LTE: result = l <= r; break;
                            case TokenType::GTE: result = l >= r; break;
                            default: result = false;
                        }
                    } else {
                        BigInt l = prev.toBigInt(), r = curr.toBigInt();
                        switch (ops[i]) {
                            case TokenType::LT:  result = l < r; break;
                            case TokenType::GT:  result = l > r; break;
                            case TokenType::LTE: result = l <= r; break;
                            case TokenType::GTE: result = l >= r; break;
                            default: result = false;
                        }
                    }
                } else {
                    double l = prev.toNumber(), r = curr.toNumber();
                    switch (ops[i]) {
                        case TokenType::LT:  result = l < r; break;
                        case TokenType::GT:  result = l > r; break;
                        case TokenType::LTE: result = l <= r; break;
                        case TokenType::GTE: result = l >= r; break;
                        default: result = false;
                    }
                }
                if (!result) return CpctValue(false);
                prev = std::move(curr);
            }
            return CpctValue(true);
        }
    }
    throw RuntimeError("Unknown expression kind");
}

CpctValue Interpreter::evalBinaryOp(const Expr* expr) {
    CpctValue left = eval(expr->left.get());
    CpctValue right = eval(expr->right.get());

    // String concatenation
    if (expr->op == TokenType::PLUS && (left.isString() || right.isString())) {
        return CpctValue(left.toString() + right.toString());
    }

    // Logical operators
    if (expr->op == TokenType::AND) return CpctValue(left.toBool() && right.toBool());
    if (expr->op == TokenType::OR)  return CpctValue(left.toBool() || right.toBool());

    // Equality — handle BigInt
    if (expr->op == TokenType::EQ) {
        if (left.isNumericInt() && right.isNumericInt())
            return CpctValue(left.toBigInt() == right.toBigInt());
        if (left.isString() && right.isString()) return CpctValue(left.asString() == right.asString());
        if (left.isBool() && right.isBool()) return CpctValue(left.asBool() == right.asBool());
        return CpctValue(left.toNumber() == right.toNumber());
    }
    if (expr->op == TokenType::NEQ) {
        if (left.isNumericInt() && right.isNumericInt())
            return CpctValue(left.toBigInt() != right.toBigInt());
        if (left.isString() && right.isString()) return CpctValue(left.asString() != right.asString());
        if (left.isBool() && right.isBool()) return CpctValue(left.asBool() != right.asBool());
        return CpctValue(left.toNumber() != right.toNumber());
    }

    // Integer arithmetic (int64 fast path + BigInt overflow promotion)
    if (left.isNumericInt() && right.isNumericInt()) {
        // Fast path: both fit in int64
        if (left.isInt() && right.isInt()) {
            int64_t l = left.asInt(), r = right.asInt();
            int64_t result;
            switch (expr->op) {
                case TokenType::PLUS:
                    if (!__builtin_add_overflow(l, r, &result)) return CpctValue(result);
                    return CpctValue(BigInt(l) + BigInt(r)); // overflow → BigInt
                case TokenType::MINUS:
                    if (!__builtin_sub_overflow(l, r, &result)) return CpctValue(result);
                    return CpctValue(BigInt(l) - BigInt(r));
                case TokenType::STAR:
                    if (!__builtin_mul_overflow(l, r, &result)) return CpctValue(result);
                    return CpctValue(BigInt(l) * BigInt(r));
                case TokenType::SLASH:
                    if (r == 0) throw RuntimeError("Division by zero at line " + std::to_string(expr->line));
                    if (l == INT64_MIN && r == -1) return CpctValue(BigInt(l) / BigInt(r));
                    return CpctValue(l / r);
                case TokenType::PERCENT:
                    if (r == 0) throw RuntimeError("Modulo by zero at line " + std::to_string(expr->line));
                    return CpctValue(l % r);
                case TokenType::DIVMOD: {
                    if (r == 0) throw RuntimeError("Division by zero in /% at line " + std::to_string(expr->line));
                    std::vector<CpctValue> res = { CpctValue(l / r), CpctValue(l % r) };
                    return CpctValue(std::move(res));
                }
                case TokenType::POWER: {
                    if (r < 0) return CpctValue(std::pow(static_cast<double>(l), static_cast<double>(r)));
                    BigInt base(l), res(int64_t(1));
                    for (int64_t i = 0; i < r; i++) res = res * base;
                    return CpctValue(std::move(res));
                }
                case TokenType::LT:  return CpctValue(l < r);
                case TokenType::GT:  return CpctValue(l > r);
                case TokenType::LTE: return CpctValue(l <= r);
                case TokenType::GTE: return CpctValue(l >= r);
                default: break;
            }
        }
        // At least one is BigInt — use BigInt arithmetic
        BigInt l = left.toBigInt(), r = right.toBigInt();
        switch (expr->op) {
            case TokenType::PLUS:    return CpctValue(l + r);
            case TokenType::MINUS:   return CpctValue(l - r);
            case TokenType::STAR:    return CpctValue(l * r);
            case TokenType::SLASH:
                if (r.isZero()) throw RuntimeError("Division by zero at line " + std::to_string(expr->line));
                return CpctValue(l / r);
            case TokenType::PERCENT:
                if (r.isZero()) throw RuntimeError("Modulo by zero at line " + std::to_string(expr->line));
                return CpctValue(l % r);
            case TokenType::DIVMOD: {
                if (r.isZero()) throw RuntimeError("Division by zero in /% at line " + std::to_string(expr->line));
                std::vector<CpctValue> res = { CpctValue(l / r), CpctValue(l % r) };
                return CpctValue(std::move(res));
            }
            case TokenType::POWER: {
                if (!r.fitsInt64() || r.toInt64() < 0)
                    return CpctValue(std::pow(l.toDouble(), r.toDouble()));
                BigInt res(int64_t(1));
                int64_t exp = r.toInt64();
                for (int64_t i = 0; i < exp; i++) res = res * l;
                return CpctValue(std::move(res));
            }
            case TokenType::LT:  return CpctValue(l < r);
            case TokenType::GT:  return CpctValue(l > r);
            case TokenType::LTE: return CpctValue(l <= r);
            case TokenType::GTE: return CpctValue(l >= r);
            default: break;
        }
    }

    // Float arithmetic
    double l = left.toNumber(), r = right.toNumber();
    switch (expr->op) {
        case TokenType::PLUS:  return CpctValue(l + r);
        case TokenType::MINUS: return CpctValue(l - r);
        case TokenType::STAR:  return CpctValue(l * r);
        case TokenType::SLASH:
            if (r == 0.0) throw RuntimeError("Division by zero at line " + std::to_string(expr->line));
            return CpctValue(l / r);
        case TokenType::PERCENT: return CpctValue(std::fmod(l, r));
        case TokenType::DIVMOD: {
            if (r == 0.0) throw RuntimeError("Division by zero in /% at line " + std::to_string(expr->line));
            std::vector<CpctValue> res = { CpctValue(std::trunc(l / r)), CpctValue(std::fmod(l, r)) };
            return CpctValue(std::move(res));
        }
        case TokenType::POWER:  return CpctValue(std::pow(l, r));
        case TokenType::LT:  return CpctValue(l < r);
        case TokenType::GT:  return CpctValue(l > r);
        case TokenType::LTE: return CpctValue(l <= r);
        case TokenType::GTE: return CpctValue(l >= r);
        default:
            throw RuntimeError("Unknown binary operator at line " + std::to_string(expr->line));
    }
}

CpctValue Interpreter::evalUnaryOp(const Expr* expr) {
    CpctValue val = eval(expr->operand.get());
    switch (expr->op) {
        case TokenType::MINUS:
            if (val.isInt()) {
                if (val.asInt() == INT64_MIN)
                    return CpctValue(-BigInt(val.asInt()));
                return CpctValue(-val.asInt());
            }
            if (val.isBigInt()) return CpctValue(-val.asBigInt());
            return CpctValue(-val.toNumber());
        case TokenType::NOT:
            return CpctValue(!val.toBool());
        default:
            throw RuntimeError("Unknown unary operator");
    }
}

CpctValue Interpreter::evalAssign(const Expr* expr) {
    CpctValue val = eval(expr->right.get());
    std::string type = currentEnv_->getType(expr->name);
    if (!type.empty() && (isSizedIntegerType(type) || isCharType(type))) {
        val = checkIntRange(type, std::move(val), expr->line);
    }
    if (!type.empty() && isFloatType(type)) {
        val = coerceFloat(type, std::move(val), expr->line);
    }
    currentEnv_->set(expr->name, val);
    return val;
}

CpctValue Interpreter::evalCompoundAssign(const Expr* expr) {
    CpctValue cur = currentEnv_->get(expr->name);
    CpctValue right = eval(expr->right.get());

    CpctValue result;
    if (cur.isNumericInt() && right.isNumericInt()) {
        // Fast path: both int64
        if (cur.isInt() && right.isInt()) {
            int64_t l = cur.asInt(), r = right.asInt();
            int64_t res;
            switch (expr->op) {
                case TokenType::PLUS_ASSIGN:
                    if (!__builtin_add_overflow(l, r, &res)) { result = CpctValue(res); break; }
                    result = CpctValue(BigInt(l) + BigInt(r)); break;
                case TokenType::MINUS_ASSIGN:
                    if (!__builtin_sub_overflow(l, r, &res)) { result = CpctValue(res); break; }
                    result = CpctValue(BigInt(l) - BigInt(r)); break;
                case TokenType::STAR_ASSIGN:
                    if (!__builtin_mul_overflow(l, r, &res)) { result = CpctValue(res); break; }
                    result = CpctValue(BigInt(l) * BigInt(r)); break;
                case TokenType::SLASH_ASSIGN:
                    if (r == 0) throw RuntimeError("Division by zero");
                    result = CpctValue(l / r); break;
                case TokenType::PERCENT_ASSIGN:
                    if (r == 0) throw RuntimeError("Division by zero");
                    result = CpctValue(l % r); break;
                default: throw RuntimeError("Unknown compound operator");
            }
        } else {
            // BigInt path
            BigInt l = cur.toBigInt(), r = right.toBigInt();
            switch (expr->op) {
                case TokenType::PLUS_ASSIGN:  result = CpctValue(l + r); break;
                case TokenType::MINUS_ASSIGN: result = CpctValue(l - r); break;
                case TokenType::STAR_ASSIGN:  result = CpctValue(l * r); break;
                case TokenType::SLASH_ASSIGN:
                    if (r.isZero()) throw RuntimeError("Division by zero");
                    result = CpctValue(l / r); break;
                case TokenType::PERCENT_ASSIGN:
                    if (r.isZero()) throw RuntimeError("Division by zero");
                    result = CpctValue(l % r); break;
                default: throw RuntimeError("Unknown compound operator");
            }
        }
    } else {
        double l = cur.toNumber(), r = right.toNumber();
        switch (expr->op) {
            case TokenType::PLUS_ASSIGN:  result = CpctValue(l + r); break;
            case TokenType::MINUS_ASSIGN: result = CpctValue(l - r); break;
            case TokenType::STAR_ASSIGN:  result = CpctValue(l * r); break;
            case TokenType::SLASH_ASSIGN:
                if (r == 0.0) throw RuntimeError("Division by zero");
                result = CpctValue(l / r); break;
            case TokenType::PERCENT_ASSIGN:
                if (r == 0.0) throw RuntimeError("Division by zero");
                result = CpctValue(std::fmod(l, r)); break;
            default: throw RuntimeError("Unknown compound operator");
        }
    }

    std::string type = currentEnv_->getType(expr->name);
    if (!type.empty() && (isSizedIntegerType(type) || isCharType(type))) {
        result = checkIntRange(type, std::move(result), expr->line);
    }
    if (!type.empty() && isFloatType(type)) {
        result = coerceFloat(type, std::move(result), expr->line);
    }
    currentEnv_->set(expr->name, result);
    return result;
}

CpctValue Interpreter::evalFunctionCall(const Expr* expr) {
    // Type method: int.max(), float32.min(), etc.
    if (expr->funcName == "__type_method") {
        std::string typeName = expr->args[0]->strVal;
        std::string method = expr->args[1]->strVal;

        if (method == "max") {
            if (typeName == "int" || typeName == "int32") return CpctValue(static_cast<int64_t>(INT32_MAX));
            if (typeName == "int8") return CpctValue(static_cast<int64_t>(INT8_MAX));
            if (typeName == "int16") return CpctValue(static_cast<int64_t>(INT16_MAX));
            if (typeName == "int64") return CpctValue(static_cast<int64_t>(INT64_MAX));
            if (typeName == "float" || typeName == "float64") return CpctValue(static_cast<double>(1.7976931348623157e+308));
            if (typeName == "float32") return CpctValue(static_cast<double>(3.4028234663852886e+38));
            if (typeName == "char") return CpctValue(static_cast<int64_t>(127));
            if (typeName == "bool") return CpctValue(static_cast<int64_t>(1));
            if (typeName == "uint" || typeName == "uint32") return CpctValue(static_cast<int64_t>(UINT32_MAX));
            if (typeName == "uint8") return CpctValue(static_cast<int64_t>(UINT8_MAX));
            if (typeName == "uint16") return CpctValue(static_cast<int64_t>(UINT16_MAX));
            if (typeName == "uint64") return CpctValue(BigInt("18446744073709551615"));
            throw RuntimeError("max() is not defined for type '" + typeName + "'");
        }
        if (method == "min") {
            if (typeName == "int" || typeName == "int32") return CpctValue(static_cast<int64_t>(INT32_MIN));
            if (typeName == "int8") return CpctValue(static_cast<int64_t>(INT8_MIN));
            if (typeName == "int16") return CpctValue(static_cast<int64_t>(INT16_MIN));
            if (typeName == "int64") return CpctValue(static_cast<int64_t>(INT64_MIN));
            if (typeName == "float" || typeName == "float64") return CpctValue(static_cast<double>(-1.7976931348623157e+308));
            if (typeName == "float32") return CpctValue(static_cast<double>(-3.4028234663852886e+38));
            if (typeName == "char") return CpctValue(static_cast<int64_t>(-128));
            if (typeName == "bool") return CpctValue(static_cast<int64_t>(0));
            if (typeName == "uint" || typeName == "uint8" || typeName == "uint16" ||
                typeName == "uint32" || typeName == "uint64") return CpctValue(static_cast<int64_t>(0));
            throw RuntimeError("min() is not defined for type '" + typeName + "'");
        }
        throw RuntimeError("Unknown type method '" + method + "' for type '" + typeName + "'");
    }

    // remove() needs to mutate the original dict — special handling
    if (expr->funcName == "remove") {
        if (expr->args.size() != 2)
            throw RuntimeError("remove() takes exactly 2 arguments (dict, key)");
        // First arg should be an identifier referencing a dict variable
        if (expr->args[0]->kind != ExprKind::Identifier)
            throw RuntimeError("remove() first argument must be a dict variable");
        CpctValue& dictVal = currentEnv_->get(expr->args[0]->name);
        if (!dictVal.isDict())
            throw RuntimeError("remove() first argument must be a dict");
        CpctValue key = eval(expr->args[1].get());
        dictVal.asDict().remove(key);
        return CpctValue(true);
    }

    // push() — append element to vector (mutates in place)
    if (expr->funcName == "push") {
        if (expr->args.size() != 2)
            throw RuntimeError("push() takes exactly 2 arguments (vector, value)");
        if (expr->args[0]->kind != ExprKind::Identifier)
            throw RuntimeError("push() first argument must be a vector variable");
        std::string varName = expr->args[0]->name;
        CpctValue& vecVal = currentEnv_->get(varName);
        if (!vecVal.isArray())
            throw RuntimeError("push() first argument must be a vector");
        std::string type = currentEnv_->getType(varName);
        CpctValue elem = eval(expr->args[1].get());
        if (isVectorType(type)) {
            std::string elemType = vectorElemType(type);
            elem = coerceArrayElement(elemType, std::move(elem), expr->line);
        }
        vecVal.asArray().push_back(std::move(elem));
        return CpctValue(true);
    }

    // pop() — remove and return last element from vector
    if (expr->funcName == "pop") {
        if (expr->args.size() != 1)
            throw RuntimeError("pop() takes exactly 1 argument (vector)");
        if (expr->args[0]->kind != ExprKind::Identifier)
            throw RuntimeError("pop() first argument must be a vector variable");
        CpctValue& vecVal = currentEnv_->get(expr->args[0]->name);
        if (!vecVal.isArray())
            throw RuntimeError("pop() first argument must be a vector");
        auto& arr = vecVal.asArray();
        if (arr.empty())
            throw RuntimeError("pop() on empty vector at line " + std::to_string(expr->line));
        CpctValue last = std::move(arr.back());
        arr.pop_back();
        return last;
    }

    // insert() — insert element at index (mutates in place)
    if (expr->funcName == "insert") {
        if (expr->args.size() != 3)
            throw RuntimeError("insert() takes exactly 3 arguments (vector, index, value)");
        if (expr->args[0]->kind != ExprKind::Identifier)
            throw RuntimeError("insert() first argument must be a vector variable");
        std::string varName = expr->args[0]->name;
        CpctValue& vecVal = currentEnv_->get(varName);
        if (!vecVal.isArray())
            throw RuntimeError("insert() first argument must be a vector");
        CpctValue idxVal = eval(expr->args[1].get());
        if (!idxVal.isInt())
            throw RuntimeError("insert() index must be an integer");
        int64_t idx = idxVal.asInt();
        auto& arr = vecVal.asArray();
        if (idx < 0 || idx > static_cast<int64_t>(arr.size()))
            throw RuntimeError("insert() index out of range at line " + std::to_string(expr->line));
        CpctValue elem = eval(expr->args[2].get());
        std::string type = currentEnv_->getType(varName);
        if (isVectorType(type)) {
            std::string elemType = vectorElemType(type);
            elem = coerceArrayElement(elemType, std::move(elem), expr->line);
        }
        arr.insert(arr.begin() + idx, std::move(elem));
        return CpctValue(true);
    }

    // erase() — remove element at index (mutates in place)
    if (expr->funcName == "erase") {
        if (expr->args.size() != 2)
            throw RuntimeError("erase() takes exactly 2 arguments (vector, index)");
        if (expr->args[0]->kind != ExprKind::Identifier)
            throw RuntimeError("erase() first argument must be a vector variable");
        CpctValue& vecVal = currentEnv_->get(expr->args[0]->name);
        if (!vecVal.isArray())
            throw RuntimeError("erase() first argument must be a vector");
        CpctValue idxVal = eval(expr->args[1].get());
        if (!idxVal.isInt())
            throw RuntimeError("erase() index must be an integer");
        int64_t idx = idxVal.asInt();
        auto& arr = vecVal.asArray();
        if (idx < 0) idx += static_cast<int64_t>(arr.size());
        if (idx < 0 || idx >= static_cast<int64_t>(arr.size()))
            throw RuntimeError("erase() index out of range at line " + std::to_string(expr->line));
        arr.erase(arr.begin() + idx);
        return CpctValue(true);
    }

    // clear() — remove all elements from vector (mutates in place)
    if (expr->funcName == "clear") {
        if (expr->args.size() != 1)
            throw RuntimeError("clear() takes exactly 1 argument (vector)");
        if (expr->args[0]->kind != ExprKind::Identifier)
            throw RuntimeError("clear() first argument must be a vector variable");
        CpctValue& vecVal = currentEnv_->get(expr->args[0]->name);
        if (!vecVal.isArray())
            throw RuntimeError("clear() first argument must be a vector");
        vecVal.asArray().clear();
        return CpctValue(true);
    }

    // empty() — check if vector/array is empty
    if (expr->funcName == "empty") {
        if (expr->args.size() != 1)
            throw RuntimeError("empty() takes exactly 1 argument");
        CpctValue val = eval(expr->args[0].get());
        if (val.isArray()) return CpctValue(val.asArray().empty());
        if (val.isTypedArray()) return CpctValue(val.asTypedArray().size() == 0);
        throw RuntimeError("empty() requires a vector or array argument");
    }

    // front() — return first element
    if (expr->funcName == "front") {
        if (expr->args.size() != 1)
            throw RuntimeError("front() takes exactly 1 argument");
        CpctValue val = eval(expr->args[0].get());
        if (val.isArray()) {
            if (val.asArray().empty())
                throw RuntimeError("front() on empty vector at line " + std::to_string(expr->line));
            return val.asArray().front();
        }
        if (val.isTypedArray()) {
            if (val.asTypedArray().size() == 0)
                throw RuntimeError("front() on empty array at line " + std::to_string(expr->line));
            return val.asTypedArray().get(0);
        }
        throw RuntimeError("front() requires a vector or array argument");
    }

    // back() — return last element
    if (expr->funcName == "back") {
        if (expr->args.size() != 1)
            throw RuntimeError("back() takes exactly 1 argument");
        CpctValue val = eval(expr->args[0].get());
        if (val.isArray()) {
            if (val.asArray().empty())
                throw RuntimeError("back() on empty vector at line " + std::to_string(expr->line));
            return val.asArray().back();
        }
        if (val.isTypedArray()) {
            size_t sz = val.asTypedArray().size();
            if (sz == 0)
                throw RuntimeError("back() on empty array at line " + std::to_string(expr->line));
            return val.asTypedArray().get(sz - 1);
        }
        throw RuntimeError("back() requires a vector or array argument");
    }

    // sort() — in-place array sort
    if (expr->funcName == "sort") {
        if (expr->args.size() < 1 || expr->args.size() > 2)
            throw RuntimeError("sort() takes 1-2 arguments");
        // Parse descending flag
        bool descending = false;
        if (expr->args.size() == 2) {
            CpctValue flag = eval(expr->args[1].get());
            descending = flag.toBool();
        }
        // Get mutable reference to receiver
        const Expr* receiver = expr->args[0].get();
        if (receiver->kind == ExprKind::Identifier) {
            CpctValue& val = currentEnv_->get(receiver->name);
            if (val.isTypedArray()) {
                sortTypedArray(val.asTypedArray(), descending);
                return CpctValue(true);
            }
            if (val.isArray()) {
                auto& vec = val.asArray();
                if (descending)
                    std::sort(vec.begin(), vec.end(), [](const CpctValue& a, const CpctValue& b) { return compareCpctValues(b, a); });
                else
                    std::sort(vec.begin(), vec.end(), compareCpctValues);
                return CpctValue(true);
            }
            throw RuntimeError("sort() requires an array argument");
        }
        // ArrayAccess: e.g. matrix[0].sort()
        if (receiver->kind == ExprKind::ArrayAccess) {
            const Expr* root = receiver;
            while (root->kind == ExprKind::ArrayAccess) root = root->arrayExpr.get();
            if (root->kind != ExprKind::Identifier)
                throw RuntimeError("sort() requires an array variable");
            CpctValue& rootVal = currentEnv_->get(root->name);
            // Walk down to get mutable ref
            std::vector<const Expr*> indices;
            const Expr* node = receiver;
            while (node->kind == ExprKind::ArrayAccess) {
                indices.push_back(node->indexExpr.get());
                node = node->arrayExpr.get();
            }
            std::reverse(indices.begin(), indices.end());
            CpctValue* current = &rootVal;
            for (size_t i = 0; i < indices.size(); i++) {
                current = &resolveArrayElement(*current, indices[i], expr->line);
            }
            if (current->isArray()) {
                auto& vec = current->asArray();
                if (descending)
                    std::sort(vec.begin(), vec.end(), [](const CpctValue& a, const CpctValue& b) { return compareCpctValues(b, a); });
                else
                    std::sort(vec.begin(), vec.end(), compareCpctValues);
                return CpctValue(true);
            }
            throw RuntimeError("sort() requires an array argument");
        }
        throw RuntimeError("sort() first argument must be a variable");
    }

    // sortkey()/sortk() — dict sort by key
    if (expr->funcName == "sortkey" || expr->funcName == "sortk") {
        if (expr->args.size() < 1 || expr->args.size() > 2)
            throw RuntimeError(expr->funcName + "() takes 1-2 arguments");
        bool descending = false;
        if (expr->args.size() == 2) {
            CpctValue flag = eval(expr->args[1].get());
            descending = flag.toBool();
        }
        if (expr->args[0]->kind != ExprKind::Identifier)
            throw RuntimeError(expr->funcName + "() first argument must be a dict variable");
        CpctValue& val = currentEnv_->get(expr->args[0]->name);
        if (!val.isDict())
            throw RuntimeError(expr->funcName + "() requires a dict argument");
        auto& dict = val.asDict();
        if (dict.size() <= 1) return CpctValue(true);
        // Create index permutation sorted by key
        std::vector<size_t> perm(dict.size());
        std::iota(perm.begin(), perm.end(), 0);
        if (descending)
            std::sort(perm.begin(), perm.end(), [&](size_t a, size_t b) { return compareCpctValues(dict.keys[b], dict.keys[a]); });
        else
            std::sort(perm.begin(), perm.end(), [&](size_t a, size_t b) { return compareCpctValues(dict.keys[a], dict.keys[b]); });
        applyDictPermutation(dict, perm);
        return CpctValue(true);
    }

    // sortval()/sortv() — dict sort by value (stable)
    if (expr->funcName == "sortval" || expr->funcName == "sortv") {
        if (expr->args.size() < 1 || expr->args.size() > 2)
            throw RuntimeError(expr->funcName + "() takes 1-2 arguments");
        bool descending = false;
        if (expr->args.size() == 2) {
            CpctValue flag = eval(expr->args[1].get());
            descending = flag.toBool();
        }
        if (expr->args[0]->kind != ExprKind::Identifier)
            throw RuntimeError(expr->funcName + "() first argument must be a dict variable");
        CpctValue& val = currentEnv_->get(expr->args[0]->name);
        if (!val.isDict())
            throw RuntimeError(expr->funcName + "() requires a dict argument");
        auto& dict = val.asDict();
        if (dict.size() <= 1) return CpctValue(true);
        std::vector<size_t> perm(dict.size());
        std::iota(perm.begin(), perm.end(), 0);
        if (descending)
            std::stable_sort(perm.begin(), perm.end(), [&](size_t a, size_t b) { return compareCpctValues(dict.values[b], dict.values[a]); });
        else
            std::stable_sort(perm.begin(), perm.end(), [&](size_t a, size_t b) { return compareCpctValues(dict.values[a], dict.values[b]); });
        applyDictPermutation(dict, perm);
        return CpctValue(true);
    }

    // sortvk() — dict sort by value, then key (deterministic)
    if (expr->funcName == "sortvk") {
        if (expr->args.size() < 1 || expr->args.size() > 2)
            throw RuntimeError("sortvk() takes 1-2 arguments");
        bool descending = false;
        if (expr->args.size() == 2) {
            CpctValue flag = eval(expr->args[1].get());
            descending = flag.toBool();
        }
        if (expr->args[0]->kind != ExprKind::Identifier)
            throw RuntimeError("sortvk() first argument must be a dict variable");
        CpctValue& val = currentEnv_->get(expr->args[0]->name);
        if (!val.isDict())
            throw RuntimeError("sortvk() requires a dict argument");
        auto& dict = val.asDict();
        if (dict.size() <= 1) return CpctValue(true);
        std::vector<size_t> perm(dict.size());
        std::iota(perm.begin(), perm.end(), 0);
        if (descending) {
            std::sort(perm.begin(), perm.end(), [&](size_t a, size_t b) {
                if (compareCpctValues(dict.values[b], dict.values[a])) return true;
                if (compareCpctValues(dict.values[a], dict.values[b])) return false;
                return compareCpctValues(dict.keys[b], dict.keys[a]); // secondary: key descending
            });
        } else {
            std::sort(perm.begin(), perm.end(), [&](size_t a, size_t b) {
                if (compareCpctValues(dict.values[a], dict.values[b])) return true;
                if (compareCpctValues(dict.values[b], dict.values[a])) return false;
                return compareCpctValues(dict.keys[a], dict.keys[b]); // secondary: key ascending
            });
        }
        applyDictPermutation(dict, perm);
        return CpctValue(true);
    }

    // type() and size() need access to declared type info
    if (expr->funcName == "type") {
        if (expr->args.size() != 1) throw RuntimeError("type() takes exactly 1 argument");
        CpctValue val = eval(expr->args[0].get());
        // Use declared type if available (returns "int[]", "float32[][]", etc.)
        if (expr->args[0]->kind == ExprKind::Identifier) {
            std::string typeName = currentEnv_->getType(expr->args[0]->name);
            if (!typeName.empty()) return CpctValue(typeName);
        }
        // Infer from runtime value
        if (val.isAnyArray()) return CpctValue(std::string("array"));
        if (val.isDict()) return CpctValue(std::string("dict"));
        std::string typeName;
        if (val.isInt()) typeName = "int";
        else if (val.isBigInt()) typeName = "bigint";
        else if (val.isFloat()) typeName = "float";
        else if (val.isBool()) typeName = "bool";
        else if (val.isString()) typeName = "string";
        else typeName = "unknown";
        return CpctValue(typeName);
    }
    if (expr->funcName == "size") {
        if (expr->args.size() != 1) throw RuntimeError("size() takes exactly 1 argument");
        CpctValue val = eval(expr->args[0].get());
        std::string typeName;
        if (expr->args[0]->kind == ExprKind::Identifier) {
            typeName = currentEnv_->getType(expr->args[0]->name);
        }

        // Vector: return element count (like C++ vector::size())
        if (!typeName.empty() && isVectorType(typeName)) {
            if (val.isArray()) {
                return CpctValue(static_cast<int64_t>(val.asArray().size()));
            }
            return CpctValue(static_cast<int64_t>(0));
        }

        // For typed arrays, use real memory size directly
        if (!typeName.empty() && isArrayType(typeName)) {
            if (val.isTypedArray()) {
                return CpctValue(static_cast<int64_t>(val.asTypedArray().totalBytes()));
            }
            // Generic array: calculate based on base type and element count
            std::string base = arrayBaseType(typeName);
            int64_t elemSz = 0;
            if (base == "bool" || base == "int8" || base == "char") elemSz = 1;
            else if (base == "int16") elemSz = 2;
            else if (base == "int" || base == "int32" || base == "float32") elemSz = 4;
            else if (base == "int64" || base == "float64" || base == "float") elemSz = 8;
            // intbig[]/bigint[] use generic vector<CpctValue>, size varies per element

            if (elemSz > 0 && val.isArray()) {
                std::function<int64_t(const CpctValue&)> countLeaves = [&](const CpctValue& v) -> int64_t {
                    if (!v.isArray()) return 1;
                    int64_t count = 0;
                    for (auto& e : v.asArray()) count += countLeaves(e);
                    return count;
                };
                return CpctValue(countLeaves(val) * elemSz);
            }
        }

        // Dict/Map: sum of key bytes + value bytes (native sizes)
        if (val.isDict()) {
            auto& dict = val.asDict();
            int64_t bytes = 0;
            auto elemBytes = [](const std::string& t, const CpctValue& v) -> int64_t {
                // If type is known, use it
                if (t == "bool") return 1;
                if (t == "int8" || t == "char") return 1;
                if (t == "int16") return 2;
                if (t == "int" || t == "int32" || t == "float32") return 4;
                if (t == "int64" || t == "float64" || t == "float") return 8;
                if (t == "string") return static_cast<int64_t>(v.isString() ? v.asString().size() : 0);
                if (t == "intbig" || t == "bigint") return v.isBigInt() ? static_cast<int64_t>(v.asBigInt().byteCount()) : 8;
                // Untyped (empty type) — infer from runtime value
                if (t.empty()) {
                    if (v.isBool()) return 1;
                    if (v.isInt()) return 8;
                    if (v.isFloat()) return 8;
                    if (v.isString()) return static_cast<int64_t>(v.asString().size());
                    if (v.isBigInt()) return static_cast<int64_t>(v.asBigInt().byteCount());
                    return 8;
                }
                return 8; // fallback
            };
            for (size_t i = 0; i < dict.size(); i++) {
                bytes += elemBytes(dict.keyType, dict.keys[i]);
                bytes += elemBytes(dict.valueType, dict.values[i]);
            }
            return CpctValue(bytes);
        }

        int64_t bytes = 0;
        if (!typeName.empty() && !isArrayType(typeName)) {
            if (typeName == "bool") bytes = 1;
            else if (typeName == "int8" || typeName == "char") bytes = 1;
            else if (typeName == "int16") bytes = 2;
            else if (typeName == "int" || typeName == "int32" || typeName == "float32") bytes = 4;
            else if (typeName == "int64" || typeName == "float64" || typeName == "float") bytes = 8;
            else if (typeName == "intbig" || typeName == "bigint") {
                bytes = val.isBigInt() ? static_cast<int64_t>(val.asBigInt().byteCount()) : 8;
            }
            else if (typeName == "string") bytes = static_cast<int64_t>(val.asString().size());
        } else if (typeName.empty()) {
            if (val.isBool()) bytes = 1;
            else if (val.isInt()) bytes = 8;
            else if (val.isBigInt()) bytes = static_cast<int64_t>(val.asBigInt().byteCount());
            else if (val.isFloat()) bytes = 8;
            else if (val.isString()) bytes = static_cast<int64_t>(val.asString().size());
            else if (val.isArray()) {
                for (auto& elem : val.asArray()) {
                    if (elem.isInt()) bytes += 8;
                    else if (elem.isBigInt()) bytes += static_cast<int64_t>(elem.asBigInt().byteCount());
                    else if (elem.isFloat()) bytes += 8;
                    else if (elem.isBool()) bytes += 1;
                    else if (elem.isString()) bytes += static_cast<int64_t>(elem.asString().size());
                }
            }
        }
        return CpctValue(bytes);
    }

    std::vector<CpctValue> args;
    for (auto& arg : expr->args) {
        args.push_back(eval(arg.get()));
    }
    return callFunction(expr->funcName, args, expr->line);
}

CpctValue Interpreter::evalArrayAccess(const Expr* expr) {
    CpctValue arr = eval(expr->arrayExpr.get());
    CpctValue idx = eval(expr->indexExpr.get());

    // Dict/Map access: d["key"]
    if (arr.isDict()) {
        auto& dict = arr.asDict();
        if (!dict.has(idx))
            throw RuntimeError("Key not found in dict at line " + std::to_string(expr->line));
        return dict.get(idx);
    }

    if (!arr.isAnyArray()) throw RuntimeError("Cannot index non-array/dict value at line " + std::to_string(expr->line));
    if (!idx.isInt()) throw RuntimeError("Array index must be an integer at line " + std::to_string(expr->line));

    int64_t i = idx.asInt();
    int64_t size = arr.isTypedArray() ? static_cast<int64_t>(arr.asTypedArray().size())
                                      : static_cast<int64_t>(arr.asArray().size());

    if (i < 0) i += size;
    if (i < 0 || i >= size) {
        throw RuntimeError("Array index out of bounds: " + std::to_string(idx.asInt()) +
                          " (size: " + std::to_string(size) + ") at line " + std::to_string(expr->line));
    }

    if (arr.isTypedArray()) return arr.asTypedArray().get(static_cast<size_t>(i));
    return arr.asArray()[i];
}

CpctValue Interpreter::evalArrayAssign(const Expr* expr) {
    CpctValue val = eval(expr->right.get());

    // Find the root variable name for type enforcement
    const Expr* root = expr->arrayExpr.get();
    std::string varName;
    // Walk down to find the identifier
    const Expr* cur = root;
    while (cur->kind == ExprKind::ArrayAccess) {
        cur = cur->arrayExpr.get();
    }
    if (cur->kind == ExprKind::Identifier) {
        varName = cur->name;
    }

    // Get base type for coercion
    if (!varName.empty()) {
        std::string type = currentEnv_->getType(varName);
        if (isArrayType(type)) {
            std::string baseType = arrayBaseType(type);
            val = coerceArrayElement(baseType, std::move(val), expr->line);
        } else if (isVectorType(type)) {
            std::string elemType = vectorElemType(type);
            val = coerceArrayElement(elemType, std::move(val), expr->line);
        }
    }

    CpctValue& rootVal = currentEnv_->get(varName);

    // Dict assign: d["key"] = value
    if (rootVal.isDict()) {
        auto& dict = rootVal.asDict();
        CpctValue key = eval(expr->indexExpr.get());
        key = coerceDictKey(dict.keyType, std::move(key), expr->line);
        val = coerceDictValue(dict.valueType, std::move(val), expr->line);
        dict.set(key, val);
        return val;
    }

    // TypedArray: direct single-index access
    if (rootVal.isTypedArray()) {
        CpctValue idx = eval(expr->indexExpr.get());
        if (!idx.isInt()) throw RuntimeError("Array index must be an integer at line " + std::to_string(expr->line));
        int64_t i = idx.asInt();
        auto& ta = rootVal.asTypedArray();
        int64_t size = static_cast<int64_t>(ta.size());
        if (i < 0) i += size;
        if (i < 0 || i >= size) {
            throw RuntimeError("Array index out of bounds: " + std::to_string(idx.asInt()) +
                              " (size: " + std::to_string(size) + ") at line " + std::to_string(expr->line));
        }
        ta.set(static_cast<size_t>(i), val);
        return val;
    }

    // Generic array: collect index expressions from root to leaf
    std::vector<const Expr*> indices;
    const Expr* node = expr->arrayExpr.get();
    while (node->kind == ExprKind::ArrayAccess) {
        indices.push_back(node->indexExpr.get());
        node = node->arrayExpr.get();
    }
    std::reverse(indices.begin(), indices.end());
    indices.push_back(expr->indexExpr.get());

    CpctValue* current = &rootVal;
    for (size_t i = 0; i < indices.size(); i++) {
        current = &resolveArrayElement(*current, indices[i], expr->line);
    }
    *current = std::move(val);
    return *current;
}

CpctValue Interpreter::evalArrayCompoundAssign(const Expr* expr) {
    // Find the root variable name
    const Expr* cur = expr->arrayExpr.get();
    std::string varName;
    while (cur->kind == ExprKind::ArrayAccess) {
        cur = cur->arrayExpr.get();
    }
    if (cur->kind == ExprKind::Identifier) varName = cur->name;

    CpctValue& rootVal = currentEnv_->get(varName);

    // Dict compound assign: d["key"] += value
    if (rootVal.isDict()) {
        CpctValue key = eval(expr->indexExpr.get());
        key = coerceDictKey(rootVal.asDict().keyType, std::move(key), expr->line);
        auto& dict = rootVal.asDict();
        if (!dict.has(key))
            throw RuntimeError("Key not found in dict at line " + std::to_string(expr->line));
        CpctValue& elemRef = dict.get(key);
        CpctValue right = eval(expr->right.get());
        // Create a binary op expression to reuse arithmetic
        auto binExpr = makeBinaryOp(expr->op, nullptr, nullptr, expr->line);
        // We need to compute elemRef op right manually
        CpctValue lhs = elemRef;
        // Use evalBinaryOp logic inline — just compute the result
        // For simplicity, handle int/float cases directly
        if (lhs.isInt() && right.isInt()) {
            int64_t l = lhs.asInt(), r = right.asInt();
            int64_t result = 0;
            switch (expr->op) {
                case TokenType::PLUS_ASSIGN: result = l + r; break;
                case TokenType::MINUS_ASSIGN: result = l - r; break;
                case TokenType::STAR_ASSIGN: result = l * r; break;
                case TokenType::SLASH_ASSIGN: result = r != 0 ? l / r : throw RuntimeError("Division by zero"); break;
                case TokenType::PERCENT_ASSIGN: result = r != 0 ? l % r : throw RuntimeError("Modulo by zero"); break;
                default: throw RuntimeError("Unknown compound assign op");
            }
            elemRef = CpctValue(result);
        } else if ((lhs.isInt() || lhs.isFloat()) && (right.isInt() || right.isFloat())) {
            double l = lhs.toNumber(), r = right.toNumber();
            double result = 0;
            switch (expr->op) {
                case TokenType::PLUS_ASSIGN: result = l + r; break;
                case TokenType::MINUS_ASSIGN: result = l - r; break;
                case TokenType::STAR_ASSIGN: result = l * r; break;
                case TokenType::SLASH_ASSIGN: result = r != 0 ? l / r : throw RuntimeError("Division by zero"); break;
                default: throw RuntimeError("Unknown compound assign op");
            }
            elemRef = CpctValue(result);
        } else if (lhs.isString() && expr->op == TokenType::PLUS_ASSIGN) {
            elemRef = CpctValue(lhs.asString() + right.toString());
        } else {
            throw RuntimeError("Unsupported compound assign operation on dict value at line " + std::to_string(expr->line));
        }
        elemRef = coerceDictValue(dict.valueType, std::move(elemRef), expr->line);
        return elemRef;
    }

    // TypedArray: direct single-index access
    if (rootVal.isTypedArray()) {
        CpctValue idx = eval(expr->indexExpr.get());
        if (!idx.isInt()) throw RuntimeError("Array index must be an integer at line " + std::to_string(expr->line));
        int64_t i = idx.asInt();
        auto& ta = rootVal.asTypedArray();
        int64_t sz = static_cast<int64_t>(ta.size());
        if (i < 0) i += sz;
        if (i < 0 || i >= sz)
            throw RuntimeError("Array index out of bounds at line " + std::to_string(expr->line));

        CpctValue elemVal = ta.get(static_cast<size_t>(i));
        CpctValue right = eval(expr->right.get());
        CpctValue result;
        // Compute (reuse numeric logic inline)
        if (elemVal.isNumericInt() && right.isNumericInt() && elemVal.isInt() && right.isInt()) {
            int64_t l = elemVal.asInt(), r = right.asInt(); int64_t res;
            switch (expr->op) {
                case TokenType::PLUS_ASSIGN: if (!__builtin_add_overflow(l,r,&res)){result=CpctValue(res);break;} result=CpctValue(BigInt(l)+BigInt(r));break;
                case TokenType::MINUS_ASSIGN: if (!__builtin_sub_overflow(l,r,&res)){result=CpctValue(res);break;} result=CpctValue(BigInt(l)-BigInt(r));break;
                case TokenType::STAR_ASSIGN: if (!__builtin_mul_overflow(l,r,&res)){result=CpctValue(res);break;} result=CpctValue(BigInt(l)*BigInt(r));break;
                case TokenType::SLASH_ASSIGN: if(r==0)throw RuntimeError("Division by zero");result=CpctValue(l/r);break;
                case TokenType::PERCENT_ASSIGN: if(r==0)throw RuntimeError("Division by zero");result=CpctValue(l%r);break;
                default: throw RuntimeError("Unknown compound operator");
            }
        } else {
            double l = elemVal.toNumber(), r = right.toNumber();
            switch (expr->op) {
                case TokenType::PLUS_ASSIGN: result=CpctValue(l+r);break;
                case TokenType::MINUS_ASSIGN: result=CpctValue(l-r);break;
                case TokenType::STAR_ASSIGN: result=CpctValue(l*r);break;
                case TokenType::SLASH_ASSIGN: if(r==0.0)throw RuntimeError("Division by zero");result=CpctValue(l/r);break;
                case TokenType::PERCENT_ASSIGN: if(r==0.0)throw RuntimeError("Division by zero");result=CpctValue(std::fmod(l,r));break;
                default: throw RuntimeError("Unknown compound operator");
            }
        }
        std::string type = currentEnv_->getType(varName);
        if (isArrayType(type)) {
            result = coerceArrayElement(arrayBaseType(type), std::move(result), expr->line);
        }
        ta.set(static_cast<size_t>(i), result);
        return result;
    }

    // Generic array: resolve element reference
    std::vector<const Expr*> indices;
    const Expr* node = expr->arrayExpr.get();
    while (node->kind == ExprKind::ArrayAccess) {
        indices.push_back(node->indexExpr.get());
        node = node->arrayExpr.get();
    }
    std::reverse(indices.begin(), indices.end());
    indices.push_back(expr->indexExpr.get());

    CpctValue* elemPtr = &rootVal;
    for (size_t i = 0; i < indices.size(); i++) {
        elemPtr = &resolveArrayElement(*elemPtr, indices[i], expr->line);
    }

    CpctValue& elem = *elemPtr;
    CpctValue right = eval(expr->right.get());

    // Compute result (similar to evalCompoundAssign)
    CpctValue result;
    if (elem.isNumericInt() && right.isNumericInt()) {
        if (elem.isInt() && right.isInt()) {
            int64_t l = elem.asInt(), r = right.asInt();
            int64_t res;
            switch (expr->op) {
                case TokenType::PLUS_ASSIGN:
                    if (!__builtin_add_overflow(l, r, &res)) { result = CpctValue(res); break; }
                    result = CpctValue(BigInt(l) + BigInt(r)); break;
                case TokenType::MINUS_ASSIGN:
                    if (!__builtin_sub_overflow(l, r, &res)) { result = CpctValue(res); break; }
                    result = CpctValue(BigInt(l) - BigInt(r)); break;
                case TokenType::STAR_ASSIGN:
                    if (!__builtin_mul_overflow(l, r, &res)) { result = CpctValue(res); break; }
                    result = CpctValue(BigInt(l) * BigInt(r)); break;
                case TokenType::SLASH_ASSIGN:
                    if (r == 0) throw RuntimeError("Division by zero");
                    result = CpctValue(l / r); break;
                case TokenType::PERCENT_ASSIGN:
                    if (r == 0) throw RuntimeError("Division by zero");
                    result = CpctValue(l % r); break;
                default: throw RuntimeError("Unknown compound operator");
            }
        } else {
            BigInt l = elem.toBigInt(), r = right.toBigInt();
            switch (expr->op) {
                case TokenType::PLUS_ASSIGN:  result = CpctValue(l + r); break;
                case TokenType::MINUS_ASSIGN: result = CpctValue(l - r); break;
                case TokenType::STAR_ASSIGN:  result = CpctValue(l * r); break;
                case TokenType::SLASH_ASSIGN:
                    if (r.isZero()) throw RuntimeError("Division by zero");
                    result = CpctValue(l / r); break;
                case TokenType::PERCENT_ASSIGN:
                    if (r.isZero()) throw RuntimeError("Division by zero");
                    result = CpctValue(l % r); break;
                default: throw RuntimeError("Unknown compound operator");
            }
        }
    } else {
        double l = elem.toNumber(), r = right.toNumber();
        switch (expr->op) {
            case TokenType::PLUS_ASSIGN:  result = CpctValue(l + r); break;
            case TokenType::MINUS_ASSIGN: result = CpctValue(l - r); break;
            case TokenType::STAR_ASSIGN:  result = CpctValue(l * r); break;
            case TokenType::SLASH_ASSIGN:
                if (r == 0.0) throw RuntimeError("Division by zero");
                result = CpctValue(l / r); break;
            case TokenType::PERCENT_ASSIGN:
                if (r == 0.0) throw RuntimeError("Division by zero");
                result = CpctValue(std::fmod(l, r)); break;
            default: throw RuntimeError("Unknown compound operator");
        }
    }

    // Type coercion for array element
    if (!varName.empty()) {
        std::string type = currentEnv_->getType(varName);
        if (isArrayType(type)) {
            std::string baseType = arrayBaseType(type);
            result = coerceArrayElement(baseType, std::move(result), expr->line);
        } else if (isVectorType(type)) {
            std::string elemType = vectorElemType(type);
            result = coerceArrayElement(elemType, std::move(result), expr->line);
        }
    }

    elem = std::move(result);
    return elem;
}

CpctValue Interpreter::evalArraySlice(const Expr* expr) {
    CpctValue arr = eval(expr->arrayExpr.get());
    if (!arr.isAnyArray())
        throw RuntimeError("Cannot slice non-array value at line " + std::to_string(expr->line));

    CpctValue startVal = eval(expr->indexExpr.get());
    CpctValue endVal = eval(expr->endIndexExpr.get());

    if (!startVal.isInt() || !endVal.isInt())
        throw RuntimeError("Slice indices must be integers at line " + std::to_string(expr->line));

    int64_t size = arr.isTypedArray() ? static_cast<int64_t>(arr.asTypedArray().size())
                                      : static_cast<int64_t>(arr.asArray().size());
    int64_t start = startVal.asInt();
    int64_t end = endVal.asInt();

    if (start < 0) start += size;
    if (end < 0) end += size;
    if (start < 0) start = 0;
    if (start > size) start = size;
    if (end < 0) end = 0;
    if (end > size) end = size;

    std::vector<CpctValue> result;
    if (start < end) {
        if (arr.isTypedArray()) {
            auto& ta = arr.asTypedArray();
            for (int64_t i = start; i < end; i++)
                result.push_back(ta.get(static_cast<size_t>(i)));
        } else {
            auto& vec = arr.asArray();
            for (int64_t i = start; i < end; i++)
                result.push_back(vec[i]);
        }
    }
    return CpctValue(std::move(result));
}

CpctValue Interpreter::callFunction(const std::string& name, const std::vector<CpctValue>& args, int line) {
    // Built-in functions
    if (name == "len") {
        if (args.size() != 1) throw RuntimeError("len() takes exactly 1 argument");
        if (args[0].isArray()) return CpctValue(static_cast<int64_t>(args[0].asArray().size()));
        if (args[0].isTypedArray()) return CpctValue(static_cast<int64_t>(args[0].asTypedArray().size()));
        if (args[0].isString()) return CpctValue(static_cast<int64_t>(args[0].asString().size()));
        if (args[0].isDict()) return CpctValue(static_cast<int64_t>(args[0].asDict().size()));
        throw RuntimeError("len() requires array, string, or dict argument");
    }

    // shape() — returns array of dimension sizes
    if (name == "shape") {
        if (args.size() != 1) throw RuntimeError("shape() takes exactly 1 argument");
        std::vector<CpctValue> dims;
        if (args[0].isTypedArray()) {
            dims.push_back(CpctValue(static_cast<int64_t>(args[0].asTypedArray().size())));
            return CpctValue(std::move(dims));
        }
        if (args[0].isArray()) {
            // Walk first element of each dimension
            const CpctValue* cur = &args[0];
            while (cur->isArray()) {
                dims.push_back(CpctValue(static_cast<int64_t>(cur->asArray().size())));
                if (cur->asArray().empty()) break;
                cur = &cur->asArray()[0];
            }
            return CpctValue(std::move(dims));
        }
        throw RuntimeError("shape() requires an array argument");
    }

    // Dict builtins: keys(), values(), has()
    if (name == "keys") {
        if (args.size() != 1 || !args[0].isDict()) throw RuntimeError("keys() requires a dict argument");
        auto& dict = args[0].asDict();
        std::vector<CpctValue> result(dict.keys.begin(), dict.keys.end());
        return CpctValue(std::move(result));
    }
    if (name == "values") {
        if (args.size() != 1 || !args[0].isDict()) throw RuntimeError("values() requires a dict argument");
        auto& dict = args[0].asDict();
        std::vector<CpctValue> result(dict.values.begin(), dict.values.end());
        return CpctValue(std::move(result));
    }
    if (name == "has") {
        if (args.size() != 2 || !args[0].isDict()) throw RuntimeError("has() requires a dict and a key argument");
        auto& dict = args[0].asDict();
        return CpctValue(dict.has(args[1]));
    }
    if (name == "remove") {
        if (args.size() != 2 || !args[0].isDict()) throw RuntimeError("remove() requires a dict and a key argument");
        // Note: args[0] is a copy, so we can't modify the original. This needs special handling.
        throw RuntimeError("remove() must be called as method: d.remove(key)");
    }
    // Type cast functions: int(x), int8(x), float32(x), string(x), char(x), bool(x), etc.
    if (isIntegerType(name) || isFloatType(name) || isCharType(name) ||
        name == "string" || name == "bool") {
        if (args.size() != 1) throw RuntimeError(name + "() takes exactly 1 argument");
        const CpctValue& arg = args[0];

        // Integer casts: int(), int8(), int16(), int32(), int64(), intbig(), bigint()
        if (isIntegerType(name)) {
            int64_t val = 0;
            if (arg.isInt()) val = arg.asInt();
            else if (arg.isBigInt()) {
                if (isDynamicIntType(name)) return arg; // intbig/bigint keep BigInt
                val = arg.asBigInt().toInt64(); // may truncate for sized types
            }
            else if (arg.isFloat()) val = static_cast<int64_t>(arg.asFloat());
            else if (arg.isBool()) val = arg.asBool() ? 1 : 0;
            else if (arg.isString()) {
                try {
                    BigInt b(arg.asString());
                    if (isDynamicIntType(name)) return CpctValue(std::move(b));
                    if (b.fitsInt64()) val = b.toInt64();
                    else throw RuntimeError("String value too large for " + name);
                }
                catch (const RuntimeError&) { throw; }
                catch (...) { throw RuntimeError("Cannot convert string to " + name); }
            }
            else throw RuntimeError("Cannot convert to " + name);

            if (name == "bigint") return CpctValue(BigInt(val));
            if (name == "intbig") {
                // intbig starts at int64, no range check
                return CpctValue(val);
            }
            return checkIntRange(name, CpctValue(val), line);
        }

        // Float casts: float(), float32(), float64()
        if (isFloatType(name)) {
            double val = arg.toNumber();
            return coerceFloat(name, CpctValue(val), line);
        }

        // char()
        if (isCharType(name)) {
            if (arg.isInt()) return checkIntRange("char", CpctValue(arg.asInt()), line);
            if (arg.isFloat()) return checkIntRange("char", CpctValue(static_cast<int64_t>(arg.asFloat())), line);
            if (arg.isString() && !arg.asString().empty())
                return CpctValue(static_cast<int64_t>(static_cast<int8_t>(arg.asString()[0])));
            if (arg.isBool()) return CpctValue(int64_t(arg.asBool() ? 1 : 0));
            throw RuntimeError("Cannot convert to char");
        }

        // string()
        if (name == "string") {
            return CpctValue(arg.toString());
        }

        // bool()
        if (name == "bool") {
            return CpctValue(arg.toBool());
        }
    }
    if (name == "divmod") {
        if (args.size() != 2) throw RuntimeError("divmod() takes exactly 2 arguments");
        if (args[0].isInt() && args[1].isInt()) {
            int64_t a = args[0].asInt(), b = args[1].asInt();
            if (b == 0) throw RuntimeError("Division by zero in divmod()");
            std::vector<CpctValue> result = { CpctValue(a / b), CpctValue(a % b) };
            return CpctValue(std::move(result));
        }
        if (args[0].isBigInt() || args[1].isBigInt()) {
            BigInt a = args[0].isBigInt() ? args[0].asBigInt() : BigInt(args[0].asInt());
            BigInt b = args[1].isBigInt() ? args[1].asBigInt() : BigInt(args[1].asInt());
            if (b.isZero()) throw RuntimeError("Division by zero in divmod()");
            std::vector<CpctValue> result = { CpctValue(a / b), CpctValue(a % b) };
            return CpctValue(std::move(result));
        }
        if (args[0].isFloat() || args[1].isFloat()) {
            double a = args[0].toNumber(), b = args[1].toNumber();
            if (b == 0.0) throw RuntimeError("Division by zero in divmod()");
            double q = std::trunc(a / b);
            double r = std::fmod(a, b);
            std::vector<CpctValue> result = { CpctValue(q), CpctValue(r) };
            return CpctValue(std::move(result));
        }
        throw RuntimeError("divmod() requires numeric arguments");
    }
    // User-defined functions
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        throw RuntimeError("Undefined function '" + name + "' at line " + std::to_string(line));
    }

    FuncDef& func = it->second;
    if (args.size() != func.params.size()) {
        throw RuntimeError("Function '" + name + "' expects " + std::to_string(func.params.size()) +
                          " arguments, got " + std::to_string(args.size()) +
                          " at line " + std::to_string(line));
    }

    // Create function scope
    Environment funcEnv(&globalEnv_);
    for (size_t i = 0; i < func.params.size(); i++) {
        funcEnv.define(func.params[i].name, args[i]);
    }

    Environment* prev = currentEnv_;
    currentEnv_ = &funcEnv;

    CpctValue result;
    try {
        for (auto* s : func.body) {
            execStmt(s);
        }
    } catch (ReturnSignal& ret) {
        result = std::move(ret.value);
    }

    currentEnv_ = prev;
    return result;
}

CpctValue Interpreter::checkIntRange(const std::string& typeName, CpctValue val, int line) {
    // 'intbig'/'bigint' types are dynamic (BigInt), no range check needed
    if (isDynamicIntType(typeName)) return val;

    // Convert to int64 if needed for sized types
    int64_t v;
    if (val.isInt()) {
        v = val.asInt();
    } else if (val.isBigInt()) {
        // BigInt → wrap into sized type range
        // Use modular reduction: get remainder within range width
        BigInt bv = val.asBigInt();
        auto& info = getIntTypeInfo(typeName);
        uint64_t range = static_cast<uint64_t>(info.maxVal) - static_cast<uint64_t>(info.minVal) + 1;
        BigInt rangeBI(static_cast<int64_t>(range));
        BigInt rem = bv % rangeBI;
        // rem is now in (-range, range), convert to int64
        v = rem.fitsInt64() ? rem.toInt64() : 0;
        // Wrap into [minVal, maxVal]
        if (v > info.maxVal) v -= static_cast<int64_t>(range);
        else if (v < info.minVal) v += static_cast<int64_t>(range);
        return CpctValue(v);
    } else if (val.isFloat()) {
        v = static_cast<int64_t>(val.asFloat());
    } else if (val.isBool()) {
        v = val.asBool() ? 1 : 0;
    } else {
        throw RuntimeError("Cannot assign non-numeric value to " + typeName +
                          " at line " + std::to_string(line));
    }

    // Wrap-around for sized integer types (like C/C++)
    auto& info = getIntTypeInfo(typeName);
    if (v < info.minVal || v > info.maxVal) {
        uint64_t range = static_cast<uint64_t>(info.maxVal) - static_cast<uint64_t>(info.minVal) + 1;
        // Normalize into [0, range) then shift to [minVal, maxVal]
        int64_t offset = v - info.minVal;
        // Use unsigned modulo to handle negative values correctly
        uint64_t uoffset = static_cast<uint64_t>(offset) % range;
        v = static_cast<int64_t>(uoffset) + info.minVal;
    }
    return CpctValue(v);
}

CpctValue Interpreter::makeDefaultValue(const std::string& typeName) {
    if (isIntegerType(typeName) || isCharType(typeName)) return CpctValue(int64_t(0));
    if (isFloatType(typeName)) return CpctValue(0.0);
    if (typeName == "bool") return CpctValue(false);
    if (typeName == "string") return CpctValue(std::string(""));
    return CpctValue(int64_t(0));
}

CpctValue Interpreter::coerceArrayElement(const std::string& baseType, CpctValue val, int line) {
    if (isSizedIntegerType(baseType) || isCharType(baseType)) {
        return checkIntRange(baseType, std::move(val), line);
    }
    if (isFloatType(baseType)) {
        return coerceFloat(baseType, std::move(val), line);
    }
    if (isIntegerType(baseType)) {
        // intbig/bigint — convert float to int (truncation), no range limit
        if (val.isFloat()) return CpctValue(static_cast<int64_t>(val.asFloat()));
    }
    if (baseType == "bool") {
        return CpctValue(val.toBool());
    }
    if (baseType == "string") {
        if (!val.isString()) throw RuntimeError("Cannot convert to string at line " + std::to_string(line));
    }
    return val;
}

CpctValue Interpreter::coerceDictKey(const std::string& keyType, CpctValue key, int line) {
    // Untyped dict — no coercion
    if (keyType.empty()) return key;
    if (keyType == "string") {
        if (key.isString()) return key;
        if (key.isInt()) return CpctValue(std::to_string(key.asInt()));
        if (key.isBool()) return CpctValue(std::string(key.asBool() ? "true" : "false"));
        if (key.isFloat()) return CpctValue(std::to_string(key.asFloat()));
        throw RuntimeError("Cannot convert to string dict key at line " + std::to_string(line));
    }
    if (keyType == "int" || keyType == "int32") {
        if (key.isInt()) return CpctValue(key.asInt());
        if (key.isBool()) return CpctValue(static_cast<int64_t>(key.asBool()));
        throw RuntimeError("Cannot convert to int dict key at line " + std::to_string(line));
    }
    if (keyType == "char") {
        if (key.isInt()) return CpctValue(key.asInt());
        if (key.isString() && !key.asString().empty()) return CpctValue(static_cast<int64_t>(key.asString()[0]));
        throw RuntimeError("Cannot convert to char dict key at line " + std::to_string(line));
    }
    if (keyType == "bool") {
        return CpctValue(key.toBool());
    }
    // int8~int64 etc.
    if (isSizedIntegerType(keyType)) {
        return checkIntRange(keyType, std::move(key), line);
    }
    return key;
}

CpctValue Interpreter::coerceDictValue(const std::string& valType, CpctValue val, int line) {
    // Untyped dict — no coercion
    if (valType.empty()) return val;
    return coerceArrayElement(valType, std::move(val), line);
}

CpctValue& Interpreter::resolveArrayElement(CpctValue& arr, const Expr* indexExpr, int line) {
    CpctValue idx = eval(indexExpr);
    if (!arr.isArray()) throw RuntimeError("Cannot index non-array value at line " + std::to_string(line));
    if (!idx.isInt()) throw RuntimeError("Array index must be an integer at line " + std::to_string(line));

    int64_t i = idx.asInt();
    auto& vec = arr.asArray();
    int64_t size = static_cast<int64_t>(vec.size());

    // Negative index support
    if (i < 0) i += size;
    if (i < 0 || i >= size) {
        throw RuntimeError("Array index out of bounds: " + std::to_string(idx.asInt()) +
                          " (size: " + std::to_string(size) + ") at line " + std::to_string(line));
    }
    return vec[i];
}

CpctValue Interpreter::coerceFloat(const std::string& typeName, CpctValue val, int line) {
    double d;
    if (val.isFloat()) d = val.asFloat();
    else if (val.isInt()) d = static_cast<double>(val.asInt());
    else if (val.isBigInt()) d = val.asBigInt().toDouble();
    else if (val.isBool()) d = val.asBool() ? 1.0 : 0.0;
    else {
        throw RuntimeError("Cannot assign non-numeric value to " + typeName +
                          " at line " + std::to_string(line));
    }

    // float32: truncate to single precision
    if (typeName == "float32") {
        d = static_cast<double>(static_cast<float>(d));
    }
    // float / float64: keep double precision (no-op)
    return CpctValue(d);
}
