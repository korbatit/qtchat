#include "connection.h"

#include <QtNetwork>

Connection::Connection(QObject *parent)
    : QTcpSocket(parent)
{
    username = tr("unknown");
    QObject::connect(this, SIGNAL(readyRead()), this, SLOT(processReadyRead()));
    QObject::connect(this, SIGNAL(connected()),this, SLOT(login()));
    QObject::connect(this, SIGNAL(disconnected()),this, SLOT(disconnected()));
    loginSent = false;
    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(who()));
    timer->setInterval(30000); // Every 30s
    position = 0;
    userAway = false;
}

void Connection::start()
{
    loginSent = false;

#ifdef QTCHAT_DEBUG
    qWarning()<<"connect to host:"<<host<<" on port 6667";
#endif

    connectToHost(host,6667);
    login();
}

void Connection::setNick(const QString& nick)
{
    username = nick;
}

QString Connection::nickName() const
{
    return username;
}

void Connection::setHost(const QString& hostname)
{
    host = hostname;
}

void Connection::sendMessage(const QString &message)
{
    if (message.isEmpty())
        return;

#ifdef QTCHAT_DEBUG
    qWarning()<<"msg: "<<message;
#endif

    QByteArray msg = message.toUtf8();
    write(msg);
}

void Connection::processReadyRead()
{
    QList<QByteArray> names;
    QByteArray server;
    QString channel;
    QString msg;
    UserInfo* newUser;

    while(bytesAvailable() > 0) {
        buffer.clear();
        buffer.append(readLine(256));

#ifdef QTCHAT_DEBUG
        qWarning()<<"got:"<<buffer;
#endif

        QList<QByteArray> list = buffer.split(' ');
        server = list.at(0).mid(1);
        if(list.size() >= 2) {
            switch(list.at(1).toInt()) {
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 20:
                    // send greeting to screen
                    if(list.size() < 3) break;
                    // <host> <code> <nick> :<msg>
                    emit newMessage(host,buffer.mid(list.at(0).length() + list.at(1).length() + list.at(2).length() + 4));
                    break;
                case 42:
                    // your unique ID, "<id> :your unique ID"
                    uniqueID = list.at(3);
                    break;
                case 200:
                    // RPL_TRACELINK, "Link <version & debug level> <destination> <next server>"
                    qWarning()<<"TODO case 200";
                    break;
                case 201:
                    // RPL_TRACECONNECTING, "Try. <class> <server>"
                    qWarning()<<"TODO case 201";
                    break;
                case 202:
                    // RPL_TRACEHANDSHAKE, "H.S. <class> <server>"
                    qWarning()<<"TODO case 202";
                    break;
                case 203:
                    // RPL_TRACEUNKNOWN, "???? <class> [<client IP address in dot form>]"
                    qWarning()<<"TODO case 203";
                    break;
                case 204:
                    // RPL_TRACEOPERATOR, "Oper <class> <nick>"
                    qWarning()<<"TODO case 204";
                    break;
                case 205:
                    // RPL_TRACEUSER, "User <class> <nick>"
                    qWarning()<<"TODO case 205";
                    break;
                case 206:
                    // RPL_TRACESERVER, "Serv <class> <int>S <int>C <server> <nick!user|*!*>@<host|server>"
                    qWarning()<<"TODO case 206";
                    break;
                case 208:
                    // RPL_TRACENEWTYPE, "<newtype> 0 <client name>"
                    qWarning()<<"TODO case 208";
                    break;
                case 211:
                    // RPL_STATSLINKINFO, "<linkname> <sendq> <sent messages> <sent bytes> <received messages> <received bytes> <time open>"
                    qWarning()<<"TODO case 211";
                    break;
                case 212:
                    // RPL_STATSCOMMANDS, "<command> <count>"
                    qWarning()<<"TODO case 212";
                    break;
                case 213:
                    // RPL_STATSCLINE, "C <host> * <name> <port> <class>"
                    qWarning()<<"TODO case 213";
                    break;
                case 214:
                    // RPL_STATSNLINE, "N <host> * <name> <port> <class>"
                    qWarning()<<"TODO case 214";
                    break;
                case 215:
                    // RPL_STATSILINE, "I <host> * <host> <port> <class>"
                    qWarning()<<"TODO case 215";
                    break;
                case 216:
                    // RPL_STATSKLINE, "K <host> * <username> <port> <class>"
                    qWarning()<<"TODO case 216";
                    break;
                case 218:
                    // RPL_STATSYLINE, "Y <class> <ping frequency> <connect frequency> <max sendq>"
                    qWarning()<<"TODO case 218";
                    break;
                case 219:
                    // RPL_ENDOFSTATS, "<stats letter> :End of /STATS report"
                    qWarning()<<"TODO case 219";
                    break;
                case 221:
                    // RPL_UMODEIS, "<user mode string>"
                    qWarning()<<"TODO case 221";
                    break;
                case 241:
                    // RPL_STATSLLINE, "L <hostmask> * <servername> <maxdepth>"
                    qWarning()<<"TODO case 241";
                    break;
                case 242:
                    // RPL_STATSUPTIME, ":Server Up %d days %d:%02d:%02d"
                    qWarning()<<"TODO case 242";
                    break;
                case 243:
                    // RPL_STATSOLINE, "O <hostmask> * <name>"
                    qWarning()<<"TODO case 243";
                    break;
                case 244:
                    // RPL_STATSHLINE, "H <hostmask> * <servername>"
                    qWarning()<<"TODO case 244";
                    break;
                case 251:
                    // RPL_LUSERCLIENT, ":There are <integer> users and <integer> invisible on <integer> servers"
                    emit newMessage("notice",buffer.mid(list.at(0).length() + list.at(1).length() + list.at(2).length() + 4));
                    break;
                case 252:
                    // RPL_LUSEROP, "<integer> :operator(s) online"
                    qWarning()<<"TODO case 252";
                    break;
                case 253:
                    // RPL_LUSERUNKNOWN, "<integer> :unknown connection(s)"
                    qWarning()<<"TODO case 253";
                    break;
                case 254:
                    // RPL_LUSERCHANNELS, "<integer> :channels formed"
                    emit newMessage("notice",buffer.mid(list.at(0).length() + list.at(1).length() + list.at(2).length() + 3));
                    break;
                case 255:
                    // RPL_LUSERME, ":I have <integer> clients and <integer> servers"
                    emit newMessage("notice",buffer.mid(list.at(0).length() + list.at(1).length() + list.at(2).length() + 4));
                    break;
                case 256:
                    // RPL_ADMINME, "<server> :Administrative info"
                    qWarning()<<"TODO case 256";
                    break;
                case 257:
                    // RPL_ADMINLOC1, ":<admin info>"
                    qWarning()<<"TODO case 257";
                    break;
                case 258:
                    // RPL_ADMINLOC2, ":<admin info>"
                    qWarning()<<"TODO case 258";
                    break;
                case 259:
                    // RPL_ADMINEMAIL, ":<admin info>"
                    qWarning()<<"TODO case 259";
                    break;
                case 261:
                    // RPL_TRACELOG, "File <logfile> <debug level>"
                    qWarning()<<"TODO case 261";
                    break;
                case 265:
                    // "Current local users <x>, max <x>"
                    emit newMessage("notice",buffer.mid(list.at(0).length() + list.at(1).length() +
                                list.at(2).length() + list.at(3).length() + list.at(4).length() + 6));
                    break;
                case 266:
                    // "Current global users <x>, max <x>"
                    emit newMessage("notice",buffer.mid(list.at(0).length() + list.at(1).length() +
                                list.at(2).length() + list.at(3).length() + list.at(4).length() + 6));
                    break;
                case 301:
                    // RPL_AWAY, "<nick> :<away message>"
                    userAway = true;
                    break;
                case 302:
                    // RPL_USERHOST, ":[<reply>{<space><reply>}]"
                    qWarning()<<"TODO case 302";
                    break;
                case 305:
                    // RPL_UNAWAY, ":You are no longer marked as being away"
                    for(int i=0;i<users.size();i++) {
                        if(qstrcmp(users.at(i)->user.toLocal8Bit().constData(),username.toLocal8Bit().constData()) == 0) {
                            users.at(i)->away = false;
                        }
                    }
                    break;
                case 306:
                    // RPL_NOWAWAY, ":You have been marked as being away"
                    for(int i=0;i<users.size();i++) {
                        if(qstrcmp(users.at(i)->user.toLocal8Bit().constData(),username.toLocal8Bit().constData()) == 0) {
                            users.at(i)->away = true;
                        }
                    }
                    break;
                case 311:
                    // RPL_WHOISUSER, "<nick> <user> <host> * :<real name>"
                    userAway = false;
                    list = buffer.split(' ');
                    msg  = list.at(3); // nick

                    for(int i=0;i<users.size();i++) {
                        if(qstrcmp(users.at(i)->user.toLocal8Bit().constData(),msg.toLocal8Bit().constData()) == 0) {
                            users.at(i)->ip = QString(list.at(5).constData());
                            list = buffer.split(':');
                            users.at(i)->realName = QString(list.at(2).constData());
                            users.at(i)->realName.remove("\r\n");
                        }
                    }
                    break;
                case 312:
                    // RPL_WHOISSERVER, "<nick> <server> :<server info>"
                    list = buffer.split(' ');
                    msg  = QString(list.at(3).constData()); // nick

                    for(int i=0;i<users.size();i++) {
                        if(qstrcmp(users.at(i)->user.toLocal8Bit().constData(),msg.toLocal8Bit().constData()) == 0) {
                            list = buffer.split(':');
                            users.at(i)->server = QString(list.at(2).constData());
                            users.at(i)->server.remove("\r\n");
                        }
                    }
                    break;
                case 313:
                    // RPL_WHOISSERVER, "<nick> <server> :<server info>"
                    qWarning()<<"TODO case 313";
                    break;
                case 314:
                    // RPL_WHOWASUSER, "<nick> <user> <host> * :<real name>"
                    qWarning()<<"TODO case 314";
                    break;
                case 315:
                    // RPL_ENDOFWHO, "<name> :End of /WHO list"
                    qWarning()<<"TODO case 315";
                    break;
                case 317:
                    // RPL_WHOISIDLE, "<nick> <integer> :seconds idle"
                    break;
                case 318:
                    // RPL_ENDOFWHOIS, "<nick> :End of /WHOIS list"
                    msg  = list.at(3); // nick
                    for(int i=0;i<users.size();i++) {
                        if(qstrcmp(users.at(i)->user.toLocal8Bit().constData(),msg.toLocal8Bit().constData()) == 0) {
                            if(userAway && !users.at(i)->away) {
                                // gone away

                                users.at(i)->away = true;
                                emit awayChanged(true,list.at(3));

                            } else if(!userAway && users.at(i)->away) {
                                // returned, unaway

                                users.at(i)->away = false;
                                emit awayChanged(false,list.at(3));
                            }
                        }
                    }
                    break;
                case 319:
                    // RPL_WHOISCHANNELS, "<nick> :{[@|+]<channel><space>}"
                    msg  = list.at(3); // nick

                    for(int i=0;i<users.size();i++) {
                        if(qstrcmp(users.at(i)->user.toLocal8Bit().constData(),msg.toLocal8Bit().constData()) == 0) {
                            list = buffer.split(':');
                            users.at(i)->channels = QString(list.at(2).constData());
                            users.at(i)->channels.remove("\r\n");
                        }
                    }
                    break;
                case 321:
                    // RPL_LISTSTART, "Channel :Users Name"
                    qWarning()<<"TODO case 321";
                    break;
                case 322:
                    // RPL_LIST, "<channel> <# visible> :<topic>"
                    msg = buffer.mid(buffer.indexOf('#'));
                    emit newMessage(host,msg);
                    break;
                case 323:
                    // RPL_LISTEND, ":End of /LIST"
                    qWarning()<<"TODO case 323";
                    break;
                case 324:
                    // RPL_CHANNELMODEIS, "<channel> <mode> <mode params>"
                    qWarning()<<"TODO case 324";
                    break;
                case 331:
                    // RPL_NOTOPIC, "<channel> :No topic is set"
                    qWarning()<<"TODO case 331";
                    break;
                case 332:
                    // RPL_TOPIC, "<channel> :<topic>"
                    qWarning()<<"TODO case 332";
                    break;
                case 341:
                    // RPL_INVITING, "<channel> <nick>"
                    qWarning()<<"TODO case 341";
                    break;
                case 342:
                    // RPL_SUMMONING, "<user> :Summoning user to IRC"
                    qWarning()<<"TODO case 342";
                    break;
                case 351:
                    // RPL_VERSION, "<version>.<debuglevel> <server> :<comments>"
                    qWarning()<<"TODO case 351";
                    break;
                case 352:
                    // RPL_WHOREPLY, "<channel> <user> <host> <server> <nick> <H|G>[*][@|+] :<hopcount> <real name>"
                    msg = list.at(7);
                    for(int i=0;i<users.size();i++) {
                        if(qstrcmp(users.at(i)->user.toLocal8Bit().constData(),msg.toLocal8Bit().constData()) == 0) {
                            users.at(i)->ip = QString(list.at(5).constData());
                            users.at(i)->realName = QString(buffer.mid(buffer.indexOf(':')).constData());
                            users.at(i)->realName = users.at(i)->realName.mid(users.at(i)->realName.indexOf(' ')+1);
                        }
                    }
                    break;
                case 353:
                    // RPL_NAMREPLY, "<channel> :[[@|+]<nick> [[@|+]<nick> [...]]]"
                    // add any new nickname to list
                    channel = QString(list.at(4).constData());
                    list = buffer.split(':');
                    names = list.at(2).split(' ');
                    //Add any new users
                    for(int i=0;i<names.size();i++) {
                        bool match = false;
                        for(int j=0;j<users.size();j++) {
                            if(qstrcmp(names.at(i).constData(),users.at(j)->user.toLocal8Bit().constData()) == 0)
                                match = true;
                        }
                        if(!match && names.at(i).length() > 2) {
                            emit newParticipant(channel,names.at(i));
                            UserInfo* newUser = new UserInfo;
                            newUser->user = names.at(i);
                            users.append(newUser);
                        }
                    }
                    list = buffer.split(' ');
                    who();
                    timer->start();
                    break;
                case 364:
                    // RPL_LINKS, "<mask> <server> :<hopcount> <server info>"
                    qWarning()<<"TODO case 364";
                    break;
                case 365:
                    // RPL_ENDOFLINKS, "<mask> :End of /LINKS list"
                    qWarning()<<"TODO case 365";
                    break;
                case 366:
                    // RPL_ENDOFNAMES, "<channel> :End of /NAMES list"
                    msg = QString("WHOIS %1\r\n").arg(username);
                    sendMessage(msg);
                    break;
                case 367:
                    // RPL_BANLIST, "<channel> <banid>"
                    qWarning()<<"TODO case 367";
                    break;
                case 368:
                    // RPL_ENDOFBANLIST, "<channel> :End of channel ban list"
                    qWarning()<<"TODO case 368";
                    break;
                case 369:
                    // RPL_ENDOFWHOWAS, "<nick> :End of WHOWAS"
                    qWarning()<<"TODO case 369";
                    break;
                case 371:
                    // RPL_INFO, ":<string>"
                    qWarning()<<"TODO case 371";
                    break;
                case 372:
                    // RPL_MOTD, ":- <text>"
                    emit newMessage("notice",buffer.mid(list.at(0).length() + list.at(1).length() + list.at(2).length() + 6));
                    break;
                case 374:
                    // RPL_ENDOFINFO, ":End of /INFO list"
                    qWarning()<<"TODO case 374";
                    break;
                case 375:
                    // RPL_MOTDSTART, ":- <server> Message of the day - "
                    emit newMessage("notice",buffer.mid(list.at(0).length() + list.at(1).length() + list.at(2).length() + 6));
                    break;
                case 376:
                    // RPL_ENDOFMOTD, ":End of /MOTD command"
                    sendMessage("LIST\r\n");
                    break;
                case 381:
                    // RPL_YOUREOPER, ":You are now an IRC operator"
                    qWarning()<<"TODO case 381";
                    break;
                case 382:
                    // RPL_REHASHING, "<config file> :Rehashing"
                    qWarning()<<"TODO case 382";
                    break;
                case 391:
                    // RPL_TIME, "<server> :<string showing server's local time>"
                    qWarning()<<"TODO case 391";
                    break;
                case 392:
                    // RPL_USERSSTART, ":UserID Terminal Host"
                    qWarning()<<"TODO case 392";
                    break;
                case 393:
                    // RPL_USERS, ":%-8s %-9s %-8s"
                    qWarning()<<"TODO case 393";
                    break;
                case 394:
                    // RPL_ENDOFUSERS, ":End of users"
                    qWarning()<<"TODO case 394";
                    break;
                case 395:
                    // RPL_NOUSERS, ":Nobody logged in"
                    qWarning()<<"TODO case 395";
                    break;
                case 421:
                    // Unknown command
                    emit newMessage("qtchat",tr("Unknown command"));
                    break;
                default:

#ifdef QTCHAT_DEBUG
                    qWarning()<<"arg[0]="<<list.at(0);
                    qWarning()<<"arg[1]="<<list.at(1);
                    if(list.size() > 2)
                        qWarning()<<"arg[2]="<<list.at(2);
#endif

                    QString us = QString(":%1!~%1@").arg(username);
                    if(qstrcmp(list.at(1).constData(),"MODE") == 0) {
                        emit newMessage(host,"");

                    } else if(qstrcmp(list.at(1).constData(),"NOTICE") == 0) {
                        list = buffer.split(':');
                        emit newMessage("notice",list.at(2));

                    } else if(qstrcmp(list.at(0).constData(),"PING") == 0) {
                        QString str = QString("PONG %1\r\n").arg(list.at(1).constData());
                        sendMessage(str);
                    } else if(qstrcmp(list.at(1).constData(),"PONG") == 0) {
                        break;

                    } else if((qstrcmp(list.at(1).constData(),"QUIT") == 0) || (qstrcmp(list.at(1).constData(),"PART") == 0)) {
                        // Elvis has left the building!
                        list = buffer.split('!');
                        msg = list.at(0);
                        msg.remove(":");
                        for(int i=0;i<users.size();i++) {
                            if(qstrcmp(msg.toLocal8Bit().constData(),users.at(i)->user.toLocal8Bit().constData()) == 0) {
                                QStringList ch = users.at(i)->channels.split(' ');
                                for(int j=0;j<ch.size();j++)
                                    emit participantLeft(ch.at(j),msg);
                                users.removeAll(users.at(i));
                                break;
                            }
                        }

                    } else if(qstrcmp(list.at(1).constData(),"JOIN") == 0) {
                        // Elvis has come online!
                        channel = list.at(2).constData();
                        channel.remove(":#");
                        channel.remove("\r\n");
                        list = buffer.split('!');
                        msg = QString(list.at(0).constData());
                        msg.remove(":");
                        bool match = false;
                        for(int i=0;i<users.size();i++) {
                            if(qstrcmp(msg.toLocal8Bit().constData(),users.at(i)->user.toLocal8Bit().constData()) == 0)
                                match = true;
                        }
                        if(!match) {
                            newUser = new UserInfo;
                            newUser->user = msg;
                            newUser->ip = QString(list.at(1).mid(list.at(1).indexOf('@')+1).constData());
                            newUser->ip.remove("\r\n");
                            users.append(newUser);
                            emit newParticipant(channel,msg);
                            who();
                        }

                    } else if(list.at(0).contains("!~")) {
                        //user command
                        QString part = QString(list.at(0).constData());
                        QStringList bits = part.split(' ');
                        part.remove(0,1);
                        QString user = part.mid(0,part.indexOf('!'));
                        QString source = part.mid(part.indexOf('~')+1);
                        source = source.mid(0,source.indexOf(' '));
                        if(bits.size() >= 2) {
                            QString command = bits.at(1);
                            QString arguments = QString(list.at(0).mid(bits.at(0).length()+bits.at(1).length()).constData());
                        }
                        if(qstrcmp(list.at(1).constData(),"PRIVMSG") == 0) {
                            if((list.size() > 3) && list.at(3).contains("DCC")) {
                                // DCC command
                                list = buffer.split(':');
                                part = buffer.mid(list.at(0).length()+list.at(1).length()+2);
                                QString tmp = QString("%1:%2:%3").arg(user).arg(source.mid(source.indexOf('@')+1)).arg(part);
                                emit dccCommand(tmp);

                            } else if(list.size() > 2 && list.at(2).startsWith("#")) {
                                // message to channel
                                QString channel = list.at(2);
                                msg = buffer.mid(list.at(0).length()+list.at(1).length()+list.at(2).length()+4);
                                msg = QString("%1: %2").arg(user).arg(msg);
                                emit newMessage(channel,msg);

                            } else {
                                msg = QString("%1: %2").arg(user).arg(buffer.mid(list.at(0).length()+list.at(1).length()+list.at(2).length()+4).constData());
                                emit newMessage(user,msg);
                            }
                        }

                    } else if(qstrcmp(list.at(0).constData(),"ERROR") == 0) {
                        emit newMessage(host,list.at(2));
                    } else {
                        list = buffer.split(':');
                        if(list.size() > 2)
                            emit newMessage(host,list.at(2));
                        else newMessage(host,buffer);
                    }
                    break;
            }
        }
    }
}

void Connection::login()
{
    if(!loginSent) {
        QString msg = QString(tr("Trying to login as user %1")).arg(username);
        emit newMessage("qtchat",msg);
        QByteArray data;
        data = "NICK " + QByteArray(username.toUtf8()) + "\r\n";
        write(data);
        data.clear();
        // USER <username> <hostname> <servername> <realname>
        data.append("USER ");
        data.append(username.toUtf8() + " ");
        data.append(QHostInfo::localHostName().toUtf8() + " ");
        data.append(peerName() + " ");
        data.append(":qtchat " + username.toUtf8());
        data.append("\r\n");
        write(data);
        loginSent = true;
    }
}

void Connection::stop()
{
    loginSent = false;
    close();
}

void Connection::disconnected()
{
    if(loginSent)
        emit newMessage("qtchat","status: disconnected from server?");
    loginSent = false;
}

void Connection::who()
{
    QString msg;

    msg = QString("WHOIS %1\r\n").arg(users.at(position)->user.toLocal8Bit().constData());
    sendMessage(msg);

    position++;
    if(position >= users.size())
        position = 0;
}
