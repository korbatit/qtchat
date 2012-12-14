#include <QtGui>

#include "settings.h"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    config = new QSettings("Nokia", "qtchat");

    nick = config->value("nickname").toString();
    host = config->value("hostname").toString();
    deviceIn = config->value("deviceIn").toString();
    deviceOut = config->value("deviceOut").toString();

    QGridLayout* mainLayout = new QGridLayout(this);

    QLabel* hostLabel = new QLabel;
    hostLabel->setText(tr("hostname"));
    hostLabel->setMinimumWidth(50);
    hostLabel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);

    QLabel* nickLabel = new QLabel;
    nickLabel->setText(tr("nickname"));
    nickLabel->setMinimumWidth(50);
    nickLabel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);

    hostLine = new QLineEdit;
    hostLine->setFocusPolicy(Qt::StrongFocus);
    hostLine->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    hostLine->setText(host);

    nickLine = new QLineEdit;
    nickLine->setFocusPolicy(Qt::StrongFocus);
    nickLine->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    nickLine->setText(nick);

    QLabel* inLabel = new QLabel;
    inLabel->setText(tr("Input device"));
    inLabel->setMinimumWidth(50);
    inLabel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
    deviceBoxIn = new QComboBox(this);
    foreach (const QAudioDeviceInfo &deviceInfo,
            QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
        deviceBoxIn->addItem(deviceInfo.deviceName(),qVariantFromValue(deviceInfo));
    connect(deviceBoxIn,SIGNAL(activated(int)),SLOT(deviceInChanged(int)));
    if (deviceIn.length() == 0 && deviceBoxIn->count() > 0) {
        deviceIn = deviceBoxIn->itemData(0).value<QAudioDeviceInfo>().deviceName();
    } else {
        for(int i = 0; i < deviceBoxIn->count(); i++) {
            if (deviceBoxIn->itemData(i).value<QAudioDeviceInfo>().deviceName()
                    == deviceIn) {
                deviceBoxIn->setCurrentIndex(i);
                break;
            }
        }
    }

    QLabel* outLabel = new QLabel;
    outLabel->setText(tr("Output device"));
    outLabel->setMinimumWidth(50);
    outLabel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
    deviceBoxOut = new QComboBox(this);
    foreach (const QAudioDeviceInfo &deviceInfo,
            QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
        deviceBoxOut->addItem(deviceInfo.deviceName(),qVariantFromValue(deviceInfo));
    connect(deviceBoxOut,SIGNAL(activated(int)),SLOT(deviceOutChanged(int)));
    if (deviceOut.length() == 0 && deviceBoxOut->count() > 0)
        deviceOut = deviceBoxOut->itemData(0).value<QAudioDeviceInfo>().deviceName();
    else {
        for(int i = 0; i < deviceBoxOut->count(); i++) {
            if (deviceBoxOut->itemData(i).value<QAudioDeviceInfo>().deviceName()
                    == deviceOut) {
                deviceBoxOut->setCurrentIndex(i);
                break;
            }
        }
    }

    closeButton = new QPushButton(tr("Save and Close"));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(saveAndClose()));

    mainLayout->addWidget(hostLabel,0,0,1,1);
    mainLayout->addWidget(hostLine,0,1,1,1);
    mainLayout->addWidget(nickLabel,1,0,1,1);
    mainLayout->addWidget(nickLine,1,1,1,1);
    mainLayout->addWidget(inLabel,2,0,1,1);
    mainLayout->addWidget(deviceBoxIn,2,1,1,1);
    mainLayout->addWidget(outLabel,3,0,1,1);
    mainLayout->addWidget(deviceBoxOut,3,1,1,1);

    mainLayout->addWidget(closeButton,4,0,1,2);

    setLayout(mainLayout);
    setWindowTitle(tr("Chat Settings"));
}

SettingsDialog::~SettingsDialog()
{
}

QString SettingsDialog::nickname()
{
    return nick;
}

QString SettingsDialog::hostname()
{
    return host;
}

QString SettingsDialog::audioDevice(QAudio::Mode mode)
{
    if (mode == QAudio::AudioInput)
        return deviceIn;
    return deviceOut;
}

void SettingsDialog::saveAndClose()
{
    host = hostLine->text();
    nick = nickLine->text();

    config->setValue("hostname",host);
    config->setValue("nickname",nick);
    config->setValue("deviceIn",deviceIn);
    config->setValue("deviceOut",deviceOut);

    close();
}

void SettingsDialog::deviceInChanged(int index)
{
    deviceIn = deviceBoxIn->itemData(index).value<QAudioDeviceInfo>().deviceName();
}

void SettingsDialog::deviceOutChanged(int index)
{
    deviceOut = deviceBoxOut->itemData(index).value<QAudioDeviceInfo>().deviceName();
}

