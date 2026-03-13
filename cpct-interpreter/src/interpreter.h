// interpreter.h
// CPC interpreter runtime system header.
// Defines core types for a tree-walking interpreter that directly traverses the AST.
// Includes runtime values (CpctValue), containers (TypedArray, CpctDict), scopes (Environment),
// control flow exceptions (ReturnSignal, etc.), and the Interpreter execution engine.
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

// Type tag for arrays with a fixed element type.
// Used to identify the active member of the std::variant inside TypedArray.
enum class ArrayElemType { Int8, Int16, Int32, Int64, UInt8, UInt16, UInt32, UInt64, Float32, Float64, Bool };

// Memory-efficient type-specialized array storage.
// Uses std::variant<vector<T>...> to allocate only one vector matching the element type.
// bool[] is stored internally as vector<uint8_t> to save memory.
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

    // Returns the byte size per element type (Bool is treated as 1 byte).
    size_t elemSize() const {
        switch (elemType) {
            case ArrayElemType::Int8: case ArrayElemType::UInt8: case ArrayElemType::Bool: return 1;
            case ArrayElemType::Int16: case ArrayElemType::UInt16: return 2;
            case ArrayElemType::Int32: case ArrayElemType::UInt32: case ArrayElemType::Float32: return 4;
            case ArrayElemType::Int64: case ArrayElemType::UInt64: case ArrayElemType::Float64: return 8;
        }
        return 0;
    }

    // Total bytes occupied by the array (size * elemSize).
    size_t totalBytes() const { return size() * elemSize(); }

    // Defined in interpreter.cpp (needs complete CpctValue)
    CpctValue get(size_t i) const;
    void set(size_t i, const CpctValue& val);

    // Creates a zero-initialized TypedArray with the given type and size.
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

// Ordered dictionary with O(1) lookup while preserving insertion order.
// dict (untyped): uses tagged prefix keys like "s:foo" / "i:42" in strIndex.
// map  (typed): integer keys → intIndex (int64 hash, no string conversion)
//                 string keys → strIndex (no tag prefix needed)
struct CpctDict {
    std::string keyType;     // "" for untyped dict, "string"/"int"/etc. for typed map
    std::string valueType;   // "" for untyped dict, "int"/"float"/etc. for typed map
    // Parallel key/value vectors to maintain insertion order
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
    // Rebuilds strIndex/intIndex entirely from the keys/values vectors.
    void rebuildIndex();

    // Hash helpers
    static std::string taggedHash(const CpctValue& key);   // "s:foo", "i:42" — for untyped dict
    static int64_t intHash(const CpctValue& key);           // raw int64 — for int-keyed map
};

// Runtime value variant type alias.
// Signed integers are stored as int64_t, unsigned as uint64_t.
// intbig/bigint values auto-promote to BigInt on overflow.
using Value = std::variant<int64_t, uint64_t, double, bool, std::string, std::vector<CpctValue>, BigInt, TypedArray, CpctDict>;

// Core type holding all runtime values of the CPC language.
// Constructors distinguish int64_t / uint64_t when storing into the variant,
// and the BigInt constructor auto-demotes to int64_t if the value fits.
struct CpctValue {
    Value data;

    CpctValue() : data(int64_t(0)) {}
    CpctValue(int v) : data(static_cast<int64_t>(v)) {}
    CpctValue(int64_t v) : data(v) {}
    CpctValue(uint64_t v) : data(v) {}
    CpctValue(double v) : data(v) {}
    CpctValue(bool v) : data(v) {}
    CpctValue(const std::string& v) : data(v) {}
    CpctValue(const char* v) : data(std::string(v)) {}
    CpctValue(std::vector<CpctValue> v) : data(std::move(v)) {}
    CpctValue(TypedArray v) : data(std::move(v)) {}
    CpctValue(CpctDict v) : data(std::move(v)) {}
    // Demotes BigInt to int64_t if it fits, avoiding unnecessary BigInt allocation.
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
    bool isUInt() const { return std::holds_alternative<uint64_t>(data); }
    bool isBigInt() const { return std::holds_alternative<BigInt>(data); }
    // Returns true if the value is one of int64_t, uint64_t, or BigInt (i.e., an integer).
    bool isNumericInt() const { return isInt() || isUInt() || isBigInt(); }
    bool isFloat() const { return std::holds_alternative<double>(data); }
    bool isBool() const { return std::holds_alternative<bool>(data); }
    bool isString() const { return std::holds_alternative<std::string>(data); }
    bool isArray() const { return std::holds_alternative<std::vector<CpctValue>>(data); }
    bool isTypedArray() const { return std::holds_alternative<TypedArray>(data); }
    bool isAnyArray() const { return isArray() || isTypedArray(); }
    bool isDict() const { return std::holds_alternative<CpctDict>(data); }

    int64_t asInt() const { return std::get<int64_t>(data); }
    uint64_t asUInt() const { return std::get<uint64_t>(data); }
    const BigInt& asBigInt() const { return std::get<BigInt>(data); }
    // Promotes int64_t / uint64_t / BigInt to BigInt and returns it.
    BigInt toBigInt() const {
        if (isInt()) return BigInt(asInt());
        if (isUInt()) return BigInt(asUInt());
        if (isBigInt()) return asBigInt();
        throw std::runtime_error("Not an integer value");
    }
    // Converts an integer type (int64/uint64/BigInt) to int64_t.
    // Reinterprets the bit pattern without sign/range checking — caller must ensure it fits.
    int64_t toInt64() const {
        if (isInt()) return asInt();
        if (isUInt()) return static_cast<int64_t>(asUInt());
        if (isBigInt()) return asBigInt().toInt64();
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

    // Defined in interpreter.cpp (requires complete CpctValue definition).
    double toNumber() const;
    bool toBool() const;
    std::string toString() const;
};

// ============================================================
// Safe signed/unsigned integer comparison helpers (C++17, replaces std::cmp_*).
// Prevents UB (Undefined Behavior) from signed-unsigned comparisons.
// int64 < uint64 comparison: always true if negative, otherwise cast to uint64 and compare.
// ============================================================
inline bool safeCmpLess(int64_t a, uint64_t b) {
    return a < 0 || static_cast<uint64_t>(a) < b;
}
inline bool safeCmpLess(uint64_t a, int64_t b) {
    return b >= 0 && a < static_cast<uint64_t>(b);
}
inline bool safeCmpLess(int64_t a, int64_t b) { return a < b; }
inline bool safeCmpLess(uint64_t a, uint64_t b) { return a < b; }

inline bool safeCmpEqual(int64_t a, uint64_t b) {
    return a >= 0 && static_cast<uint64_t>(a) == b;
}
inline bool safeCmpEqual(uint64_t a, int64_t b) {
    return b >= 0 && a == static_cast<uint64_t>(b);
}
inline bool safeCmpEqual(int64_t a, int64_t b) { return a == b; }
inline bool safeCmpEqual(uint64_t a, uint64_t b) { return a == b; }

inline bool safeCmpLessEqual(int64_t a, uint64_t b) {
    return a < 0 || static_cast<uint64_t>(a) <= b;
}
inline bool safeCmpLessEqual(uint64_t a, int64_t b) {
    return b >= 0 && a <= static_cast<uint64_t>(b);
}
inline bool safeCmpLessEqual(int64_t a, int64_t b) { return a <= b; }
inline bool safeCmpLessEqual(uint64_t a, uint64_t b) { return a <= b; }

// Valid range information per integer type.
// uint64 is special-cased with isUint64Full flag since its max value doesn't fit in int64_t.
struct IntTypeInfo {
    int64_t minVal;
    int64_t maxVal;      // For uint64, maxVal is INT64_MAX (actual max handled via isUint64Full)
    bool isUint64Full;   // true only for uint64 (max exceeds int64_t range)
    const char* name;
};

// Returns IntTypeInfo for a given type name. Unknown types fall back to int (int32).
// Each IntTypeInfo is created once as a static local variable and reused.
inline const IntTypeInfo& getIntTypeInfo(const std::string& typeName) {
    static const IntTypeInfo info_char   = { INT8_MIN,  INT8_MAX,  false, "char"   };
    static const IntTypeInfo info_int8   = { INT8_MIN,  INT8_MAX,  false, "int8"   };
    static const IntTypeInfo info_int16  = { INT16_MIN, INT16_MAX, false, "int16"  };
    static const IntTypeInfo info_int32  = { INT32_MIN, INT32_MAX, false, "int32"  };
    static const IntTypeInfo info_int64  = { INT64_MIN, INT64_MAX, false, "int64"  };
    static const IntTypeInfo info_int    = { INT32_MIN, INT32_MAX, false, "int"    }; // int = int32 fixed
    static const IntTypeInfo info_intbig = { INT32_MIN, INT32_MAX, false, "intbig" }; // intbig = starts at int32 range, promotes to BigInt
    static const IntTypeInfo info_uint8  = { 0, UINT8_MAX,  false, "uint8"  };
    static const IntTypeInfo info_uint16 = { 0, UINT16_MAX, false, "uint16" };
    static const IntTypeInfo info_uint32 = { 0, static_cast<int64_t>(UINT32_MAX), false, "uint32" };
    static const IntTypeInfo info_uint64 = { 0, 0, true, "uint64" }; // full uint64 range, stored as uint64_t
    static const IntTypeInfo info_uint   = { 0, static_cast<int64_t>(UINT32_MAX), false, "uint" }; // uint = uint32

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

// Checks if the type is a signed/unsigned fixed-size or BigInt-family integer type.
inline bool isIntegerType(const std::string& typeName) {
    return typeName == "int" || typeName == "int8" || typeName == "int16" ||
           typeName == "int32" || typeName == "int64" ||
           typeName == "intbig" || typeName == "bigint" ||
           typeName == "uint" || typeName == "uint8" || typeName == "uint16" ||
           typeName == "uint32" || typeName == "uint64";
}

// Checks if the type is a fixed-size integer that wraps on overflow (excludes intbig/bigint).
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

// intbig / bigint: dynamic integer types that auto-promote to BigInt on overflow.
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

// Checks if the type string starts with "map<..." (typed map declaration).
inline bool isMapType(const std::string& typeName) {
    return typeName.size() >= 4 && typeName.substr(0, 4) == "map<";
}

// Splits "map<KeyType,ValueType>" into a pair of key type and value type strings.
inline std::pair<std::string, std::string> mapKeyValueTypes(const std::string& typeName) {
    // "map<string,int>" -> ("string", "int")
    auto inner = typeName.substr(4, typeName.size() - 5); // strip "map<" and ">"
    auto comma = inner.find(',');
    return { inner.substr(0, comma), inner.substr(comma + 1) };
}

// Checks if the type string is a "vector<ElemType>" dynamic array declaration.
inline bool isVectorType(const std::string& typeName) {
    return typeName.size() >= 8 && typeName.substr(0, 7) == "vector<" && typeName.back() == '>';
}

// Extracts the element type string from "vector<ElemType>".
inline std::string vectorElemType(const std::string& typeName) {
    // "vector<int>" -> "int"
    return typeName.substr(7, typeName.size() - 8);
}

// Treats a type string ending with "[]" as a static array type (e.g., "int[]", "float[][]").
inline bool isArrayType(const std::string& typeName) {
    return typeName.size() >= 2 && typeName.substr(typeName.size() - 2) == "[]";
}

// Strips all "[]" suffixes to return the base element type, e.g., "int[][]" → "int".
inline std::string arrayBaseType(const std::string& typeName) {
    // Strip all trailing "[]" to get the base element type
    std::string base = typeName;
    while (base.size() >= 2 && base.substr(base.size() - 2) == "[]") {
        base = base.substr(0, base.size() - 2);
    }
    return base;
}

// Returns the array dimension count, e.g., "int[][]" → 2.
inline int arrayDimCount(const std::string& typeName) {
    int count = 0;
    std::string s = typeName;
    while (s.size() >= 2 && s.substr(s.size() - 2) == "[]") {
        count++;
        s = s.substr(0, s.size() - 2);
    }
    return count;
}

// Converts an element type name string to an ArrayElemType enum value.
// Unknown types fall back to Int64.
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

// Checks if a type string ends with '@' (reference type).
inline bool isReferenceType(const std::string& typeName) {
    return !typeName.empty() && typeName.back() == '@';
}

// Strips the trailing '@' from a reference type string to get the base type.
inline std::string stripRefQualifier(const std::string& typeName) {
    if (isReferenceType(typeName)) return typeName.substr(0, typeName.size() - 1);
    return typeName;
}

// Checks if the element type can use TypedArray.
// string, intbig, bigint cannot use TypedArray since they are not fixed-size elements.
inline bool supportsTypedArray(const std::string& baseType) {
    // intbig/bigint can overflow to BigInt, so can't use fixed-size TypedArray
    return baseType != "string" && baseType != "intbig" && baseType != "bigint";
}

// ============================================================
// Control flow signal exceptions.
// Implements return / break / continue semantics using C++ exception mechanism.
// The Interpreter's execution loop catches these exceptions and handles them accordingly.
// ============================================================

// Wraps the return value and propagates it up to the calling function.
struct ReturnSignal { CpctValue value; };
// Thrown to exit the nearest enclosing loop/switch.
struct BreakSignal {};
// Thrown to skip the current loop iteration.
struct ContinueSignal {};

class RuntimeError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Scope chain (lexical environment).
// Maintains two maps (name → value / declared type) and links to the outer scope via parent_.
// Variable lookup starts at the current scope and walks up the chain to global scope.
class Environment {
public:
    explicit Environment(Environment* parent = nullptr);
    // Declares a new variable in the current scope (overwrites if already exists).
    void define(const std::string& name, CpctValue value, const std::string& type = "");
    // Declares a reference binding: name becomes an alias for targetName in targetEnv.
    void defineRef(const std::string& name, Environment* targetEnv, const std::string& targetName, const std::string& type = "");
    // Walks the chain to find the variable and returns a reference. Throws RuntimeError if not found.
    CpctValue& get(const std::string& name);
    // Walks the chain to find the variable and updates its value. Throws RuntimeError if not found.
    void set(const std::string& name, CpctValue value);
    bool has(const std::string& name) const;
    std::string getType(const std::string& name) const;
    // Returns true if name is a reference binding in this scope.
    bool isRef(const std::string& name) const;
    // Returns the Environment that owns the variable (for ref binding at call sites).
    Environment* findOwner(const std::string& name);

private:
    std::unordered_map<std::string, CpctValue> vars_;
    std::unordered_map<std::string, std::string> types_; // declared types (used for type coercion checks)
    // Reference bindings: name → (target Environment, target variable name)
    std::unordered_map<std::string, std::pair<Environment*, std::string>> refs_;
    Environment* parent_; // outer scope (nullptr if none)
};

// Function definition record stored at runtime.
// Holds the parameter list and non-owning pointers to body AST nodes.
struct FuncDef {
    std::string returnType;
    std::vector<FuncParam> params;
    std::vector<Stmt*> body; // non-owning pointers
};

// Tree-walking interpreter core.
// run() executes the entire program, eval() evaluates individual expressions.
// Manages the global Environment and the currently active Environment pointer.
class Interpreter {
public:
    Interpreter();
    // Executes the parsed AST top-level statements in order.
    void run(const std::vector<StmtPtr>& program);
    // Recursively evaluates an expression node and returns a CpctValue.
    CpctValue eval(const Expr* expr);

private:
    Environment globalEnv_;
    Environment* currentEnv_; // currently active scope (swapped on function call / block entry)
    std::unordered_map<std::string, FuncDef> functions_; // global function table

    // ---- Statement execution methods ----
    // Dispatches to the appropriate exec* function based on stmt's dynamic type.
    void execStmt(const Stmt* stmt);
    void execVarDecl(const Stmt* stmt);   // variable declaration and initialization
    void execBlock(const Stmt* stmt);     // block statement (creates new scope)
    // Sets the given Environment as the active scope and executes the statement list.
    void execBlockWithEnv(const std::vector<StmtPtr>& stmts, Environment& env);
    void execIf(const Stmt* stmt);        // if / else if / else branching
    void execWhile(const Stmt* stmt);     // while loop (handles BreakSignal/ContinueSignal)
    void execDoWhile(const Stmt* stmt);   // do-while loop
    void execFor(const Stmt* stmt);       // C-style for loop
    void execForEach(const Stmt* stmt);   // for-each (iterates over arrays/dicts)
    void execForN(const Stmt* stmt);      // for(N) shorthand repeat syntax
    void execFuncDecl(const Stmt* stmt);  // registers function declaration in functions_ table
    void execReturn(const Stmt* stmt);    // throws ReturnSignal to return from function
    void execSwitch(const Stmt* stmt);    // switch-case (handles BreakSignal)
    void execPrint(const Stmt* stmt);     // built-in print statement

    // ---- Expression evaluation methods ----
    CpctValue evalBinaryOp(const Expr* expr);            // +, -, *, /, %, comparison, logical ops
    CpctValue evalUnaryOp(const Expr* expr);             // !, -, ++, --
    CpctValue evalAssign(const Expr* expr);              // = simple assignment
    CpctValue evalCompoundAssign(const Expr* expr);      // +=, -=, *=, /= compound assignment
    CpctValue evalFunctionCall(const Expr* expr);        // user-defined and built-in function calls
    CpctValue evalArrayAccess(const Expr* expr);         // arr[i] array element read
    CpctValue evalArrayAssign(const Expr* expr);         // arr[i] = val array element write
    CpctValue evalArrayCompoundAssign(const Expr* expr); // arr[i] += val array compound assignment
    CpctValue evalArraySlice(const Expr* expr);          // arr[a:b] slicing

    // ---- Helper methods ----
    // Coerces val to match the array element type baseType.
    CpctValue coerceArrayElement(const std::string& baseType, CpctValue val, int line);
    // Coerces val to match the map key type keyType.
    CpctValue coerceDictKey(const std::string& keyType, CpctValue key, int line);
    // Coerces val to match the map value type valType.
    CpctValue coerceDictValue(const std::string& valType, CpctValue val, int line);
    // Creates the default value (0, false, "", etc.) for the given type name.
    CpctValue makeDefaultValue(const std::string& typeName);
    // Evaluates index expressions in a multi-dimensional array and returns a reference to the element.
    CpctValue& resolveArrayElement(CpctValue& arr, const Expr* indexExpr, int line);
    // Looks up name in the function table, calls it with args, and returns the result.
    // argExprs are the raw AST expression nodes (needed to detect @ref arguments).
    CpctValue callFunction(const std::string& name, const std::vector<CpctValue>& args,
                           const std::vector<ExprPtr>* argExprs, int line);
    // Throws RuntimeError if val is outside typeName's valid range (intbig promotes to BigInt).
    CpctValue checkIntRange(const std::string& typeName, CpctValue val, int line);
    // Adjusts val's precision to match float32/float64 type and returns it.
    CpctValue coerceFloat(const std::string& typeName, CpctValue val, int line);
};
