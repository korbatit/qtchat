#include <QtGui>

#include "phonecall.h"

//#define ULAW 1

PhoneCallDialog::PhoneCallDialog(QWidget *parent,const QString& nick,
                                 const QString& ip, bool caller)
    : QDialog(parent)
{
    QGridLayout* mainLayout = new QGridLayout(this);

    m_nick = nick;
    m_host = ip;
    dest.setAddress(ip);

    statusLabel = new QLabel;
    statusLabel->setText("Please Wait...");
    statusLabel->setMinimumWidth(50);
    statusLabel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);

    QLabel* hostLabel = new QLabel;
    hostLabel->setText(QString("User: %1").arg(m_nick));
    hostLabel->setMinimumWidth(50);
    hostLabel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);

    QLabel* nickLabel = new QLabel;
    nickLabel->setText(QString("Host: %1").arg(m_host));
    nickLabel->setMinimumWidth(50);
    nickLabel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);

    durationLabel = new QLabel;
    durationLabel->setText("Duration: 0s");
    durationLabel->setMinimumWidth(50);
    durationLabel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);

    hangUpButton = new QPushButton(tr("Hang Up"));
    connect(hangUpButton, SIGNAL(clicked()), this, SLOT(finished()));

    mainLayout->addWidget(statusLabel,0,0,1,1);
    mainLayout->addWidget(hostLabel,1,0,1,1);
    mainLayout->addWidget(nickLabel,2,0,1,1);
    mainLayout->addWidget(durationLabel,3,0,1,1);
    mainLayout->addWidget(hangUpButton,4,0,1,1);

    setLayout(mainLayout);
    setWindowTitle(tr("Call Dialog"));

    format.setFrequency(8000);
    format.setChannels(1);
    format.setCodec("audio/pcm");
#ifdef ULAW
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
#else
    format.setSampleSize(8);
    format.setSampleType(QAudioFormat::UnSignedInt);
#endif
    bool fallback = true;
    QSettings* config = new QSettings("Nokia", "qtchat");

    QString deviceIn = config->value("deviceIn").toString();
    foreach (const QAudioDeviceInfo &deviceInfo,
            QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (deviceInfo.deviceName() == deviceIn) {
            audioOutput = new QAudioOutput(deviceInfo, format, this);
            fallback = false;
            break;
        }
    }
    if (fallback)
        audioInput = new QAudioInput(format, this);

    fallback = true;
    QString deviceOut = config->value("deviceOut").toString();
    foreach (const QAudioDeviceInfo &deviceInfo,
            QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        if (deviceInfo.deviceName() == deviceOut) {
            audioInput = new QAudioInput(deviceInfo, format, this);
            fallback = false;
            break;
        }
    }
    if (fallback)
        audioInput = new QAudioInput(format, this);

    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),SLOT(processMore()));

    connect(audioInput,SIGNAL(notify()),SLOT(inputStatus()));
    connect(audioInput,SIGNAL(stateChanged(QAudio::State)),SLOT(inputState(QAudio::State)));
    connect(audioOutput,SIGNAL(notify()),SLOT(outputStatus()));
    connect(audioOutput,SIGNAL(stateChanged(QAudio::State)),SLOT(outputState(QAudio::State)));

    connect(&server, SIGNAL(newConnection()),this,SLOT(acceptConnection()));
    connect(&client, SIGNAL(connected()),this,SLOT(startTransfer()));
    connect(&client, SIGNAL(disconnected()),this,SLOT(finished()));

    this->caller = caller;
    if(caller)
        server.listen(QHostAddress::Any, AUDIO_PORT);
    else
        server.listen(QHostAddress::Any, AUDIO_PORT+1);

    sendReady = false;
    receiveReady = false;
    sync = true;

    bufferIn = (char*)malloc(32768);
    bufferOut = (char*)malloc(32768);
}

PhoneCallDialog::~PhoneCallDialog()
{
    free(bufferIn);
    free(bufferOut);
}

void PhoneCallDialog::outputStatus()
{
    //qWarning()<<"OUT: bytesFree = "<<audioOutput->bytesFree()<<" bytes, clock = "<<
    //    audioOutput->clock()<<"ms, processedUSecs = "<<audioOutput->prcoessedUSecs()/1000<<"ms";
}

void PhoneCallDialog::inputStatus()
{
    durationLabel->setText(QString("Duration: %1s").arg(audioInput->processedUSecs()/1000000));;

    //qWarning()<<"IN: bytesReady = "<<audioInput->bytesReady()<<" bytes, clock = "<<
    //    audioInput->clock()<<"ms, processedUSecs = "<<audioInput->processedUSecs()/1000<<"ms";
}

void PhoneCallDialog::outputState(QAudio::State state)
{
    Q_UNUSED(state)

    //qWarning()<<"OUT: state="<<state;
}

void PhoneCallDialog::inputState(QAudio::State state)
{
    Q_UNUSED(state)

    //qWarning()<<"IN: state="<<state;
}

void PhoneCallDialog::acceptConnection()
{
    //qWarning()<<"acceptConnection()";
    tcpServerConnection = server.nextPendingConnection();
    statusLabel->setText("Active Call");
    receiveReady = true;
    if(!caller) {
        //qWarning()<<"connect back to other end "<<tcpServerConnection->peerAddress()<<QString(":%1").arg(AUDIO_PORT);
        client.connectToHost(tcpServerConnection->peerAddress(),AUDIO_PORT);
    }
}

void PhoneCallDialog::startTransfer()
{
    //qWarning()<<"startTransfer()";
    sendReady = true;
    AudioIn = audioInput->start();
    AudioOut = audioOutput->start();
#ifdef ULAW
    timer->start(40);
#else
    timer->start(20);
#endif
}

void PhoneCallDialog::processMore()
{
    // 20ms interval
    // bytes = 20ms * 8000 / 1000 = 160 bytes

    // AUDIO SYSTEM DATA
    //qWarning()<<"IN : bytesReady = "<<audioInput->bytesReady()<<", periodSize = "<<audioInput->periodSize();
#ifdef ULAW
    bufferInLength = AudioIn->read(bufferIn,audioInput->periodSize()*2); // audio from audio system
#else
    bufferInLength = AudioIn->read(bufferIn,audioInput->periodSize()); // audio from audio system
#endif

    if(bufferInLength != 0) {
#ifdef ULAW
        qint16* sample = (qint16*)bufferIn;
        unsigned char* p = (unsigned char*)bufferIn;
        for(int i=0;i<bufferInLength/2;i++) {
            //pcm to u-law
            unsigned char tmp = toULaw(sample[i]);
            //unsigned char tmp = toALaw(sample[i]);
            p[i] = tmp;
            //qWarning()<<"pcm="<<sample[i]<<", mulaw="<<p[i]<<", check="<<fromULaw(p[i]);
        }
        bufferInLength = bufferInLength/2;
#endif
        client.write(bufferIn,bufferInLength);
    }

    // NETWORK DATA
    //qWarning()<<"OUT: bytesFree = "<<audioOutput->bytesFree()<<", periodSize = "<<audioOutput->periodSize();
    int output = tcpServerConnection->bytesAvailable();
    if(output > 2000) {
        // 250ms out???
        //qWarning()<<"250ms buffered in network socket!!!, drop 100ms to catch up.";
        bufferOutLength = tcpServerConnection->read(bufferOut, audioOutput->periodSize());  // audio from network.DROPPED
        output = output - bufferOutLength;
    }
    if(output < audioOutput->periodSize()) {
        //qWarning()<<"not enough space in audio output for a period, next time";
        return;
    }
    while(output > audioOutput->periodSize()) {
        bufferOutLength = tcpServerConnection->read(bufferOut,audioOutput->periodSize());  // audio from network
        if(bufferOutLength != 0) {
#ifdef ULAW
            unsigned char* sample = (unsigned char*)bufferOut;
            qint16* outputSamples = (qint16*)malloc(bufferOutLength*8);
            for(int i=0;i<bufferOutLength;i++) {
                // u-law to pcm
                outputSamples[i] = fromULaw(sample[i]);
                //outputSamples[i] = fromALaw(sample[i]);
                //qWarning()<<"mulaw="<<sample[i]<<", pcm="<<outputSamples[i]<<", check="<<toULaw(outputSamples[i]);
            }
            bufferOutLength = bufferOutLength*2;
            memcpy(bufferOut,outputSamples,bufferOutLength);
            free(outputSamples);
#endif
            int l = AudioOut->write(bufferOut,bufferOutLength);
            output = output - l;
        } else
            output = 0;
    }
}

void PhoneCallDialog::connectToHost()
{
    if(caller) {
        //qWarning()<<"outBound() connect to:"<<dest.toString()<<QString(":%1").arg(AUDIO_PORT+1);
        client.connectToHost(dest,AUDIO_PORT+1);
    } else {
        //qWarning()<<"outBound() connect to:"<<dest.toString()<<QString(":%1").arg(AUDIO_PORT);
        client.connectToHost(dest,AUDIO_PORT);
    }
}

void PhoneCallDialog::hangup()
{
    finished();
}

void PhoneCallDialog::finished()
{
    //qWarning()<<"finished()";
    audioInput->stop();
    audioOutput->stop();
    client.disconnectFromHost();
    tcpServerConnection->disconnectFromHost();
    server.close();
    server.listen(QHostAddress::Any, AUDIO_PORT);
    sendReady = false;
    receiveReady = false;
    sync = true;
    timer->stop();
    close();
}

#define MAXPCM         32635
#define OFFSETBIAS     0x84

unsigned char PhoneCallDialog::toULaw(qint16 sample)
{
    qint16 pcm = sample;

    int sign = (pcm & 0x8000) >> 8;
    if (sign != 0) pcm = -pcm;
    if (pcm > MAXPCM) pcm = MAXPCM;
    pcm += OFFSETBIAS;

    /* Finding the "exponent"
     * Bits:
     * 1 2 3 4 5 6 7 8 9 A B C D E F G
     * S 7 6 5 4 3 2 1 0 . . . . . . .
     * We want to find where the first 1 after the sign bit is.
     * We take the corresponding value from
     * the second row as the exponent value.
     * (i.e. if first 1 at position 7 -> exponent = 2) */
    int exponent = 7;
    //Move to the right and decrement exponent until we hit the 1

    for (int expMask = 0x4000; (pcm & expMask) == 0;
            exponent--, expMask >>= 1) { }

    /* The last part - the "mantissa"
     * We need to take the four bits after the 1 we just found.
     * To get it, we shift 0x0f :
     * 1 2 3 4 5 6 7 8 9 A B C D E F G
     * S 0 0 0 0 0 1 . . . . . . . . . (meaning exponent is 2)
     * . . . . . . . . . . . . 1 1 1 1
     * We shift it 5 times for an exponent of two, meaning
     * we will shift our four bits (exponent + 3) bits.
     * For convenience, we will actually just shift
     * the number, then and with 0x0f. */
    int mantissa = (pcm >> (exponent + 3)) & 0x0f;

    //The mu-law byte bit arrangement

    //is SEEEMMMM (Sign, Exponent, and Mantissa.)

    unsigned char mulaw = (unsigned char)(sign | exponent << 4 | mantissa);
    //Last is to flip the bits
    return (unsigned char)~mulaw;
}

qint16 PhoneCallDialog::fromULaw(unsigned char sample)
{
    unsigned char mulaw = sample;

    //Flip all the bits

    mulaw = (unsigned char)~mulaw;

    //Pull out the value of the sign bit

    int sign = mulaw & 0x80;
    //Pull out and shift over the value of the exponent

    int exponent = (mulaw & 0x70) >> 4;
    //Pull out the four bits of data

    int data = mulaw & 0x0f;

    //Add on the implicit fifth bit (we know
    //the four data bits followed a one bit)

    data |= 0x10;
    /* Add a 1 to the end of the data by
     * shifting over and adding one. Why?
     * Mu-law is not a one-to-one function.
     * There is a range of values that all
     * map to the same mu-law byte.
     * Adding a one to the end essentially adds a
     * "half byte", which means that
     * the decoding will return the value in the
     * middle of that range. Otherwise, the mu-law
     * decoding would always be
     * less than the original data. */
    data <<= 1;
    data += 1;
    /* Shift the five bits to where they need
     * to be: left (exponent + 2) places
     * Why (exponent + 2) ?
     * 1 2 3 4 5 6 7 8 9 A B C D E F G
     * . 7 6 5 4 3 2 1 0 . . . . . . . <-- starting bit (based on exponent)
     * . . . . . . . . . . 1 x x x x 1 <-- our data
     * We need to move the one under the value of the exponent,
     * which means it must move (exponent + 2) times
     */
    data <<= exponent + 2;
    //Remember, we added to the original,

    //so we need to subtract from the final

    data -= OFFSETBIAS;
    //If the sign bit is 0, the number

    //is positive. Otherwise, negative.
    return (short)(sign == 0 ? data : -data);
}

unsigned char PhoneCallDialog::toALaw(qint16 sample)
{
    qint16 pcm = sample;

    int sign = (pcm & 0x8000) >> 8;
    if (sign != 0) pcm = -pcm;
    if (pcm > MAXPCM) pcm = MAXPCM;

    /* Finding the "exponent"
     * Bits:
     * 1 2 3 4 5 6 7 8 9 A B C D E F G
     * S 7 6 5 4 3 2 1 0 0 0 0 0 0 0 0
     * We want to find where the first 1 after the sign bit is.
     * We take the corresponding value
     * from the second row as the exponent value.
     * (i.e. if first 1 at position 7 -> exponent = 2)
     * The exponent is 0 if the 1 is not found in bits 2 through 8.
     * This means the exponent is 0 even if the "first 1" doesn't exist.
     */

    int exponent = 7;
    for (int expMask=0x4000; (pcm & expMask)==0 && exponent > 0; exponent--,expMask >>= 1) {}

    /* The last part - the "mantissa"
     * We need to take the four bits after the 1 we just found.
     * To get it, we shift 0x0f :
     * 1 2 3 4 5 6 7 8 9 A B C D E F G
     * S 0 0 0 0 0 1 . . . . . . . . . (say that exponent is 2)
     * . . . . . . . . . . . . 1 1 1 1
     * We shift it 5 times for an exponent of two, meaning
     * we will shift our four bits (exponent + 3) bits.
     * For convenience, we will actually just
     * shift the number, then AND with 0x0f.
     *
     * NOTE: If the exponent is 0:
     * 1 2 3 4 5 6 7 8 9 A B C D E F G
     * S 0 0 0 0 0 0 0 Z Y X W V U T S (we know nothing about bit 9)
     * . . . . . . . . . . . . 1 1 1 1
     * We want to get ZYXW, which means a shift of 4 instead of 3
     */

    int mantissa = (pcm >> ((exponent == 0) ? 4 : (exponent + 3))) & 0x0f;
    //The a-law byte bit arrangement is SEEEMMMM
    //(Sign, Exponent, and Mantissa.)
    unsigned char alaw = (unsigned char)(sign | exponent << 4 | mantissa);
    //Last is to flip every other bit, and the sign bit (0xD5 = 1101 0101)
    return (unsigned char)(alaw^0xD5);
}

qint16 PhoneCallDialog::fromALaw(unsigned char sample)
{
    unsigned char alaw = sample;

    //Invert every other bit, and the sign bit (0xD5 = 1101 0101)
    alaw ^= 0xD5;

    //Pull out the value of the sign bit

    int sign = alaw & 0x80;
    //Pull out and shift over the value of the exponent

    int exponent = (alaw & 0x70) >> 4;
    //Pull out the four bits of data

    int data = alaw & 0x0f;

    //Shift the data four bits to the left
    data <<= 4;
    //Add 8 to put the result in the middle
    //of the range (like adding a half)
    data += 8;
    //If the exponent is not 0, then we know the four bits followed a 1,
    //and can thus add this implicit 1 with 0x100.
    if (exponent != 0) data += 0x100;
    /* Shift the bits to where they need to be: left (exponent - 1) places
     * Why (exponent - 1) ?
     * 1 2 3 4 5 6 7 8 9 A B C D E F G
     * . 7 6 5 4 3 2 1 . . . . . . . . <-- starting bit (based on exponent)
     * . . . . . . . Z x x x x 1 0 0 0 <-- our data (Z is 0 only when
     * exponent is 0)
     * We need to move the one under the value of the exponent,
     * which means it must move (exponent - 1) times
     * It also means shifting is unnecessary if exponent is 0 or 1.
     */
    if (exponent > 1)
        data <<= (exponent - 1);

    return (qint16)(sign == 0 ? data : -data);
}


