// src/flight/utils.hpp — shared helpers for the Mosaico Flight layer.
#pragma once

#include <string>

namespace mosaico {

// Strip leading slashes from a string.
inline std::string stripLeadingSlashes(const std::string& s) {
    size_t pos = s.find_first_not_of('/');
    return pos == std::string::npos ? "" : s.substr(pos);
}

// Build "seq_name/topic_name" resource string.
inline std::string packResource(const std::string& seq,
                                const std::string& topic) {
    return stripLeadingSlashes(seq) + "/" + stripLeadingSlashes(topic);
}

} // namespace mosaico
