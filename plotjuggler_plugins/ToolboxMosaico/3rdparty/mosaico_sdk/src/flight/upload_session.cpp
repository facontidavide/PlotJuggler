// src/flight/upload_session.cpp
#include "flight/upload_session.hpp"

#include "flight/json_utils.hpp"
#include "flight/utils.hpp"

#include <arrow/api.h>
#include <arrow/flight/api.h>
#include <nlohmann/json.hpp>

#include <utility>

namespace fl = arrow::flight;
using json = nlohmann::json;

namespace mosaico {

// ---------------------------------------------------------------------------
// Construction / Destruction / Move
// ---------------------------------------------------------------------------

UploadSession::UploadSession(ConnectionPool::Handle conn, int timeout,
                             std::string sequence_name,
                             std::string sequence_key,
                             std::string session_uuid)
    : conn_(std::move(conn)),
      timeout_(timeout),
      sequence_name_(std::move(sequence_name)),
      sequence_key_(std::move(sequence_key)),
      session_uuid_(std::move(session_uuid)) {}

UploadSession::~UploadSession() {
    (void)cleanup();
}

UploadSession::UploadSession(UploadSession&& other) noexcept
    : conn_(std::move(other.conn_)),
      timeout_(other.timeout_),
      sequence_name_(std::move(other.sequence_name_)),
      sequence_key_(std::move(other.sequence_key_)),
      session_uuid_(std::move(other.session_uuid_)),
      topic_key_(std::move(other.topic_key_)),
      finalized_(other.finalized_),
      put_writer_(std::move(other.put_writer_)),
      put_reader_(std::move(other.put_reader_)) {
    other.finalized_ = true;  // prevent source from cleaning up
    other.sequence_key_.clear();
    other.session_uuid_.clear();
}

UploadSession& UploadSession::operator=(UploadSession&& other) noexcept {
    if (this != &other) {
        // Clean up current state first.
        (void)cleanup();

        conn_ = std::move(other.conn_);
        timeout_ = other.timeout_;
        sequence_name_ = std::move(other.sequence_name_);
        sequence_key_ = std::move(other.sequence_key_);
        session_uuid_ = std::move(other.session_uuid_);
        topic_key_ = std::move(other.topic_key_);
        finalized_ = other.finalized_;
        put_writer_ = std::move(other.put_writer_);
        put_reader_ = std::move(other.put_reader_);

        other.finalized_ = true;
        other.sequence_key_.clear();
        other.session_uuid_.clear();
    }
    return *this;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

fl::FlightCallOptions UploadSession::callOpts() const {
    fl::FlightCallOptions opts;
    opts.timeout = fl::TimeoutDuration{static_cast<double>(timeout_)};
    return opts;
}

arrow::Status UploadSession::doAction(const std::string& action_type,
                                      const std::string& json_body,
                                      std::string* response) {
    if (!conn_.valid()) {
        return arrow::Status::Invalid("UploadSession: connection not valid");
    }

    fl::Action action;
    action.type = action_type;
    action.body = arrow::Buffer::FromString(json_body);

    auto opts = callOpts();
    ARROW_ASSIGN_OR_RAISE(auto results, conn_->DoAction(opts, action));

    std::string response_data;
    while (true) {
        ARROW_ASSIGN_OR_RAISE(auto result, results->Next());
        if (!result) break;
        response_data.append(
            reinterpret_cast<const char*>(result->body->data()),
            result->body->size());
    }

    if (response_data.empty() && response) {
        return arrow::Status::IOError("DoAction '", action_type,
                                      "' returned empty response");
    }

    if (response) {
        *response = std::move(response_data);
    }

    return arrow::Status::OK();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

arrow::Status UploadSession::createTopic(
    const std::string& topic_name,
    const std::string& ontology_tag,
    const std::shared_ptr<arrow::Schema>& schema) {
    if (!conn_.valid()) {
        return arrow::Status::Invalid("UploadSession: connection not valid");
    }

    // Close any previously open topic stream.
    if (put_writer_) {
        ARROW_RETURN_NOT_OK(closeTopic());
    }

    std::string resource = packResource(sequence_name_, topic_name);

    json payload = {{"session_uuid", session_uuid_},
                    {"locator", resource},
                    {"serialization_format", "default"},
                    {"ontology_tag", ontology_tag},
                    {"user_metadata", json::object()}};

    std::string response;
    ARROW_RETURN_NOT_OK(doAction("topic_create", payload.dump(), &response));

    // Server returns {"action":"topic_create","response":{"uuid":"..."}}.
    // We store the uuid — the DoPut descriptor requires it for authorization.
    if (response.empty()) {
        return arrow::Status::IOError("topic_create returned empty response");
    }
    try {
        // Parse → unwrap → extract, one step per line.
        const json root = json::parse(response);
        const json& payload = unwrapResponse(root);

        const auto uuid = tryGetString(payload, "uuid");
        if (!uuid.has_value()) {
            return arrow::Status::IOError(
                "topic_create response missing 'uuid': ", response);
        }
        topic_key_ = *uuid;
    } catch (const json::exception& e) {
        return arrow::Status::Invalid("malformed topic_create response: ",
                                      e.what());
    }

    // DoPut descriptor per server contract: {resource_locator, topic_uuid}.
    // (mosaicod-marshal/src/flight.rs DoPutCmd struct.)
    std::string descriptor_cmd;
    try {
        descriptor_cmd = json{{"resource_locator", resource},
                              {"topic_uuid", topic_key_}}.dump();
    } catch (const json::exception& e) {
        return arrow::Status::Invalid("cannot serialize DoPut descriptor: ",
                                      e.what());
    }
    auto descriptor = fl::FlightDescriptor::Command(descriptor_cmd);

    auto opts = callOpts();
    ARROW_ASSIGN_OR_RAISE(auto put_result,
                          conn_->DoPut(opts, descriptor, schema));
    put_writer_ = std::move(put_result.writer);
    put_reader_ = std::move(put_result.reader);

    return arrow::Status::OK();
}

arrow::Status UploadSession::putBatch(
    const std::shared_ptr<arrow::RecordBatch>& batch) {
    if (!put_writer_) {
        return arrow::Status::Invalid(
            "UploadSession: no open topic stream (call createTopic first)");
    }
    return put_writer_->WriteRecordBatch(*batch);
}

arrow::Status UploadSession::closeTopic() {
    if (!put_writer_) {
        return arrow::Status::OK();
    }

    ARROW_RETURN_NOT_OK(put_writer_->DoneWriting());
    auto close_status = put_writer_->Close();
    put_writer_.reset();
    put_reader_.reset();
    topic_key_.clear();
    return close_status;
}

arrow::Status UploadSession::finalize() {
    if (finalized_) {
        return arrow::Status::Invalid(
            "UploadSession: already finalized");
    }

    // Close any open topic stream first.
    if (put_writer_) {
        ARROW_RETURN_NOT_OK(closeTopic());
    }

    json payload = {{"session_uuid", session_uuid_}};
    ARROW_RETURN_NOT_OK(doAction("session_finalize", payload.dump()));

    finalized_ = true;
    return arrow::Status::OK();
}

arrow::Status UploadSession::cleanup() {
    // Close any open topic stream.
    if (put_writer_) {
        (void)put_writer_->DoneWriting();
        (void)put_writer_->Close();
        put_writer_.reset();
        put_reader_.reset();
    }

    // If not finalized, abort the session to release server-side state.
    if (!finalized_ && !session_uuid_.empty() && conn_.valid()) {
        json payload = {{"session_uuid", session_uuid_}};
        (void)doAction("session_delete", payload.dump());
    }

    sequence_key_.clear();
    session_uuid_.clear();
    topic_key_.clear();
    finalized_ = true;  // prevent double cleanup

    return arrow::Status::OK();
}

const std::string& UploadSession::sequenceName() const {
    return sequence_name_;
}

} // namespace mosaico
