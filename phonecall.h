#ifndef PHONECALL_H
#define PHONECALL_H

#include <QDialog>
#include <QLabel>

#include <QTcpSocket>
#include <QTcpServer>
#include <QHostAddress>
#include <QTimer>

#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioInput>

#define AUDIO_PORT 2345

class QPushButton;
class QLineEdit;

class PhoneCallDialog : public QDialog
{
    Q_OBJECT

public:
    PhoneCallDialog(QWidget *parent = 0,const QString& nick = 0, const QString& ip = 0, bool caller = true);
    ~PhoneCallDialog();

    void connectToHost();
    void hangup();

private slots:
    void inputState(QAudio::State s);
    void outputState(QAudio::State s);
    void inputStatus();
    void outputStatus();
    void processMore();
    void acceptConnection();
    void startTransfer();
    void finished();

private:
    unsigned char toULaw(qint16 sample);
    qint16 fromULaw(unsigned char sample);

    unsigned char toALaw(qint16 sample);
    qint16 fromALaw(unsigned char sample);

    QPushButton* hangUpButton;
    QString m_host;
    QString m_nick;

    QLabel* statusLabel;
    QLabel* durationLabel;

    QAudioFormat      format;
    QAudioOutput*     audioOutput;
    QAudioInput*      audioInput;

    bool sendReady;
    bool receiveReady;
    QHostAddress dest;
    QTcpServer        server;
    QTcpSocket        client;
    QTcpSocket*       tcpServerConnection;

    QTimer*           timer;
    QIODevice*        AudioIn;
    QIODevice*        AudioOut;

    bool              caller;
    bool              sync;

    char*             bufferIn;
    int               bufferInLength;
    char*             bufferOut;
    int               bufferOutLength;
};

#endif
