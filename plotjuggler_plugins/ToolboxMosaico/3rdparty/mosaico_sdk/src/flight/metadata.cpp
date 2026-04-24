#include "flight/metadata.hpp"

#include <arrow/api.h>
#include <nlohmann/json.hpp>

namespace mosaico {

namespace {
using json = nlohmann::json;
} // namespace

std::optional<std::string> extractOntologyTag(
    const std::shared_ptr<arrow::KeyValueMetadata>& metadata)
{
    if (!metadata)
    {
        return std::nullopt;
    }

    auto idx = metadata->FindKey("mosaico:properties");
    if (idx < 0)
    {
        return std::nullopt;
    }

    try
    {
        auto j = json::parse(metadata->value(idx));
        if (j.contains("ontology_tag") && j["ontology_tag"].is_string())
        {
            return j["ontology_tag"].get<std::string>();
        }
    }
    catch (const json::exception&)
    {
    }
    return std::nullopt;
}

std::unordered_map<std::string, std::string> extractUserMetadata(
    const std::shared_ptr<arrow::KeyValueMetadata>& metadata)
{
    std::unordered_map<std::string, std::string> result;
    if (!metadata)
    {
        return result;
    }

    auto idx = metadata->FindKey("mosaico:user_metadata");
    if (idx < 0)
    {
        return result;
    }

    try
    {
        auto j = json::parse(metadata->value(idx));
        for (auto& [key, value] : j.items())
        {
            // Filter out internal ROS keys (matches Python SDK behavior).
            if (key.rfind("ros:", 0) == 0)
            {
                continue;
            }
            if (value.is_string())
            {
                result[key] = value.get<std::string>();
            }
            else
            {
                result[key] = value.dump();
            }
        }
    }
    catch (const json::exception&)
    {
    }
    return result;
}

std::optional<std::string> detectOntologyTag(
    const std::shared_ptr<arrow::Schema>& schema)
{
    if (!schema)
    {
        return std::nullopt;
    }

    auto has_field = [&](const std::string& name) -> bool {
        return schema->GetFieldIndex(name) >= 0;
    };

    // IMU: acceleration + angular_velocity
    if (has_field("acceleration") && has_field("angular_velocity"))
    {
        return "imu";
    }

    // GPS: position + status
    if (has_field("position") && has_field("status"))
    {
        return "gps";
    }

    // Twist / velocity: linear + angular
    if (has_field("linear") && has_field("angular"))
    {
        return "velocity";
    }

    // Pose: position + orientation
    if (has_field("position") && has_field("orientation"))
    {
        return "pose";
    }

    // Transform: translation + rotation
    if (has_field("translation") && has_field("rotation"))
    {
        return "transform";
    }

    // ForceTorque: force + torque
    if (has_field("force") && has_field("torque"))
    {
        return "force_torque";
    }

    // NMEA sentence
    if (has_field("sentence"))
    {
        return "nmea_sentence";
    }

    // Image / compressed image: single binary "data" field
    {
        auto data_idx = schema->GetFieldIndex("data");
        if (data_idx >= 0 && schema->num_fields() <= 3)
        {
            auto field = schema->field(data_idx);
            if (field->type()->id() == arrow::Type::BINARY ||
                field->type()->id() == arrow::Type::LARGE_BINARY)
            {
                // Distinguish image (has "height"/"width") vs compressed_image (has "format")
                if (has_field("height") || has_field("width"))
                {
                    return "image";
                }
                if (has_field("format"))
                {
                    return "compressed_image";
                }
                return "compressed_image";
            }
        }
    }

    return std::nullopt;
}

} // namespace mosaico
