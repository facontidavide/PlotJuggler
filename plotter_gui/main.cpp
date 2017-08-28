#include "mainwindow.h"
#include <QApplication>
#include <QSplashScreen>
#include <QThread>
#include <QCommandLineParser>

QString getFunnySubtitle(){

    qsrand(time(NULL));
    int n = qrand() % 18;
    switch(n)
    {
    case 0: return "The best excuse to buy a second monitor";
    case 1: return "Now with 100% more splashscreens";
    case 2: return "Because command line tools suck";
    case 3: return "Time-series at your fingertips";
    case 4: return "Changing the world, one plot at a time";
    case 5: return "Have you starred me on Github?";
    case 6: return "Insert [useless message] here";
    case 7: return "\"Harry Plotter\" was also an option";
    case 8: return "Add data and mix vigorously";
    case 9: return "Just Plot It!";
    case 10: return "I didn't find a better name...";
    case 11: return "Happy Plotting, or get your money back";
    case 12: return "\"It won't take long to code that\".. Davide, 2014";
    case 13: return "Startup is actually fast. I added splashscreens for fun";
    case 14: return "Graphic-less version coming soon";
    case 15: return "Visualize data responsibly";
    }
    return "Juggle with data";
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("PlotJuggler");

    qApp->setStyleSheet(QString("QToolTip {\n"
                                "   border: 1px solid black;\n"
                                "   border-radius: 4px;\n"
                                "   background: white;\n"
                                "   color: black; }" ));

    QString VERSION_STRING = QString::number(PJ_MAJOR_VERSION) + QString(".") +
            QString::number(PJ_MINOR_VERSION) + QString(".") +
            QString::number(PJ_PATCH_VERSION);

    app.setApplicationVersion(VERSION_STRING);

    QCommandLineParser parser;
    parser.setApplicationDescription("PlotJuggler: the time series visualization tool that you deserve ");
    parser.addVersionOption();
    parser.addHelpOption();

    QCommandLineOption nosplash_option(QStringList() << "n" << "nosplash",
                                       QCoreApplication::translate("main", "Don't display the splashscreen"));
    parser.addOption(nosplash_option);

    QCommandLineOption test_option(QStringList() << "t" << "test",
                                   QCoreApplication::translate("main", "Generate test curves at startup"));
    parser.addOption(test_option);

    QCommandLineOption loadfile_option(QStringList() << "d" << "datafile",
                                       QCoreApplication::translate("main", "Load a file containing data"),
                                       QCoreApplication::translate("main", "file") );
    parser.addOption(loadfile_option);

    QCommandLineOption config_option(QStringList() << "l" << "layout",
                                     QCoreApplication::translate("main", "Load a file containing the layout configuration"),
                                     QCoreApplication::translate("main", "file") );
    parser.addOption(config_option);

    QCommandLineOption buffersize_option(QStringList() << "buffer_size",
                                     QCoreApplication::translate("main", "Change the maximum size of the streaming buffer (minimum: 10 default: 60)"),
                                     QCoreApplication::translate("main", "seconds") );
    parser.addOption(buffersize_option);

    parser.process( *qApp );

    /*
     * You, fearless code reviewer, decided to start a journey into my source code.
     * For your bravery, you deserve to know the truth, no matter how hard it is to accept it.
     * The splashscreen is useless; not only it is useless, it will make your start-up
     * time slower by a couple of seconds for absolutely no reason.
     * But what are two seconds compared with the time that PlotJuggler will save you?
     * The splashscreen is the connection between me and my users, the glue that keep
     * together our invisible relationship.
     * Now it is up to you to decide: you can block the splashscreen forever or not,
     * reject a message that brings a little of happiness into your day, spent analyzing data.
     * Please don't do it.
     */

    if( parser.isSet(nosplash_option) == false)
    // if(false) // if you uncomment this line, a kitten will die somewhere in the world.
    {
        QPixmap main_pixmap(":/splash/resources/splash.jpg");

        QPainter painter;
        painter.begin( &main_pixmap);
        painter.setPen(QColor(77, 77, 77));

        QString subtitle = getFunnySubtitle();
        int font_size = 36;
        do{
            painter.setFont( QFont("Arial", font_size-- ) );
        }while( painter.fontMetrics().width(subtitle) > 550 );

        painter.drawText( QRect(50, 200, 580, 100), Qt::AlignHCenter | Qt::AlignVCenter, subtitle );
        painter.end();

        QSplashScreen splash(main_pixmap);
        splash.show();

        MainWindow w( parser );

        for (int i =0; i<(25 + subtitle.size()/2) && !splash.isHidden(); i++ ) {
            app.processEvents();
            QThread::msleep(100);
            splash.raise();
        }

        splash.close();
        w.show();
        return app.exec();
    }
    else{
        MainWindow w( parser );
        w.show();
        return app.exec();
    }

    return -1;
}
