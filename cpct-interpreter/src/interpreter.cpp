// interpreter.cpp
// Tree-walking interpreter implementation — directly traverses the AST to execute the program.
//
// Main sections:
//   CpctDict        — dictionary/map lookup, insert, delete (swap-remove technique)
//   TypedArray       — type-specialized array element get/set
//   Sort helpers     — array/dictionary sort helpers
//   CpctValue        — toNumber/toBool/toString conversions
//   Environment      — scope chain variable management
//   Interpreter      — statement execution (exec*) and expression evaluation (eval*)
//   evalBinaryOp     — fast path: int64 → uint64 → mixed → BigInt → float
//   evalFunctionCall — built-in functions (push/pop/sort/type/size, etc.) + type casts + user functions
//   checkIntRange    — C-style integer wrapping (modular wrap-around on overflow)
//   coerce*          — type coercion helpers
#include "interpreter.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <cctype>

// ============== CpctDict (dictionary/map operations) ==============

// Tagged hash for untyped dict — type prefix prevents "1" (string) vs 1 (int) collision
std::string CpctDict::taggedHash(const CpctValue& key) {
    if (key.isInt()) return "i:" + std::to_string(key.asInt());
    if (key.isUInt()) return "u:" + std::to_string(key.asUInt());
    if (key.isString()) return "s:" + key.asString();
    if (key.isBool()) return key.asBool() ? "b:true" : "b:false";
    if (key.isBigInt()) return "B:" + key.asBigInt().toString();
    throw std::runtime_error("Unhashable dict key type");
}

// Int hash for typed map with int/char/bool keys — no string conversion
int64_t CpctDict::intHash(const CpctValue& key) {
    if (key.isInt()) return key.asInt();
    if (key.isUInt()) return static_cast<int64_t>(key.asUInt()); // may wrap for very large uint64
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

// ============== TypedArray (type-specialized array element access) ==============

CpctValue TypedArray::get(size_t i) const {
    switch (elemType) {
        case ArrayElemType::Int8:    return CpctValue(static_cast<int64_t>(std::get<std::vector<int8_t>>(data)[i]));
        case ArrayElemType::Int16:   return CpctValue(static_cast<int64_t>(std::get<std::vector<int16_t>>(data)[i]));
        case ArrayElemType::Int32:   return CpctValue(static_cast<int64_t>(std::get<std::vector<int32_t>>(data)[i]));
        case ArrayElemType::Int64:   return CpctValue(std::get<std::vector<int64_t>>(data)[i]);
        case ArrayElemType::UInt8:   return CpctValue(static_cast<uint64_t>(std::get<std::vector<uint8_t>>(data)[i]));
        case ArrayElemType::UInt16:  return CpctValue(static_cast<uint64_t>(std::get<std::vector<uint16_t>>(data)[i]));
        case ArrayElemType::UInt32:  return CpctValue(static_cast<uint64_t>(std::get<std::vector<uint32_t>>(data)[i]));
        case ArrayElemType::UInt64:  return CpctValue(std::get<std::vector<uint64_t>>(data)[i]);
        case ArrayElemType::Float32: return CpctValue(static_cast<double>(std::get<std::vector<float>>(data)[i]));
        case ArrayElemType::Float64: return CpctValue(std::get<std::vector<double>>(data)[i]);
        case ArrayElemType::Bool:    return CpctValue(static_cast<bool>(std::get<std::vector<uint8_t>>(data)[i]));
    }
    return CpctValue();
}

void TypedArray::set(size_t i, const CpctValue& val) {
    switch (elemType) {
        case ArrayElemType::Int8:    std::get<std::vector<int8_t>>(data)[i] = static_cast<int8_t>(val.toInt64()); break;
        case ArrayElemType::Int16:   std::get<std::vector<int16_t>>(data)[i] = static_cast<int16_t>(val.toInt64()); break;
        case ArrayElemType::Int32:   std::get<std::vector<int32_t>>(data)[i] = static_cast<int32_t>(val.toInt64()); break;
        case ArrayElemType::Int64:   std::get<std::vector<int64_t>>(data)[i] = val.toInt64(); break;
        case ArrayElemType::UInt8:   std::get<std::vector<uint8_t>>(data)[i] = static_cast<uint8_t>(val.isUInt() ? val.asUInt() : static_cast<uint64_t>(val.toInt64())); break;
        case ArrayElemType::UInt16:  std::get<std::vector<uint16_t>>(data)[i] = static_cast<uint16_t>(val.isUInt() ? val.asUInt() : static_cast<uint64_t>(val.toInt64())); break;
        case ArrayElemType::UInt32:  std::get<std::vector<uint32_t>>(data)[i] = static_cast<uint32_t>(val.isUInt() ? val.asUInt() : static_cast<uint64_t>(val.toInt64())); break;
        case ArrayElemType::UInt64:  std::get<std::vector<uint64_t>>(data)[i] = val.isUInt() ? val.asUInt() : static_cast<uint64_t>(val.toInt64()); break;
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
    if (a.isUInt() && b.isUInt()) return a.asUInt() < b.asUInt();
    // Signed/unsigned cross comparison via safe helpers
    if (a.isInt() && b.isUInt()) return safeCmpLess(a.asInt(), b.asUInt());
    if (a.isUInt() && b.isInt()) return safeCmpLess(a.asUInt(), b.asInt());
    if (a.isFloat() && b.isFloat()) return a.asFloat() < b.asFloat();
    if (a.isString() && b.isString()) return a.asString() < b.asString();
    if (a.isBool() && b.isBool()) return !a.asBool() && b.asBool(); // false < true
    if (a.isBigInt() && b.isBigInt()) return a.asBigInt() < b.asBigInt();
    // BigInt vs int/uint
    if (a.isBigInt() && b.isInt()) return a.asBigInt() < BigInt(b.asInt());
    if (a.isInt() && b.isBigInt()) return BigInt(a.asInt()) < b.asBigInt();
    if (a.isBigInt() && b.isUInt()) return a.asBigInt() < BigInt(b.asUInt());
    if (a.isUInt() && b.isBigInt()) return BigInt(a.asUInt()) < b.asBigInt();
    // Mixed numeric → double
    if ((a.isNumericInt() || a.isFloat()) && (b.isNumericInt() || b.isFloat())) {
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

// ============== CpctValue (runtime value conversion methods) ==============

double CpctValue::toNumber() const {
    if (isInt()) return static_cast<double>(asInt());
    if (isUInt()) return static_cast<double>(asUInt());
    if (isBigInt()) return asBigInt().toDouble();
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
    if (isUInt()) return asUInt() != 0;
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
    if (isUInt()) return std::to_string(asUInt());
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

// ============== Environment (scope chain implementation) ==============

Environment::Environment(Environment* parent) : parent_(parent) {}

void Environment::define(const std::string& name, CpctValue value, const std::string& type) {
    vars_[name] = std::move(value);
    if (!type.empty()) {
        types_[name] = type;
    }
}

void Environment::defineRef(const std::string& name, Environment* targetEnv, const std::string& targetName, const std::string& type) {
    // Flatten ref chains: if target is itself a ref, follow to the final target
    auto refIt = targetEnv->refs_.find(targetName);
    if (refIt != targetEnv->refs_.end()) {
        refs_[name] = refIt->second; // point directly to the final target
    } else {
        refs_[name] = {targetEnv, targetName};
    }
    if (!type.empty()) {
        types_[name] = type;
    }
}

CpctValue& Environment::get(const std::string& name) {
    // Check moved first
    if (moved_.count(name)) {
        throw RuntimeError("Variable '" + name + "' has been moved and can no longer be used");
    }
    // Check refs first in current scope
    auto refIt = refs_.find(name);
    if (refIt != refs_.end()) {
        return refIt->second.first->get(refIt->second.second);
    }
    auto it = vars_.find(name);
    if (it != vars_.end()) return it->second;
    if (parent_) return parent_->get(name);
    throw RuntimeError("Undefined variable '" + name + "'");
}

void Environment::set(const std::string& name, CpctValue value) {
    // Check const first
    if (consts_.count(name)) {
        throw RuntimeError("Cannot assign to const variable '" + name + "'");
    }
    // Check moved
    if (moved_.count(name)) {
        throw RuntimeError("Variable '" + name + "' has been moved and can no longer be used");
    }
    // Check refs first in current scope
    auto refIt = refs_.find(name);
    if (refIt != refs_.end()) {
        refIt->second.first->set(refIt->second.second, std::move(value));
        return;
    }
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
    if (refs_.count(name)) return true;
    if (vars_.count(name)) return true;
    if (parent_) return parent_->has(name);
    return false;
}

std::string Environment::getType(const std::string& name) const {
    auto it = types_.find(name);
    if (it != types_.end()) return it->second;
    // Check refs — follow to target's type
    auto refIt = refs_.find(name);
    if (refIt != refs_.end()) {
        return refIt->second.first->getType(refIt->second.second);
    }
    if (parent_) return parent_->getType(name);
    return "";
}

bool Environment::isRef(const std::string& name) const {
    if (refs_.count(name)) return true;
    if (parent_) return parent_->isRef(name);
    return false;
}

Environment* Environment::findOwner(const std::string& name) {
    // If it's a ref, return the owner of the final target
    auto refIt = refs_.find(name);
    if (refIt != refs_.end()) {
        return refIt->second.first->findOwner(refIt->second.second);
    }
    if (vars_.count(name)) return this;
    if (parent_) return parent_->findOwner(name);
    return nullptr;
}

void Environment::markConst(const std::string& name) {
    consts_.insert(name);
}

void Environment::markMoved(const std::string& name) {
    moved_.insert(name);
}

bool Environment::isConst(const std::string& name) const {
    if (consts_.count(name)) return true;
    // Check refs — if this is a ref, check target's const status
    auto refIt = refs_.find(name);
    if (refIt != refs_.end()) {
        return refIt->second.first->isConst(refIt->second.second);
    }
    if (vars_.count(name)) return false;
    if (parent_) return parent_->isConst(name);
    return false;
}

bool Environment::isMoved(const std::string& name) const {
    if (moved_.count(name)) return true;
    if (vars_.count(name) || refs_.count(name)) return false;
    if (parent_) return parent_->isMoved(name);
    return false;
}

// ============== Interpreter (statement execution engine) ==============

Interpreter::Interpreter() : currentEnv_(&globalEnv_) {}

int Interpreter::run(const std::vector<StmtPtr>& program) {
    // Check if a main() function exists in the program
    bool hasMain = false;
    for (auto& stmt : program) {
        if (stmt->kind == StmtKind::FuncDecl && stmt->funcName == "main") {
            hasMain = true;
            break;
        }
    }

    if (hasMain) {
        // Main mode: register function declarations only, then call main()
        for (auto& stmt : program) {
            if (stmt->kind == StmtKind::FuncDecl) {
                execFuncDecl(stmt.get());
            }
        }
        // Validate main() signature
        auto& mainFunc = functions_["main"];
        if (mainFunc.returnType != "int" && mainFunc.returnType != "void") {
            throw std::runtime_error("main() must return 'int' or 'void', got '" + mainFunc.returnType + "'");
        }
        if (!mainFunc.params.empty()) {
            if (mainFunc.params.size() != 1 || mainFunc.params[0].type != "string[]") {
                throw std::runtime_error("main() parameters must be empty or (string[] args)");
            }
        }
        // Call main()
        CpctValue result = callFunction("main", {}, nullptr, 0);
        // Return exit code (int main → return value, void main → 0)
        if (mainFunc.returnType == "int" && result.isInt()) {
            return static_cast<int>(result.asInt());
        }
        return 0;
    } else {
        // Script mode: execute all top-level statements in order
        for (auto& stmt : program) {
            execStmt(stmt.get());
        }
        return 0;
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

// Variable declaration execution: dict / map / vector / array (1D→TypedArray, multi-dim) / scalar
// Coerces initialization values based on type and performs range checking (checkIntRange).
void Interpreter::execVarDecl(const Stmt* stmt) {
    // Static variable: persist across function calls via globalEnv_ with mangled name
    if (stmt->isStatic) {
        std::string baseType = isReferenceType(stmt->varType) ? stripRefQualifier(stmt->varType) : stmt->varType;
        std::string staticKey = std::to_string(stmt->line) + ":" + stmt->varName;
        if (staticKeys_.find(staticKey) == staticKeys_.end()) {
            // First encounter: initialize and store in globalEnv_
            std::string mangled = "__static_" + std::to_string(staticCounter_++);
            CpctValue initVal;
            if (stmt->initExpr) {
                initVal = eval(stmt->initExpr.get());
                // Apply type coercion
                if (isSizedIntegerType(baseType) || isCharType(baseType)) {
                    initVal = checkIntRange(baseType, std::move(initVal), stmt->line);
                } else if (isFloatType(baseType)) {
                    initVal = coerceFloat(baseType, std::move(initVal), stmt->line);
                }
            } else {
                initVal = makeDefaultValue(baseType);
            }
            globalEnv_.define(mangled, std::move(initVal), baseType);
            staticKeys_[staticKey] = mangled;
            if (stmt->isConst) globalEnv_.markConst(mangled);
        }
        // Bind local name as ref to the global static storage
        std::string& mangled = staticKeys_[staticKey];
        currentEnv_->defineRef(stmt->varName, &globalEnv_, mangled, baseType);
        if (stmt->isConst) currentEnv_->markConst(stmt->varName);
        return;
    }

    // Let variable: ownership transfer (source becomes invalid)
    if (stmt->isLet) {
        if (!stmt->initExpr) {
            throw RuntimeError("'let' variable '" + stmt->varName + "' must be initialized at line " + std::to_string(stmt->line));
        }
        if (stmt->initExpr->kind != ExprKind::Identifier) {
            throw RuntimeError("'let' variable '" + stmt->varName + "' must be initialized with a variable at line " + std::to_string(stmt->line));
        }
        std::string sourceName = stmt->initExpr->name;
        CpctValue val = eval(stmt->initExpr.get());
        currentEnv_->define(stmt->varName, std::move(val), stmt->varType);
        // Mark source as moved
        currentEnv_->markMoved(sourceName);
        if (stmt->isConst) currentEnv_->markConst(stmt->varName);
        return;
    }

    // Const validation: must have initializer
    if (stmt->isConst && !isReferenceType(stmt->varType)) {
        if (!stmt->initExpr) {
            throw RuntimeError("'const' variable '" + stmt->varName + "' must be initialized at line " + std::to_string(stmt->line));
        }
    }

    // Reference variable declaration: type@ name = var;
    if (isReferenceType(stmt->varType)) {
        std::string baseType = stripRefQualifier(stmt->varType);
        if (!stmt->initExpr) {
            throw RuntimeError("Reference variable '" + stmt->varName + "' must be initialized at line " + std::to_string(stmt->line));
        }
        if (stmt->initExpr->kind != ExprKind::Identifier) {
            throw RuntimeError("Reference variable '" + stmt->varName + "' must be initialized with a variable, not a literal or expression at line " + std::to_string(stmt->line));
        }
        std::string targetName = stmt->initExpr->name;
        Environment* ownerEnv = currentEnv_->findOwner(targetName);
        if (!ownerEnv) {
            throw RuntimeError("Undefined variable '" + targetName + "' in reference initialization at line " + std::to_string(stmt->line));
        }
        currentEnv_->defineRef(stmt->varName, ownerEnv, targetName, baseType);
        if (stmt->isConst) currentEnv_->markConst(stmt->varName);
        return;
    }

    // dict declaration (untyped — any key/value allowed)
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
        if (stmt->isConst) currentEnv_->markConst(stmt->varName);
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
        if (stmt->isConst) currentEnv_->markConst(stmt->varName);
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
        if (stmt->isConst) currentEnv_->markConst(stmt->varName);
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
        if (stmt->isConst) currentEnv_->markConst(stmt->varName);
        return;
    }

    // Scalar declaration
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
    if (stmt->isConst) currentEnv_->markConst(stmt->varName);
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

    // float types are not allowed in switch (IEEE 754 precision issues)
    if (switchVal.isFloat())
        throw std::runtime_error("switch expression cannot be float type");

    // Find matching case (or default)
    int matchIdx = -1;
    int defaultIdx = -1;
    for (size_t i = 0; i < stmt->caseValues.size(); i++) {
        if (!stmt->caseValues[i]) {
            defaultIdx = static_cast<int>(i);
            continue;
        }
        CpctValue caseVal = eval(stmt->caseValues[i].get());
        if (caseVal.isFloat())
            throw std::runtime_error("case value cannot be float type");
        // Equality comparison
        bool equal = false;
        if (switchVal.isInt() && caseVal.isInt())
            equal = switchVal.asInt() == caseVal.asInt();
        else if (switchVal.isUInt() && caseVal.isUInt())
            equal = switchVal.asUInt() == caseVal.asUInt();
        else if (switchVal.isInt() && caseVal.isUInt())
            equal = safeCmpEqual(switchVal.asInt(), caseVal.asUInt());
        else if (switchVal.isUInt() && caseVal.isInt())
            equal = safeCmpEqual(switchVal.asUInt(), caseVal.asInt());
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
        case ExprKind::BigIntLiteral: return CpctValue(BigInt(expr->strVal));
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
            if (currentEnv_->isConst(expr->operand->name))
                throw RuntimeError("Cannot modify const variable '" + expr->operand->name + "' at line " + std::to_string(expr->line));
            CpctValue& ref = currentEnv_->get(expr->operand->name);
            if (ref.isInt()) {
                int64_t v = ref.asInt();
                CpctValue nv = (v == INT64_MAX) ? CpctValue(BigInt(v) + BigInt(int64_t(1))) : CpctValue(v + 1);
                std::string type = currentEnv_->getType(expr->operand->name);
                if (!type.empty() && (isSizedIntegerType(type) || isCharType(type)))
                    nv = checkIntRange(type, std::move(nv), expr->line);
                ref = nv;
            } else if (ref.isUInt()) {
                uint64_t v = ref.asUInt();
                CpctValue nv = (v == UINT64_MAX) ? CpctValue(BigInt(v) + BigInt(int64_t(1))) : CpctValue(static_cast<uint64_t>(v + 1));
                std::string type = currentEnv_->getType(expr->operand->name);
                if (!type.empty() && (isSizedIntegerType(type) || isCharType(type)))
                    nv = checkIntRange(type, std::move(nv), expr->line);
                ref = nv;
            } else if (ref.isNumericInt()) {
                CpctValue nv = CpctValue(ref.toBigInt() + BigInt(int64_t(1)));
                std::string type = currentEnv_->getType(expr->operand->name);
                if (!type.empty() && (isSizedIntegerType(type) || isCharType(type)))
                    nv = checkIntRange(type, std::move(nv), expr->line);
                ref = nv;
            } else if (ref.isFloat()) ref = CpctValue(ref.asFloat() + 1.0);
            return ref;
        }
        case ExprKind::PreDecrement: {
            if (currentEnv_->isConst(expr->operand->name))
                throw RuntimeError("Cannot modify const variable '" + expr->operand->name + "' at line " + std::to_string(expr->line));
            CpctValue& ref = currentEnv_->get(expr->operand->name);
            if (ref.isInt()) {
                int64_t v = ref.asInt();
                CpctValue nv = (v == INT64_MIN) ? CpctValue(BigInt(v) - BigInt(int64_t(1))) : CpctValue(v - 1);
                std::string type = currentEnv_->getType(expr->operand->name);
                if (!type.empty() && (isSizedIntegerType(type) || isCharType(type)))
                    nv = checkIntRange(type, std::move(nv), expr->line);
                ref = nv;
            } else if (ref.isUInt()) {
                uint64_t v = ref.asUInt();
                CpctValue nv = (v == 0) ? CpctValue(BigInt(int64_t(0)) - BigInt(int64_t(1))) : CpctValue(static_cast<uint64_t>(v - 1));
                std::string type = currentEnv_->getType(expr->operand->name);
                if (!type.empty() && (isSizedIntegerType(type) || isCharType(type)))
                    nv = checkIntRange(type, std::move(nv), expr->line);
                ref = nv;
            } else if (ref.isNumericInt()) {
                CpctValue nv = CpctValue(ref.toBigInt() - BigInt(int64_t(1)));
                std::string type = currentEnv_->getType(expr->operand->name);
                if (!type.empty() && (isSizedIntegerType(type) || isCharType(type)))
                    nv = checkIntRange(type, std::move(nv), expr->line);
                ref = nv;
            } else if (ref.isFloat()) ref = CpctValue(ref.asFloat() - 1.0);
            return ref;
        }
        case ExprKind::PostIncrement: {
            if (currentEnv_->isConst(expr->operand->name))
                throw RuntimeError("Cannot modify const variable '" + expr->operand->name + "' at line " + std::to_string(expr->line));
            CpctValue& ref = currentEnv_->get(expr->operand->name);
            CpctValue old = ref;
            if (ref.isInt()) {
                int64_t v = ref.asInt();
                CpctValue nv = (v == INT64_MAX) ? CpctValue(BigInt(v) + BigInt(int64_t(1))) : CpctValue(v + 1);
                std::string type = currentEnv_->getType(expr->operand->name);
                if (!type.empty() && (isSizedIntegerType(type) || isCharType(type)))
                    nv = checkIntRange(type, std::move(nv), expr->line);
                ref = nv;
            } else if (ref.isUInt()) {
                uint64_t v = ref.asUInt();
                CpctValue nv = (v == UINT64_MAX) ? CpctValue(BigInt(v) + BigInt(int64_t(1))) : CpctValue(static_cast<uint64_t>(v + 1));
                std::string type = currentEnv_->getType(expr->operand->name);
                if (!type.empty() && (isSizedIntegerType(type) || isCharType(type)))
                    nv = checkIntRange(type, std::move(nv), expr->line);
                ref = nv;
            } else if (ref.isNumericInt()) {
                CpctValue nv = CpctValue(ref.toBigInt() + BigInt(int64_t(1)));
                std::string type = currentEnv_->getType(expr->operand->name);
                if (!type.empty() && (isSizedIntegerType(type) || isCharType(type)))
                    nv = checkIntRange(type, std::move(nv), expr->line);
                ref = nv;
            } else if (ref.isFloat()) ref = CpctValue(ref.asFloat() + 1.0);
            return old;
        }
        case ExprKind::PostDecrement: {
            if (currentEnv_->isConst(expr->operand->name))
                throw RuntimeError("Cannot modify const variable '" + expr->operand->name + "' at line " + std::to_string(expr->line));
            CpctValue& ref = currentEnv_->get(expr->operand->name);
            CpctValue old = ref;
            if (ref.isInt()) {
                int64_t v = ref.asInt();
                CpctValue nv = (v == INT64_MIN) ? CpctValue(BigInt(v) - BigInt(int64_t(1))) : CpctValue(v - 1);
                std::string type = currentEnv_->getType(expr->operand->name);
                if (!type.empty() && (isSizedIntegerType(type) || isCharType(type)))
                    nv = checkIntRange(type, std::move(nv), expr->line);
                ref = nv;
            } else if (ref.isUInt()) {
                uint64_t v = ref.asUInt();
                CpctValue nv = (v == 0) ? CpctValue(BigInt(int64_t(0)) - BigInt(int64_t(1))) : CpctValue(static_cast<uint64_t>(v - 1));
                std::string type = currentEnv_->getType(expr->operand->name);
                if (!type.empty() && (isSizedIntegerType(type) || isCharType(type)))
                    nv = checkIntRange(type, std::move(nv), expr->line);
                ref = nv;
            } else if (ref.isNumericInt()) {
                CpctValue nv = CpctValue(ref.toBigInt() - BigInt(int64_t(1)));
                std::string type = currentEnv_->getType(expr->operand->name);
                if (!type.empty() && (isSizedIntegerType(type) || isCharType(type)))
                    nv = checkIntRange(type, std::move(nv), expr->line);
                ref = nv;
            } else if (ref.isFloat()) ref = CpctValue(ref.asFloat() - 1.0);
            return old;
        }
        case ExprKind::Ternary: {
            CpctValue cond = eval(expr->operand.get());
            return cond.toBool() ? eval(expr->left.get()) : eval(expr->right.get());
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
                    } else if (prev.isUInt() && curr.isUInt()) {
                        uint64_t l = prev.asUInt(), r = curr.asUInt();
                        switch (ops[i]) {
                            case TokenType::LT:  result = l < r; break;
                            case TokenType::GT:  result = l > r; break;
                            case TokenType::LTE: result = l <= r; break;
                            case TokenType::GTE: result = l >= r; break;
                            default: result = false;
                        }
                    } else if ((prev.isInt() || prev.isUInt()) && (curr.isInt() || curr.isUInt())) {
                        // Mixed signed/unsigned — use safe comparison helpers
                        switch (ops[i]) {
                            case TokenType::LT:  result = prev.isInt() ? safeCmpLess(prev.asInt(), curr.asUInt()) : safeCmpLess(prev.asUInt(), curr.asInt()); break;
                            case TokenType::GT:  result = prev.isInt() ? safeCmpLess(curr.asUInt(), prev.asInt()) : safeCmpLess(curr.asInt(), prev.asUInt()); break;
                            case TokenType::LTE: result = prev.isInt() ? safeCmpLessEqual(prev.asInt(), curr.asUInt()) : safeCmpLessEqual(prev.asUInt(), curr.asInt()); break;
                            case TokenType::GTE: result = prev.isInt() ? safeCmpLessEqual(curr.asUInt(), prev.asInt()) : safeCmpLessEqual(curr.asInt(), prev.asUInt()); break;
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
        case ExprKind::RefArg:
            // RefArg (ref variable) should only appear as function call arguments.
            throw RuntimeError("'ref " + expr->name + "' can only be used as a function argument for pass-by-reference at line " + std::to_string(expr->line));
        case ExprKind::LetArg:
            // LetArg (let variable) should only appear as function call arguments.
            throw RuntimeError("'let " + expr->name + "' can only be used as a function argument for ownership transfer at line " + std::to_string(expr->line));
    }
    throw RuntimeError("Unknown expression kind");
}

// Binary operator evaluation.
// Fast path order: int64×int64 → uint64×uint64 → mixed signed/unsigned → BigInt → float
// Overflow is detected via __builtin_*_overflow and auto-promotes to BigInt.
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

    // Equality — handle int/uint/BigInt
    if (expr->op == TokenType::EQ) {
        // Fast paths for same-type
        if (left.isInt() && right.isInt()) return CpctValue(left.asInt() == right.asInt());
        if (left.isUInt() && right.isUInt()) return CpctValue(left.asUInt() == right.asUInt());
        // Safe cross-type int/uint comparison
        if (left.isInt() && right.isUInt()) return CpctValue(safeCmpEqual(left.asInt(), right.asUInt()));
        if (left.isUInt() && right.isInt()) return CpctValue(safeCmpEqual(left.asUInt(), right.asInt()));
        if (left.isNumericInt() && right.isNumericInt())
            return CpctValue(left.toBigInt() == right.toBigInt());
        if (left.isString() && right.isString()) return CpctValue(left.asString() == right.asString());
        if (left.isBool() && right.isBool()) return CpctValue(left.asBool() == right.asBool());
        return CpctValue(left.toNumber() == right.toNumber());
    }
    if (expr->op == TokenType::NEQ) {
        if (left.isInt() && right.isInt()) return CpctValue(left.asInt() != right.asInt());
        if (left.isUInt() && right.isUInt()) return CpctValue(left.asUInt() != right.asUInt());
        if (left.isInt() && right.isUInt()) return CpctValue(!safeCmpEqual(left.asInt(), right.asUInt()));
        if (left.isUInt() && right.isInt()) return CpctValue(!safeCmpEqual(left.asUInt(), right.asInt()));
        if (left.isNumericInt() && right.isNumericInt())
            return CpctValue(left.toBigInt() != right.toBigInt());
        if (left.isString() && right.isString()) return CpctValue(left.asString() != right.asString());
        if (left.isBool() && right.isBool()) return CpctValue(left.asBool() != right.asBool());
        return CpctValue(left.toNumber() != right.toNumber());
    }

    // Integer arithmetic (int64/uint64 fast path + BigInt overflow promotion)
    if (left.isNumericInt() && right.isNumericInt()) {
        // Fast path: both signed int64
        if (left.isInt() && right.isInt()) {
            int64_t l = left.asInt(), r = right.asInt();
            int64_t result;
            switch (expr->op) {
                case TokenType::PLUS:
                    if (!__builtin_add_overflow(l, r, &result)) return CpctValue(result);
                    return CpctValue(BigInt(l) + BigInt(r));
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
                case TokenType::BIT_AND: return CpctValue(l & r);
                case TokenType::BIT_OR:  return CpctValue(l | r);
                case TokenType::BIT_XOR: return CpctValue(l ^ r);
                case TokenType::LSHIFT:  return CpctValue(l << r);
                case TokenType::RSHIFT:  return CpctValue(l >> r);
                default: break;
            }
        }
        // Fast path: both unsigned uint64
        if (left.isUInt() && right.isUInt()) {
            uint64_t l = left.asUInt(), r = right.asUInt();
            switch (expr->op) {
                case TokenType::PLUS: {
                    uint64_t result;
                    if (!__builtin_add_overflow(l, r, &result)) return CpctValue(result);
                    return CpctValue(BigInt(l) + BigInt(r));
                }
                case TokenType::MINUS: {
                    if (l >= r) return CpctValue(static_cast<uint64_t>(l - r));
                    // Underflow → BigInt (result is negative)
                    return CpctValue(BigInt(l) - BigInt(r));
                }
                case TokenType::STAR: {
                    uint64_t result;
                    if (!__builtin_mul_overflow(l, r, &result)) return CpctValue(result);
                    return CpctValue(BigInt(l) * BigInt(r));
                }
                case TokenType::SLASH:
                    if (r == 0) throw RuntimeError("Division by zero at line " + std::to_string(expr->line));
                    return CpctValue(static_cast<uint64_t>(l / r));
                case TokenType::PERCENT:
                    if (r == 0) throw RuntimeError("Modulo by zero at line " + std::to_string(expr->line));
                    return CpctValue(static_cast<uint64_t>(l % r));
                case TokenType::DIVMOD: {
                    if (r == 0) throw RuntimeError("Division by zero in /% at line " + std::to_string(expr->line));
                    std::vector<CpctValue> res = { CpctValue(static_cast<uint64_t>(l / r)), CpctValue(static_cast<uint64_t>(l % r)) };
                    return CpctValue(std::move(res));
                }
                case TokenType::POWER: {
                    BigInt base(l), res(int64_t(1));
                    BigInt exp(r);
                    for (uint64_t i = 0; i < r; i++) res = res * base;
                    return CpctValue(std::move(res));
                }
                case TokenType::LT:  return CpctValue(l < r);
                case TokenType::GT:  return CpctValue(l > r);
                case TokenType::LTE: return CpctValue(l <= r);
                case TokenType::GTE: return CpctValue(l >= r);
                case TokenType::BIT_AND: return CpctValue(static_cast<uint64_t>(l & r));
                case TokenType::BIT_OR:  return CpctValue(static_cast<uint64_t>(l | r));
                case TokenType::BIT_XOR: return CpctValue(static_cast<uint64_t>(l ^ r));
                case TokenType::LSHIFT:  return CpctValue(static_cast<uint64_t>(l << r));
                case TokenType::RSHIFT:  return CpctValue(static_cast<uint64_t>(l >> r));
                default: break;
            }
        }
        // Mixed signed/unsigned fast path — use safe comparison helpers for comparisons
        if ((left.isInt() && right.isUInt()) || (left.isUInt() && right.isInt())) {
            switch (expr->op) {
                case TokenType::LT:
                    if (left.isInt()) return CpctValue(safeCmpLess(left.asInt(), right.asUInt()));
                    return CpctValue(safeCmpLess(left.asUInt(), right.asInt()));
                case TokenType::GT:
                    if (left.isInt()) return CpctValue(safeCmpLess(right.asUInt(), left.asInt()));
                    return CpctValue(safeCmpLess(right.asInt(), left.asUInt()));
                case TokenType::LTE:
                    if (left.isInt()) return CpctValue(safeCmpLessEqual(left.asInt(), right.asUInt()));
                    return CpctValue(safeCmpLessEqual(left.asUInt(), right.asInt()));
                case TokenType::GTE:
                    if (left.isInt()) return CpctValue(safeCmpLessEqual(right.asUInt(), left.asInt()));
                    return CpctValue(safeCmpLessEqual(right.asInt(), left.asUInt()));
                // Bitwise ops on mixed signed/unsigned — cast both to int64
                case TokenType::BIT_AND: return CpctValue(left.toInt64() & right.toInt64());
                case TokenType::BIT_OR:  return CpctValue(left.toInt64() | right.toInt64());
                case TokenType::BIT_XOR: return CpctValue(left.toInt64() ^ right.toInt64());
                case TokenType::LSHIFT:  return CpctValue(left.toInt64() << right.toInt64());
                case TokenType::RSHIFT:  return CpctValue(left.toInt64() >> right.toInt64());
                default: break; // arithmetic falls through to BigInt
            }
        }
        // Bitwise ops on BigInt — use BigInt operators
        if (expr->op == TokenType::BIT_AND || expr->op == TokenType::BIT_OR ||
            expr->op == TokenType::BIT_XOR || expr->op == TokenType::LSHIFT ||
            expr->op == TokenType::RSHIFT) {
            BigInt l = left.toBigInt(), r = right.toBigInt();
            switch (expr->op) {
                case TokenType::BIT_AND: return CpctValue(l & r);
                case TokenType::BIT_OR:  return CpctValue(l | r);
                case TokenType::BIT_XOR: return CpctValue(l ^ r);
                case TokenType::LSHIFT: {
                    if (!r.fitsInt64()) throw RuntimeError("Shift amount too large at line " + std::to_string(expr->line));
                    return CpctValue(l << r.toInt64());
                }
                case TokenType::RSHIFT: {
                    if (!r.fitsInt64()) throw RuntimeError("Shift amount too large at line " + std::to_string(expr->line));
                    return CpctValue(l >> r.toInt64());
                }
                default: break;
            }
        }
        // At least one is BigInt or mixed signed/unsigned arithmetic — use BigInt
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
            if (val.isUInt()) {
                // Negating unsigned: result may be negative → BigInt or int64
                uint64_t uv = val.asUInt();
                if (uv == 0) return CpctValue(uint64_t(0));
                if (uv <= static_cast<uint64_t>(INT64_MAX))
                    return CpctValue(-static_cast<int64_t>(uv));
                return CpctValue(-BigInt(uv));
            }
            if (val.isBigInt()) return CpctValue(-val.asBigInt());
            return CpctValue(-val.toNumber());
        case TokenType::NOT:
            return CpctValue(!val.toBool());
        case TokenType::BIT_NOT:
            if (val.isInt()) return CpctValue(~val.asInt());
            if (val.isUInt()) return CpctValue(~val.asUInt());
            if (val.isBigInt()) return CpctValue(~val.asBigInt());
            throw RuntimeError("Bitwise NOT requires integer operand at line " + std::to_string(expr->line));
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
        // Fast path: both signed int64
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
                case TokenType::BIT_AND_ASSIGN: result = CpctValue(l & r); break;
                case TokenType::BIT_OR_ASSIGN:  result = CpctValue(l | r); break;
                case TokenType::BIT_XOR_ASSIGN: result = CpctValue(l ^ r); break;
                case TokenType::LSHIFT_ASSIGN:  result = CpctValue(l << r); break;
                case TokenType::RSHIFT_ASSIGN:  result = CpctValue(l >> r); break;
                default: throw RuntimeError("Unknown compound operator");
            }
        } else if (cur.isUInt() && right.isUInt()) {
            // Fast path: both unsigned uint64
            uint64_t l = cur.asUInt(), r = right.asUInt();
            switch (expr->op) {
                case TokenType::PLUS_ASSIGN: {
                    uint64_t res;
                    if (!__builtin_add_overflow(l, r, &res)) { result = CpctValue(res); break; }
                    result = CpctValue(BigInt(l) + BigInt(r)); break;
                }
                case TokenType::MINUS_ASSIGN:
                    if (l >= r) { result = CpctValue(static_cast<uint64_t>(l - r)); break; }
                    result = CpctValue(BigInt(l) - BigInt(r)); break;
                case TokenType::STAR_ASSIGN: {
                    uint64_t res;
                    if (!__builtin_mul_overflow(l, r, &res)) { result = CpctValue(res); break; }
                    result = CpctValue(BigInt(l) * BigInt(r)); break;
                }
                case TokenType::SLASH_ASSIGN:
                    if (r == 0) throw RuntimeError("Division by zero");
                    result = CpctValue(static_cast<uint64_t>(l / r)); break;
                case TokenType::PERCENT_ASSIGN:
                    if (r == 0) throw RuntimeError("Division by zero");
                    result = CpctValue(static_cast<uint64_t>(l % r)); break;
                case TokenType::BIT_AND_ASSIGN: result = CpctValue(static_cast<uint64_t>(l & r)); break;
                case TokenType::BIT_OR_ASSIGN:  result = CpctValue(static_cast<uint64_t>(l | r)); break;
                case TokenType::BIT_XOR_ASSIGN: result = CpctValue(static_cast<uint64_t>(l ^ r)); break;
                case TokenType::LSHIFT_ASSIGN:  result = CpctValue(static_cast<uint64_t>(l << r)); break;
                case TokenType::RSHIFT_ASSIGN:  result = CpctValue(static_cast<uint64_t>(l >> r)); break;
                default: throw RuntimeError("Unknown compound operator");
            }
        } else {
            // BigInt path (mixed signed/unsigned or BigInt involved)
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
                case TokenType::BIT_AND_ASSIGN: result = CpctValue(l & r); break;
                case TokenType::BIT_OR_ASSIGN:  result = CpctValue(l | r); break;
                case TokenType::BIT_XOR_ASSIGN: result = CpctValue(l ^ r); break;
                case TokenType::LSHIFT_ASSIGN: {
                    if (!r.fitsInt64()) throw RuntimeError("Shift amount too large");
                    result = CpctValue(l << r.toInt64()); break;
                }
                case TokenType::RSHIFT_ASSIGN: {
                    if (!r.fitsInt64()) throw RuntimeError("Shift amount too large");
                    result = CpctValue(l >> r.toInt64()); break;
                }
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

// Function call evaluation: type methods (int.max(), etc.) → built-in functions (push/pop/sort, etc.) → type casts → user functions
CpctValue Interpreter::evalFunctionCall(const Expr* expr) {
    // Type methods: int.max(), float32.min(), etc.
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
            if (typeName == "uint" || typeName == "uint32") return CpctValue(static_cast<uint64_t>(UINT32_MAX));
            if (typeName == "uint8") return CpctValue(static_cast<uint64_t>(UINT8_MAX));
            if (typeName == "uint16") return CpctValue(static_cast<uint64_t>(UINT16_MAX));
            if (typeName == "uint64") return CpctValue(UINT64_MAX);
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
                typeName == "uint32" || typeName == "uint64") return CpctValue(uint64_t(0));
            throw RuntimeError("min() is not defined for type '" + typeName + "'");
        }
        throw RuntimeError("Unknown type method '" + method + "' for type '" + typeName + "'");
    }

    // Const check for mutating built-in functions
    {
        static const std::unordered_set<std::string> mutatingFuncs = {
            "remove", "push", "pop", "insert", "erase", "clear", "sort", "sortkey", "sortk", "sortval", "sortv"
        };
        if (mutatingFuncs.count(expr->funcName) && !expr->args.empty() &&
            expr->args[0]->kind == ExprKind::Identifier &&
            currentEnv_->isConst(expr->args[0]->name)) {
            throw RuntimeError("Cannot modify const variable '" + expr->args[0]->name +
                "' via " + expr->funcName + "() at line " + std::to_string(expr->line));
        }
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
        if (!idxVal.isInt() && !idxVal.isUInt())
            throw RuntimeError("insert() index must be an integer");
        int64_t idx = idxVal.toInt64();
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
        if (!idxVal.isInt() && !idxVal.isUInt())
            throw RuntimeError("erase() index must be an integer");
        int64_t idx = idxVal.toInt64();
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

    // is_empty() — check if vector/array is empty
    if (expr->funcName == "is_empty") {
        if (expr->args.size() != 1)
            throw RuntimeError("is_empty() takes exactly 1 argument");
        CpctValue val = eval(expr->args[0].get());
        if (val.isArray()) return CpctValue(val.asArray().empty());
        if (val.isTypedArray()) return CpctValue(val.asTypedArray().size() == 0);
        throw RuntimeError("is_empty() requires a vector or array argument");
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
        else if (val.isUInt()) typeName = "uint";
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
        if (arg->kind == ExprKind::RefArg) {
            // Don't evaluate ref var — just push a placeholder; callFunction handles it via argExprs
            args.push_back(CpctValue(int64_t(0))); // placeholder
        } else if (arg->kind == ExprKind::LetArg) {
            // Evaluate let var normally — value will be moved in callFunction
            args.push_back(eval(makeIdentifier(arg->name, arg->line).get()));
        } else {
            args.push_back(eval(arg.get()));
        }
    }
    return callFunction(expr->funcName, args, &expr->args, expr->line);
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
    if (!idx.isInt() && !idx.isUInt()) throw RuntimeError("Array index must be an integer at line " + std::to_string(expr->line));

    int64_t i = idx.toInt64();
    int64_t size = arr.isTypedArray() ? static_cast<int64_t>(arr.asTypedArray().size())
                                      : static_cast<int64_t>(arr.asArray().size());

    if (i < 0) i += size;
    if (i < 0 || i >= size) {
        throw RuntimeError("Array index out of bounds: " + idx.toString() +
                          " (size: " + std::to_string(size) + ") at line " + std::to_string(expr->line));
    }

    if (arr.isTypedArray()) return arr.asTypedArray().get(static_cast<size_t>(i));
    return arr.asArray()[i];
}

CpctValue Interpreter::evalArrayAssign(const Expr* expr) {
    // Check if the array variable is const
    {
        const Expr* root = expr->arrayExpr.get();
        while (root->kind == ExprKind::ArrayAccess) root = root->arrayExpr.get();
        if (root->kind == ExprKind::Identifier && currentEnv_->isConst(root->name)) {
            throw RuntimeError("Cannot modify element of const variable '" + root->name + "' at line " + std::to_string(expr->line));
        }
    }

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
        if (!idx.isInt() && !idx.isUInt()) throw RuntimeError("Array index must be an integer at line " + std::to_string(expr->line));
        int64_t i = idx.toInt64();
        auto& ta = rootVal.asTypedArray();
        int64_t size = static_cast<int64_t>(ta.size());
        if (i < 0) i += size;
        if (i < 0 || i >= size) {
            throw RuntimeError("Array index out of bounds: " + idx.toString() +
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
    // Check if the array variable is const
    {
        const Expr* root = expr->arrayExpr.get();
        while (root->kind == ExprKind::ArrayAccess) root = root->arrayExpr.get();
        if (root->kind == ExprKind::Identifier && currentEnv_->isConst(root->name)) {
            throw RuntimeError("Cannot modify element of const variable '" + root->name + "' at line " + std::to_string(expr->line));
        }
    }

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
        } else if (lhs.isUInt() && right.isUInt()) {
            uint64_t l = lhs.asUInt(), r = right.asUInt();
            uint64_t result = 0;
            switch (expr->op) {
                case TokenType::PLUS_ASSIGN: result = l + r; break;
                case TokenType::MINUS_ASSIGN: result = l - r; break;
                case TokenType::STAR_ASSIGN: result = l * r; break;
                case TokenType::SLASH_ASSIGN: result = r != 0 ? l / r : throw RuntimeError("Division by zero"); break;
                case TokenType::PERCENT_ASSIGN: result = r != 0 ? l % r : throw RuntimeError("Modulo by zero"); break;
                default: throw RuntimeError("Unknown compound assign op");
            }
            elemRef = CpctValue(result);
        } else if ((lhs.isInt() || lhs.isUInt() || lhs.isFloat()) && (right.isInt() || right.isUInt() || right.isFloat())) {
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
        if (!idx.isInt() && !idx.isUInt()) throw RuntimeError("Array index must be an integer at line " + std::to_string(expr->line));
        int64_t i = idx.toInt64();
        auto& ta = rootVal.asTypedArray();
        int64_t sz = static_cast<int64_t>(ta.size());
        if (i < 0) i += sz;
        if (i < 0 || i >= sz)
            throw RuntimeError("Array index out of bounds at line " + std::to_string(expr->line));

        CpctValue elemVal = ta.get(static_cast<size_t>(i));
        CpctValue right = eval(expr->right.get());
        CpctValue result;
        // Compute (reuse numeric logic inline)
        if (elemVal.isInt() && right.isInt()) {
            int64_t l = elemVal.asInt(), r = right.asInt(); int64_t res;
            switch (expr->op) {
                case TokenType::PLUS_ASSIGN: if (!__builtin_add_overflow(l,r,&res)){result=CpctValue(res);break;} result=CpctValue(BigInt(l)+BigInt(r));break;
                case TokenType::MINUS_ASSIGN: if (!__builtin_sub_overflow(l,r,&res)){result=CpctValue(res);break;} result=CpctValue(BigInt(l)-BigInt(r));break;
                case TokenType::STAR_ASSIGN: if (!__builtin_mul_overflow(l,r,&res)){result=CpctValue(res);break;} result=CpctValue(BigInt(l)*BigInt(r));break;
                case TokenType::SLASH_ASSIGN: if(r==0)throw RuntimeError("Division by zero");result=CpctValue(l/r);break;
                case TokenType::PERCENT_ASSIGN: if(r==0)throw RuntimeError("Division by zero");result=CpctValue(l%r);break;
                default: throw RuntimeError("Unknown compound operator");
            }
        } else if (elemVal.isUInt() && right.isUInt()) {
            uint64_t l = elemVal.asUInt(), r = right.asUInt();
            switch (expr->op) {
                case TokenType::PLUS_ASSIGN: {uint64_t res; if(!__builtin_add_overflow(l,r,&res)){result=CpctValue(res);break;} result=CpctValue(BigInt(l)+BigInt(r));break;}
                case TokenType::MINUS_ASSIGN: if(l>=r){result=CpctValue(static_cast<uint64_t>(l-r));break;} result=CpctValue(BigInt(l)-BigInt(r));break;
                case TokenType::STAR_ASSIGN: {uint64_t res; if(!__builtin_mul_overflow(l,r,&res)){result=CpctValue(res);break;} result=CpctValue(BigInt(l)*BigInt(r));break;}
                case TokenType::SLASH_ASSIGN: if(r==0)throw RuntimeError("Division by zero");result=CpctValue(static_cast<uint64_t>(l/r));break;
                case TokenType::PERCENT_ASSIGN: if(r==0)throw RuntimeError("Division by zero");result=CpctValue(static_cast<uint64_t>(l%r));break;
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
                case TokenType::BIT_AND_ASSIGN: result = CpctValue(l & r); break;
                case TokenType::BIT_OR_ASSIGN:  result = CpctValue(l | r); break;
                case TokenType::BIT_XOR_ASSIGN: result = CpctValue(l ^ r); break;
                case TokenType::LSHIFT_ASSIGN:  result = CpctValue(l << r); break;
                case TokenType::RSHIFT_ASSIGN:  result = CpctValue(l >> r); break;
                default: throw RuntimeError("Unknown compound operator");
            }
        } else if (elem.isUInt() && right.isUInt()) {
            uint64_t l = elem.asUInt(), r = right.asUInt();
            switch (expr->op) {
                case TokenType::PLUS_ASSIGN: {
                    uint64_t res;
                    if (!__builtin_add_overflow(l, r, &res)) { result = CpctValue(res); break; }
                    result = CpctValue(BigInt(l) + BigInt(r)); break;
                }
                case TokenType::MINUS_ASSIGN:
                    if (l >= r) { result = CpctValue(static_cast<uint64_t>(l - r)); break; }
                    result = CpctValue(BigInt(l) - BigInt(r)); break;
                case TokenType::STAR_ASSIGN: {
                    uint64_t res;
                    if (!__builtin_mul_overflow(l, r, &res)) { result = CpctValue(res); break; }
                    result = CpctValue(BigInt(l) * BigInt(r)); break;
                }
                case TokenType::SLASH_ASSIGN:
                    if (r == 0) throw RuntimeError("Division by zero");
                    result = CpctValue(static_cast<uint64_t>(l / r)); break;
                case TokenType::PERCENT_ASSIGN:
                    if (r == 0) throw RuntimeError("Division by zero");
                    result = CpctValue(static_cast<uint64_t>(l % r)); break;
                case TokenType::BIT_AND_ASSIGN: result = CpctValue(static_cast<uint64_t>(l & r)); break;
                case TokenType::BIT_OR_ASSIGN:  result = CpctValue(static_cast<uint64_t>(l | r)); break;
                case TokenType::BIT_XOR_ASSIGN: result = CpctValue(static_cast<uint64_t>(l ^ r)); break;
                case TokenType::LSHIFT_ASSIGN:  result = CpctValue(static_cast<uint64_t>(l << r)); break;
                case TokenType::RSHIFT_ASSIGN:  result = CpctValue(static_cast<uint64_t>(l >> r)); break;
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
                case TokenType::BIT_AND_ASSIGN: result = CpctValue(l & r); break;
                case TokenType::BIT_OR_ASSIGN:  result = CpctValue(l | r); break;
                case TokenType::BIT_XOR_ASSIGN: result = CpctValue(l ^ r); break;
                case TokenType::LSHIFT_ASSIGN: {
                    if (!r.fitsInt64()) throw RuntimeError("Shift amount too large");
                    result = CpctValue(l << r.toInt64()); break;
                }
                case TokenType::RSHIFT_ASSIGN: {
                    if (!r.fitsInt64()) throw RuntimeError("Shift amount too large");
                    result = CpctValue(l >> r.toInt64()); break;
                }
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

// Built-in functions (len, shape, keys, values, has, divmod, type casts) and user-defined function calls.
// User function calls create a new Environment on top of the global scope,
// and receive the return value via ReturnSignal exception.
CpctValue Interpreter::callFunction(const std::string& name, const std::vector<CpctValue>& args,
                                    const std::vector<ExprPtr>* argExprs, int line) {
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
    // ============== String methods ==============
    // split(s, sep) — split string by separator, returns array of strings
    if (name == "split") {
        if (args.size() != 2) throw RuntimeError("split() takes exactly 2 arguments (string, separator)");
        if (!args[0].isString() || !args[1].isString())
            throw RuntimeError("split() requires string arguments");
        const std::string& s = args[0].asString();
        const std::string& sep = args[1].asString();
        std::vector<CpctValue> result;
        if (sep.empty()) {
            // Split into individual characters
            for (char c : s) result.push_back(CpctValue(std::string(1, c)));
        } else {
            size_t start = 0, pos;
            while ((pos = s.find(sep, start)) != std::string::npos) {
                result.push_back(CpctValue(s.substr(start, pos - start)));
                start = pos + sep.size();
            }
            result.push_back(CpctValue(s.substr(start)));
        }
        return CpctValue(std::move(result));
    }

    // join(sep, arr) — join array elements into string with separator
    if (name == "join") {
        if (args.size() != 2) throw RuntimeError("join() takes exactly 2 arguments (separator, array)");
        if (!args[0].isString()) throw RuntimeError("join() first argument must be a string separator");
        if (!args[1].isArray()) throw RuntimeError("join() second argument must be an array");
        const std::string& sep = args[0].asString();
        const auto& arr = args[1].asArray();
        std::string result;
        for (size_t i = 0; i < arr.size(); i++) {
            if (i > 0) result += sep;
            result += arr[i].toString();
        }
        return CpctValue(std::move(result));
    }

    // upper(s) — convert string to uppercase
    if (name == "upper") {
        if (args.size() != 1) throw RuntimeError("upper() takes exactly 1 argument");
        if (!args[0].isString()) throw RuntimeError("upper() requires a string argument");
        std::string result = args[0].asString();
        for (char& c : result) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return CpctValue(std::move(result));
    }

    // lower(s) — convert string to lowercase
    if (name == "lower") {
        if (args.size() != 1) throw RuntimeError("lower() takes exactly 1 argument");
        if (!args[0].isString()) throw RuntimeError("lower() requires a string argument");
        std::string result = args[0].asString();
        for (char& c : result) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return CpctValue(std::move(result));
    }

    // find(s, sub) — find index of substring, returns -1 if not found
    if (name == "find") {
        if (args.size() != 2) throw RuntimeError("find() takes exactly 2 arguments (string, substring)");
        if (!args[0].isString() || !args[1].isString())
            throw RuntimeError("find() requires string arguments");
        size_t pos = args[0].asString().find(args[1].asString());
        return CpctValue(pos == std::string::npos ? int64_t(-1) : static_cast<int64_t>(pos));
    }

    // replace(s, old, new) — replace all occurrences of old with new
    if (name == "replace") {
        if (args.size() != 3) throw RuntimeError("replace() takes exactly 3 arguments (string, old, new)");
        if (!args[0].isString() || !args[1].isString() || !args[2].isString())
            throw RuntimeError("replace() requires string arguments");
        std::string result = args[0].asString();
        const std::string& oldStr = args[1].asString();
        const std::string& newStr = args[2].asString();
        if (!oldStr.empty()) {
            size_t pos = 0;
            while ((pos = result.find(oldStr, pos)) != std::string::npos) {
                result.replace(pos, oldStr.size(), newStr);
                pos += newStr.size();
            }
        }
        return CpctValue(std::move(result));
    }

    // trim(s) — remove leading and trailing whitespace
    if (name == "trim") {
        if (args.size() != 1) throw RuntimeError("trim() takes exactly 1 argument");
        if (!args[0].isString()) throw RuntimeError("trim() requires a string argument");
        const std::string& s = args[0].asString();
        size_t start = s.find_first_not_of(" \t\n\r\f\v");
        if (start == std::string::npos) return CpctValue(std::string(""));
        size_t end = s.find_last_not_of(" \t\n\r\f\v");
        return CpctValue(s.substr(start, end - start + 1));
    }

    // substr(s, start, len) — extract substring
    if (name == "substr") {
        if (args.size() != 3) throw RuntimeError("substr() takes exactly 3 arguments (string, start, length)");
        if (!args[0].isString()) throw RuntimeError("substr() first argument must be a string");
        const std::string& s = args[0].asString();
        int64_t start = args[1].toNumber();
        int64_t len = args[2].toNumber();
        if (start < 0) start = 0;
        if (start >= static_cast<int64_t>(s.size())) return CpctValue(std::string(""));
        return CpctValue(s.substr(static_cast<size_t>(start), static_cast<size_t>(len)));
    }

    // reverse(s) — reverse a string
    if (name == "reverse") {
        if (args.size() != 1) throw RuntimeError("reverse() takes exactly 1 argument");
        if (!args[0].isString()) throw RuntimeError("reverse() requires a string argument");
        std::string result = args[0].asString();
        std::reverse(result.begin(), result.end());
        return CpctValue(std::move(result));
    }

    // is_contains(s, sub) — check if string contains substring
    if (name == "is_contains") {
        if (args.size() != 2) throw RuntimeError("is_contains() takes exactly 2 arguments (string, substring)");
        if (!args[0].isString() || !args[1].isString())
            throw RuntimeError("is_contains() requires string arguments");
        return CpctValue(args[0].asString().find(args[1].asString()) != std::string::npos);
    }

    // is_starts_with(s, prefix) — check if string starts with prefix
    if (name == "is_starts_with") {
        if (args.size() != 2) throw RuntimeError("is_starts_with() takes exactly 2 arguments (string, prefix)");
        if (!args[0].isString() || !args[1].isString())
            throw RuntimeError("is_starts_with() requires string arguments");
        const std::string& s = args[0].asString();
        const std::string& pre = args[1].asString();
        return CpctValue(s.size() >= pre.size() && s.compare(0, pre.size(), pre) == 0);
    }

    // is_ends_with(s, suffix) — check if string ends with suffix
    if (name == "is_ends_with") {
        if (args.size() != 2) throw RuntimeError("is_ends_with() takes exactly 2 arguments (string, suffix)");
        if (!args[0].isString() || !args[1].isString())
            throw RuntimeError("is_ends_with() requires string arguments");
        const std::string& s = args[0].asString();
        const std::string& suf = args[1].asString();
        return CpctValue(s.size() >= suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0);
    }

    // is_digit(s) — check if all characters are digits
    if (name == "is_digit") {
        if (args.size() != 1) throw RuntimeError("is_digit() takes exactly 1 argument");
        if (!args[0].isString()) throw RuntimeError("is_digit() requires a string argument");
        const std::string& s = args[0].asString();
        if (s.empty()) return CpctValue(false);
        for (char c : s) if (!std::isdigit(static_cast<unsigned char>(c))) return CpctValue(false);
        return CpctValue(true);
    }

    // is_alpha(s) — check if all characters are alphabetic
    if (name == "is_alpha") {
        if (args.size() != 1) throw RuntimeError("is_alpha() takes exactly 1 argument");
        if (!args[0].isString()) throw RuntimeError("is_alpha() requires a string argument");
        const std::string& s = args[0].asString();
        if (s.empty()) return CpctValue(false);
        for (char c : s) if (!std::isalpha(static_cast<unsigned char>(c))) return CpctValue(false);
        return CpctValue(true);
    }

    // is_upper(s) — check if all characters are uppercase
    if (name == "is_upper") {
        if (args.size() != 1) throw RuntimeError("is_upper() takes exactly 1 argument");
        if (!args[0].isString()) throw RuntimeError("is_upper() requires a string argument");
        const std::string& s = args[0].asString();
        if (s.empty()) return CpctValue(false);
        for (char c : s) if (!std::isupper(static_cast<unsigned char>(c))) return CpctValue(false);
        return CpctValue(true);
    }

    // is_lower(s) — check if all characters are lowercase
    if (name == "is_lower") {
        if (args.size() != 1) throw RuntimeError("is_lower() takes exactly 1 argument");
        if (!args[0].isString()) throw RuntimeError("is_lower() requires a string argument");
        const std::string& s = args[0].asString();
        if (s.empty()) return CpctValue(false);
        for (char c : s) if (!std::islower(static_cast<unsigned char>(c))) return CpctValue(false);
        return CpctValue(true);
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
        if (func.params[i].isRef) {
            // Reference parameter: bind to caller's variable
            if (!argExprs || i >= argExprs->size() || (*argExprs)[i]->kind != ExprKind::RefArg) {
                throw RuntimeError("Reference parameter '" + func.params[i].name +
                    "' requires 'ref variable' argument at line " + std::to_string(line));
            }
            std::string targetName = (*argExprs)[i]->name;
            Environment* ownerEnv = currentEnv_->findOwner(targetName);
            if (!ownerEnv) {
                throw RuntimeError("Undefined variable '" + targetName +
                    "' in reference argument at line " + std::to_string(line));
            }
            funcEnv.defineRef(func.params[i].name, ownerEnv, targetName, func.params[i].type);
        } else if (func.params[i].isLet) {
            // Let parameter: ownership transfer
            if (!argExprs || i >= argExprs->size() || (*argExprs)[i]->kind != ExprKind::LetArg) {
                throw RuntimeError("Ownership transfer parameter '" + func.params[i].name +
                    "' requires 'let variable' argument at line " + std::to_string(line));
            }
            std::string sourceName = (*argExprs)[i]->name;
            funcEnv.define(func.params[i].name, std::move(args[i]), func.params[i].type);
            // Mark source as moved in caller's environment
            currentEnv_->markMoved(sourceName);
        } else {
            // Value parameter: check that caller didn't pass ref/let to a non-ref/let param
            if (argExprs && i < argExprs->size() && (*argExprs)[i]->kind == ExprKind::RefArg) {
                throw RuntimeError("Parameter '" + func.params[i].name +
                    "' is not a reference parameter, cannot pass 'ref variable' at line " + std::to_string(line));
            }
            if (argExprs && i < argExprs->size() && (*argExprs)[i]->kind == ExprKind::LetArg) {
                throw RuntimeError("Parameter '" + func.params[i].name +
                    "' is not a let parameter, cannot pass 'let variable' at line " + std::to_string(line));
            }
            funcEnv.define(func.params[i].name, args[i], func.params[i].type);
        }
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

// Checks the range of fixed-size integer types and applies C-style modular wrap-around on overflow.
// uint64 uses a separate uint64_t storage path; intbig/bigint pass through without range limits.
CpctValue Interpreter::checkIntRange(const std::string& typeName, CpctValue val, int line) {
    // intbig/bigint are dynamic BigInt types, no range checking needed
    if (isDynamicIntType(typeName)) return val;

    auto& info = getIntTypeInfo(typeName);

    // Special handling for uint64: store as uint64_t natively
    if (info.isUint64Full) {
        uint64_t uv;
        if (val.isUInt()) {
            uv = val.asUInt();
        } else if (val.isInt()) {
            // Negative int wraps around: -1 → UINT64_MAX
            uv = static_cast<uint64_t>(val.asInt());
        } else if (val.isBigInt()) {
            BigInt bv = val.asBigInt();
            // Wrap into [0, UINT64_MAX] via modular reduction
            BigInt range = BigInt("18446744073709551616"); // 2^64
            BigInt rem = bv % range;
            if (rem < BigInt(int64_t(0))) rem = rem + range;
            if (rem.fitsInt64()) uv = static_cast<uint64_t>(rem.toInt64());
            else {
                // Value exceeds int64 but fits uint64 — extract from string
                std::string s = rem.toString();
                uv = std::stoull(s);
            }
        } else if (val.isFloat()) {
            uv = static_cast<uint64_t>(val.asFloat());
        } else if (val.isBool()) {
            uv = val.asBool() ? 1 : 0;
        } else {
            throw RuntimeError("Cannot assign non-numeric value to " + typeName +
                              " at line " + std::to_string(line));
        }
        return CpctValue(uv);
    }

    // For other unsigned types (uint8~uint32), convert to uint64_t then range-check
    bool isUnsigned = isUnsignedType(typeName);

    // Convert to int64 for range checking
    int64_t v;
    if (val.isInt()) {
        v = val.asInt();
    } else if (val.isUInt()) {
        uint64_t uv = val.asUInt();
        // If value fits int64, use directly; otherwise wrap via BigInt
        if (uv <= static_cast<uint64_t>(INT64_MAX)) {
            v = static_cast<int64_t>(uv);
        } else {
            // Large uint64 → BigInt path for wrapping
            BigInt bv(uv);
            uint64_t range = static_cast<uint64_t>(info.maxVal) - static_cast<uint64_t>(info.minVal) + 1;
            BigInt rangeBI(static_cast<int64_t>(range));
            BigInt rem = bv % rangeBI;
            v = rem.fitsInt64() ? rem.toInt64() : 0;
            if (v > info.maxVal) v -= static_cast<int64_t>(range);
            else if (v < info.minVal) v += static_cast<int64_t>(range);
            if (isUnsigned) return CpctValue(static_cast<uint64_t>(v));
            return CpctValue(v);
        }
    } else if (val.isBigInt()) {
        BigInt bv = val.asBigInt();
        uint64_t range = static_cast<uint64_t>(info.maxVal) - static_cast<uint64_t>(info.minVal) + 1;
        BigInt rangeBI(static_cast<int64_t>(range));
        BigInt rem = bv % rangeBI;
        v = rem.fitsInt64() ? rem.toInt64() : 0;
        if (v > info.maxVal) v -= static_cast<int64_t>(range);
        else if (v < info.minVal) v += static_cast<int64_t>(range);
        if (isUnsigned) return CpctValue(static_cast<uint64_t>(v));
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
    if (v < info.minVal || v > info.maxVal) {
        uint64_t range = static_cast<uint64_t>(info.maxVal) - static_cast<uint64_t>(info.minVal) + 1;
        int64_t offset = v - info.minVal;
        uint64_t uoffset = static_cast<uint64_t>(offset) % range;
        v = static_cast<int64_t>(uoffset) + info.minVal;
    }
    if (isUnsigned) return CpctValue(static_cast<uint64_t>(v));
    return CpctValue(v);
}

CpctValue Interpreter::makeDefaultValue(const std::string& typeName) {
    if (isUnsignedType(typeName)) return CpctValue(uint64_t(0));
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
    if (!idx.isInt() && !idx.isUInt()) throw RuntimeError("Array index must be an integer at line " + std::to_string(line));

    int64_t i = idx.toInt64();
    auto& vec = arr.asArray();
    int64_t size = static_cast<int64_t>(vec.size());

    // Negative index support
    if (i < 0) i += size;
    if (i < 0 || i >= size) {
        throw RuntimeError("Array index out of bounds: " + idx.toString() +
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
