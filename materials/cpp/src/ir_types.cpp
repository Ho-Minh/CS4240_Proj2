#include "ir.hpp"

using namespace ircpp;

namespace {
struct TypePtrHash {
    std::size_t operator()(const std::shared_ptr<IRType>& p) const noexcept {
        return std::hash<const void*>()(p.get());
    }
};
struct TypePtrEq {
    bool operator()(const std::shared_ptr<IRType>& a, const std::shared_ptr<IRType>& b) const noexcept {
        return a.get() == b.get();
    }
};
}

static std::shared_ptr<IRIntType> g_int_singleton;
static std::shared_ptr<IRFloatType> g_float_singleton;
// Cache: key = element type singleton, value = map from size to array type instance
static std::unordered_map<std::shared_ptr<IRType>, std::unordered_map<int, std::shared_ptr<IRArrayType>>, TypePtrHash, TypePtrEq> g_array_cache;

std::shared_ptr<IRIntType> IRIntType::get() {
    if (!g_int_singleton) g_int_singleton = std::shared_ptr<IRIntType>(new IRIntType());
    return g_int_singleton;
}

std::shared_ptr<IRFloatType> IRFloatType::get() {
    if (!g_float_singleton) g_float_singleton = std::shared_ptr<IRFloatType>(new IRFloatType());
    return g_float_singleton;
}

IRArrayType::IRArrayType(std::shared_ptr<IRType> t, int s) : elementType(std::move(t)), size(s) {}

std::shared_ptr<IRArrayType> IRArrayType::get(std::shared_ptr<IRType> elementType, int size) {
    auto& perElem = g_array_cache[elementType];
    auto it = perElem.find(size);
    if (it != perElem.end()) return it->second;
    auto created = std::shared_ptr<IRArrayType>(new IRArrayType(elementType, size));
    perElem[size] = created;
    return created;
}


