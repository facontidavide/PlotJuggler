#pragma once

#include "ontology/ontology_builder.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace mosaico {

// Closed registry of ontology tag → builder. Populated eagerly in the
// constructor; the set of tags matches the Python SDK's built-in Serializable
// subclasses and must stay in lockstep with the server's known schemas.
class OntologyRegistry {
 public:
    OntologyRegistry();

    OntologyBuilder* find(const std::string& tag);

 private:
    std::unordered_map<std::string, std::unique_ptr<OntologyBuilder>> builders_;
};

}  // namespace mosaico
