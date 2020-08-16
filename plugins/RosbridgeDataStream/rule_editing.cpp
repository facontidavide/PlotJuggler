#include "rule_editing.h"
#include "ui_rule_editing.h"
#include <QDomDocument>
#include <QSettings>
#include <QPlainTextEdit>
#include <QTimer>
#include <QMessageBox>

const char* DEFAULT_RULES = "<SubstitutionRules>\n"
                            "\n"
                            "<RosType name=\"sensor_msgs/JointState\">\n"
                            "  <rule pattern=\"position.#\" alias=\"name.#\" substitution=\"@/pos\" />\n"
                            "  <rule pattern=\"velocity.#\" alias=\"name.#\" substitution=\"@/vel\" />\n"
                            "  <rule pattern=\"effort.#\"   alias=\"name.#\" substitution=\"@/eff\" />\n"
                            "</RosType>\n"
                            "\n"
                            "<RosType name=\"tf/tfMessage\">"
                            "  <rule pattern=\"transforms.#/header\"                "
                            "alias=\"transforms.#/child_frame_id\"  substitution=\"@/header\" />\n"
                            "  <rule pattern=\"transforms.#/transform/rotation\"    "
                            "alias=\"transforms.#/child_frame_id\"  substitution=\"@/rotation\" />\n"
                            "  <rule pattern=\"transforms.#/transform/translation\" "
                            "alias=\"transforms.#/child_frame_id\"  substitution=\"@/translation\" />\n"
                            "</RosType>\n"
                            "\n"
                            "<RosType name=\"tf2_msgs/TFMessage\">"
                            "  <rule pattern=\"transforms.#/header\"                "
                            "alias=\"transforms.#/child_frame_id\"  substitution=\"@/header\" />\n"
                            "  <rule pattern=\"transforms.#/transform/rotation\"    "
                            "alias=\"transforms.#/child_frame_id\"  substitution=\"@/rotation\" />\n"
                            "  <rule pattern=\"transforms.#/transform/translation\" "
                            "alias=\"transforms.#/child_frame_id\"  substitution=\"@/translation\" />\n"
                            "</RosType>\n"
                            "\n"
                            "<RosType name=\"pal_statistics_msgs/Statistics\">\n"
                            "  <rule pattern=\"statistics.#/value\" alias=\"statistics.#/name\" substitution=\"@\" />\n"
                            "</RosType>\n"
                            "</SubstitutionRules>\n";

XMLSyntaxHighlighter::XMLSyntaxHighlighter(QObject* parent) : QSyntaxHighlighter(parent)
{
  setRegexes();
  setFormats();
}

XMLSyntaxHighlighter::XMLSyntaxHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent)
{
  setRegexes();
  setFormats();
}

XMLSyntaxHighlighter::XMLSyntaxHighlighter(QTextEdit* parent) : QSyntaxHighlighter(parent)
{
  setRegexes();
  setFormats();
}

void XMLSyntaxHighlighter::highlightBlock(const QString& text)
{
  // Special treatment for xml element regex as we use captured text to emulate lookbehind
  int xmlElementIndex = _xmlElementRegex.indexIn(text);
  while (xmlElementIndex >= 0)
  {
    int matchedPos = _xmlElementRegex.pos(1);
    int matchedLength = _xmlElementRegex.cap(1).length();
    setFormat(matchedPos, matchedLength, _xmlElementFormat);

    xmlElementIndex = _xmlElementRegex.indexIn(text, matchedPos + matchedLength);
  }

  // Highlight xml keywords *after* xml elements to fix any occasional / captured into the enclosing element
  typedef QList<QRegExp>::const_iterator Iter;
  Iter xmlKeywordRegexesEnd = _xmlKeywordRegexes.end();
  for (Iter it = _xmlKeywordRegexes.begin(); it != xmlKeywordRegexesEnd; ++it)
  {
    const QRegExp& regex = *it;
    highlightByRegex(_xmlKeywordFormat, regex, text);
  }

  highlightByRegex(_xmlAttributeFormat, _xmlAttributeRegex, text);
  highlightByRegex(_xmlCommentFormat, _xmlCommentRegex, text);
  highlightByRegex(_xmlValueFormat, _xmlValueRegex, text);
}

void XMLSyntaxHighlighter::highlightByRegex(const QTextCharFormat& format, const QRegExp& regex, const QString& text)
{
  int index = regex.indexIn(text);
  while (index >= 0)
  {
    int matchedLength = regex.matchedLength();
    setFormat(index, matchedLength, format);
    index = regex.indexIn(text, index + matchedLength);
  }
}

void XMLSyntaxHighlighter::setRegexes()
{
  _xmlElementRegex.setPattern("<[\\s]*[/]?[\\s]*([^\\n]\\w*)(?=[\\s/>])");
  _xmlAttributeRegex.setPattern("\\w+(?=\\=)");
  _xmlValueRegex.setPattern("\"[^\\n\"]+\"(?=[\\s/>])");
  _xmlCommentRegex.setPattern("<!--[^\\n]*-->");

  _xmlKeywordRegexes = QList<QRegExp>() << QRegExp("<\\?") << QRegExp("/>") << QRegExp(">") << QRegExp("<")
                                        << QRegExp("</") << QRegExp("\\?>");
}

void XMLSyntaxHighlighter::setFormats()
{
  _xmlKeywordFormat.setForeground(Qt::blue);
  _xmlElementFormat.setForeground(Qt::darkMagenta);

  _xmlAttributeFormat.setForeground(Qt::darkGreen);
  _xmlAttributeFormat.setFontItalic(true);

  _xmlValueFormat.setForeground(Qt::darkRed);
  _xmlCommentFormat.setForeground(Qt::gray);
}

RuleEditing::RuleEditing(QWidget* parent) : QDialog(parent), ui(new Ui::RuleEditing)
{
  ui->setupUi(this);

  _highlighter = new XMLSyntaxHighlighter(ui->textEdit);

  QSettings settings;
  restoreGeometry(settings.value("RuleEditing.geometry").toByteArray());

  ui->textEdit->setPlainText(getRenamingXML());

  _timer.setInterval(200);
  _timer.setSingleShot(false);
  _timer.start();

  const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

  ui->textEdit->setFont(fixedFont);

  connect(&_timer, &QTimer::timeout, this, &RuleEditing::on_timer);
}

RuleEditing::~RuleEditing()
{
  delete ui;
}

bool RuleEditing::isValidXml()
{
  QString errorStr;
  int errorLine, errorColumn;
  bool valid = true;

  QDomDocument domDocument;
  QString text = ui->textEdit->toPlainText();

  if (!domDocument.setContent(text, true, &errorStr, &errorLine, &errorColumn))
  {
    ui->labelValidSyntax->setText("Invalid XML: " + errorStr);
    return false;
  }
  QDomElement root = domDocument.namedItem("SubstitutionRules").toElement();

  if (root.isNull())
  {
    errorStr = tr("the root node should be <SubstitutionRules>");
    ui->labelValidSyntax->setText(tr("Invalid: ") + errorStr);
    return false;
  }

  for (auto type_el = root.firstChildElement(); type_el.isNull() == false; type_el = type_el.nextSiblingElement())
  {
    if (type_el.nodeName() != "RosType")
    {
      errorStr = tr("<SubstitutionRules> must have children named <RosType>");
      ui->labelValidSyntax->setText(tr("Invalid: ") + errorStr);
      return false;
    }
    if (type_el.hasAttribute("name") == false)
    {
      errorStr = tr("node <RosType> must have the attribute [name]");
      ui->labelValidSyntax->setText(tr("Invalid: ") + errorStr);
      return false;
    }

    for (auto rule_el = type_el.firstChildElement(); rule_el.isNull() == false; rule_el = rule_el.nextSiblingElement())
    {
      if (rule_el.nodeName() != "rule")
      {
        errorStr = tr("<RosType> must have children named <rule>");
        ui->labelValidSyntax->setText(tr("Invalid: ") + errorStr);
        return false;
      }
      if (!rule_el.hasAttribute("pattern") || !rule_el.hasAttribute("alias") || !rule_el.hasAttribute("substitution"))
      {
        errorStr = tr("<rule> must have the attributes 'pattern', 'alias' and 'substitution'");
        ui->labelValidSyntax->setText(tr("Invalid: ") + errorStr);
        return false;
      }
    }
  }
  ui->labelValidSyntax->setText(tr("Valid") + errorStr);
  return true;
}

void RuleEditing::on_pushButtonSave_pressed()
{
  QSettings settings;
  settings.setValue("RuleEditing.text", ui->textEdit->toPlainText());
  this->close();
}

void RuleEditing::closeEvent(QCloseEvent* event)
{
  QSettings settings;
  settings.setValue("RuleEditing.geometry", saveGeometry());
  QWidget::closeEvent(event);
}

void RuleEditing::on_timer()
{
  bool valid = isValidXml();
  ui->pushButtonSave->setEnabled(valid);
}

void RuleEditing::on_pushButtonCancel_pressed()
{
  this->close();
}

void RuleEditing::on_pushButtonReset_pressed()
{
  QMessageBox::StandardButton reply;
  reply = QMessageBox::question(0, tr("Warning"), tr("Do you really want to overwrite these rules\n"),
                                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (reply == QMessageBox::Yes)
  {
    ui->textEdit->setPlainText(DEFAULT_RULES);
  }
}

QString RuleEditing::getRenamingXML()
{
  QSettings settings;
  if (settings.contains("RuleEditing.text"))
  {
    return settings.value("RuleEditing.text").toString();
  }
  else
  {
    return DEFAULT_RULES;
  }
}

