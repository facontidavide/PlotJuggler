#pragma once

#include <optional>
#include <string>
#include <unordered_map>

namespace mosaico {

class TagResolver {
 public:
    void map(const std::string& source_tag, const std::string& ontology_tag);
    std::optional<std::string> resolve(const std::string& source_tag) const;

 private:
    std::unordered_map<std::string, std::string> map_;
};

TagResolver createRosTagResolver();

}  // namespace mosaico
