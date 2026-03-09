#pragma once
#include "ast.h"
#include "bigint.h"
#include <string>
#include <variant>
#include <unordered_map>
#include <vector>
#include <functional>
#include <stdexcept>
#include <cstdint>
#include <limits>

// Forward declarations
struct CpctValue;
struct CpctDict;

// Type-specialized array storage for memory-efficient arrays
enum class ArrayElemType { Int8, Int16, Int32, Int64, UInt8, UInt16, UInt32, UInt64, Float32, Float64, Bool };

struct TypedArray {
    ArrayElemType elemType;
    std::variant<
        std::vector<int8_t>,
        std::vector<int16_t>,
        std::vector<int32_t>,
        std::vector<int64_t>,
        std::vector<uint8_t>,
        std::vector<uint16_t>,
        std::vector<uint32_t>,
        std::vector<uint64_t>,
        std::vector<float>,
        std::vector<double>
    > data;

    size_t size() const {
        return std::visit([](const auto& v) { return v.size(); }, data);
    }

    size_t elemSize() const {
        switch (elemType) {
            case ArrayElemType::Int8: case ArrayElemType::UInt8: case ArrayElemType::Bool: return 1;
            case ArrayElemType::Int16: case ArrayElemType::UInt16: return 2;
            case ArrayElemType::Int32: case ArrayElemType::UInt32: case ArrayElemType::Float32: return 4;
            case ArrayElemType::Int64: case ArrayElemType::UInt64: case ArrayElemType::Float64: return 8;
        }
        return 0;
    }

    size_t totalBytes() const { return size() * elemSize(); }

    // Defined in interpreter.cpp (needs complete CpctValue)
    CpctValue get(size_t i) const;
    void set(size_t i, const CpctValue& val);

    static TypedArray create(ArrayElemType type, size_t sz) {
        TypedArray ta;
        ta.elemType = type;
        switch (type) {
            case ArrayElemType::Int8:    ta.data = std::vector<int8_t>(sz, 0); break;
            case ArrayElemType::Int16:   ta.data = std::vector<int16_t>(sz, 0); break;
            case ArrayElemType::Int32:   ta.data = std::vector<int32_t>(sz, 0); break;
            case ArrayElemType::Int64:   ta.data = std::vector<int64_t>(sz, 0); break;
            case ArrayElemType::UInt8:   ta.data = std::vector<uint8_t>(sz, 0); break;
            case ArrayElemType::UInt16:  ta.data = std::vector<uint16_t>(sz, 0); break;
            case ArrayElemType::UInt32:  ta.data = std::vector<uint32_t>(sz, 0); break;
            case ArrayElemType::UInt64:  ta.data = std::vector<uint64_t>(sz, 0); break;
            case ArrayElemType::Float32: ta.data = std::vector<float>(sz, 0.0f); break;
            case ArrayElemType::Float64: ta.data = std::vector<double>(sz, 0.0); break;
            case ArrayElemType::Bool:    ta.data = std::vector<uint8_t>(sz, 0); break;
        }
        return ta;
    }
};

// Dictionary type — ordered insertion, O(1) lookup via hash index
// dict (untyped): uses strIndex with type-tagged keys ("s:", "i:", "b:")
// map  (typed):   int keys → intIndex (O(1) int hash, no string conversion)
//                 string keys → strIndex (no tag prefix needed)
struct CpctDict {
    std::string keyType;     // "" for untyped dict, "string"/"int"/etc. for typed map
    std::string valueType;   // "" for untyped dict, "int"/"float"/etc. for typed map
    std::vector<CpctValue> keys;
    std::vector<CpctValue> values;
    std::unordered_map<std::string, size_t> strIndex; // for untyped dict & string-keyed map
    std::unordered_map<int64_t, size_t> intIndex;     // for int/char/bool-keyed map

    size_t size() const { return keys.size(); }

    // Returns true if this dict/map uses int-based indexing (map with non-string key)
    bool usesIntKey() const { return !keyType.empty() && keyType != "string"; }

    // All defined in interpreter.cpp (needs complete CpctValue)
    bool has(const CpctValue& key) const;
    CpctValue& get(const CpctValue& key);
    const CpctValue& get(const CpctValue& key) const;
    void set(const CpctValue& key, const CpctValue& val);
    void remove(const CpctValue& key);
    void rebuildIndex();

    // Hash helpers
    static std::string taggedHash(const CpctValue& key);   // "s:foo", "i:42" — for untyped dict
    static int64_t intHash(const CpctValue& key);           // raw int64 — for int-keyed map
};

// Runtime value type — int stored as int64_t to support int8~int64 uniformly
// BigInt for 'intbig'/'bigint' types that auto-promote on overflow
using Value = std::variant<int64_t, double, bool, std::string, std::vector<CpctValue>, BigInt, TypedArray, CpctDict>;

struct CpctValue {
    Value data;

    CpctValue() : data(int64_t(0)) {}
    CpctValue(int v) : data(static_cast<int64_t>(v)) {}
    CpctValue(int64_t v) : data(v) {}
    CpctValue(double v) : data(v) {}
    CpctValue(bool v) : data(v) {}
    CpctValue(const std::string& v) : data(v) {}
    CpctValue(const char* v) : data(std::string(v)) {}
    CpctValue(std::vector<CpctValue> v) : data(std::move(v)) {}
    CpctValue(TypedArray v) : data(std::move(v)) {}
    CpctValue(CpctDict v) : data(std::move(v)) {}
    CpctValue(const BigInt& v) {
        // Auto-demote to int64 if it fits
        if (v.fitsInt64()) data = v.toInt64();
        else data = v;
    }
    CpctValue(BigInt&& v) {
        if (v.fitsInt64()) data = v.toInt64();
        else data = std::move(v);
    }

    bool isInt() const { return std::holds_alternative<int64_t>(data); }
    bool isBigInt() const { return std::holds_alternative<BigInt>(data); }
    bool isNumericInt() const { return isInt() || isBigInt(); }
    bool isFloat() const { return std::holds_alternative<double>(data); }
    bool isBool() const { return std::holds_alternative<bool>(data); }
    bool isString() const { return std::holds_alternative<std::string>(data); }
    bool isArray() const { return std::holds_alternative<std::vector<CpctValue>>(data); }
    bool isTypedArray() const { return std::holds_alternative<TypedArray>(data); }
    bool isAnyArray() const { return isArray() || isTypedArray(); }
    bool isDict() const { return std::holds_alternative<CpctDict>(data); }

    int64_t asInt() const { return std::get<int64_t>(data); }
    const BigInt& asBigInt() const { return std::get<BigInt>(data); }
    BigInt toBigInt() const {
        if (isInt()) return BigInt(asInt());
        if (isBigInt()) return asBigInt();
        throw std::runtime_error("Not an integer value");
    }
    double asFloat() const { return std::get<double>(data); }
    bool asBool() const { return std::get<bool>(data); }
    const std::string& asString() const { return std::get<std::string>(data); }
    std::vector<CpctValue>& asArray() { return std::get<std::vector<CpctValue>>(data); }
    const std::vector<CpctValue>& asArray() const { return std::get<std::vector<CpctValue>>(data); }
    TypedArray& asTypedArray() { return std::get<TypedArray>(data); }
    const TypedArray& asTypedArray() const { return std::get<TypedArray>(data); }
    CpctDict& asDict() { return std::get<CpctDict>(data); }
    const CpctDict& asDict() const { return std::get<CpctDict>(data); }

    double toNumber() const;
    bool toBool() const;
    std::string toString() const;
};

// Integer type range information
struct IntTypeInfo {
    int64_t minVal;
    int64_t maxVal;
    const char* name;
};

inline const IntTypeInfo& getIntTypeInfo(const std::string& typeName) {
    static const IntTypeInfo info_char   = { INT8_MIN,  INT8_MAX,  "char"   };
    static const IntTypeInfo info_int8   = { INT8_MIN,  INT8_MAX,  "int8"   };
    static const IntTypeInfo info_int16  = { INT16_MIN, INT16_MAX, "int16"  };
    static const IntTypeInfo info_int32  = { INT32_MIN, INT32_MAX, "int32"  };
    static const IntTypeInfo info_int64  = { INT64_MIN, INT64_MAX, "int64"  };
    static const IntTypeInfo info_int    = { INT32_MIN, INT32_MAX, "int"    }; // int = int32 고정
    static const IntTypeInfo info_intbig = { INT32_MIN, INT32_MAX, "intbig" }; // intbig = int32 범위 시작, BigInt 승격
    static const IntTypeInfo info_uint8  = { 0, UINT8_MAX,  "uint8"  };
    static const IntTypeInfo info_uint16 = { 0, UINT16_MAX, "uint16" };
    static const IntTypeInfo info_uint32 = { 0, static_cast<int64_t>(UINT32_MAX), "uint32" };
    static const IntTypeInfo info_uint64 = { 0, INT64_MAX,  "uint64" }; // uint64 max는 int64로 표현 불가, 별도 처리
    static const IntTypeInfo info_uint   = { 0, static_cast<int64_t>(UINT32_MAX), "uint" }; // uint = uint32

    if (typeName == "char")   return info_char;
    if (typeName == "int8")   return info_int8;
    if (typeName == "int16")  return info_int16;
    if (typeName == "int32")  return info_int32;
    if (typeName == "int64")  return info_int64;
    if (typeName == "intbig") return info_intbig;
    if (typeName == "uint8")  return info_uint8;
    if (typeName == "uint16") return info_uint16;
    if (typeName == "uint32") return info_uint32;
    if (typeName == "uint64") return info_uint64;
    if (typeName == "uint")   return info_uint;
    return info_int;
}

inline bool isIntegerType(const std::string& typeName) {
    return typeName == "int" || typeName == "int8" || typeName == "int16" ||
           typeName == "int32" || typeName == "int64" ||
           typeName == "intbig" || typeName == "bigint" ||
           typeName == "uint" || typeName == "uint8" || typeName == "uint16" ||
           typeName == "uint32" || typeName == "uint64";
}

inline bool isSizedIntegerType(const std::string& typeName) {
    return typeName == "int" || typeName == "int8" || typeName == "int16" ||
           typeName == "int32" || typeName == "int64" ||
           typeName == "uint" || typeName == "uint8" || typeName == "uint16" ||
           typeName == "uint32" || typeName == "uint64";
}

inline bool isUnsignedType(const std::string& typeName) {
    return typeName == "uint" || typeName == "uint8" || typeName == "uint16" ||
           typeName == "uint32" || typeName == "uint64";
}

inline bool isDynamicIntType(const std::string& typeName) {
    return typeName == "intbig" || typeName == "bigint";
}

inline bool isCharType(const std::string& typeName) {
    return typeName == "char";
}

inline bool isFloatType(const std::string& typeName) {
    return typeName == "float" || typeName == "float32" || typeName == "float64";
}

inline bool isDictType(const std::string& typeName) {
    return typeName == "dict";
}

inline bool isMapType(const std::string& typeName) {
    return typeName.size() >= 4 && typeName.substr(0, 4) == "map<";
}

inline std::pair<std::string, std::string> mapKeyValueTypes(const std::string& typeName) {
    // "map<string,int>" -> ("string", "int")
    auto inner = typeName.substr(4, typeName.size() - 5); // strip "map<" and ">"
    auto comma = inner.find(',');
    return { inner.substr(0, comma), inner.substr(comma + 1) };
}

inline bool isVectorType(const std::string& typeName) {
    return typeName.size() >= 8 && typeName.substr(0, 7) == "vector<" && typeName.back() == '>';
}

inline std::string vectorElemType(const std::string& typeName) {
    // "vector<int>" -> "int"
    return typeName.substr(7, typeName.size() - 8);
}

inline bool isArrayType(const std::string& typeName) {
    return typeName.size() >= 2 && typeName.substr(typeName.size() - 2) == "[]";
}

inline std::string arrayBaseType(const std::string& typeName) {
    // Strip all trailing "[]" to get the base element type
    std::string base = typeName;
    while (base.size() >= 2 && base.substr(base.size() - 2) == "[]") {
        base = base.substr(0, base.size() - 2);
    }
    return base;
}

inline int arrayDimCount(const std::string& typeName) {
    int count = 0;
    std::string s = typeName;
    while (s.size() >= 2 && s.substr(s.size() - 2) == "[]") {
        count++;
        s = s.substr(0, s.size() - 2);
    }
    return count;
}

inline ArrayElemType getArrayElemType(const std::string& baseType) {
    if (baseType == "int8" || baseType == "char") return ArrayElemType::Int8;
    if (baseType == "int16") return ArrayElemType::Int16;
    if (baseType == "int" || baseType == "int32") return ArrayElemType::Int32;
    if (baseType == "int64") return ArrayElemType::Int64;
    if (baseType == "uint8") return ArrayElemType::UInt8;
    if (baseType == "uint16") return ArrayElemType::UInt16;
    if (baseType == "uint" || baseType == "uint32") return ArrayElemType::UInt32;
    if (baseType == "uint64") return ArrayElemType::UInt64;
    if (baseType == "float32") return ArrayElemType::Float32;
    if (baseType == "float" || baseType == "float64") return ArrayElemType::Float64;
    if (baseType == "bool") return ArrayElemType::Bool;
    return ArrayElemType::Int64;
}

inline bool supportsTypedArray(const std::string& baseType) {
    // intbig/bigint can overflow to BigInt, so can't use fixed-size TypedArray
    return baseType != "string" && baseType != "intbig" && baseType != "bigint";
}

// Signal exceptions for control flow
struct ReturnSignal { CpctValue value; };
struct BreakSignal {};
struct ContinueSignal {};

class RuntimeError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Environment (scope) — tracks both value and declared type
class Environment {
public:
    explicit Environment(Environment* parent = nullptr);
    void define(const std::string& name, CpctValue value, const std::string& type = "");
    CpctValue& get(const std::string& name);
    void set(const std::string& name, CpctValue value);
    bool has(const std::string& name) const;
    std::string getType(const std::string& name) const;

private:
    std::unordered_map<std::string, CpctValue> vars_;
    std::unordered_map<std::string, std::string> types_;
    Environment* parent_;
};

// Function definition stored at runtime
struct FuncDef {
    std::string returnType;
    std::vector<FuncParam> params;
    std::vector<Stmt*> body; // non-owning pointers
};

class Interpreter {
public:
    Interpreter();
    void run(const std::vector<StmtPtr>& program);
    CpctValue eval(const Expr* expr);

private:
    Environment globalEnv_;
    Environment* currentEnv_;
    std::unordered_map<std::string, FuncDef> functions_;

    // Statements
    void execStmt(const Stmt* stmt);
    void execVarDecl(const Stmt* stmt);
    void execBlock(const Stmt* stmt);
    void execBlockWithEnv(const std::vector<StmtPtr>& stmts, Environment& env);
    void execIf(const Stmt* stmt);
    void execWhile(const Stmt* stmt);
    void execDoWhile(const Stmt* stmt);
    void execFor(const Stmt* stmt);
    void execForEach(const Stmt* stmt);
    void execForN(const Stmt* stmt);
    void execFuncDecl(const Stmt* stmt);
    void execReturn(const Stmt* stmt);
    void execSwitch(const Stmt* stmt);
    void execPrint(const Stmt* stmt);

    // Expressions
    CpctValue evalBinaryOp(const Expr* expr);
    CpctValue evalUnaryOp(const Expr* expr);
    CpctValue evalAssign(const Expr* expr);
    CpctValue evalCompoundAssign(const Expr* expr);
    CpctValue evalFunctionCall(const Expr* expr);
    CpctValue evalArrayAccess(const Expr* expr);
    CpctValue evalArrayAssign(const Expr* expr);
    CpctValue evalArrayCompoundAssign(const Expr* expr);
    CpctValue evalArraySlice(const Expr* expr);

    // Helpers
    CpctValue coerceArrayElement(const std::string& baseType, CpctValue val, int line);
    CpctValue coerceDictKey(const std::string& keyType, CpctValue key, int line);
    CpctValue coerceDictValue(const std::string& valType, CpctValue val, int line);
    CpctValue makeDefaultValue(const std::string& typeName);
    CpctValue& resolveArrayElement(CpctValue& arr, const Expr* indexExpr, int line);
    CpctValue callFunction(const std::string& name, const std::vector<CpctValue>& args, int line);
    CpctValue checkIntRange(const std::string& typeName, CpctValue val, int line);
    CpctValue coerceFloat(const std::string& typeName, CpctValue val, int line);
};
