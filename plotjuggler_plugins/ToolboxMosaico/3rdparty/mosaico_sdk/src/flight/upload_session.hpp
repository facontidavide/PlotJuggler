// src/flight/upload_session.hpp
#pragma once

#include "flight/connection_pool.hpp"

#include <arrow/flight/api.h>
#include <arrow/status.h>
#include <arrow/type_fwd.h>

#include <memory>
#include <string>

namespace mosaico {

class UploadSession {
 public:
    ~UploadSession();

    UploadSession(const UploadSession&) = delete;
    UploadSession& operator=(const UploadSession&) = delete;
    UploadSession(UploadSession&&) noexcept;
    UploadSession& operator=(UploadSession&&) noexcept;

    arrow::Status createTopic(const std::string& topic_name,
                              const std::string& ontology_tag,
                              const std::shared_ptr<arrow::Schema>& schema);
    arrow::Status putBatch(const std::shared_ptr<arrow::RecordBatch>& batch);
    arrow::Status closeTopic();
    arrow::Status finalize();
    arrow::Status cleanup();

    const std::string& sequenceName() const;

 private:
    friend class MosaicoClient;
    UploadSession(ConnectionPool::Handle conn, int timeout,
                  std::string sequence_name, std::string sequence_key,
                  std::string session_uuid);

    arrow::Status doAction(const std::string& action_type,
                           const std::string& json_body,
                           std::string* response = nullptr);
    arrow::flight::FlightCallOptions callOpts() const;

    ConnectionPool::Handle conn_;
    int timeout_;
    std::string sequence_name_;
    std::string sequence_key_;
    std::string session_uuid_;
    std::string topic_key_;
    bool finalized_ = false;

    std::unique_ptr<arrow::flight::FlightStreamWriter> put_writer_;
    std::unique_ptr<arrow::flight::FlightMetadataReader> put_reader_;
};

} // namespace mosaico
