/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "multifile_prefix.h"
#include "ui_multifile_prefix.h"

#include <QLabel>
#include <QFormLayout>
#include <QFileInfo>
#include <QSettings>
#include <filesystem>

DialogMultifilePrefix::DialogMultifilePrefix(QStringList filenames, QWidget* parent)
  : QDialog(parent), ui(new Ui::DialogMultifilePrefix)
{
  ui->setupUi(this);

  QVBoxLayout* vlayout = ui->verticalLayoutFrame;

  QSettings settings;

  QStringList prev_prefixes = settings.value("DialogMultifilePrefix::previous").toStringList();

  for (int i = 0; i < prev_prefixes.size(); i += 2)
  {
    _previous_prefixes.insert({ prev_prefixes[i], prev_prefixes[i + 1] });
  }

  int index = 0;
  for (QString filename : filenames)
  {
    // look for = in the parent directory, if there is one the data is probably in a hive structure
    std::filesystem::path path = filename.toStdString();
    std::string hive_path;
    bool found_hive = false;
    while (path.has_parent_path() &&
           path.parent_path().filename().string().find('=') != std::string::npos)
    {
      path = path.parent_path();
      std::string path_str = path.filename().string();
      hive_path.insert(0, path_str.erase(0, path_str.find("=") + 1));
      hive_path.insert(0, "/");
      found_hive = true;
    }

    auto label_file = new QLabel(filename, this);
    label_file->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto form_layout = new QFormLayout();
    auto label = new QLabel("Prefix: ");
    auto line_edit = new QLineEdit();

    form_layout->addRow(label, line_edit);

    vlayout->insertWidget(index++, label_file);
    vlayout->insertLayout(index++, form_layout);

    if (found_hive)
    {
      line_edit->setText(hive_path.c_str());
    }
    else
    {
      if (_previous_prefixes.count(filename))
      {
        line_edit->setText(_previous_prefixes[filename]);
      }
      else
      {
        line_edit->setText(QFileInfo(filename).baseName());
      }
    }
    _prefixes[filename] = line_edit->text();
    _line_edits.insert({ filename, line_edit });
  }
}

std::map<QString, QString> DialogMultifilePrefix::getPrefixes() const
{
  return _prefixes;
}

DialogMultifilePrefix::~DialogMultifilePrefix()
{
  delete ui;
}

void DialogMultifilePrefix::accept()
{
  QSettings settings;

  QStringList prev_prefixes;

  // merge new with old
  for (const auto& [filename, line_edit] : _line_edits)
  {
    _prefixes[filename] = line_edit->text();
    _previous_prefixes[filename] = line_edit->text();
  }

  for (const auto& [filename, prefix] : _previous_prefixes)
  {
    prev_prefixes.push_back(filename);
    prev_prefixes.push_back(prefix);
  }

  settings.setValue("DialogMultifilePrefix::previous", prev_prefixes);

  QDialog::accept();
}
