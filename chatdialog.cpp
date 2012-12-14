#include <QtGui>

#include "chatdialog.h"

TabWidget::TabWidget(QWidget *parent)
    : QTabWidget(parent)
{
}

TabWidget::~TabWidget()
{
}

void TabWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
        emit rightClick(event->pos());

    event->ignore();
}

void TabWidget::setTabTextColor(int index, const QColor & color )
{
    tabBar()->setTabTextColor(index,color);
}

ChatDialog::ChatDialog(QWidget *parent)
    : QDialog(parent)
{
   QDesktopWidget desktop;
   QRect space = desktop.availableGeometry();

   mainLayout = new QGridLayout(this);

   tabWidget = new TabWidget;
   tabWidget->setTabPosition(QTabWidget::South);
   if(space.width() > 640) {
       tabWidget->setMinimumWidth(540);
   } else {
       tabWidget->setMinimumWidth(space.width()*3/4);
   }

   addChannelTab("server");
   focusTabName = "server";

   connect(tabWidget,SIGNAL(currentChanged(int)),this,SLOT(updateList(int)));
   connect(tabWidget,SIGNAL(rightClick(const QPoint&)),this,SLOT(displayTabMenu(const QPoint&)));

   listWidget = new QListWidget;
   connect(listWidget,SIGNAL(itemActivated(QListWidgetItem*)),this,SLOT(userCall(QListWidgetItem*)));

   if(space.width() > 640) {
       listWidget->setMinimumWidth(100);
   } else {
       listWidget->setMinimumWidth(space.width()*1/4);
   }
   listWidget->setFocusPolicy(Qt::NoFocus);
   listWidget->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);

   label = new QLabel;
   label->setText(tr("Message"));
   label->setMinimumWidth(50);
   label->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);

   lineEdit = new QLineEdit;
   lineEdit->setFocusPolicy(Qt::StrongFocus);
   lineEdit->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
   connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(returnPressed()));

   mainLayout->addWidget(tabWidget,0,0,1,2);
   mainLayout->addWidget(listWidget,0,2,1,1);
   mainLayout->addWidget(label,1,0,1,1);
   mainLayout->addWidget(lineEdit,1,1,1,2);

   setLayout(mainLayout);
   setWindowTitle(tr("Chat Panel"));

   connection = new Connection(this);

   connect(connection, SIGNAL(newMessage(const QString &, const QString &)),
           this, SLOT(displayMessage(const QString &, const QString &)));
   connect(connection, SIGNAL(newParticipant(const QString &, const QString &)),
           this, SLOT(addUser(const QString &, const QString &)));
   connect(connection, SIGNAL(participantLeft(const QString &, const QString &)),
           this, SLOT(rmUser(const QString &, const QString &)));
   connect(connection, SIGNAL(dccCommand(const QString &)),
           this, SLOT(dccCommand(const QString &)));
   connect(connection, SIGNAL(awayChanged(bool, const QString &)),
           this, SLOT(markAsAway(bool, const QString &)));

   dcc = new DCCDialog(this);
   connect(dcc,SIGNAL(ircMsg(const QString &)),connection,SLOT(sendMessage(const QString &)));

   away = false;
   channelListView = 0;
}

ChatDialog::~ChatDialog()
{
}

void ChatDialog::clearTab()
{
    QString currentTab = tabs.at(tabWidget->currentIndex())->windowTitle();
    QTextEdit* textEdit = findChannelTab(currentTab);
    if(textEdit)
        textEdit->clear();
}

void ChatDialog::closeCurrentWindow()
{
    QString currentTab = tabs.at(tabWidget->currentIndex())->windowTitle();

    if(qstrcmp(currentTab.toLocal8Bit().constData(),"server") == 0)
        exit(0);

    QString msg = QString("PART %1\r\n").arg(currentTab);
    for(int i=0;i<channelLists.size();i++) {
        if(qstrcmp(currentTab.toLocal8Bit().constData(),channelLists.at(i)->channel.toLocal8Bit().constData()) == 0) {
            // must be a channel not a user window, add the hash!
            msg = QString("PART #%1\r\n").arg(currentTab);
            connection->sendMessage(msg);
            break;
        }
    }

    // just close current tab
    for(int i=0;i<tabs.size();i++) {
        if(tabs.at(i)->windowTitle().contains(currentTab)) {
            tabs.removeAll(tabs.at(i));
            tabWidget->removeTab(tabWidget->currentIndex());
            break;
        }
    }
}

void ChatDialog::markAsAway(bool away, const QString &nick)
{
    QString msg;

    if(away)
        msg = QString("%1 is away").arg(nick);
    else
        msg = QString("%1 has returned").arg(nick);

    // Send message to any tab channels the nick on part of.
    for(int i=0;i<channelLists.size();i++) {
        for(int j=0;j<channelLists.at(i)->users.size();j++) {
            if(channelLists.at(i)->users.at(j).contains(nick))
                channelMessage(channelLists.at(i)->channel,msg,Qt::gray);
        }
    }

    // In current channel list
    QListWidgetItem* item = 0;
    item = findUser(nick);
    if(!item) return;

    if(!away) {
        // Change current channel list state to Available
        QColor msgColor = Qt::black;
        item->setForeground(QBrush(msgColor,Qt::SolidPattern));
        item->setText(item->text());
        if(nickname.contains(nick) && this->away) {
            // Send server AWAY message
            this->away = false;
            msg = QString(":%1 AWAY\r\n").arg(nick);
            connection->sendMessage(msg);
        }
    } else {
        // Change current channel list state to Not Available
        QColor msgColor = Qt::gray;
        item->setForeground(QBrush(msgColor,Qt::SolidPattern));
        item->setText(item->text());
        if(nickname.contains(nick) && !this->away) {
            // Send server NOT AWAY message
            this->away = true;
            msg = "AWAY :Not available\r\n";
            connection->sendMessage(msg);
        }
    }
}

void ChatDialog::setNick(QString nick)
{
    nickname = nick;
    connection->setNick(nick);
}

void ChatDialog::setHost(QString host)
{
    hostname = host;
    connection->setHost(host);
}

void ChatDialog::displayMessage(const QString &from, const QString &message)
{
    if(from.isEmpty() || message.isEmpty())
        return;

#ifdef QTCHAT_DEBUG
    qWarning()<<"displayMessage() "<<from<<", "<<message;
#endif

    QColor msgColor = Qt::gray;
    QTextEdit* textEdit = tabs.at(0);

    for(int i=0;i<tabs.size();i++) {
        if(from.contains(hostname) && tabs.at(i)->windowTitle().contains("server")) {
            // message from server
            msgColor = Qt::black;
            textEdit = tabs.at(i);
            break;
        } else if(from.contains("qtchat")) {
            // program message
            msgColor = Qt::magenta;
            textEdit = tabs.at(i);
            break;
        } else if(from.contains("notice")) {
            // message notice
            msgColor = Qt::green;
            textEdit = tabs.at(i);
            break;
        } else if(from.startsWith("#")) {
            // message from channel
            QString ch = from;
            ch.remove("#");
            for(int i=0;i<tabs.size();i++) {
                if(tabs.at(i)->windowTitle().contains(ch)) {
                    msgColor = Qt::blue;
                    textEdit = tabs.at(i);
                    if(i != tabWidget->currentIndex()) {
                        tabWidget->setTabTextColor(i,Qt::red);
                        tabWidget->setTabText(i,tabWidget->tabText(i));
                        tabWidget->update();
                    }
                    break;
                }
            }
            break;
        } else {
            // message from another user
            bool matched = false;
            for(int i=0;i<tabs.size();i++) {
                if(tabs.at(i)->windowTitle().contains(from)) {
                    msgColor = Qt::blue;
                    textEdit = tabs.at(i);
                    if(i != tabWidget->currentIndex()) {
                        // no current focused window
                        tabWidget->setTabTextColor(i,Qt::red);
                        tabWidget->setTabText(i,tabWidget->tabText(i));
                        tabWidget->update();
                    }
                    matched = true;
                    break;
                }
            }
            if(!matched) {
                // create a new tab!
                textEdit = addChannelTab(from);
                tabWidget->setCurrentIndex(tabWidget->indexOf(textEdit));
                focusTabName = from;
                msgColor = Qt::blue;
            }
            break;
        }
    }
    if (from.contains(hostname) && message.startsWith("#")) {
        // Entry from channel list
        QStringList list = message.split(" ");
        if(list.size() >= 3) {
            QString ch = list.at(0); ch.remove("#");
            int count = list.at(1).toInt();
            QString topic = message.mid(list.at(0).length()+list.at(1).length()+3);
            topic.remove("\r\n");
            bool match = false;
            for (int i=0;i<allChannels.size();i++) {
                if (allChannels.at(i)->channel == ch) {
                    match = true;
                    break;
                }
            }
            if(!match) {
                ChannelInfo *info = new ChannelInfo;
                info->channel = ch;
                info->userCount = count;
                info->topic = topic;
                allChannels.append(info);
            }
        }
        return;
    }

    QString msg = QString(message);
    msg.remove("\r\n");
    channelMessage(from,msg,msgColor);
}

void ChatDialog::returnPressed()
{
    QString msg;
    QStringList list;
    QString text = lineEdit->text();

    QString currentTab = tabs.at(tabWidget->currentIndex())->windowTitle();

    if(away) {
        // any activity after away, changes back to available
        markAsAway(false,nickname);
    }

    if(text.isEmpty())
        return;

    // - starts with capital letters, raw irc command.
    // - starts with '/' builtin command.
    // - \xxx should represent special chars.

    if(text.startsWith("/")) {
        list = text.split(' ');
        if (text.contains("/r ", Qt::CaseInsensitive)) {
            if (lastNick.length() > 0) {
                msg = QString("PRIVMSG %1 :%2").arg(lastNick).arg(text.mid(list.at(0).length()+1));
                msg.append("\r\n");
                connection->sendMessage(msg);
                msg = QString("%1: %2").arg(lastNick).arg(text.mid(list.at(0).length()+1));
                displayMessage(lastNick, msg);
            }

        } else if (text.contains("/w ", Qt::CaseInsensitive)) {
            if(list.size() >= 3) {
                msg = QString("PRIVMSG %1 :%2").arg(list.at(1)).arg(text.mid(list.at(0).length()+list.at(1).length()+2));
                msg.append("\r\n");
                connection->sendMessage(msg);
                msg = QString("%1: %2").arg(list.at(1)).arg(text.mid(list.at(0).length()+list.at(1).length()+2));
                displayMessage(list.at(1), msg);
            } else {
                displayMessage("qtchat","Unknown command or format, should be /w <nick> <message>");
            }

        } else if (text.contains("/clearall", Qt::CaseInsensitive)) {
            for (int i = 0; i < tabs.size(); i++)
                tabs.at(i)->clear();

        } else if (text.contains("/clear", Qt::CaseInsensitive)) {
            clearTab();

        } else if (text.contains("/close", Qt::CaseInsensitive)) {
            closeCurrentWindow();

        } else if (text.contains("/exit", Qt::CaseInsensitive)) {
            exit(0);

        } else if (text.contains("/quit", Qt::CaseInsensitive)) {
            closeCurrentWindow();

        } else if (text.contains("/list", Qt::CaseInsensitive)) {
            msg = QString("LIST");
            displayMessage("server", msg);
            msg.append("\r\n");
            connection->sendMessage(msg);

        } else if (text.contains("/join ", Qt::CaseInsensitive)) {
            msg = QString("JOIN #%1").arg(list.at(1));
            displayMessage(QString("#%1").arg(list.at(1)), msg);
            msg.append("\r\n");
            connection->sendMessage(msg);

        } else if (text.contains("/raw ", Qt::CaseInsensitive)) {
            msg = QString("%1").arg(text.mid(5));
            displayMessage(currentTab, msg);
            msg.append("\r\n");
            connection->sendMessage(msg);

        } else {
            text.append("\r\n");
            connection->sendMessage(text);
            displayMessage(nickname, text);
        }
    } else {
        QString msg = QString("%1: ").arg(nickname);
        msg.append(text);

        if(!currentTab.contains("server")) {
            if(isUser(currentTab))
                text = QString("PRIVMSG %1 :%2").arg(currentTab).arg(text);
            else
                text = QString("PRIVMSG #%1 :%2").arg(currentTab).arg(text);
        }
        displayMessage(currentTab, msg);

        text.append("\r\n");
        connection->sendMessage(text);
    }
    lineEdit->clear();
}

bool ChatDialog::isUser(const QString &nick)
{
    for(int j=0;j<connection->users.size();j++) {
        if(qstrcmp(connection->users.at(j)->user.toLocal8Bit().constData(),nick.toLocal8Bit().constData()) == 0)
            return true;
    }
    return false;
}

void ChatDialog::addUser(const QString &channel, const QString &nick)
{
    if(nick.isEmpty() || nick.length() <= 2)
        return;

    QString ch = channel;
    ch.remove("#");

    QTextEdit* textEdit = findChannelTab(ch);

    if(!textEdit) {
        textEdit = addChannelTab(ch);
        tabWidget->setCurrentIndex(tabWidget->indexOf(textEdit));
        focusTabName = ch;

        ChannelInfo* newChannel = new ChannelInfo;
        newChannel->channel = ch;
        newChannel->users.append(nick);
        channelLists.append(newChannel);
    }

    for(int i=0;i<channelLists.size();i++) {
        if(channelLists.at(i)->channel.contains(ch)) {
            channelLists.at(i)->users.append(nick);
            channelLists.at(i)->users.removeDuplicates();
        }
    }

    QString message = QString("%1 has joined").arg(nick);
    channelMessage(channel,message,Qt::gray);

    updateList(tabWidget->currentIndex());
}

void ChatDialog::updateList(int index)
{
    // Check current tab, reload listWidget with current channel list.
    QString currentChannel = tabs.at(index)->windowTitle();
    listWidget->clear();
    for(int i=0;i<channelLists.size();i++) {
        if(channelLists.at(i)->channel.contains(currentChannel)) {
            listWidget->insertItems(0,channelLists.at(i)->users);
            break;
        }
    }

    tabWidget->setTabTextColor(tabWidget->currentIndex(),Qt::black);
    tabWidget->update();
    tabWidget->setTabText(tabWidget->currentIndex(),tabWidget->tabText(tabWidget->currentIndex()));

    // Set any aways
    QString msg;
    QListWidgetItem* item = 0;

    for(int i=0;i<listWidget->count();i++) {
        item = listWidget->item(i);
        for(int j=0;j<connection->users.size();j++) {
            if(connection->users.at(j)->user.length() > 0) {
                if(connection->users.at(j)->away) {
                    if(connection->users.at(j)->user.contains(item->text())) {
                        // mark as away
                        QColor msgColor = Qt::gray;
                        item->setForeground(QBrush(msgColor,Qt::SolidPattern));
                        item->setText(item->text());
                    }
                }
            }
        }
    }
}

void ChatDialog::rmUser(const QString &channel, const QString &nick)
{
    if(nick.isEmpty())
        return;

    int idx = 0;
    QTextEdit* textEdit = tabs.at(0);

#ifdef QTCHAT_DEBUG
    qWarning()<<"rmUser "<<nick<<", ch = "<<channel;
#endif

    // Find tab for channel
    for(int i=0;i<tabs.size();i++) {
        if(tabs.at(i)->windowTitle().contains(channel)) {
            textEdit = tabs.at(i);
            idx = i;
            break;
        }
    }
    // Message to channel tab 'left'
    QString message = QString("%1 has left").arg(nick);
    channelMessage(channel,message,Qt::gray);

    // Remove user, channel tab if needed
    for(int i=0;i<channelLists.size();i++) {
        if(channelLists.at(i)->channel.contains(channel)) {
            for(int j=0;j<channelLists.at(i)->users.size();j++) {
                if(channelLists.at(i)->users.at(j).contains(nick)) {
                    // Remove user from channel list
                    channelLists.at(i)->users.removeAt(j);
                    if(channelLists.at(i)->users.size() == 0) {
                        // No more users, remove channelList,tab and textedit
                        channelLists.removeAt(i);
                        tabs.removeAt(i);
                        tabWidget->removeTab(idx);
                    }
                    break;
                }
            }
        }
    }
    updateList(tabWidget->currentIndex());
}

void ChatDialog::start()
{
    QString msg = QString(tr("Trying to connect to %1 on port 6667...").arg(hostname));
    displayMessage("qtchat", msg);

    connection->start();

    allChannels.clear();
}

void ChatDialog::disconnect()
{
    QString msg = QString(tr("User disconnection from %1").arg(hostname));
    displayMessage("qtchat", msg);

    connection->stop();
}

QListWidgetItem* ChatDialog::findUser(const QString &nick)
{
    // Returns the nicks entry in the user list.

    QListWidgetItem* item = 0;
    for(int i=0;i<listWidget->count();i++) {
        if(listWidget->item(i)->text().contains(nick)) {
            item = listWidget->item(i);
            break;
        }
    }
    return item;
}

QTextEdit* ChatDialog::addChannelTab(const QString &channel)
{
    // Add a new chat tab
    QString name = channel;
    name.remove("#");
    QTextEdit* textEdit = 0;

    tabs.append(new QTextEdit(this));
    tabs.last()->setWindowTitle(name);
    tabs.last()->setFocusPolicy(Qt::NoFocus);
    tabs.last()->setReadOnly(true);
    tabs.last()->setFontPointSize(8);
    tabWidget->addTab(tabs.last(),name);
    textEdit = tabs.last();
    tabWidget->setCurrentIndex(tabWidget->indexOf(textEdit));
    focusTabName = name;

    return textEdit;
}

QTextEdit* ChatDialog::findChannelTab(const QString &channel)
{
    QString name = channel;
    name.remove("#");
    QTextEdit* textEdit = 0;

    for(int i=0;i<tabs.size();i++) {
        if(tabs.at(i)->windowTitle().contains(name)) {
            textEdit = tabs.at(i);
            break;
        }
    }

    return textEdit;
}

void ChatDialog::channelMessage(const QString &channel, const QString &message, QColor color)
{
#ifdef QTCHAT_DEBUG
    qWarning()<<"channelMessage ch="<<channel<<", msg="<<message;
#endif

    QTextEdit* textEdit = findChannelTab(channel);
    if(!textEdit)
        textEdit = findChannelTab("server");

    if(!findUser(channel)) {
        // Must be a private message
        lastNick = channel;
    }

    QDateTime now(QDateTime::currentDateTime());
    QString msg = QString("%1: %2").arg(now.toString("dd/MM/yy HH:mm")).arg(message);
    QColor oldColor = textEdit->textColor();
    textEdit->setTextColor(color);
    textEdit->append(msg);
    textEdit->setTextColor(oldColor);
}

void ChatDialog::dccCommand(const QString &cmd)
{
    dcc->command(cmd);
}

void ChatDialog::userCall(QListWidgetItem* item)
{
    for(int i=0;i<connection->users.size();i++) {
        if(qstrcmp(item->text().toLocal8Bit().constData(),connection->users.at(i)->user.toLocal8Bit().constData()) == 0) {
#ifdef QTCHAT_DEBUG
            qWarning()<<"user = "<<connection->users.at(i)->user<<", realName = "<<connection->users.at(i)->realName<<", ip = "<<connection->users.at(i)->ip;
            qWarning()<<"channels = "<<connection->users.at(i)->channels<<", server = "<<connection->users.at(i)->server<<", comment = "<<connection->users.at(i)->comment;
#endif
            QMessageBox msgBox;
            if(connection->users.at(i)->realName.contains("qtchat")) {
                QString msg = QString("Request to call %1@%2").arg(connection->users.at(i)->user).arg(connection->users.at(i)->ip);
                msgBox.setText(msg);
                msgBox.setInformativeText("Do you want to call this person?");
                msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                msgBox.setDefaultButton(QMessageBox::Cancel);
                int ret = msgBox.exec();
                if(ret == QMessageBox::Ok) {
                    QString msg = QString("PRIVMSG %1 :\001DCC CHAT call %2 %3 0\001\r\n").arg(connection->users.at(i)->user).arg(connection->users.at(i)->ip).arg(2345);
                    // setup listener for incoming connection, accept connection.
                    connection->sendMessage(msg);
                }


            } else {
                msgBox.setText("Not a qtchat client, cannot use extra functionality!");
                msgBox.exec();
            }
        }
    }
}

void ChatDialog::displayTabMenu(const QPoint& pos)
{
    Q_UNUSED(pos)

    QString currentTab = tabs.at(tabWidget->currentIndex())->windowTitle();
    closeCurrentWindow();
}

void ChatDialog::channelSelection()
{
    if(!channelListView) {
        channelListView = new QListView;
        connect(channelListView,SIGNAL(pressed(const QModelIndex&)),
                this,SLOT(joinChannel(const QModelIndex&)));
        QStandardItemModel *model = new QStandardItemModel();
        for(int i = 0;i<allChannels.size();i++) {
            QStandardItem *item = new QStandardItem(allChannels.at(i)->channel);
            model->setItem(i,0,item);
        }
        channelListView->setModel(model);
    }
    channelListView->show();
}

void ChatDialog::joinChannel(const QModelIndex& index)
{
    channelListView->hide();
    qWarning()<<index.data();
    QString msg = QString("JOIN #%1").arg(index.data().toString());
    displayMessage(QString("#%1").arg(index.data().toString()), msg);
    msg.append("\r\n");
    connection->sendMessage(msg);
}

