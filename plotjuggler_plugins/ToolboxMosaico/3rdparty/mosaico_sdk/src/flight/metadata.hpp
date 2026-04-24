#pragma once

#include <arrow/type_fwd.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace mosaico {

// Extract ontology tag from Arrow schema KeyValueMetadata.
// Parses "mosaico:properties" JSON, returns "ontology_tag" field.
std::optional<std::string> extractOntologyTag(
    const std::shared_ptr<arrow::KeyValueMetadata>& metadata);

// Extract user metadata from Arrow schema KeyValueMetadata.
// Parses "mosaico:user_metadata" JSON to string map.
std::unordered_map<std::string, std::string> extractUserMetadata(
    const std::shared_ptr<arrow::KeyValueMetadata>& metadata);

// Detect ontology tag from Arrow schema column structure (heuristic).
// Used when server metadata has no ontology tag.
std::optional<std::string> detectOntologyTag(
    const std::shared_ptr<arrow::Schema>& schema);

} // namespace mosaico
