#include "timing.hpp"

#include <cstdio>
#include <fstream>

namespace mosaico::test {

void TimingWaterfall::emit(const std::string& path) const {
    std::ofstream out(path);
    if (!out) {
        std::fprintf(stderr, "[TimingWaterfall] cannot open %s\n", path.c_str());
        return;
    }
    out << "{\n  \"label\": \"" << label_ << "\",\n  \"marks\": [\n";
    for (size_t i = 0; i < marks_.size(); ++i) {
        const auto& [name, us] = marks_[i];
        out << "    { \"name\": \"" << name << "\", \"us\": " << us << " }";
        if (i + 1 < marks_.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n}\n";
}

}  // namespace mosaico::test
