#ifndef CHARDIALOG_H
#define CHATDIALOG_H

#include <QList>
#include <QTextEdit>
#include <QDialog>
#include <QTabWidget>
#include <QModelIndex>
#include <QListView>

#include "dcc.h"
#include "connection.h"

class QGridLayout;
class QListWidget;
class QLabel;
class QLineEdit;
class QListWidgetItem;

class ChannelInfo
{
public:
    QString channel;
    QStringList users;
    int userCount;
    QString topic;
};

class TabWidget : public QTabWidget
{
    Q_OBJECT
public:
    TabWidget(QWidget *parent = 0);
    ~TabWidget();

    void setTabTextColor(int index, const QColor & color );

signals:
    void rightClick(const QPoint& pos);

protected:
    void mousePressEvent(QMouseEvent *event);
};

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    ChatDialog(QWidget *parent = 0);
    ~ChatDialog();

    void setNick(QString nick);
    void setHost(QString host);
    void start();
    void disconnect();

    QListWidgetItem* findUser(const QString &nick);
    QTextEdit* addChannelTab(const QString &channel);
    QTextEdit* findChannelTab(const QString &channel);
    bool isUser(const QString &nick);
    void channelMessage(const QString &channel, const QString &msg, QColor color);
    void clearTab();
    void closeCurrentWindow();
    void channelSelection();

public slots:
    void displayMessage(const QString &from, const QString &message);
    void returnPressed();
    void addUser(const QString &channel, const QString &nick);
    void rmUser(const QString &channel, const QString &nick);
    void userCall(QListWidgetItem* item);
    void dccCommand(const QString &cmd);
    void markAsAway(bool away, const QString &nick);
    void updateList(int index);
    void displayTabMenu(const QPoint& pos);
    void joinChannel(const QModelIndex& index);

private:
    QGridLayout *mainLayout;
    TabWidget   *tabWidget;
    QListWidget *listWidget;
    QLabel      *label;
    QLineEdit   *lineEdit;
    DCCDialog   *dcc;
    QString     focusTabName;
    QString     lastNick;
    QListView   *channelListView;

    QList<QTextEdit*> tabs;
    QList<ChannelInfo*> channelLists;
    QList<ChannelInfo*> allChannels;

    Connection* connection;
    QString nickname;
    QString hostname;
    bool away;
};

#endif
