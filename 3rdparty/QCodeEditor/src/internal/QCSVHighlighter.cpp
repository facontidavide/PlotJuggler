/*
MIT License

Copyright (c) 2013-2019 Megaxela

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

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
