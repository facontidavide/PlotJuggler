#pragma once

// QCodeEditor
#include <QStyleSyntaxHighlighter> // Required for inheritance
#include <QHighlightRule>
#include <QHighlightBlockRule>

// Qt
#include <QRegularExpression>
#include <QVector>
#include <QMap>
#include <QChar>

class QSyntaxStyle;

/**
 * @brief Class, that describes C++ code
 * highlighter.
 */
class QCSVHighlighter : public QStyleSyntaxHighlighter
{
    Q_OBJECT
public:

    /**
     * @brief Constructor.
     * @param document Pointer to document.
     */
    explicit QCSVHighlighter(QTextDocument* document=nullptr);

    QChar delimiter = QChar(',');

protected:
    void highlightBlock(const QString& text) override;

private:
    QRegularExpression m_delimiter;

};
