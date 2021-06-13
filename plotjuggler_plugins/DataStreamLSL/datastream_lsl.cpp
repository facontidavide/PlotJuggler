#include "datastream_lsl.h"
#include "ui_datastream_lsl.h"

#include <QMessageBox>
#include <QDebug>
#include <QDialog>

StreamLSLDialog::StreamLSLDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DataStreamLSL)
{
    ui->setupUi(this);

    ui->tableView->setModel(&_model);

    _model.setColumnCount(3);

    _model.setHeaderData(0, Qt::Horizontal, tr("ID"));
    _model.setHeaderData(1, Qt::Horizontal, tr("Name"));
    _model.setHeaderData(2, Qt::Horizontal, tr("Type"));

    ui->tableView->horizontalHeader()->setMinimumWidth(200);
    ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    resolveLSLStreams();

    _timer = new QTimer(this);
    connect(_timer, &QTimer::timeout,
            this, &StreamLSLDialog::resolveLSLStreams);

    _timer->start(1000);

    connect(ui->pushButtonSelectAll, &QPushButton::clicked, this, [&](){
        ui->tableView->selectAll();
    });
}

StreamLSLDialog::~StreamLSLDialog()
{
    delete ui;
}

QStringList StreamLSLDialog::getSelectedStreams()
{
    QStringList selected_streams;

    QModelIndexList list =ui->tableView->selectionModel()->selectedRows(0);

    for(const QModelIndex &index: list)
    {
        selected_streams.append(index.data(Qt::DisplayRole).toString());
    }

    return selected_streams;
}

void StreamLSLDialog::resolveLSLStreams()
{
    std::vector<lsl::stream_info> streams = lsl::resolve_streams(0.75);

    std::set<std::string> streams_id;
    for (const auto& stream: streams)
    {
       streams_id.insert(stream.uid());
    }
    if( streams_id == prev_streams_)
    {
        return;
    }
    prev_streams_ = streams_id;

    auto selectionModel = ui->tableView->selectionModel();

    _model.setRowCount(int(streams.size()));

    QStringList selected_ids;

    QModelIndexList selected_rows = selectionModel->selectedRows();
    for( const auto& row_index : selected_rows)
    {
        selected_ids.push_back( _model.item(row_index.row(), 0)->text() );
    }

    for (unsigned row = 0; row < streams.size(); ++row) {

        lsl::stream_info info = streams.at(row);

        auto source_id = QString::fromStdString(info.source_id());

        _model.setItem(row, 0, new QStandardItem(source_id));
        _model.setItem(row, 1, new QStandardItem(QString::fromStdString(info.name())));
        _model.setItem(row, 2, new QStandardItem(QString::fromStdString(info.type())));

        if( selected_ids.contains(source_id) )
        {
            ui->tableView->selectRow( row );
        }
    }

    ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
}

DataStreamLSL::DataStreamLSL():
    _running(false)
{
}

DataStreamLSL::~DataStreamLSL()
{
    shutdown();
}

bool DataStreamLSL::start(QStringList*)
{
    if (_running) {
        return _running;
    }

    StreamLSLDialog* dialog = new StreamLSLDialog();

    int res = dialog->exec();

    QStringList selection = dialog->getSelectedStreams();

    if(res == QDialog::Rejected || selection.empty())
    {
        _running = false;
        return false;
    }

    _running = true;

    for (QString name: selection) {
        Streamer *streamer = new Streamer;
        if(!streamer->queryStream(name)) {
            delete streamer;
            continue;
        }

        QThread *thread = new QThread(this);
        thread->setObjectName("Streamer Thread...");

        _streams.insert(thread, streamer);

        streamer->moveToThread(thread);
        connect(thread, &QThread::started, streamer, &Streamer::stream);
        connect(streamer, &Streamer::dataReceived, this, &DataStreamLSL::onDataReceived, Qt::QueuedConnection);
        connect(thread, &QThread::finished, streamer, &Streamer::deleteLater);

        thread->start();
    }

    dialog->deleteLater();
    return _running;
}

void DataStreamLSL::shutdown()
{
    if(_running) {
        _running = false;
        for (QThread *t : _streams.keys()) {
            t->requestInterruption();
            t->quit();
            t->wait(100);
        }
        _running = false;
    }
}

void DataStreamLSL::onDataReceived(std::vector<std::vector<double> > *chunk, std::vector<double> *stamps)
{
    Streamer *streamer = qobject_cast<Streamer*>(sender());

    if (streamer && (stamps->size() > 0))
    {
        //    qInfo() << chunk->size() << chunk->at(0).size();
        std::lock_guard<std::mutex> lock( mutex() );
        std::vector<std::string> channel_names = streamer->channelList();
        for (unsigned int i = 0; i < channel_names.size(); ++i) {

            auto& data = dataMap().getOrCreateNumberic( channel_names[i] );

            for (unsigned int j = 0; j < chunk->size(); ++j) {
                data.pushBack(PJ::PlotData::Point( stamps->at(j), chunk->at(j).at(i)));
            }
        }
    }

    emit this->dataReceived();
}
