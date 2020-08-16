#ifndef RULE_EDITING_H
#define RULE_EDITING_H

#include <QDialog>
#include <QTimer>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTextEdit>

namespace Ui
{
class RuleEditing;
}

class XMLSyntaxHighlighter : public QSyntaxHighlighter
{
  Q_OBJECT
public:
  XMLSyntaxHighlighter(QObject* parent);
  XMLSyntaxHighlighter(QTextDocument* parent);
  XMLSyntaxHighlighter(QTextEdit* parent);

protected:
  virtual void highlightBlock(const QString& text);

private:
  void highlightByRegex(const QTextCharFormat& format, const QRegExp& regex, const QString& text);
  void setRegexes();
  void setFormats();

private:
  QTextCharFormat _xmlKeywordFormat;
  QTextCharFormat _xmlElementFormat;
  QTextCharFormat _xmlAttributeFormat;
  QTextCharFormat _xmlValueFormat;
  QTextCharFormat _xmlCommentFormat;

  QList<QRegExp> _xmlKeywordRegexes;
  QRegExp _xmlElementRegex;
  QRegExp _xmlAttributeRegex;
  QRegExp _xmlValueRegex;
  QRegExp _xmlCommentRegex;
};

class RuleEditing : public QDialog
{
  Q_OBJECT

public:
  explicit RuleEditing(QWidget* parent = 0);
  ~RuleEditing();

  static QString getRenamingXML();

private slots:

  void on_pushButtonSave_pressed();

  void on_pushButtonReset_pressed();

  void on_timer();

  void on_pushButtonCancel_pressed();

private:
  bool isValidXml();

  Ui::RuleEditing* ui;

  XMLSyntaxHighlighter* _highlighter;
  QTimer _timer;

private slots:

  virtual void closeEvent(QCloseEvent* event) override;
};

#endif  // RULE_EDITING_H
