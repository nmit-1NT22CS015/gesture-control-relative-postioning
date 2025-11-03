#include "mainwindow.h"
// #include "thread2.cpp"

#include <QWidget>
#include<QDebug>
#include <QtCore>
#include <QCryptographicHash>
#include <QInputDialog>
#include <QApplication>
#include <QUdpSocket>
#include <QHostAddress>
#include <QPushButton>
#include <QLineEdit>
#include <QFrame>
#include <cmath>
// #include <chrono>
#include <QVBoxLayout>


QUdpSocket* lis;
QFrame* f1;
QVBoxLayout* l2;
QString nam="";
MainWindow* www;
QHash<QString, QList<QString>> devices;
bool connected=false;
double PI = acos(-1.0);
double pitch = 0, roll = 0, yaw = 0;  // Orientation angles (degrees)
double Q_angle = 0.001;  // Process noise variance for the accelerometer
double Q_bias = 0.003;   // Process noise variance for the gyroscope bias
double R_measure = 0.03; // Measurement noise variance
double angle = 0, bias = 0, rate = 0; // Kalman filter state variables
double P[2][2] = {{0, 0}, {0, 0}};  // Error covariance matrix
// chrono::time_point<chrono::system_clock> lastTime = 0;//std::chrono::system_clock::now();

void func(QString name, QString key,QHostAddress ip){
    if(!devices.contains(name)){
        QList<QString> a;
        a.append(key);
        a.append(ip.toString());
        QPushButton *b=new QPushButton(name,f1);
        QString temp =b->text();
        devices[name]=a;
        QObject::connect(b,&QPushButton::clicked,b,[temp](){
            bool ok;
            QString text = QInputDialog::getText(www, ("Input Dialog"),
                                                 ("Enter your name:"), QLineEdit::Normal,
                                                 QDir::home().dirName(), &ok);
            if (ok && !text.isEmpty()) {
                QUdpSocket udpSocket;
                QByteArray datagram = "Glove Thingi auth Pass:";
                datagram +=QString(QCryptographicHash::hash((text+devices[temp][0]).toUtf8(), QCryptographicHash::Md5).toHex()).toUtf8();
                QHostAddress destinationAddress(devices[temp][1]);
                quint16 destinationPort = 5555;
                nam=temp;
                qCritical( temp.toUtf8());
                udpSocket.writeDatagram(datagram.data(), datagram.size(), destinationAddress, destinationPort);
            }
        });
        f1->layout()->addWidget(b);
        b->show();
    }
}


double Kalman_filter(double angle, double gyroRate, double accelAngle) {
    // Predict
    rate = gyroRate - bias;
    angle += 0.020 * rate; //0.020 is the time diff thingi

    P[0][0] += 0.020 * (0.020 * P[1][1] - P[0][1] - P[1][0] + Q_angle);
    P[0][1] -= 0.020 * P[1][1];
    P[1][0] -= 0.020 * P[1][1];
    P[1][1] += Q_bias *0.020 ;

    // Update
    double S = P[0][0] + R_measure; // Estimate error
    double K[2];                    // Kalman gain
    K[0] = P[0][0] / S;
    K[1] = P[1][0] / S;

    double y = accelAngle - angle; // Angle difference
    angle += K[0] * y;
    bias += K[1] * y;

    double P00_temp = P[0][0];
    double P01_temp = P[0][1];

    P[0][0] -= K[0] * P00_temp;
    P[0][1] -= K[0] * P01_temp;
    P[1][0] -= K[1] * P00_temp;
    P[1][1] -= K[1] * P01_temp;

    return angle;
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
            if(s.startsWith("Glove Thingi response sucessfully connected Name:")){
                QString name="";
                int i=49;
                while(i<s.length() && s[i]!=' '){
                    name=name.append(s[i++]);
                }
                if(nam==name) {
                    connected=true;}

            }
            if(s.startsWith("Glove Thingi values ") && devices.contains(nam) && sender.toString()==devices[nam][1] && connected==true){

                // qCritical("val.toUtf8()");
                QString val=s.slice(20);
                // qCritical(val.toUtf8());
                QList<float> l;
                foreach(QString num,val.split(" ")){
                    l.append(num.toFloat());
                }

                // chrono::time_point<chrono::system_clock> currentTime = millis();
                // if(lastTime==0){lastTime=currentTime-10;}
                // dt = (currentTime - lastTime) / 1000.0; // Calculate time in seconds
                // if (dt == 0) dt = 0.001;               // Prevent division by zero
                // lastTime = currentTime;
                double op=ptich,oy=yaw;
                pitch = Kalman_filter(pitch, l[3], atan2(-l[0], sqrt(l[1] * l[1] + l[2] * l[2])) * 180.0 / PI);
                roll = Kalman_filter(roll, l[4], atan2(l[1], l[2]) * 180.0 / PI);
                int i=24;

                // --- YAW CALCULATION USING MAGNETOMETER ---
                yaw = atan2(l[7], l[6]); // Calculate yaw from magnetometer readings
                float declinationAngle = -0.1783; // Declination in radians for -10° 13'
                yaw += declinationAngle;          // Adjust for magnetic declination

                // Normalize yaw to 0-360 degrees
                if (yaw < 0) yaw += 2 * PI;
                if (yaw > 2 * PI) yaw -= 2 * PI;
                yaw = yaw * 180.0 / PI; // Convert yaw to degrees
                qDebug(QString::number(pitch,'f',15).toUtf8());
                qDebug(QString::number(roll,'f',15).toUtf8());
                qDebug(QString::number(yaw,'f',15).toUtf8());
                if(pitch!=0){
                    mouseMove((pitch-op)/5,(yaw-oy)/5);
                }

                /*
                while(i<s.length() && s[i]!=' '){
                    name=name.append(s[i++]);
                }
                i+=5;
                nam=name;*/
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
