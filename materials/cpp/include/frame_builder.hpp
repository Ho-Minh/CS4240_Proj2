#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>

#include "ir.hpp"

namespace ircpp {

struct FrameInfo {
    std::unordered_map<std::string,int> varOffset; // offset from $fp (>=8)
    std::unordered_set<std::string> paramArrayNames; // array params passed by pointer
    std::unordered_set<std::string> localArrayNames; // arrays allocated in frame
    int frameBytes{0};
};

// Build stack frame layout for a function
FrameInfo buildFrame(const IRFunction& func);

// Qualify an IR label with function name to ensure uniqueness
std::string qualLabel(const std::string& fn, const std::string& lbl);

} // namespace ircpp


