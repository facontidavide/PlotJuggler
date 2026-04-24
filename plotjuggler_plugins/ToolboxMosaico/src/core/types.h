/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <map>
#include <string>
#include <vector>

// Metadata is a flat map of key -> value, both strings.
using Metadata = std::map<std::string, std::string, std::less<>>;

// Available metadata schema: key -> sorted unique values.
using Schema = std::map<std::string, std::vector<std::string>, std::less<>>;
