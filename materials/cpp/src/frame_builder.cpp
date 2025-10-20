#include "frame_builder.hpp"

namespace ircpp {

FrameInfo buildFrame(const IRFunction& func) {
    FrameInfo fi;
    // Reserve 8 bytes at frame top: 0($fp)=ra, 4($fp)=fp
    int off = 8;
    // Mark which variables are array parameters
    std::unordered_set<std::string> paramNames;
    for (const auto& p : func.parameters) if (p) paramNames.insert(p->getName());

    auto slotSize = [&](const std::shared_ptr<IRVariableOperand>& v){
        auto arr = std::dynamic_pointer_cast<IRArrayType>(v->type);
        if (!arr) return 4; // scalars
        if (paramNames.count(v->getName())) {
            // Array parameter: store pointer only
            fi.paramArrayNames.insert(v->getName());
            return 4;
        }
        // Local array: allocate full space in frame
        fi.localArrayNames.insert(v->getName());
        return arr->size * 4;
    };

    for (const auto& v : func.variables) {
        if (!v) continue;
        int sz = slotSize(v);
        fi.varOffset[v->getName()] = off;
        off += sz;
    }
    // Align to 8 bytes
    if (off % 8) off += (8 - (off % 8));
    fi.frameBytes = off;
    return fi;
}

std::string qualLabel(const std::string& fn, const std::string& lbl) {
    return fn + std::string("_") + lbl;
}

} // namespace ircpp


