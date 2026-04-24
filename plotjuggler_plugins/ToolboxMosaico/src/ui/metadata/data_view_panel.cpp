/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

// Arrow headers MUST come before Qt (arrow/util/cancel.h vs Qt 'signals' macro).
#include <arrow/api.h>

#include "data_view_panel.h"

#include "../format_utils.h"

#include <QDateTime>
#include <QFontDatabase>
#include <QLabel>
#include <QTextEdit>
#include <QTimeZone>
#include <QVBoxLayout>

#include <algorithm>
#include <map>

DataViewPanel::DataViewPanel(QWidget* parent) : QWidget(parent)
{
  header_ = new QLabel("Info", this);
  auto header_font = header_->font();
  header_font.setBold(true);
  header_->setFont(header_font);

  text_view_ = new QTextEdit(this);
  text_view_->setReadOnly(true);
  QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  text_view_->setFont(mono);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(header_);
  layout->addWidget(text_view_);
  setLayout(layout);
}

void DataViewPanel::clear()
{
  header_->setText("Info");
  sequence_text_.clear();
  topic_texts_.clear();
  text_view_->clear();
}

void DataViewPanel::clearTopics()
{
  topic_texts_.clear();
  rebuildText();
}

template <typename MapType>
QString DataViewPanel::formatMetadata(const MapType& metadata, const QString& indent)
{
  if (metadata.empty())
  {
    return {};
  }
  QString text;
  for (const auto& [key, value] : metadata)
  {
    auto qkey = QString::fromStdString(key);
    auto qval = QString::fromStdString(value);
    text += QString("%1%2:\n%1  %3\n").arg(indent, qkey, qval);
  }
  return text;
}

// Explicit instantiations for both map types.
template QString DataViewPanel::formatMetadata(const std::map<std::string, std::string>&,
                                               const QString&);
template QString DataViewPanel::formatMetadata(const std::unordered_map<std::string, std::string>&,
                                               const QString&);

void DataViewPanel::showSequenceInfo(const SequenceInfo& info)
{
  topic_texts_.clear();

  QString text;
  text += QString("Sequence : %1\n").arg(QString::fromStdString(info.name));

  if (info.max_ts_ns > 0)
  {
    auto dt = QDateTime::fromMSecsSinceEpoch(info.max_ts_ns / 1'000'000LL, QTimeZone::utc());
    text += QString("Date     : %1\n").arg(dt.toString("dd/MM/yyyy"));
  }

  if (info.total_size_bytes > 0)
  {
    text += QString("Size     : %1\n").arg(formatBytes(info.total_size_bytes));
  }

  if (!info.user_metadata.empty())
  {
    text += "\nMetadata:\n";
    text += formatMetadata(info.user_metadata);
  }

  sequence_text_ = text;
  header_->setText(QString("Info \u2014 %1").arg(QString::fromStdString(info.name)));
  rebuildText();
}

// Recursively formats an Arrow field type with vertical indentation for structs.
static void formatFieldType(const std::shared_ptr<arrow::Field>& field, const QString& indent,
                            QString& out)
{
  auto type = field->type();
  if (type->id() == arrow::Type::STRUCT)
  {
    out += QString("%1%2\n").arg(indent, QString::fromStdString(field->name()));
    auto st = std::static_pointer_cast<arrow::StructType>(type);
    for (int i = 0; i < st->num_fields(); ++i)
    {
      formatFieldType(st->field(i), indent + "  ", out);
    }
  }
  else if (type->id() == arrow::Type::LIST)
  {
    auto inner = std::static_pointer_cast<arrow::ListType>(type)->value_field();
    out += QString("%1%2 []\n").arg(indent, QString::fromStdString(field->name()));
    if (inner->type()->id() == arrow::Type::STRUCT)
    {
      auto st = std::static_pointer_cast<arrow::StructType>(inner->type());
      for (int i = 0; i < st->num_fields(); ++i)
      {
        formatFieldType(st->field(i), indent + "  ", out);
      }
    }
    else
    {
      out += QString("%1  %2\n").arg(indent, QString::fromStdString(inner->type()->ToString()));
    }
  }
  else
  {
    out += QString("%1%2 : %3\n")
               .arg(indent)
               .arg(QString::fromStdString(field->name()))
               .arg(QString::fromStdString(type->ToString()));
  }
}

static QString formatSchemaFields(const std::shared_ptr<arrow::Schema>& schema)
{
  QString text;
  if (!schema)
  {
    return text;
  }
  text += QString("Fields (%1):\n").arg(schema->num_fields());
  for (int i = 0; i < schema->num_fields(); ++i)
  {
    formatFieldType(schema->field(i), "  ", text);
  }
  return text;
}

static QString formatDateTime(int64_t ts_ns)
{
  auto dt = QDateTime::fromMSecsSinceEpoch(ts_ns / 1'000'000LL, QTimeZone::utc());
  return dt.toString("dd/MM/yyyy HH:mm:ss 'UTC'");
}

void DataViewPanel::showTopicInfo(const TopicInfo& info)
{
  QString text;
  text += QString("Topic    : %1\n").arg(QString::fromStdString(info.topic_name));
  text += QString("Tag      : %1\n").arg(QString::fromStdString(info.ontology_tag));

  if (info.created_at_ns > 0)
  {
    text += QString("Created  : %1\n").arg(formatDateTime(info.created_at_ns));
  }
  if (info.locked)
  {
    if (info.completed_at_ns.has_value() && *info.completed_at_ns > 0)
    {
      text += QString("Status   : sealed (%1)\n").arg(formatDateTime(*info.completed_at_ns));
    }
    else
    {
      text += "Status   : sealed\n";
    }
  }
  else
  {
    text += "Status   : live\n";
  }
  if (info.chunks_number > 0)
  {
    text += QString("Chunks   : %1\n").arg(info.chunks_number);
  }
  if (!info.resource_locator.empty())
  {
    text += QString("Resource : %1\n").arg(QString::fromStdString(info.resource_locator));
  }

  text += formatSchemaFields(info.schema);

  if (!info.user_metadata.empty())
  {
    text += "\nMetadata:\n";
    text += formatMetadata(info.user_metadata);
  }

  topic_texts_.append(text);
  rebuildText();
}

void DataViewPanel::rebuildText()
{
  QString full;

  if (!sequence_text_.isEmpty())
  {
    full += sequence_text_;
  }

  for (int i = 0; i < topic_texts_.size(); ++i)
  {
    full += "\n" + QString(40, QChar('-')) + "\n\n";
    full += topic_texts_[i];
  }

  text_view_->setPlainText(full);
}
