#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ChatDialog;
class SettingsDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

private slots:
    void about();
    void settings();
    void login();
    void logout();
    void markAsAway();
    void clear();
    void closeWindow();
    void channelList();

protected:
    void closeEvent(QCloseEvent *event);

private:
    QMenu   *serverMenu;
    QMenu   *preferencesMenu;
    QMenu   *windowMenu;
    QMenu   *helpMenu;

    QAction *loginAct;
    QAction *logoutAct;
    QAction *awayAct;
    QAction *clearAct;
    QAction *settingsAct;
    QAction *aboutAct;
    QAction *closeWindowAct;
    QAction *listAct;

    ChatDialog* chatDialog;
    SettingsDialog* settingsDialog;
};

#endif
