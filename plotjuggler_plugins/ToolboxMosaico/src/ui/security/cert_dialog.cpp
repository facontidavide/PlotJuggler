#include "cert_dialog.h"
#include "tls_utils.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

CertDialog::CertDialog(QWidget* parent) : QDialog(parent)
{
  setWindowTitle("Certificate & API Key");
  setMinimumWidth(450);

  auto* layout = new QVBoxLayout(this);

  // --- API Key row (shown first; the cert below is optional) ---
  auto* key_row = new QHBoxLayout();

  key_tick_ = new QLabel(this);
  key_tick_->setFixedWidth(16);
  key_tick_->setAlignment(Qt::AlignCenter);

  key_edit_ = new QLineEdit(this);
  key_edit_->setEchoMode(QLineEdit::Password);
  key_edit_->setPlaceholderText("API key (msco_...)");

  show_btn_ = new QPushButton("Show", this);
  show_btn_->setCheckable(true);

  key_row->addWidget(key_tick_);
  key_row->addWidget(key_edit_, 1);
  key_row->addWidget(show_btn_);

  // --- Certificate row (optional) ---
  auto* cert_row = new QHBoxLayout();

  cert_tick_ = new QLabel(this);
  cert_tick_->setFixedWidth(16);
  cert_tick_->setAlignment(Qt::AlignCenter);

  cert_edit_ = new QLineEdit(this);
  cert_edit_->setPlaceholderText("Path to CA certificate (.pem) (optional)");

  auto* browse_btn = new QPushButton("Browse", this);

  cert_row->addWidget(cert_tick_);
  cert_row->addWidget(cert_edit_, 1);
  cert_row->addWidget(browse_btn);

  // Match the two trailing buttons to the wider of their natural hints so
  // "Show"/"Hide" and "Browse" line up visually without a hard fixed width.
  const int btn_min_w =
      std::max(show_btn_->sizeHint().width(), browse_btn->sizeHint().width());
  show_btn_->setMinimumWidth(btn_min_w);
  browse_btn->setMinimumWidth(btn_min_w);

  // --- Allow-insecure checkbox ---
  // Per-URL opt-in to plaintext fallback. Off by default for every server
  // the user hasn't explicitly enabled. The tooltip spells out the
  // consequences so it's harder to flip on by accident.
  allow_insecure_ = new QCheckBox("Allow insecure (plaintext) connection", this);
  allow_insecure_->setToolTip(
      "When enabled, if the TLS handshake fails and no custom certificate is set, "
      "the plugin retries the connection in plaintext. API keys will be sent "
      "unencrypted — only enable for trusted networks or local servers.");

  // --- Button box ---
  auto* button_box = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

  layout->addLayout(key_row);
  layout->addLayout(cert_row);
  layout->addWidget(allow_insecure_);
  layout->addWidget(button_box);

  // --- Connections ---
  connect(browse_btn, &QPushButton::clicked, this, &CertDialog::onBrowse);
  connect(cert_edit_, &QLineEdit::textChanged, this, &CertDialog::validateCert);
  connect(key_edit_, &QLineEdit::textChanged, this, &CertDialog::validateKey);
  connect(show_btn_, &QPushButton::toggled, this, &CertDialog::onShowToggled);
  connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void CertDialog::setCertPath(const QString& path) { cert_edit_->setText(path); }
QString CertDialog::certPath() const { return cert_edit_->text(); }
void CertDialog::setApiKey(const QString& key) { key_edit_->setText(key); }
QString CertDialog::apiKey() const { return key_edit_->text(); }
void CertDialog::setAllowInsecure(bool allow) { allow_insecure_->setChecked(allow); }
bool CertDialog::allowInsecure() const { return allow_insecure_->isChecked(); }

void CertDialog::closeEvent(QCloseEvent* event)
{
  show_btn_->setChecked(false);
  show_btn_->setText("Show");
  key_edit_->setEchoMode(QLineEdit::Password);
  QDialog::closeEvent(event);
}

void CertDialog::onBrowse()
{
  QString path = QFileDialog::getOpenFileName(
      this, "Select CA Certificate", QString(), "PEM files (*.pem)");
  if (!path.isEmpty())
    cert_edit_->setText(path);
}

void CertDialog::validateCert()
{
  const QString path = cert_edit_->text();
  if (path.isEmpty())
  {
    cert_tick_->setText("");
    cert_tick_->setStyleSheet("");
  }
  else if (isCertReadable(path))
  {
    cert_tick_->setText(QString::fromUtf8("\u2713"));
    cert_tick_->setStyleSheet("color: green; font-weight: bold;");
  }
  else
  {
    cert_tick_->setText(QString::fromUtf8("\u2717"));
    cert_tick_->setStyleSheet("color: red; font-weight: bold;");
  }
}

void CertDialog::validateKey()
{
  const QString key = key_edit_->text();
  if (key.isEmpty())
  {
    key_tick_->setText("");
    key_tick_->setStyleSheet("");
  }
  else if (isValidApiKey(key))
  {
    key_tick_->setText(QString::fromUtf8("\u2713"));
    key_tick_->setStyleSheet("color: green; font-weight: bold;");
  }
  else
  {
    key_tick_->setText(QString::fromUtf8("\u2717"));
    key_tick_->setStyleSheet("color: red; font-weight: bold;");
  }
}

void CertDialog::onShowToggled(bool checked)
{
  key_edit_->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
  show_btn_->setText(checked ? "Hide" : "Show");
}
