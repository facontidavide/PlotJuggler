#include "dataload_csv.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QProgressDialog>
#include "selectlistdialog.h"

DataLoadCSV::DataLoadCSV()
{
    _extensions.push_back( "csv");
}

const QRegExp csv_separator("(\\,|\\;|\\ |\\t|\\|)");

const std::vector<const char*> &DataLoadCSV::compatibleFileExtensions() const
{
    return _extensions;
}

int DataLoadCSV::parseHeader(QFile *file,
                             std::vector<std::pair<bool,QString> >& ordered_names)
{
    QTextStream inA(file);

    QString first_line = inA.readLine();
    QString second_line = inA.readLine();

    int linecount = 1;

    QStringList string_items = first_line.split(csv_separator);
    QStringList secondline_items = second_line.split(csv_separator);

    for (int i=0; i < string_items.size(); i++ )
    {
        // remove annoying prefix
        QStringRef field_name ( &string_items[i] );
        if( field_name.startsWith( "field." ) )
        {
            field_name = field_name.mid(6);
        }

        QString qname = field_name.toString();
        if( qname.isEmpty())
        {
            qname = QString("_Column_%1").arg(i);
        }
        ordered_names.push_back( std::make_pair(true,qname) );
    }

    for (unsigned i=0; i < ordered_names.size(); i++ )
    {
        QString field_name ( ordered_names[i].second );
        if( field_name.endsWith( ".name" ) && i < ordered_names.size()-1)
        {
            QString prefix =  field_name.left(field_name.size() - 5 );
            QString replace_name = secondline_items[i];

            ordered_names[i].first = false;
            i++;
            //----------------------------------------

            QString value_prefix =  prefix + ".value";

            if( value_prefix.contains("vectors3d") )
            {
                ordered_names[i].second =  prefix + "/" + replace_name + "/x";
                i++;
                ordered_names[i].second =  prefix + "/" + replace_name + "/y";
                i++;
                ordered_names[i].second =  prefix + "/" + replace_name + "/z";
            }
            else
            {
                while( i < ordered_names.size() &&
                       ordered_names[i].second.startsWith( value_prefix ) )
                {
                    QString name = ordered_names[i].second;
                    QString suffix = name.right( name.size() - value_prefix.size());
                    ordered_names[i].second = prefix + "/" + replace_name;
                    if( suffix.size() > 0)
                    {
                        ordered_names[i].second.append( "/" + suffix );
                    }
                    i++;
                }
                i--;
            }
        }
    }

    while (!inA.atEnd())
    {
        inA.readLine();
        linecount++;
    }

    return linecount;
}

PlotDataMap DataLoadCSV::readDataFromFile(const QString &file_name, bool use_previous_configuration)
{
    const int TIME_INDEX_NOT_DEFINED = -2;

    int time_index = TIME_INDEX_NOT_DEFINED;

    PlotDataMap plot_data;

    QFile file( file_name );
    file.open(QFile::ReadOnly);

    std::vector<std::pair<bool, QString> > ordered_names;

    int linecount = parseHeader( &file, ordered_names);

    file.close();
    file.open(QFile::ReadOnly);
    QTextStream inB( &file );

    std::vector<PlotDataPtr> plots_vector;

    bool interrupted = false;

    int tot_lines = linecount -1;
    linecount = 0;

    QProgressDialog progress_dialog;
    progress_dialog.setLabelText("Loading... please wait");
    progress_dialog.setWindowModality( Qt::ApplicationModal );
    progress_dialog.setRange(0, tot_lines -1);
    progress_dialog.setAutoClose( true );
    progress_dialog.setAutoReset( true );
    progress_dialog.show();

    double prev_time = -1;

    // remove first line (header
    inB.readLine();

    //---- build plots_vector from header  ------
    QStringList valid_field_names;

    QSettings settings( "IcarusTechnology", "PlotJuggler");
    if( _default_time_axis.isEmpty() )
    {
        QVariant def = settings.value("DataLoadCSV/default_time_axis");
        if( !def.isNull() && def.isValid())
        {
            _default_time_axis = def.toString();
        }
    }

    for (unsigned i=0; i < ordered_names.size(); i++ )
    {
        bool valid = ordered_names[i].first;
        if( valid )
        {
            QString&   qname = ( ordered_names[i].second );
            std::string name = qname.toStdString();

            PlotDataPtr plot( new PlotData(name.c_str()) );
            plot_data.numeric.insert( std::make_pair( name, plot ) );

            valid_field_names.push_back( qname );
            plots_vector.push_back( plot );

            if (time_index == TIME_INDEX_NOT_DEFINED)
            {
                if( _default_time_axis == qname )
                {
                    time_index = i;
                }
            }
        }
    }

    if( time_index == TIME_INDEX_NOT_DEFINED && !use_previous_configuration)
    {
        QStringList field_names;
        field_names.push_back( "INDEX (auto-generated)" );
        field_names.append( valid_field_names );

        SelectFromListDialog* dialog = new SelectFromListDialog( &field_names );
        dialog->setWindowTitle("Select the time axis");
        int res = dialog->exec();

        if (res == QDialog::Rejected )
        {
            return PlotDataMap();
        }

        // vector is supposed to have only one element
        time_index = dialog->getSelectedRowNumber().at(0) -1;
        _default_time_axis = field_names.at( time_index + 1 ) ;
        settings.setValue("DataLoadCSV/default_time_axis", _default_time_axis);
    }

    //-----------------

    while (!inB.atEnd())
    {
        QString line = inB.readLine();

        QStringList string_items = line.split(csv_separator);
        double t = linecount;

        if( time_index >= 0)
        {
            t = string_items[ time_index ].toDouble();
            if( t <= prev_time)
            {
                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(0, tr("Error reading file"),
                                              tr("Selected time in notstrictly  monotonic. Do you want to abort?\n"
                                                 "(Clicking \"NO\" you continue loading)") );

                interrupted = (reply == QMessageBox::Yes);
                break;
            }
            prev_time = t;
        }

        int index = 0;
        for (int i=0; i < string_items.size(); i++ )
        {
            if( ordered_names[i].first )
            {
                double y = string_items[i].toDouble();
                PlotData::Point point( t,y );
                plots_vector[index]->pushBack( point );
                index++;
            }
        }

        if(linecount++ %100 == 0)
        {
            progress_dialog.setValue( linecount );
            QApplication::processEvents();
            if( progress_dialog.wasCanceled() ) {
                interrupted = true;
                break;
            }
        }
    }
    file.close();

    if(interrupted)
    {
        progress_dialog.cancel();
        plot_data.numeric.erase( plot_data.numeric.begin(), plot_data.numeric.end() );
    }

    return plot_data;
}



DataLoadCSV::~DataLoadCSV()
{

}

QDomElement DataLoadCSV::xmlSaveState(QDomDocument &doc) const
{
    QDomElement elem = doc.createElement("default");
    elem.setAttribute("time_axis", _default_time_axis );
    return elem;
}

bool DataLoadCSV::xmlLoadState(QDomElement &parent_element)
{
    QDomElement elem = parent_element.firstChildElement( "default" );
    if( !elem.isNull()    )
    {
        if( elem.hasAttribute("time_axis") )
        {
            _default_time_axis = elem.attribute("time_axis");
            return true;
        }
    }
    return false;
}
