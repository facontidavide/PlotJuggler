#pragma once

#include <map>
#include <string>
#include <vector>

// Metadata is a flat map of key -> value, both strings.
using Metadata = std::map<std::string, std::string, std::less<>>;

// Available metadata schema: key -> sorted unique values.
using Schema = std::map<std::string, std::vector<std::string>, std::less<>>;
