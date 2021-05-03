#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QString>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <mutex>
#include <unistd.h>
#include <memory.h>
#include <future>
#include <QDebug>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJSValueList>
#include <QQuickItem>
#include <chrono>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>

class BackEnd : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(int motorPosition READ motorPosition NOTIFY motorPositionChanged)
    Q_PROPERTY(int motorVelocity READ motorVelocity NOTIFY motorVelocityChanged)
    Q_PROPERTY(QVariantList status READ status NOTIFY statusChanged)
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)

public:
    explicit BackEnd(QObject *parent = nullptr);

    enum MovementMode
    {
        ProfilePostionMode = 0x01,
        HomingMode = 0x06,
        ProfileVelocityMode = 0x03,
        PositionMode = 0xFF,
        VelocityMode = 0xFE,
        CurrentMode = 0xFD
    };
    Q_ENUMS(MovementMode)

    bool connected();
    int motorPosition();
    int motorVelocity();
    QVariantList status();
    QVariantList messages();
    Q_INVOKABLE void initCANConnection();
    Q_INVOKABLE void initEPOSController(MovementMode mode);
    Q_INVOKABLE void sendPosition(int);
    Q_INVOKABLE void sendProfilePosition(int);
    Q_INVOKABLE void sendProfileVelocity(int);
    Q_INVOKABLE void resetComm();
    Q_INVOKABLE void resetApp();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void setPreOperational();
    Q_INVOKABLE void clearErrors();
    Q_INVOKABLE void setMovementMode(MovementMode mode);
//    Q_INVOKABLE void registerValuesForUpdate(const QString& select0,
//                                             const QString& select1,
//                                             const QString& select2,
//                                             const QString& select3);
    Q_INVOKABLE void registerValuesForUpdate(const QJSValue& select0,
                                             const QJSValue& select1,
                                             const QJSValue& select2,
                                             const QJSValue& select3);
//    Q_INVOKABLE void sendMessages(const QVariant& newValue0, const QJSValue& select0,
//                                  const QVariant& newValue1, const QJSValue& select1,
//                                  const QVariant& newValue2, const QJSValue& select2,
//                                  const QVariant& newValue3, const QJSValue& select3);
    Q_INVOKABLE void sendMessages(const QVariantList& newValues, const QJSValue& selects);

    //Q_INVOKABLE void registerValuesForUpdate(QObject* select0);
    //Q_INVOKABLE void registerValuesForUpdate(QJsonObject select0);
    //Q_INVOKABLE void registerValuesForUpdate(QVariantList select0);
    //Q_INVOKABLE void registerValuesForUpdate(QQuickItem* select0);

    class MessageType {
    public:
        MessageType():waitingAnswer(false), name(""), dataLength(0), indexSubIndex(0), unit("")
        {
            value = QVariant(0);
        };
        MessageType(QString name, int dataLength, int indexSubIndex, QString unit):
            waitingAnswer(true), name(name), dataLength(dataLength), indexSubIndex(indexSubIndex), unit(unit)
        {
            value = QVariant(0);
        };
//        MessageType(const QJsonObject& json):
//            waitingAnswer(true)
//        {
//            name        = json["name"].toString();
//            dataLength  = json["dataLength"].toInt();
//            indexSubIndex= json["indexSubIndex"].toInt();
//            unit        = json["unit"].toString();
//            if(json.has)
//            value = QVariant(0);
//        }
        MessageType(const QJSValue& jsValue):
            waitingAnswer(true)
        {
            name        = jsValue.property("name").toString();
            dataLength  = jsValue.property("dataLength").toInt();
            indexSubIndex= jsValue.property("indexSubIndex").toInt();
            unit        = jsValue.property("unit").toString();
            if (jsValue.hasProperty("value"))
            {
                value = jsValue.property("value").toInt();
            }
            else
            {
                value = QVariant(0);
            }
        }

        bool waitingAnswer = false;
        bool write = false;
        QString name;
        int dataLength;
        int indexSubIndex;
        QString unit;
        QVariant value;
        int getSubIndex()
        {
            return indexSubIndex & 0xFF;
        }
        int getIndex()
        {
            return (indexSubIndex >> 8) & 0xFFFF;
        }
        void setWrite()
        {
            write = true;
        }
        void unsetWrite()
        {
            write = false;
        }
    };

signals:
    void connectedChanged();
    void motorPositionChanged();
    void motorVelocityChanged();
    void statusChanged();
    void messagesChanged();

private:

    void initObjectDictionary();
    bool m_isConnected;
    int m_socketCan;

    std::array<MessageType, 4> m_messageBox;


    struct can_frame m_canFrameRead;
    std::atomic_bool m_newCANMessageArrived;

    struct can_frame m_canFrameWrite;
    struct can_frame m_canFrameReadPDOActualPosition;

    struct can_frame m_NMTGoOperational;
    struct can_frame m_NMTStop;
    struct can_frame m_NMTGoPreOperational;

    struct can_frame m_NMTResetApplication;
    struct can_frame m_NMTResetCommunication;

    struct can_frame createSDO(const uint8_t address, const bool &doWrite, int indexSubindex, int dataType, const long &value);

    int32_t m_motorPosition;
    int32_t m_motorVelocity;

    bool readCAN();
    void sendCAN(struct can_frame&);

    std::atomic<bool> m_keepReadingCANMessages;
    std::thread m_threadReadCANMessages;
    void readNewMessage();

    std::atomic<bool> m_keepThreadDoInterfaceUpdate;
    std::atomic<bool> m_doInterfaceUpdate;
    std::thread m_threadDoInterfaceUpdate;
    void doInterfaceUpdate(int sleep_ms);

    struct timeval m_tvRead;
    fd_set m_readfds;
    int m_noConnectionCounter;

    bool m_status[16];
    bool m_status_new[16];

    void clearFault();

    void shutDown();

    void switchOn();

    void prepareStatusRead();

    QMap<int, int> m_objectDictionaryByIndexSubIndex;
    QMap<QString, int*> m_objectDictionaryByName;

};

#endif // BACKEND_H
