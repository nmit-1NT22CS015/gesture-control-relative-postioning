// #include <QtCore>
// #include <QUdpSocket>
// #include <QHostAddress>

// class Thread : public QThread
// {
// private:
//     void run()
//     {
//         lis.bind(5555);
//         connect(&lis,&QUdpSocket::readyRead,this,readyRead);
//     }
//     void static readyRead(){
//         while(lis.hasPendingDatagrams())
//         {
//             QByteArray UDPBuffer;

//             UDPBuffer.resize(lis.pendingDatagramSize());
//             QHostAddress sender;
//             quint16 senderPort;
//             lis.readDatagram(UDPBuffer.data(),UDPBuffer.size(), &sender, &senderPort);
//             qDebug()<<"UDPBuffer.data()\n";
//         }
//     }
// };
