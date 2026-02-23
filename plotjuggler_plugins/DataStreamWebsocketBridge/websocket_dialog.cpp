#include "websocket_dialog.h"

#include <QScrollBar>
#include <QPushButton>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

#include "ui_websocket_client.h"

WebsocketDialog::WebsocketDialog(const WebsocketClientConfig& config)
  : QDialog(nullptr), ui(new Ui::WebSocketDialog)
{
  ui->setupUi(this);
  setWindowTitle("WebSocket Client");

  ui->lineEditPort->setValidator(new QIntValidator(1, 65535, this));
  ui->comboBox->setEnabled(false);

  ui->lineEditAddress->setText(config.address);
  ui->lineEditPort->setText(QString::number(config.port));

  auto okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
  if (okBtn)
  {
    okBtn->setText("Connect");
  }

  // Wire up internal filter
  connect(ui->lineEditFilter, &QLineEdit::textChanged, this, &WebsocketDialog::applyFilter);
}

WebsocketDialog::~WebsocketDialog()
{
  delete ui;
}

// --- Address / port ---

QString WebsocketDialog::address() const
{
  return ui->lineEditAddress->text().trimmed();
}

int WebsocketDialog::port(bool* ok) const
{
  return ui->lineEditPort->text().toUShort(ok);
}

// --- Topic list management ---

void WebsocketDialog::setTopics(const QJsonArray& topics, const QStringList& preselectNames)
{
  auto* view = ui->topicsList;

  // Save current scroll position
  auto* vsb = view->verticalScrollBar();
  const int scroll_y = vsb ? vsb->value() : 0;

  // Merge persisted + current selection for restore
  QStringList wanted = preselectNames;
  for (auto* it : view->selectedItems())
  {
    const auto n = it->text(0);
    if (!wanted.contains(n))
    {
      wanted << n;
    }
  }

  // Batch-update without triggering signals
  view->setUpdatesEnabled(false);
  view->blockSignals(true);
  view->setVisible(false);
  view->clear();

  for (const auto& v : topics)
  {
    if (!v.isObject())
    {
      continue;
    }
    const auto t = v.toObject();
    const auto name = t.value("name").toString();
    const auto type = t.value("type").toString();
    if (name.isEmpty())
    {
      continue;
    }

    auto* item = new QTreeWidgetItem(view);
    item->setText(0, name);
    item->setText(1, type);

    if (wanted.contains(name))
    {
      item->setSelected(true);
    }
  }

  applyFilter(ui->lineEditFilter->text());
  view->resizeColumnToContents(0);

  view->setVisible(true);
  view->blockSignals(false);
  view->setUpdatesEnabled(true);

  // Restore scroll position after layout update
  QTimer::singleShot(0, view, [view, scroll_y]() {
    if (auto* sb = view->verticalScrollBar())
    {
      sb->setValue(scroll_y);
    }
  });
}

bool WebsocketDialog::hasSelection() const
{
  return !ui->topicsList->selectedItems().isEmpty();
}

QStringList WebsocketDialog::selectedTopicNames() const
{
  QStringList names;
  for (auto* it : ui->topicsList->selectedItems())
  {
    const auto name = it->text(0);
    if (!name.isEmpty())
    {
      names << name;
    }
  }
  return names;
}

std::vector<TopicInfo> WebsocketDialog::selectedTopics() const
{
  std::vector<TopicInfo> result;
  for (auto* it : ui->topicsList->selectedItems())
  {
    const auto name = it->text(0);
    if (name.isEmpty())
    {
      continue;
    }
    TopicInfo info;
    info.name = name;
    info.type = it->text(1);
    result.push_back(std::move(info));
  }
  return result;
}

void WebsocketDialog::clearTopics()
{
  ui->topicsList->clear();
}

// --- OK button ---

void WebsocketDialog::setOkButton(const QString& text, bool enabled)
{
  auto b = ui->buttonBox->button(QDialogButtonBox::Ok);
  if (b)
  {
    b->setText(text);
    b->setEnabled(enabled);
  }
}

// Signal access for external connections
QDialogButtonBox* WebsocketDialog::buttonBox() const
{
  return ui->buttonBox;
}

QTreeWidget* WebsocketDialog::topicsWidget() const
{
  return ui->topicsList;
}

void WebsocketDialog::applyFilter(const QString& filter)
{
  auto* list = ui->topicsList;
  const QString fl = filter.trimmed().toLower();

  for (int i = 0; i < list->topLevelItemCount(); i++)
  {
    auto* it = list->topLevelItem(i);
    const bool selected = it->isSelected();

    if (fl.isEmpty())
    {
      it->setHidden(false);
      continue;
    }

    const QString name = it->text(0).toLower();
    const QString type = it->text(1).toLower();
    const bool match = name.contains(fl) || type.contains(fl);
    it->setHidden(!(match || selected));
  }
}
