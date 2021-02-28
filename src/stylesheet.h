#ifndef STYLESHEET_H
#define STYLESHEET_H

#include <QString>
#include <map>
#include <QColor>
#include <QSettings>
#include <QApplication>

inline QString SetApplicationStyleSheet(QString style)
{
  QString out;
  QStringList lines = style.split("\n");

  std::map<QString, QString> palette;

  int i = 0;

  while( i < lines.size() )
  {
    if( lines[i++].contains("PALETTE START") )
    {
      break;
    }
  }

  while( i < lines.size() )
  {
    auto parts = lines[i].split(":");
    if( parts.size() == 2 )
    {
       QString value = parts[1].remove(" ");
       value.remove("\r");
       palette.insert( {parts[0].remove(" ") ,value } );
    }

    if( lines[i++].contains("PALETTE END") )
    {
      break;
    }
  }


  while( i < lines.size() )
  {
    QString line = lines[i];

    int pos_start = line.indexOf("${", 0 );
    if( pos_start != -1 )
    {
      int pos_end = line.indexOf("}",pos_start );
      if( pos_end == -1 )
      {
        throw std::runtime_error("problem loading stylesheet. Unclosed ${}");
      }
      int mid_length = pos_end -( pos_start+2 );
      QString id = line.mid(pos_start+2, mid_length);

      if (palette.count(id) == 0)
      {
        std::string msg = "Problem loading stylesheet: can't find palette id: ";
        msg += id.toStdString();
        throw std::runtime_error(msg);
      }
      QString color = palette[id];
      line = line.left(pos_start) + color + line.right(line.size() - pos_end -1);
    }

    out += line + "\n";
    i++;
  }

  QSettings settings;
  settings.setValue("StyleSheet::theme", palette["theme"]);

  dynamic_cast<QApplication*>(QCoreApplication::instance())->setStyleSheet(out);

  return palette["theme"];
}

#endif // STYLESHEET_H
