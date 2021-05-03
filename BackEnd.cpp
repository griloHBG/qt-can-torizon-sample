#include "BackEnd.h"

#define SLEEP_ms(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms))

#define DO_READ 1
//#undef DO_READ

BackEnd::BackEnd(QObject *parent) : QObject(parent), m_isConnected(false)
{
    initObjectDictionary();

    m_canFrameWrite.can_dlc = 8;

    m_NMTGoOperational.can_dlc = 2;
    m_NMTGoOperational.can_id = 0;
    m_NMTGoOperational.data[0] = 0x01;
    m_NMTGoOperational.data[1] = 0;

    m_NMTStop.can_dlc = 2;
    m_NMTStop.can_id = 0;
    m_NMTStop.data[0] = 0x02;
    m_NMTStop.data[1] = 0;

    m_NMTGoPreOperational.can_dlc = 2;
    m_NMTGoPreOperational.can_id = 0;
    m_NMTGoPreOperational.data[0] = 0x80;
    m_NMTGoPreOperational.data[1] = 0;

    m_NMTResetApplication.can_dlc = 2;
    m_NMTResetApplication.can_id = 0;
    m_NMTResetApplication.data[0] = 0x81;
    m_NMTResetApplication.data[1] = 0;

    m_NMTResetCommunication.can_dlc = 2;
    m_NMTResetCommunication.can_id = 0;
    m_NMTResetCommunication.data[0] = 0x82;
    m_NMTResetCommunication.data[1] = 0;

    m_keepReadingCANMessages = false;

    m_noConnectionCounter = 0;
    m_tvRead.tv_sec = m_tvRead.tv_usec = 0;

    m_newCANMessageArrived = false;

    for(int i = 0; i < 16; i++)
    {
        m_status[i] =  false;
        m_status_new[i] = false;
    }
}

void BackEnd::initObjectDictionary()
{
    m_objectDictionaryByIndexSubIndex.insert(0x606500, 2000); // MaxFollowingError, 32bit, qc
    m_objectDictionaryByIndexSubIndex.insert(0x607D01, -2147483648); // MinPositionLimit, 32bit, qc
    m_objectDictionaryByIndexSubIndex.insert(0x607D02, 2147483647); // MaxPositionLimit, 32bit, qc
    m_objectDictionaryByIndexSubIndex.insert(0x607F00, 25000); // MaxProfileVelocity, 32bit, rpm
    m_objectDictionaryByIndexSubIndex.insert(0x608100, 1000); // ProfileVelocity, 32bit, rpm
    m_objectDictionaryByIndexSubIndex.insert(0x608300, 10000); // Profile Acceleration, 32bit, rpm/s
    m_objectDictionaryByIndexSubIndex.insert(0x608400, 10000); // Profile Deceleration, 32 bit, rpm/s

    m_objectDictionaryByName.insert("MaxFollowingError"     , &(m_objectDictionaryByIndexSubIndex[0x606500])); // MaxFollowingError, 32bit, qc
    m_objectDictionaryByName.insert("MinPositionLimit"      , &(m_objectDictionaryByIndexSubIndex[0x607D01])); // MinPositionLimit, 32bit, qc
    m_objectDictionaryByName.insert("MaxPositionLimit"      , &(m_objectDictionaryByIndexSubIndex[0x607D02])); // MaxPositionLimit, 32bit, qc
    m_objectDictionaryByName.insert("MaxProfileVelocity"    , &(m_objectDictionaryByIndexSubIndex[0x607F00])); // MaxProfileVelocity, 32bit, rpm
    m_objectDictionaryByName.insert("ProfileVelocity"       , &(m_objectDictionaryByIndexSubIndex[0x608100])); // ProfileVelocity, 32bit, rpm
    m_objectDictionaryByName.insert("ProfileAcceleration"   , &(m_objectDictionaryByIndexSubIndex[0x608300])); // Profile Acceleration, 32bit, rpm/2
    m_objectDictionaryByName.insert("ProfileDeceleration"   , &(m_objectDictionaryByIndexSubIndex[0x608400])); // Profile Deceleration, 32 bit, rpm/s
}

void BackEnd::initCANConnection() {

    if(!m_isConnected)
    {
        m_socketCan = socket(PF_CAN, SOCK_RAW, CAN_RAW);

        struct sockaddr_can addr;

        struct ifreq ifr;

        strcpy(ifr.ifr_name, "can0");

        ioctl(m_socketCan, SIOCGIFINDEX, &ifr);

        addr.can_family = AF_CAN;

        addr.can_ifindex = ifr.ifr_ifindex;

        bind(m_socketCan, (struct sockaddr *) &addr, sizeof(addr));

        m_isConnected = true;

        sendCAN(m_NMTResetApplication);
        SLEEP_ms(10);
        sendCAN(m_NMTResetCommunication);
        SLEEP_ms(10);
        sendCAN(m_NMTGoPreOperational);
        SLEEP_ms(10);

        qDebug() << "init connection\n";
        emit connectedChanged();

        m_keepReadingCANMessages = true;
        m_threadReadCANMessages = std::thread(&BackEnd::readNewMessage, this);

        m_keepThreadDoInterfaceUpdate = true;
        m_threadDoInterfaceUpdate = std::thread(&BackEnd::doInterfaceUpdate, this, 200);

        can_frame aux;
        // transmit PDO1: setting inhibit time to 10 (x 100us) = 1ms
        aux = createSDO(1, true, 0x180003, 16, 10);
        sendCAN(aux);
        SLEEP_ms(100);
        // transmit PDO2: setting inhibit time to 10 (x 100us) = 1ms
        aux = createSDO(1, true, 0x180103, 16, 10);
        sendCAN(aux);
        SLEEP_ms(100);
        // transmit PDO3: setting inhibit time to 10 (x 100us) = 1ms
        aux = createSDO(1, true, 0x180203, 16, 10);
        sendCAN(aux);
        SLEEP_ms(100);
        // transmit PDO4: setting inhibit time to 10 (x 100us) = 1ms
        aux = createSDO(1, true, 0x180303, 16, 10);
        sendCAN(aux);
        SLEEP_ms(100);
    }
}

void BackEnd::initEPOSController(MovementMode mode) {
    qDebug() << "initEPOSController";

    //Set mode of operation (0x6060-00) to Position mode (0xFF)
    //m_canFrameWrite = createSDO(1, true, 0x606000, 8, 0xFF);
    //m_canFrameWrite = createSDO(1, true, 0x606000, 8, 0x01);
    //sendCAN(m_canFrameWrite);

    //profile acceleration
    //m_canFrameWrite = createSDO(1, true, 0x608300, 32, 1000);
    //sendCAN(m_canFrameWrite);

    //profile deacceleration
    //m_canFrameWrite = createSDO(1, true, 0x608400, 32, 1000);
    //sendCAN(m_canFrameWrite);

    //ControlWord (0x6040-00) to shutdown (0x0006)
    m_canFrameWrite = createSDO(1, true, 0x604000, 16, 0x0006);
    sendCAN(m_canFrameWrite);
    SLEEP_ms(10);

    //ControlWord (0x6040-00) to SwitchOn (0x000F)
    m_canFrameWrite = createSDO(1, true, 0x604000, 16, 0x000F);
    sendCAN(m_canFrameWrite);
    SLEEP_ms(10);

    //Set Position Mode Setting Value (0x2062-00) to 0
    //m_canFrameWrite = createSDO(1, true, 0x206200, 32, 0);
    //m_canFrameWrite = createSDO(1, true, 0x604000, 32, 0x007F);
    //sendCAN(m_canFrameWrite);

    sendCAN(m_NMTGoPreOperational);
    SLEEP_ms(10);
    setMovementMode(mode);
    SLEEP_ms(10);
    sendCAN(m_NMTGoOperational);

    qDebug() << "initEPOSController end";
}

void BackEnd::readNewMessage()
{
    qDebug() << "readNewMessage THREAD STARTED";

    bool needStatusUpdate = false;
    //bool needStatusUpdateOnGUI = false;
    std::string status_string("0000000000000000");

    int auxStatus = 0;

    int i = 0;

    can_frame auxCANFrame;
    auxCANFrame.can_dlc = 8;
    auxCANFrame = createSDO(1, false, 0x604100, 16, 0);

    std::stringstream ss;

    while(m_keepReadingCANMessages)
    {
        if(readCAN())
        {
            if(m_canFrameRead.can_id == 0x81) //emergency message
            {
                ss << std::setfill('0') << std::setw(3) << std::uppercase << std::hex << (int)m_canFrameRead.can_id << '#';
                for(int i = 0; i < 8; i++)
                {
                    ss << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (int)m_canFrameRead.data[i];
                    if (i != 7)
                    {
                        ss << ".";
                    }
                }

                qDebug() << QString::fromStdString(ss.str());

                ss.str("");
                ss.clear();
            }            
            else if(m_canFrameRead.can_id == 0x181)
            {
                m_motorPosition =  (m_canFrameRead.data[0] <<  0) +
                                    (m_canFrameRead.data[1] <<  8) +
                                    (m_canFrameRead.data[2] << 16) +
                                    (m_canFrameRead.data[3] << 24);
                emit motorPositionChanged();

                auxStatus = (m_canFrameRead.data[6] << 0) +
                            (m_canFrameRead.data[7] << 8);

                needStatusUpdate = true;
            }
            else if(m_canFrameRead.can_id == 0x381)
            {
                m_motorPosition =  (m_canFrameRead.data[2] <<  0) +
                                   (m_canFrameRead.data[3] <<  8) +
                                   (m_canFrameRead.data[4] << 16) +
                                   (m_canFrameRead.data[5] << 24);
                emit motorPositionChanged();

                auxStatus = (m_canFrameRead.data[0] << 0) +
                            (m_canFrameRead.data[1] << 8);

                needStatusUpdate = true;
            }
            else if(m_canFrameRead.can_id == 0x481)
            {
                m_motorVelocity =  (m_canFrameRead.data[2] <<  0) +
                                   (m_canFrameRead.data[3] <<  8) +
                                   (m_canFrameRead.data[4] << 16) +
                                   (m_canFrameRead.data[5] << 24);
                emit motorVelocityChanged();

                auxStatus = (m_canFrameRead.data[0] << 0) +
                            (m_canFrameRead.data[1] << 8);

                needStatusUpdate = true;
            }
            else
            {
                if(m_canFrameRead.can_id == 0x581) //SDO from EPOS
                {
                    if (m_canFrameRead.data[1] == 0x41 &&
                        m_canFrameRead.data[2] == 0x60 &&
                        m_canFrameRead.data[3] == 0x00)
                    {
                        auxStatus = (m_canFrameRead.data[4] << 0) +
                                    (m_canFrameRead.data[5] << 8);

                        needStatusUpdate = true;
                    }
                    if (m_messageBox[0].waitingAnswer || m_messageBox[1].waitingAnswer || m_messageBox[2].waitingAnswer || m_messageBox[3].waitingAnswer)
                    {
                        for(int i = 0; i < 4; i++)
                        {
                            //qDebug() << "[i=" << i << "] can_id: " << m_canFrameRead.can_id;
                            if(((m_canFrameRead.data[2] << 16) | (m_canFrameRead.data[1] << 8) | (m_canFrameRead.data[3])) == m_messageBox[i].indexSubIndex)
                            {
                                qDebug() << "resposta do " << m_messageBox[i].name << " chegou!" << (int)((m_canFrameRead.data[4] <<  0) |
                                                                                                            (m_canFrameRead.data[5] <<  8) |
                                                                                                            (m_canFrameRead.data[6] << 16) |
                                                                                                            (m_canFrameRead.data[7] << 24));
                                if(m_messageBox[i].write == false)
                                {
                                    m_messageBox[i].value = QVariant((int)((m_canFrameRead.data[4] <<  0) |
                                                                     (m_canFrameRead.data[5] <<  8) |
                                                                     (m_canFrameRead.data[6] << 16) |
                                                                     (m_canFrameRead.data[7] << 24)) );
                                }

                                if(m_messageBox[i].indexSubIndex != 0x606000)
                                {
                                    m_objectDictionaryByIndexSubIndex[m_messageBox[i].indexSubIndex] = m_messageBox[i].value.toInt();
                                }

                                m_messageBox[i].waitingAnswer = false;
                                m_messageBox[i].write = false;
                            }
                        }

                        emit messagesChanged();
                    }
                }
            }
            m_newCANMessageArrived = false;
            //qDebug() << "(" << m_keepReadingPDOActualPosition << ") thread: " << m_motorPosition;
        }

        if(needStatusUpdate)
        {
            for(i = 0; i < 16; i++)
            {
                m_status_new[i] = (bool)((auxStatus >> i) & 1);

                if(m_status_new[i] != m_status[i])
                {
                    m_status[i] = m_status_new[i];
                    //needStatusUpdateOnGUI = true;
                }
            }

            needStatusUpdate = false;
        }

        //if(needStatusUpdateOnGUI)
        if(m_doInterfaceUpdate && m_newCANMessageArrived)
        {
            emit statusChanged();
            //needStatusUpdateOnGUI = false;
            m_doInterfaceUpdate = false;
        }
    }
    qDebug() << "end of readNewMessage";

    m_keepThreadDoInterfaceUpdate = false;

    if(m_threadDoInterfaceUpdate.joinable())
    {
        m_threadDoInterfaceUpdate.join();
    }
}

void BackEnd::doInterfaceUpdate(int sleep_ms)
{
    while(m_keepThreadDoInterfaceUpdate)
    {
        SLEEP_ms(sleep_ms);
        m_doInterfaceUpdate = true;
    }
}

QVariantList BackEnd::status()
{
    QVariantList aux;

    for (int i = 0; i < 16; i++)
    {
        aux << m_status[i];
    }

    //qDebug() << aux << " (status function)";

    return aux;
}

QVariantList BackEnd::messages()
{
    QVariantList aux;

    for (int i = 0; i < 16; i++)
    {
        aux << m_messageBox[i].value.toInt();
    }

    //qDebug() << aux << " (status function)";

    return aux;
}

void BackEnd::sendCAN(struct can_frame& canFrame)
{
    if(m_isConnected)
    {
        write(m_socketCan, &canFrame, sizeof(struct can_frame));
    }
}

bool BackEnd::readCAN()
{
    //qDebug() << "entering readCAN";
#ifdef DO_READ
    if(m_isConnected)
    {
        FD_ZERO(&m_readfds);
        FD_SET(m_socketCan, &m_readfds);
        //qDebug() << "\t\tFD set";
        m_tvRead.tv_sec = m_tvRead.tv_usec = 0;

        int selectRet = select(m_socketCan+1, &m_readfds, NULL, NULL, &m_tvRead);
        //qDebug() << "\t\tselect done";
        if(selectRet == -1)
        {
            //qDebug() << "\t\tselect is *** -1 ***";
            m_isConnected = false;
            //qDebug() << "select call returned -1 (CANopenCommunication.cpp)!\n";

            throw std::runtime_error("select call returned -1 (CANopenCommunication.cpp:" + std::to_string(__LINE__) + ")!\n");

            exit(-1);
        }
        else if(FD_ISSET(m_socketCan, &m_readfds))
        {
            //qDebug() << "\t\tselect is " << selectRet;
            read(m_socketCan, &m_canFrameRead, sizeof(struct can_frame));
            m_newCANMessageArrived = true;

            //qDebug() << "\t\tjust read " << nbytes;
        }
    }
#else //DUMMY CASE, just testing stuff
    emit connectedChanged();
#endif

    return m_newCANMessageArrived;
}

bool BackEnd::connected()
{
    return m_isConnected;
}

int BackEnd::motorPosition()
{
    return m_motorPosition;
}

int BackEnd::motorVelocity()
{
    return m_motorVelocity;
}

void BackEnd::sendPosition(int value)
{
    if(m_isConnected)
    {
        //qDebug() << "sending position" << value;
        m_canFrameWrite = createSDO(1, true, 0x206200, 32, value);
        //m_canFrameWrite = createSDO(1, true, 0x607A00, 32, value);
        sendCAN(m_canFrameWrite);
        SLEEP_ms(10);
        m_canFrameWrite = createSDO(1, true, 0x604000, 16, 0x003F);
        sendCAN(m_canFrameWrite);
        SLEEP_ms(10);
    }
}

void BackEnd::sendProfilePosition(int value)
{
    if(m_isConnected)
    {
        qDebug() << "sending profilePosition" << value;

        m_canFrameWrite = createSDO(1, true, 0x607A00, 32, value);
        sendCAN(m_canFrameWrite);
        SLEEP_ms(10);
        m_canFrameWrite = createSDO(1, true, 0x604000, 16, 0x003F);
        sendCAN(m_canFrameWrite);
        SLEEP_ms(10);
    }
}

void BackEnd::sendProfileVelocity(int value)
{
    if(m_isConnected)
    {
        qDebug() << "sending profileVelocity" << value;

        m_canFrameWrite = createSDO(1, true, 0x60FF00, 32, value);
        sendCAN(m_canFrameWrite);
        SLEEP_ms(10);

        m_canFrameWrite = createSDO(1, true, 0x604000, 16, 0x000F);
        sendCAN(m_canFrameWrite);
        //SLEEP_ms(10);
    }
}

void BackEnd::resetComm()
{
    if(m_isConnected)
    {
        qDebug() << "resetComm";
        sendCAN(m_NMTResetCommunication);
    }
}

void BackEnd::resetApp()
{
    if(m_isConnected)
    {
        qDebug() << "resetApp";
        sendCAN(m_NMTResetApplication);
        SLEEP_ms(10);
        //clear fault
        struct can_frame a = createSDO(1, true, 0x604000, 16, 0x0080);
        sendCAN(a);
    }
}

void BackEnd::stop()
{
    if(m_isConnected)
    {
        qDebug() << "stop";
        m_keepReadingCANMessages = false;
        if(m_threadReadCANMessages.joinable())
        {
            qDebug() << "waiting thread (m_keepReadingPDOActualPosition = " << m_keepReadingCANMessages << ")";
            if(m_threadReadCANMessages.joinable())
            {
                m_threadReadCANMessages.join();
            }
            qDebug() << "thread joined";
        }
    #ifdef DO_READ
        //send shutdown
        m_canFrameWrite = createSDO(1, true, 0x604000, 16, 0x0006);
        sendCAN(m_canFrameWrite);
        SLEEP_ms(10);

        sendCAN(m_NMTStop);

        m_isConnected = false;
        close(m_socketCan);
    #endif
        emit connectedChanged();
    }
}

void BackEnd::setPreOperational()
{
    if(m_isConnected)
    {
        qDebug() << "setPreOperational";
        sendCAN(m_NMTGoPreOperational);
        qDebug() << "read PDO thread joined";
    }
}

void BackEnd::clearErrors()
{
    qDebug() << "clearErrors";
    struct can_frame a = createSDO(1, true, 0x604000, 16, 0x0080);
    sendCAN(a);
    SLEEP_ms(10);
    sendCAN(m_NMTResetApplication);
}

struct can_frame BackEnd::createSDO(const uint8_t address, const bool &doWrite, int indexSubindex,
                   int dataType, const long &value)
{
    // flag for error at data
    bool dataError = false;

    // amount of data inn bytes that will not be used in this SDO message (related to n value of the SDO message)
    uint8_t invalidData = 4;

    // data type analysis
    switch(dataType)
    {
        case 8:
            invalidData = 4 - 1;
            dataError = ((value >> 8) != 0);
            break;
        case 16:
            invalidData = 4 - 2;
            dataError = ((value >> 8*2) != 0);
            break;
        case 32:
            invalidData = 4 - 4;
            break;
        case 64:
            throw std::invalid_argument("Not implemented for type UINT64 nor STRING! (CANopenCommunication.cpp:" + std::to_string(__LINE__) + ")\n");
            break;
        default:
            //if read, do nothing
            if(doWrite)
            {
                dataError = true;
            }
    }

    // if error, print so
    if(dataError)
    {
        throw std::invalid_argument("maximum type expected is 4 bytes! (CANopenCommunication.cpp:" + std::to_string(__LINE__) + ")\n");
    }

    // message that will return
    struct can_frame SDOmessage;

    // amount of bytes used
    SDOmessage.can_dlc = 8;

    // SDO message ID
    SDOmessage.can_id = 0x600 + address;

    // alias for the first byte of data (command byte for SDO message)
    uint8_t& commandByte = SDOmessage.data[0];

    // set object index
    SDOmessage.data[1] = (indexSubindex >> 8) & 0xFF;
    SDOmessage.data[2] = indexSubindex >> 16;

    // set object subindex
    SDOmessage.data[3] = indexSubindex & 0xFF;

    // data for payload of SDO message
    std::array<uint8_t, 4> data{};

    // if it is and SDO download message (that writes in the node that receives the message - the server)
    if (doWrite) //SDO download
    {
        //client command specifier (ccs)
        uint8_t ccs = 1;
        //message is not segmented!
        uint8_t expedited = 1;
        //size IS indicated
        uint8_t size = 1;

        // commandByte is made of (from MSB to LSB): ccs (3bits), 1 reserved bit, n (2bits), e (1bit), s (1bit)
        commandByte = (ccs << 5) | (invalidData << 2) | (expedited << 1) | size;

        // set data (for payload)
        for(int i = 0; i < (4 - invalidData); i++)
        {
            data.at(i) = (value >> 8*i) & (0xFF);
        }
    }
    else // read: SDO upload
    {
        //client command specifier (ccs)
        uint8_t ccs = 2;

        // commandByte is made of (from MSB to LSB): ccs (3bits), 1 reserved bit, n (2bits), e (1bit), s (1bit)
        // online ccs is needed for upload (read)
        commandByte = (ccs << 5);
    }

    // the first byte at data
    SDOmessage.data[0] = commandByte;

    // set payload
    for(int i = 0; i < 4; i++)
    {
        SDOmessage.data[i+4] = data.at(i);
    }

    return SDOmessage;
}

void BackEnd::shutDown()
{
    m_canFrameWrite = createSDO(1, true, 0x604000, 16, 0x0006);
    sendCAN(m_canFrameWrite);
}

void BackEnd::switchOn()
{
    m_canFrameWrite = createSDO(1, true, 0x604000, 16, 0x000F);
    sendCAN(m_canFrameWrite);
}

void BackEnd::prepareStatusRead()
{
    m_canFrameWrite = createSDO(1, false, 0x604100, 16, 0x0);
    sendCAN(m_canFrameWrite);
}

void BackEnd::setMovementMode(MovementMode mode)
{
    if(!m_isConnected)
    {
        return;
    }

    //set mode
    m_canFrameWrite = createSDO(1, true, 0x606000, 8, mode);
    sendCAN(m_canFrameWrite);
    SLEEP_ms(10);

    switch (mode)
    {
        case MovementMode::ProfilePostionMode:
            qDebug() << "setMode ProfilePostionMode (" << mode << ")";
            //set max following error
            m_canFrameWrite = createSDO(1, true, 0x606500, 32, 2000); //qc
            m_canFrameWrite = createSDO(1, true, 0x606500, 32, m_objectDictionaryByIndexSubIndex[0x606500]); //qc
            qDebug() << "max following error" << m_objectDictionaryByIndexSubIndex[0x606500];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);


            //set minimum position limit
            m_canFrameWrite = createSDO(1, true, 0x607D01, 32, -2147483648); //qc
            m_canFrameWrite = createSDO(1, true, 0x607D01, 32, m_objectDictionaryByIndexSubIndex[0x607D01]); //qc
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);


            //set maximum position limit
            m_canFrameWrite = createSDO(1, true, 0x607D02, 32, 2147483647); //qc
            m_canFrameWrite = createSDO(1, true, 0x607D02, 32, m_objectDictionaryByIndexSubIndex[0x607D02]); //qc
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);


            //set maximum profile velocity
            m_canFrameWrite = createSDO(1, true, 0x607F00, 32, 25000); //rpm
            m_canFrameWrite = createSDO(1, true, 0x607F00, 32, m_objectDictionaryByIndexSubIndex[0x607F00]); //rpm
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];

            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);


            //set profile velocity
            m_canFrameWrite = createSDO(1, true, 0x608100, 32, 100); //rpm
            m_canFrameWrite = createSDO(1, true, 0x608100, 32, m_objectDictionaryByIndexSubIndex[0x608100]); //rpm
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);


            //set profile acceleration
            m_canFrameWrite = createSDO(1, true, 0x608300, 32, 100); //rpm/s
            m_canFrameWrite = createSDO(1, true, 0x608300, 32, m_objectDictionaryByIndexSubIndex[0x608300]); //rpm/s
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);


            //set profile deceleration
            m_canFrameWrite = createSDO(1, true, 0x608400, 32, 100); //rpm/s
            m_canFrameWrite = createSDO(1, true, 0x608400, 32, m_objectDictionaryByIndexSubIndex[0x608400]); //rpm/s
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);


            //set quickstop deceleration
            m_canFrameWrite = createSDO(1, true, 0x608500, 32, 100); //rpm/s
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);


            //set motion profile type
            m_canFrameWrite = createSDO(1, true, 0x608600, 16, 0); //0: linear, 1: sin²
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);
        break;
        case MovementMode::HomingMode:
            qDebug() << "setMode HomingMode (" << mode << ")";
            //set CURRENT POSITION as home position
            m_canFrameWrite = createSDO(1, true, 0x609800, 8, 0x35);
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);
        break;
        case MovementMode::ProfileVelocityMode:
            qDebug() << "setMode ProfileVelocityMode (" << mode << ")";
            //set max profile velocity mode
            m_canFrameWrite = createSDO(1, true, 0x607F00, 32, 25000); //rpm
            m_canFrameWrite = createSDO(1, true, 0x607F00, 32, m_objectDictionaryByIndexSubIndex[0x607F00]); //rpm
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);


            //set profile acceleration
            m_canFrameWrite = createSDO(1, true, 0x608300, 32, 10000); //rpm/s
            m_canFrameWrite = createSDO(1, true, 0x608300, 32, m_objectDictionaryByIndexSubIndex[0x608300]); //rpm/s
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);


            //set profile deceleration
            m_canFrameWrite = createSDO(1, true, 0x608400, 32, 10000); //rpm/s
            m_canFrameWrite = createSDO(1, true, 0x608400, 32, m_objectDictionaryByIndexSubIndex[0x608400]); //rpm/s
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);


            //set quick stop deceleration
            m_canFrameWrite = createSDO(1, true, 0x608500, 32, 10000); //rpm/s
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);

            //set motion profile type
            m_canFrameWrite = createSDO(1, true, 0x608600, 16, 0);  //0: linear, 1: sin²
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);
        break;
        case MovementMode::PositionMode:
            qDebug() << "setMode PositionMode (" << mode << ")";
            //set max following error
            m_canFrameWrite = createSDO(1, true, 0x606500, 32, 2000); //qc
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);


            //set minimum position limit
            m_canFrameWrite = createSDO(1, true, 0x607D01, 32, -2147483648); //qc
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);


            //set maximum position limit
            m_canFrameWrite = createSDO(1, true, 0x607D02, 32, 2147483647); //qc
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);
        break;
        case MovementMode::VelocityMode:
            qDebug() << "setMode VelocityMode (" << mode << ")";
        break;
        case MovementMode::CurrentMode:
        qDebug() << "setMode CurrentMode (" << mode << ")";
        break;
    default:
        qDebug() << "NO MODE " << mode <<"!\n";
        exit(1);
        break;
    }

    shutDown();
    SLEEP_ms(10);
    switchOn();
    SLEEP_ms(10);

    switch (mode)
    {
        case MovementMode::ProfilePostionMode:

        break;
        case MovementMode::HomingMode:
            switchOn();
            SLEEP_ms(10);
            m_canFrameWrite = createSDO(1, true, 0x604000, 16, 0x001F);
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);
        break;
        case MovementMode::ProfileVelocityMode:
            //set target velocity
            m_canFrameWrite = createSDO(1, true, 0x60FF00, 32, 0); //rpm
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);

            //set controlword
            m_canFrameWrite = createSDO(1, true, 0x604000, 16, 0x000F);
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);
        break;
        case MovementMode::PositionMode:

            //set position mode setting value
            m_canFrameWrite = createSDO(1, true, 0x206200, 32, 0); //qc
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);
        break;
        case MovementMode::VelocityMode:

            //set velocity mode setting value
            m_canFrameWrite = createSDO(1, true, 0x206B00, 32, 0); //rpm
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);
        break;
        case MovementMode::CurrentMode:

            //set current mode setting value
            m_canFrameWrite = createSDO(1, true, 0x203000, 32, 0); //mA
            qDebug() << "minimum position limit" << m_objectDictionaryByIndexSubIndex[0x607D01];
            sendCAN(m_canFrameWrite);
            SLEEP_ms(10);
        break;
    }
}

void BackEnd::clearFault()
{
    //set controlword
    m_canFrameWrite = createSDO(1, true, 0x604000, 8, 0x0080);
    sendCAN(m_canFrameWrite);
    SLEEP_ms(10);
}

//void BackEnd::registerValuesForUpdate(const QString& select0,
//                                      const QString& select1,
//                                      const QString& select2,
//                                      const QString& select3)
void BackEnd::registerValuesForUpdate(const QJSValue& select0,
                                      const QJSValue& select1,
                                      const QJSValue& select2,
                                      const QJSValue& select3)
{
//    QJsonObject objectSelect0 = QJsonDocument::fromJson(select0.toUtf8()).object();
//    QJsonObject objectSelect1 = QJsonDocument::fromJson(select1.toUtf8()).object();
//    QJsonObject objectSelect2 = QJsonDocument::fromJson(select2.toUtf8()).object();
//    QJsonObject objectSelect3 = QJsonDocument::fromJson(select3.toUtf8()).object();

    int i = 0;

    qDebug() << "requisições recebidas";

    can_frame aux;

    std::stringstream ss;

//    for(auto& select : {objectSelect0, objectSelect1, objectSelect2, objectSelect3})
    for(auto& select : {select0, select1, select2, select3})
    {
//        m_messageBox[i] = MessageType(select);
        m_messageBox[i] = MessageType(select);

//        qDebug() << m_messageBox[i].name;
//        qDebug() << "\t" << m_messageBox[i].indexSubIndex;
//        qDebug() << "\t" << m_messageBox[i].dataLength;
//        qDebug() << "\t" << m_messageBox[i].unit;
        aux = createSDO(1, false, m_messageBox[i].indexSubIndex, m_messageBox[i].dataLength, 0);
        sendCAN(aux);
        qDebug() << "criando SDO da msg " << i + 1 << " (" << m_messageBox[i].name << ")";
        ss << std::setfill('0') << std::setw(3) << std::hex << (int)aux.can_id << '#';
        for(int j = 0; j < 8; j++)
        {
           ss << std::setfill('0') << std::setw(2) << std::hex << (int)aux.data[j] << '.';
        }
        qDebug() << QString::fromStdString(ss.str());

        ss.str("");
        ss.clear();

        i++;
        SLEEP_ms(10);
    }
}

//void BackEnd::sendMessages(const QVariant& newValue0, const QJSValue& select0,
//                           const QVariant& newValue1, const QJSValue& select1,
//                           const QVariant& newValue2, const QJSValue& select2,
//                           const QVariant& newValue3, const QJSValue& select3)
void BackEnd::sendMessages(const QVariantList& newValues, const QJSValue& selects)
{
    std::stringstream ss;

    can_frame aux;

    for(int i = 0; i < (int)m_messageBox.size(); i++)
    {
        if(!newValues[i].isNull() && selects.property(i).property("indexSubIndex").toInt() != 0x606000)
        {
            selects.property(i).setProperty("value", newValues[i].toInt());
            m_messageBox[i] = MessageType(selects.property(i));
            m_messageBox[i].setWrite();

            ss << m_messageBox[i].name.toStdString() << " " << std::setw(6) << std::hex << std::uppercase << m_messageBox[i].indexSubIndex << " " << std::dec << m_messageBox[i].value.toInt();
            qDebug() << QString::fromStdString(ss.str());
            ss.str("");
            ss.clear();
        }
        else
        {
            selects.property(i).setProperty("value", 0);
            m_messageBox[i] = MessageType(selects.property(i));
        }

        aux = createSDO(1, m_messageBox[i].write, m_messageBox[i].indexSubIndex, m_messageBox[i].dataLength, m_messageBox[i].value.toInt());
        sendCAN(aux);
        SLEEP_ms(10);
    }
}

