#ifndef CONNECTION_H
#define CONNECTION_H

#include <QHostAddress>
#include <QString>
#include <QStringList>
#include <QTcpSocket>
#include <QTimer>

class UserInfo
{
public:
    UserInfo() { user.clear(); realName.clear(); ip.clear(); channels.clear(); server.clear(); comment.clear(); away = false; }
    ~UserInfo() { }

    QString user;
    QString realName;
    QString ip;
    QString channels;
    QString server;
    QString comment;
    bool    away;
};

class Connection : public QTcpSocket
{
    Q_OBJECT
public:
    Connection(QObject *parent = 0);

    void setNick(const QString &name);
    QString nickName() const;
    void setHost(const QString &host);
    void start();
    void stop();

    QString uniqueID;
    QList<UserInfo*> users;

public slots:
    void sendMessage(const QString &message);

signals:
    void readyForUse();
    void newMessage(const QString &from, const QString &message);
    void newParticipant(const QString  &channel, const QString &nick);
    void participantLeft(const QString &channel, const QString &nick);
    void dccCommand(const QString &cmd);
    void awayChanged(bool away,const QString &nick);

private slots:
    void login();
    void disconnected();
    void processReadyRead();
    void who();

private:
    QTimer*     timer;
    QString     host;
    QString     username;
    QByteArray  buffer;
    bool        loginSent;
    int         position;
    bool        userAway;
};

#endif
