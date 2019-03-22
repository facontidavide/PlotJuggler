/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "video_cheatsheet.h"

#include <QtWidgets>
#include <QVideoWidget>
#include <QDebug>
#include <QSize>

HelpVideo::HelpVideo(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Cheatsheet");
    _media_player = new QMediaPlayer(this, QMediaPlayer::VideoSurface);
    QVideoWidget *videoWidget = new QVideoWidget;

    videoWidget->setAutoFillBackground(true);
    auto palette = videoWidget->palette();
    palette.setColor( QPalette::Window, Qt::white);
    videoWidget->setPalette(palette);
    videoWidget->setAttribute(Qt::WA_OpaquePaintEvent, true);

    _playlist = new QMediaPlaylist();
    QListWidget* list_widget = new QListWidget();
    _text = new QLabel("placeholder");

    _media_player->setPlaylist(_playlist);

    _playlist->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);

    list_widget->setSelectionMode(QAbstractItemView::SingleSelection);

    _text->setFixedWidth(600);

    videoWidget->setFixedSize( QSize(900,600) );
    list_widget->setMinimumWidth(200);

    QBoxLayout *videoLayout = new QVBoxLayout;

    videoLayout->setMargin(20);
    videoLayout->setSpacing(20);
    videoLayout->addWidget(_text);
    videoLayout->addWidget(videoWidget);

    videoLayout->setStretch(0,1);
    videoLayout->setStretch(1,0);

    QBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(list_widget);
    layout->addLayout(videoLayout);

    setLayout(layout);

    _media_player->setVideoOutput(videoWidget);

    connect(_media_player,
            static_cast<void(QMediaPlayer::*)(QMediaPlayer::Error)>(&QMediaPlayer::error),
            this, &HelpVideo::handleError);

    setupHelps();

    for(const auto& section: _help_sections)
    {
        list_widget->addItem( section.title );
        _playlist->addMedia( section.video_url );
    }

    _text->setWordWrap(true);

    connect( list_widget, &QListWidget::currentRowChanged, this, [this](int row)
    {
         _playlist->setCurrentIndex(row);
         QString label_text = QString("<h2>%1</h2>%2").arg(_help_sections[row].title, _help_sections[row].text);
         _text->setText( label_text );
         _media_player->play();
    });

    list_widget->item(0)->setSelected(true);
}

HelpVideo::~HelpVideo()
{
}


void HelpVideo::setupHelps()
{
    _help_sections.push_back( {"Add timeseries to plot",
                               "Drag and Drop timeseries from the the list on the left side"
                               " using the <b>Left Mouse</b> button.",
                              QUrl("qrc:/cheatsheet/video/cheatsheet-drag-drop.mp4")});

    _help_sections.push_back( {"Create XY plots",
                               "Using two timeseries, sharing the same time axis, it is possible"
                               " to create a XY plot. Drag and Drop the Y axis as usual, using the"
                               " <b>Left Mouse</b> and the X axis with the <b>Right Mouse</b>.",
                              QUrl("qrc:/cheatsheet/video/cheatsheet-xy.mp4")});

    _help_sections.push_back( {"Pan view",
                               "To pan the plot area, either use the <b>Middle Mouse</b> button or"
                               " <b>CTRL + Left Mouse</b>.",
                              QUrl("qrc:/cheatsheet/video/cheatsheet-pan-view.mp4")});

    _help_sections.push_back( {"Remove Columns/Rows",
                               "To remove entire columns or/and rows, you must first clear all the curves.",
                              QUrl("qrc:/cheatsheet/video/cheatsheet-remove-column.mp4")});

    _help_sections.push_back( {"Resize Fonts",
                               "To change the size of the fonts used in the legend or the list of timeseries"
                               " on the left side, use <b>CTRL + Mouse Wheel</b>",
                              QUrl("qrc:/cheatsheet/video/cheatsheet-resize-font.mp4")});

    _help_sections.push_back( {"Swap Plots",
                              "Swap two plots using <b>CTRL + Right Mouse</b>.",
                              QUrl("qrc:/cheatsheet/video/cheatsheet-swap.mp4")});

    _help_sections.push_back( {"Time tracker",
                              "The time tracker is a vertical line that is helpful to visualize the"
                               " value of the timeseries at a given time.<br>"
                               "Furthermore, it is connected to the Publishers plugins: every time the time"
                               " tracker is activated, all the active publisher are called."
                               " Move the tracker either using the <b>Slider</b> at the bottom or pressing"
                               "<b> SHIFT + Left Mouse</b>.",
                              QUrl("qrc:/cheatsheet/video/cheatsheet-tracker.mp4")});


    _help_sections.push_back( {"Zoom Area",
                              "Click with the <b>Left Mouse</b> button on the plot area"
                              " and select the rectangle to zoom in.",
                              QUrl("qrc:/cheatsheet/video/cheatsheet-zoom-area.mp4")});

    _help_sections.push_back( {"Zoom In and Out",
                               "Use the <b>Mouse Wheel</b> to zoom in or out.<br>"
                               "Additionally, you can zoom a single axis moving the mouse cursor"
                               " either on the bottom X scale (<b>zoom horizontally only</b>) or on the left"
                               " Y scale (<b>zoom vertically only</b>).",
                              QUrl("qrc:/cheatsheet/video/cheatsheet-zoom-in-out.mp4")});
}



void HelpVideo::handleError(QMediaPlayer::Error)
{

    const QString errorString = _media_player->errorString();
    QString message = "Error: ";
    if (errorString.isEmpty())
        message += " #" + QString::number(int(_media_player->error()));
    else
        message += errorString;
    qDebug() << message;
}
