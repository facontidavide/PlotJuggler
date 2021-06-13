#ifndef TAB_BAR_H
#define TAB_BAR_H

#include <QTabBar>
#include <QLabel>
#include <QPixmap>
#include <QDebug>
#include <QTabWidget>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QApplication>

class TabWidget : public QTabWidget
{
  Q_OBJECT
public:
  TabWidget(QWidget* parent) : QTabWidget(parent)
  {
    this->tabBar()->setFixedHeight(40);
    setAcceptDrops(true);
  }

  void dragEnterEvent(QDragEnterEvent* ev) override
  {
    if (ev->pos().y() > 43)
    {
      ev->ignore();
      return;
    }
    const QMimeData* mimeData = ev->mimeData();
    QStringList mimeFormats = mimeData->formats();
    for (const QString& format : mimeFormats)
    {
      QByteArray encoded = mimeData->data(format);
      QDataStream stream(&encoded, QIODevice::ReadOnly);

      QStringList curves;

      while (!stream.atEnd())
      {
        QString curve_name;
        stream >> curve_name;
        if (!curve_name.isEmpty())
        {
          curves.push_back(curve_name);
        }
      }
    }
    ev->ignore();
  }

  void dragMoveEvent(QDragMoveEvent* ev) override
  {
    if (ev->pos().y() > 43)
    {
      ev->ignore();
      return;
    }
    ev->accept();
  }

  void dragLeaveEvent(QDragLeaveEvent* ev) override
  {
    _plot_to_move = "";
    QApplication::restoreOverrideCursor();
  }

  void dropEvent(QDropEvent* ev) override
  {
    emit movingPlotWidgetToTab(_plot_to_move);
    _plot_to_move = "";
    QApplication::restoreOverrideCursor();
  }

private:
  QString _plot_to_move;

signals:

  void movingPlotWidgetToTab(QString plot_name);
};

#endif  // TAB_BAR_H
