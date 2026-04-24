// src/flight/query.hpp — client-side query builders for the `query` action.
//
// Mirrors the Python SDK's QueryTopic / QuerySequence / QueryOntologyCatalog
// (mosaicolabs/models/query/builders.py). The server expects a payload of the
// form {"<filter-name>": {<field>: {"$op": <value>}, ...}, ...}. Builders
// produce one such (name, body) pair; MosaicoClient::query composes them.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mosaico {

// One top-level filter clause. `name` is "topic", "sequence", or "ontology".
// `body_json` is a pre-rendered JSON object (no outer braces quoting issues).
struct QueryFilter {
    std::string name;
    std::string body_json;  // serialized object, e.g. {"locator":{"$eq":"x"}}
};

// Per-topic result entry.
struct QueryResponseTopic {
    std::string locator;            // full "sequence/topic" path
    std::optional<int64_t> ts_start_ns;
    std::optional<int64_t> ts_end_ns;
};

// One sequence in a query result, plus the topics that matched.
struct QueryResponseItem {
    std::string sequence;
    std::vector<QueryResponseTopic> topics;
};

struct QueryResponse {
    std::vector<QueryResponseItem> items;
};

// Fluent builder for QueryTopic filters.
class QueryTopicBuilder {
 public:
    // Exact match: {"locator": {"$eq": "<name>"}}
    QueryTopicBuilder& withName(const std::string& name);
    // Partial match: {"locator": {"$match": "<substring>"}}
    QueryTopicBuilder& withNameMatch(const std::string& partial);
    // Exact tag: {"ontology_tag": {"$eq": "<tag>"}}
    QueryTopicBuilder& withOntologyTag(const std::string& tag);
    // Created-timestamp range.
    QueryTopicBuilder& withCreatedAfter(int64_t ns);
    QueryTopicBuilder& withCreatedBefore(int64_t ns);

    [[nodiscard]] QueryFilter build() const;

 private:
    std::vector<std::pair<std::string, std::string>> clauses_;  // (field, op_obj_json)
};

// Fluent builder for QuerySequence filters.
class QuerySequenceBuilder {
 public:
    QuerySequenceBuilder& withName(const std::string& name);
    QuerySequenceBuilder& withNameMatch(const std::string& partial);
    QuerySequenceBuilder& withCreatedAfter(int64_t ns);
    QuerySequenceBuilder& withCreatedBefore(int64_t ns);

    [[nodiscard]] QueryFilter build() const;

 private:
    std::vector<std::pair<std::string, std::string>> clauses_;
};

}  // namespace mosaico
