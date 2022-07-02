// QCodeEditor
#include <QCSVHighlighter>
#include <QSyntaxStyle>
#include <QLanguage>

// Qt
#include <QFile>


QCSVHighlighter::QCSVHighlighter(QTextDocument* document) :
    QStyleSyntaxHighlighter(document),
    m_delimiter(QRegularExpression(","))
{
    Q_INIT_RESOURCE(qcodeeditor_resources);

}

void QCSVHighlighter::highlightBlock(const QString& text)
{
    { // Checking for require
        QRegularExpression m_delimiter;
        //m_delimiter = QRegularExpression(delimiter);
        m_delimiter.setPattern(delimiter);
        auto matchIterator = m_delimiter.globalMatch(text);

        while (matchIterator.hasNext())
        {
            auto match = matchIterator.next();

            setFormat(
                match.capturedStart(),
                match.capturedLength(),
                syntaxStyle()->getFormat("VisualWhitespace")
            );

        }
    }
}
