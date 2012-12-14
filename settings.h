#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QDialog>
#include <QAudioDeviceInfo>

class QPushButton;
class QLineEdit;
class QComboBox;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    QString hostname();
    QString nickname();
    QString audioDevice(QAudio::Mode mode);

private slots:
    void deviceInChanged(int index);
    void deviceOutChanged(int index);
    void saveAndClose();

private:
    QSettings* config;
    QPushButton* closeButton;
    QLineEdit* hostLine;
    QLineEdit* nickLine;
    QString host;
    QString nick;

    QComboBox* deviceBoxIn;
    QComboBox* deviceBoxOut;
    QString    deviceIn;
    QString    deviceOut;
};

#endif
