#include <QtGui>

#include "settings.h"
#include "chatdialog.h"
#include "mainwindow.h"

MainWindow::MainWindow()
{
    settingsDialog = new SettingsDialog;
    chatDialog = new ChatDialog;
    setCentralWidget(chatDialog);

    loginAct = new QAction(tr("&Connect"), this);
    connect(loginAct, SIGNAL(triggered()), this, SLOT(login()));
    logoutAct = new QAction(tr("&Disconnect"), this);
    connect(logoutAct, SIGNAL(triggered()), this, SLOT(logout()));
    awayAct = new QAction(tr("&Mark As Away"), this);
    connect(awayAct, SIGNAL(triggered()), this, SLOT(markAsAway()));
    awayAct->setCheckable(true);
    listAct = new QAction(tr("&Join Channel"), this);
    connect(listAct, SIGNAL(triggered()), this, SLOT(channelList()));
    closeWindowAct = new QAction(tr("&Close Current Window"), this);
    connect(closeWindowAct, SIGNAL(triggered()), this, SLOT(closeWindow()));

    serverMenu = new QMenu(tr("&Server"), this);
    serverMenu->addAction(loginAct);
    serverMenu->addAction(logoutAct);
    serverMenu->addAction(awayAct);
    serverMenu->addAction(listAct);
    serverMenu->addAction(closeWindowAct);

    menuBar()->addMenu(serverMenu);


    settingsAct = new QAction(tr("&Settings"), this);
    connect(settingsAct, SIGNAL(triggered()), this, SLOT(settings()));

    preferencesMenu = new QMenu(tr("&Preferences"), this);
    preferencesMenu->addAction(settingsAct);

    menuBar()->addMenu(preferencesMenu);


    clearAct = new QAction(tr("&Clear Text"), this);
    connect(clearAct, SIGNAL(triggered()), this, SLOT(clear()));

    windowMenu = new QMenu(tr("&Window"), this);
    windowMenu->addAction(clearAct);

    menuBar()->addMenu(windowMenu);


    aboutAct = new QAction(tr("&About"), this);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    helpMenu = new QMenu(tr("&Help"), this);
    helpMenu->addAction(aboutAct);

    menuBar()->addMenu(helpMenu);

    setWindowTitle(tr("qtchat"));
    resize(500, 500);
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About qtchat"),
            tr("<p>qtchat is a special irc chat client that allows you to make "
               "audio calls to other qtchat users on your channel.</p>"
               "<p>COMMANDS:</p>"
               "/r {message} (Reply to previous message<br>"
               "/w {nickname} {message} (send message to user)<br>"
               "/clear (Clears the entire scrollback buffer of the current window)<br>"
               "/clearall (Clears the entire scrollback buffer of all windows)<br>"
               "/close (Closes current window)<br>"
               "/exit (Close All windows and exit application)<br>"
               "/quit (Leave current channel, closing all associated windows)<br>"
               "/list (List all available channels)<br>"
               "/join {#channel} (Join a channel)<br>"
               "/raw {raw cammand} (Send a raw irc message)<br>"
               ));
}

void MainWindow::settings()
{
    settingsDialog->show();
}

void MainWindow::login()
{
    chatDialog->setHost(settingsDialog->hostname());
    chatDialog->setNick(settingsDialog->nickname());
    chatDialog->start();
}

void MainWindow::logout()
{
    chatDialog->disconnect();
}

void MainWindow::markAsAway()
{
    if(awayAct->isChecked())
        chatDialog->markAsAway(true,settingsDialog->nickname());
    else
        chatDialog->markAsAway(false,settingsDialog->nickname());
}

void MainWindow::clear()
{
    chatDialog->clearTab();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
}

void MainWindow::closeWindow()
{
    chatDialog->closeCurrentWindow();
}

void MainWindow::channelList()
{
    chatDialog->channelSelection();
}
