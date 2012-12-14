#ifndef DCC_H
#define DCC_H

#include <QDialog>
#include "phonecall.h"

class QPushButton;
class QTableWidget;
class QTcpSocket;
class QFile;

class TransferData
{
public:
    QString user;
    QString status;
    QString file;
    qint64  size;
    qint64  position;
    int     complete;
    int     rate;
    qint64  eta;

    QFile* destination;
    QTcpSocket* comms;
};

class DCCDialog : public QDialog
{
    Q_OBJECT

public:
    DCCDialog(QWidget *parent = 0);
    ~DCCDialog();

    void command(const QString &cmd);

private slots:
    void saveAndClose();
    void processReadyRead();
    void startTransfer();

signals:
    void ircMsg(const QString &cmd);

private:
    QPushButton* closeButton;
    QTableWidget* table;
    PhoneCallDialog* call;

    QList<TransferData*> transfers;
};

#endif
