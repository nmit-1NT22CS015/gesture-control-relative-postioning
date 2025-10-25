#include "mainwindow.h"
// #include "thread2.cpp"

#include <QWidget>
#include<QDebug>
#include <QtCore>
#include <QCryptographicHash>
#include <QApplication>
#include <QUdpSocket>
#include <QHostAddress>
#include <QPushButton>
#include <QLineEdit>
#include <QFrame>
#include <QVBoxLayout>


QUdpSocket* lis;
QFrame* f1;
QVBoxLayout* l2;
MainWindow* www;
QHash<QString, QVariantList> devices;

void pass(){
    QFrame f;
    f.setFrameStyle(QFrame::Box | QFrame::Raised);
    QLineEdit l(&f);
    QPushButton b(&f);
    connect(&b,&QPushButton::clicked,www,[](QLineEdit l, QPushButton b){
        QUdpSocket udpSocket;
        QByteArray datagram = "Glove Thingi auth Pass:"+QString(QCryptographicHash::hash((l.text()+devices[b.text()].at(0).toString()).toUtf8(), QCryptographicHash::Md5).toHex()).toUtf8();
        // QByteArray md5Hex = QString(hashResult.toHex()).toUtf8();

        QHostAddress destinationAddress(devices[b.text()].at(1).toString());
        quint16 destinationPort = 5555;
        QLayoutItem* child;
        while(l2->count()!=0)
        {
            child = l2->takeAt(0);
            delete child;
        }
        devices.clear();
        udpSocket.writeDatagram(datagram.data(), datagram.size(), destinationAddress, destinationPort);
    });

}

void func(QString name, QString key,QHostAddress ip){
    if(!devices.contains(name)){
        // qCritical( "UDP");
        QVariantList a;
        a.append(key);
        a.append(ip.toString());
        devices[name]=a;
        QPushButton *b=new QPushButton(name,f1);
        connect(b,&QPushButton::clicked,www,&pass);
        f1->layout()->addWidget(b);
        f1->show();
    }
}

class Thread : public QThread
{

public:
    Thread(){
        // qCritical( "UDP");
        lis= new QUdpSocket();
        lis->bind(QHostAddress::Any,5555);
        connect(lis,&QUdpSocket::readyRead,this,&Thread::readyRead,Qt::QueuedConnection);
    }

    void run()
    {
    }
public slots:
    void readyRead(){
        while(lis->hasPendingDatagrams())
        {
            // qCritical( "UDPBuffer.data()");
            QByteArray UDPBuffer;

            UDPBuffer.resize(lis->pendingDatagramSize());
            QHostAddress sender;
            quint16 senderPort;
            lis->readDatagram(UDPBuffer.data(),UDPBuffer.size(), &sender, &senderPort);
            QString s = UDPBuffer.data();
            if(s.startsWith("Glove Thingi response Name:")){
                QString name="";
                int i=27;
                while(s[i]!=' '){
                    name=name.append(s[i++]);
                }
                i+=5;
                QString key="";
                while(i<s.length() && s[i]!=' '){
                    key=key.append(s[i++]);
                }
                func(name,key,sender);
            }
        }
    }

};


void ping(){
    QUdpSocket udpSocket;
    QByteArray datagram = "Glove Thingi ping";
    QHostAddress destinationAddress = QHostAddress::Broadcast;
    quint16 destinationPort = 5555;
    QLayoutItem* child;
    while(l2->count()!=0)
    {
        child = l2->takeAt(0);
        delete child;
    }
    devices.clear();
    udpSocket.writeDatagram(datagram.data(), datagram.size(), destinationAddress, destinationPort);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    www=&w;
    QVBoxLayout l1;
    f1=new QFrame();
    l2= new QVBoxLayout();
    w.centralWidget()->setLayout(&l1);
    QPushButton b("Reload");
    Thread t;
    w.connect(&b,&QPushButton::clicked,&w,ping);
    l1.addWidget(&b);
    l1.addWidget(f1);
    l1.setSizeConstraint(QLayout::SetFixedSize);
    f1->setFrameStyle(QFrame::Box | QFrame::Raised);
    f1->setMinimumSize(400,400);
    f1->setLayout(l2);
    // w.setLayout(&l1);
    w.setWindowTitle("IOT Proj");
    w.show();
    return a.exec();
}
