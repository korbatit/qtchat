#include <QtGui>

#include <QTcpSocket>

#include "phonecall.h"
#include "dcc.h"

DCCDialog::DCCDialog(QWidget *parent)
    : QDialog(parent)
{
    QStringList list;
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    table = new QTableWidget(this);
    table->setColumnCount(8);
    list << tr("Status") << tr("File") << tr("Size") << tr("Position") << "%" << "KB/s" << "ETA" << tr("Nick");
    table->setHorizontalHeaderLabels(list);

    mainLayout->addWidget(table);

    closeButton = new QPushButton(tr("Save and Close"));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(saveAndClose()));

    mainLayout->addWidget(closeButton);

    setLayout(mainLayout);
    setWindowTitle(tr("File Transfers"));
    call = 0;
}

DCCDialog::~DCCDialog()
{
}

void DCCDialog::command(const QString &cmd)
{
    //user:ip:DCC SEND <filename> <ip> <port> <filesize>
    //user:ip:DCC CHAT accept <ip> <audio port> <video port>
    //user:ip:DCC CHAT call <ip> <audio port> <video port>

    //qWarning()<<"cmd: "<<cmd;

    QStringList list = cmd.split(':');
    QString user = list.at(0);
    QString ip = list.at(1);

    QStringList params = cmd.split(' ');

    //qWarning()<<"params "<<params;
    //qWarning()<<"list "<<list;

    if(params.size() < 6) return;

    if(params.at(1).contains("SEND")) {
        QMessageBox msgBox;
        QString text = QString("%1 wants to send you a file called %2").arg(user).arg(params.at(2));
        msgBox.setText(text);
        msgBox.setInformativeText("Do you want to accept the file transfer?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        int ret = msgBox.exec();

        if(ret == QMessageBox::Save) {
            QFileDialog dialog(this);
            dialog.setFileMode(QFileDialog::AnyFile);
            dialog.selectFile(params.at(2));
            if(dialog.exec()) {
                QStringList files = dialog.selectedFiles();
                //qWarning()<<"destination = "<<files.at(0);
                TransferData* data = new TransferData;
                data->user = user;
                data->status = "Waiting";
                data->file = params.at(2);
                data->destination = new QFile(files.at(0));
                data->size = params.at(5).toInt();
                data->position = 0;
                data->complete = 0;
                data->rate = 0;
                data->eta = -1;
                data->comms = new QTcpSocket;

                int currentRow = table->rowCount();
                //qWarning()<<"currentRow="<<currentRow;
                table->insertRow(currentRow);
                table->setItem(currentRow, 0, new QTableWidgetItem(data->status));
                table->setItem(currentRow, 1, new QTableWidgetItem(data->file));
                table->setItem(currentRow, 2, new QTableWidgetItem(QString("%1kB").arg(data->size/1024)));
                table->setItem(currentRow, 3, new QTableWidgetItem(QString("%1").arg(data->position)));
                table->setItem(currentRow, 4, new QTableWidgetItem(QString("%1\%").arg(data->complete)));
                table->setItem(currentRow, 5, new QTableWidgetItem(QString("%1Kb/s").arg(data->rate)));
                table->setItem(currentRow, 6, new QTableWidgetItem(QString("%1min").arg(data->eta)));
                table->setItem(currentRow, 7, new QTableWidgetItem(data->user));

                connect(data->comms,SIGNAL(connected()),this,SLOT(startTransfer()));
                connect(data->comms,SIGNAL(readyRead()),this,SLOT(processReadyRead()));
                transfers.append(data);
                table->resizeColumnsToContents();

                if(table->selectedItems().isEmpty()) {
                    table->selectRow(0);
                    // start transfer...
                }
            }
        }
        show();

    } else if(params.at(1).contains("CHAT")) {
        //qWarning()<<"got a CHAT message";
        if(params.at(2).contains("call")) {
            //qWarning()<<"got a CHAT call";
            // someone has requested a call
            QMessageBox msgBox;
            QString text = QString("%1 wants to talk").arg(user);
            msgBox.setText(text);
            msgBox.setInformativeText("Do you want to accept the call?");
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            int ret = msgBox.exec();

            if(ret == QMessageBox::Ok) {
                // send accept message and setup for incoming.
                QString msg = QString("PRIVMSG %1  :\001DCC CHAT accept %2 %3 %4\001\r\n").arg(user).arg(ip).arg(AUDIO_PORT).arg(0);
                PhoneCallDialog* incoming = new PhoneCallDialog(this,user,ip,false);
                incoming->show();
                emit ircMsg(msg);
            }

        } else if(params.at(2).contains("accept")) {
            //qWarning()<<"got a CHAT accept";
            // your request has been accepted, connect to other end.
            //qWarning()<<"I should be able to connect to "<<ip<<QString(" on port %1").arg(AUDIO_PORT);
            if(call) delete call;
            call = new PhoneCallDialog(this,user,ip,true);
            call->show();
            call->connectToHost();
        }
    }
}

void DCCDialog::startTransfer()
{
    //qWarning()<<"TODO:startTransfer()";
}

void DCCDialog::processReadyRead()
{
    //qWarning()<<"TODO:processReadyRead()";
}

void DCCDialog::saveAndClose()
{
    close();
}

