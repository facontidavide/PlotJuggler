#include "PlotJuggler/lua_highlighter.h"
#include <QSettings>

namespace PJ
{

enum
{
  bs_none = -1,
  bs_comment = 2
};

LuaHighlighter::LuaHighlighter( QTextDocument *parent )
  : QSyntaxHighlighter( parent )
{
  QSettings settings;
  QString theme = settings.value("Preferences::theme", "light").toString();
  setTheme(theme);

  HighlightingRule rule;

  // function calls
  rule.pattern = QRegExp( QLatin1String( "\\b[A-Za-z0-9_]+(?=\\()" ) );
  rule.format = &functionFormat;
  highlightingRules.append( rule );

  // keywords (from grammar)
  QStringList keywordPatterns;
  keywordPatterns << QLatin1String( "\\bfunction\\b" )
                  << QLatin1String( "\\bbreak\\b" )
                  << QLatin1String( "\\bdo\\b" )
                  << QLatin1String( "\\bend\\b" )
                  << QLatin1String( "\\bwhile\\b" )
                  << QLatin1String( "\\brepeat\\b" )
                  << QLatin1String( "\\buntil\\b" )
                  << QLatin1String( "\\bif\\b" )
                  << QLatin1String( "\\bthen\\b" )
                  << QLatin1String( "\\belseif\\b" )
                  << QLatin1String( "\\belse\\b" )
                  << QLatin1String( "\\bfor\\b" )
                  << QLatin1String( "\\bin\\b" )
                  << QLatin1String( "\\blocal\\b" )
                  << QLatin1String( "\\bor\\b" )
                  << QLatin1String( "\\band\\b" )
                  << QLatin1String( "\\bnot\\b" )
                  << QLatin1String( "\\breturn\\b" );

  keywordFormat.setFontWeight( QFont::Bold );

  for( auto const& pattern : keywordPatterns )
  {
    rule.pattern = QRegExp( pattern );
    rule.format = &keywordFormat;
    highlightingRules.append( rule );
  }

  // numbers, boolean, nil
  QStringList valuePatterns;
  valuePatterns << QLatin1String( "\\bnil\\b" )
                << QLatin1String( "\\btrue\\b" )
                << QLatin1String( "\\bfalse\\b" )
                << QLatin1String( "\\b\\d+\\b" )
                << QLatin1String( "\\b\\d+.\\b" )
                << QLatin1String( "\\b\\d+e\\b" )
                << QLatin1String( "\\b\\[\\dA-Fa-F]+\\b" );

  valueFormat.setFontWeight( QFont::Normal );

  for( auto const& pattern : valuePatterns )
  {
    rule.pattern = QRegExp( pattern );
    rule.format = &valueFormat;
    highlightingRules.append( rule );
  }

  // double quote "
  rule.pattern = QRegExp( QLatin1String( "\"[^\"]*\"" ) );
  rule.format = &quotationFormat;
  highlightingRules.append( rule );

  // single quote '
  rule.pattern = QRegExp( QLatin1String( "\'[^\']*\'" ) );
  rule.format = &quotationFormat;
  highlightingRules.append( rule );

  // single line comments
  rule.pattern = QRegExp( QLatin1String( "--[^\n]*") );
  rule.format = &singleLineCommentFormat;
  highlightingRules.append( rule );

  //Multi Line Comment --[[ ]]
  commentStartExpression = QRegExp( QLatin1String( "--\\[\\[" ) ); // --[[
  commentEndExpression = QRegExp( QLatin1String( "\\]\\]" ) ); // ]]

  rule.pattern.setMinimal(false);
}

void LuaHighlighter::setTheme(QString theme)
{
  if( theme == "light" )
  {
    functionFormat.setForeground( Qt::blue );
    keywordFormat.setForeground( Qt::darkBlue );
    valueFormat.setForeground( Qt::red );
    quotationFormat.setForeground( Qt::darkGreen );
    singleLineCommentFormat.setForeground( QColor( Qt::darkGray ).darker( 120 ) );
  }
  else{
    functionFormat.setForeground( Qt::cyan );
    keywordFormat.setForeground( Qt::cyan );
    valueFormat.setForeground( Qt::magenta );
    quotationFormat.setForeground( Qt::green );
    singleLineCommentFormat.setForeground( QColor( Qt::gray ) );
  }
}

void LuaHighlighter::highlightBlock(const QString &text)
{
  for( auto const& rule : highlightingRules )
  {
    QRegExp expression( rule.pattern );
    int index = expression.indexIn( text );
    while( index >= 0 )
    {
      int length = expression.matchedLength();
      setFormat( index, length, *(rule.format) );
      index = expression.indexIn( text, index + length );
    }
  }

  setCurrentBlockState( -1 );

  int prev = previousBlockState();
  int start = ( prev == bs_comment ) ? 0 : -1;

  //
  // multi-line comments
  if( prev == bs_comment )
  {
    start = 0;
  }
  if( prev == -1 )
  {
    start = commentStartExpression.indexIn( text );
  }

  while( start >= 0 )
  {
    int end = commentEndExpression.indexIn( text, start );
    int length;

    if( end == -1 )
    {
      setCurrentBlockState( bs_comment );
      length = text.length() - start;
    }
    else
    {
      length = end - start + commentEndExpression.matchedLength();
    }

    setFormat( start, length, singleLineCommentFormat );
    start = commentStartExpression.indexIn( text, start + length );
  }
}

}
