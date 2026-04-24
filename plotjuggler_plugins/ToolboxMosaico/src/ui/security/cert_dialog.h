#pragma once

#include <QDialog>
#include <QString>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

class CertDialog : public QDialog
{
  Q_OBJECT

public:
  explicit CertDialog(QWidget* parent = nullptr);

  void setCertPath(const QString& path);
  [[nodiscard]] QString certPath() const;

  void setApiKey(const QString& key);
  [[nodiscard]] QString apiKey() const;

  // Per-URL opt-in: when the TLS handshake fails and no custom cert is set,
  // retry the connection in plaintext. Default off for any server the user
  // hasn't explicitly enabled.
  void setAllowInsecure(bool allow);
  [[nodiscard]] bool allowInsecure() const;

protected:
  void closeEvent(QCloseEvent* event) override;

private slots:
  void onBrowse();
  void validateCert();
  void validateKey();
  void onShowToggled(bool checked);

private:
  QLineEdit* cert_edit_ = nullptr;
  QLineEdit* key_edit_ = nullptr;
  QLabel* cert_tick_ = nullptr;
  QLabel* key_tick_ = nullptr;
  QPushButton* show_btn_ = nullptr;
  QCheckBox* allow_insecure_ = nullptr;
};
