/***** MIT License ****
*
*   Copyright (c) 2016-2022 Davide Faconti
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included in all
*   copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*/
#pragma once

#include <unordered_set>

#include "rosx_introspection/stringtree_leaf.hpp"
#include "rosx_introspection/deserializer.hpp"

namespace RosMsgParser{

struct FlatMessage {

  std::shared_ptr<MessageSchema> schema;

  /// List of all those parsed fields that can be represented by a
  /// builtin value different from "string".
  std::vector< std::pair<FieldsVector, Variant> > value;

  /// List of all those parsed fields that can be represented by a "string".
  std::vector< std::pair<FieldsVector, std::string> > name;

  /// Store "blobs", i.e all those fields which are vectors of BYTES (AKA uint8_t),
  /// where the vector size is greater than the argument [max_array_size].
  std::vector< std::pair<FieldsVector, Span<const uint8_t>>> blob;

  std::vector<std::vector<uint8_t>> blob_storage;
};


class Parser{

public:
  /**
   *
   * @param topic_name   name of the topic to be used as node of the StringTree
   * @param msg_type     message type of the topic.
   * @param msg_definition text obtained by either:
   *                       - topic_tools::ShapeShifter::getMessageDefinition()
   *                       - rosbag::MessageInstance::getMessageDefinition()
   *                       - ros::message_traits::Definition< __your_type__ >::value()
   * */
  Parser(const std::string& topic_name,
         const ROSType& msg_type,
         const std::string& definition);

  enum MaxArrayPolicy: bool {
    DISCARD_LARGE_ARRAYS = true,
    KEEP_LARGE_ARRAYS = false
  };

  /// Default values are DISCARD_LARGE_ARRAYS and 100.
  /// The maximum permissible value of max_array_size is 10.000 (but don't)
  void setMaxArrayPolicy( MaxArrayPolicy discard_entire_array, size_t max_array_size )
  {
      _discard_large_array = discard_entire_array;
      _max_array_size = max_array_size;
      if( _max_array_size > 10000 )
      {
        throw std::runtime_error("max_array_size limited to 10000 at most");
      }
  }

  MaxArrayPolicy maxArrayPolicy() const
  {
    return _discard_large_array;
  }

  size_t maxArraySize() const
  {
    return _max_array_size;
  }

  enum BlobPolicy {
    STORE_BLOB_AS_COPY,
    STORE_BLOB_AS_REFERENCE};

  // If set to STORE_BLOB_AS_COPY, a copy of the original vector will be stored in the FlatMessage.
  // This may have a large impact on performance.
  // if STORE_BLOB_AS_REFERENCE is used instead, it is dramatically faster, but you must be careful with
  // dangling pointers.
  void setBlobPolicy( BlobPolicy policy )
  {
    _blob_policy = policy;
  }

  BlobPolicy blobPolicy() const
  {
    return _blob_policy;
  }

  /**
   * @brief getSchema provides some metadata amout a registered ROSMessage.
   */
  const std::shared_ptr<MessageSchema>& getSchema() const;

  ROSMessage::Ptr getMessageByType(const ROSType& type) const;

  /**
   * @brief deserializeIntoFlatContainer takes a raw buffer of memory
   *  and extract information from it.
   *  This data is stored in two key/value vectors, FlatMessage::value and FlatMessage::name.
   *  It must be noted that the key type is FieldsVector. This type is
   *  not particularly user-friendly, but allows a much faster post-processing.
   *
   * IMPORTANT: this approach is not meant to be used with use arrays such as maps,
   * point clouds and images.For this reason the argument max_array_size is used.
   *
   * This funtion is almost always followed by CreateRenamedValues,
   * which provide a more human-readable key-value representation.
   *
   * @param buffer         raw memory to be parsed.
   * @param flat_output    output to store the result. It is recommended to
   *                       reuse the same object multiple times to
   *                       avoid memory allocations and speed up the parsing.
   *
   * @return true if the entire message was parsed or false if parts of the message were
   *         skipped because an array has (size > max_array_size)
   */
  bool deserialize(Span<const uint8_t> buffer,
                   FlatMessage* flat_output,
                   Deserializer *deserializer) const;

  typedef std::function<void(const ROSType&, Span<uint8_t>&)> VisitingCallback;

  /**
   * @brief applyVisitorToBuffer is used to pass a callback that is invoked every time
   *        a chunk of memory storing an instance of ROSType = monitored_type
   *        is reached.
   *        Note that the VisitingCallback can modify the original message, but can NOT
   *        change its size. This means that strings and vectors can not be change their length.
   *
   * @param msg_identifier    String ID to identify the registered message (use registerMessageDefinition first).
   * @param monitored_type    ROSType that triggers the invokation to the callback
   * @param buffer            Original buffer, passed as mutable since it might be modified.
   * @param callback          The callback.
   */
  void applyVisitorToBuffer(const ROSType &msg_type,
                            Span<uint8_t> &buffer,
                            VisitingCallback callback) const;

  /// Change where the warning messages are displayed.
  void setWarningsStream(std::ostream* output) { _global_warnings = output; }

private:

  std::shared_ptr<MessageSchema> _schema;

  std::ostream* _global_warnings;

  std::string _topic_name;

  std::vector<int> _alias_array_pos;
  std::vector<std::string> _formatted_string;
  std::vector<int8_t> _substituted;
  MaxArrayPolicy _discard_large_array;
  size_t _max_array_size;
  BlobPolicy _blob_policy;
  std::shared_ptr<ROSField> _dummy_root_field;

  std::unique_ptr<Deserializer> _deserializer;
};

typedef std::vector<std::pair<std::string, double> > RenamedValues;

void CreateRenamedValues(const FlatMessage& flat_msg, RenamedValues& renamed);


template <class DeserializerT>
class ParsersCollection{

public:

  ParsersCollection()
  {
    _deserializer = std::make_unique<DeserializerT>();
  }

  void registerParser(const std::string& topic_name,
                      const ROSType& msg_type,
                      const std::string& definition)
  {
    if (_pack.count(topic_name) == 0)
    {
      Parser parser(topic_name, msg_type, definition);
      CachedPack pack = { std::move(parser), {} };
      _pack.insert({ topic_name, std::move(pack) });
    }
  }

  const Parser* getParser(const std::string& topic_name) const
  {
    auto it = _pack.find(topic_name);
    if (it != _pack.end())
    {
      return &it->second.parser;
    }
    return nullptr;
  }

  const FlatMessage *deserialize(const std::string& topic_name,
                                 Span<const uint8_t> buffer)
  {
    auto it = _pack.find(topic_name);
    if (it != _pack.end())
    {
      CachedPack& pack = it->second;
      Parser& parser = pack.parser;

      parser.deserialize(buffer, &pack.msg, _deserializer.get());
      return &pack.msg;
    }
    return nullptr;
  }

private:
  struct CachedPack{
    Parser parser;
    FlatMessage msg;
  };
  std::unordered_map<std::string, CachedPack> _pack;
  std::vector<uint8_t> _buffer;
  std::unique_ptr<Deserializer> _deserializer;
};



}

