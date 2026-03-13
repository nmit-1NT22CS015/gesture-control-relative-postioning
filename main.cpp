#include "mainwindow.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QScreen>
#include "globalmouse.h"
#include <iostream>
#include <windows.h>
#include <QTimer>
#include <QPoint>

// #include "thread2.cpp"

#include <QWidget>
#include<QDebug>
#include <QtCore>
#include <QCryptographicHash>
#include <QInputDialog>
#include <QApplication>
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QMessageBox>
#include <QHostAddress>
#include <QPushButton>
#include <QLineEdit>
#include <QFrame>
#include <cmath>
#include <ctime>
// #include <chrono>
#include <QVBoxLayout>
// #include "MadgwickAHRS.h"
// #include "MadgwickAHRS.cpp"

// Madgwick filter;


QUdpSocket* lis;
QFrame* f1;
QVBoxLayout* l2;
QString nam="";
MainWindow* www;
QHash<QString, QList<QString>> devices;
bool connected=false;
double op=0,oy=0,oor=0;
bool left=false,right=false;
double PI = acos(-1.0) ,dt=0;
double ax=0,ay=0,az=0;
double vx=0,vy=0,vz=0;
double dx=0,dy=0,dz=0;
double pitch = 0, roll = 0, yaw = 0, meow;  // Orientation angles (degrees)
double Q_angle = 0.001;  // Process noise variance for the accelerometer
double Q_bias = 0.003;   // Process noise variance for the gyroscope bias
double R_measure = 0.03; // Measurement noise variance
double angle = 0, bias = 0, rate = 0; // Kalman filter state variables
double yangle = 0, ybias = 0, yrate = 0; // Kalman filter state variables
double rangle = 0, rbias = 0, rrate = 0; // Kalman filter state variables
double ccc=0;  // Error covariance matrix
double amx=0,amy=0,amz=0;  // Error covariance matrix
double rP[2][2] = {{0, 0}, {0, 0}};  // Error covariance matrix
double lastTime = 0;//std::chrono::system_clock::now();
time_t start_time=0;

class Kalman{
    double P[2][2] = {{0, 0}, {0, 0}};
    double angle = 0, bias = 0, rate = 0;

    public :
    double Kalman_filter(double pangle, double gyroRate, double accelAngle) {
        // Predict
        this->rate = gyroRate - bias;
        pangle += dt * rate; //0.1 is the time diff thingi

        this->P[0][0] += dt * (dt * this->P[1][1] - this->P[0][1] - this->P[1][0] + Q_angle);
        this->P[0][1] -= dt * this->P[1][1];
        this->P[1][0] -= dt * this->P[1][1];
        this->P[1][1] += Q_bias * dt ;

        // Update
        double S = P[0][0] + R_measure; // Estimate error
        double K[2];                    // Kalman gain
        K[0] = this->P[0][0] / S;
        K[1] = this->P[1][0] / S;

        double y = accelAngle - pangle; // Angle difference
        pangle += K[0] * y;
        this->bias += K[1] * y;

        double P00_temp = this->P[0][0];
        double P01_temp = this->P[0][1];

        this->P[0][0] -= K[0] * P00_temp;
        this->P[0][1] -= K[0] * P01_temp;
        this->P[1][0] -= K[1] * P00_temp;
        this->P[1][1] -= K[1] * P01_temp;

        return pangle;
    }
};

Kalman yaww,pitchh,rolll;

void cursor(int x, int y){
    QPoint poss = QCursor::pos();
    int xx = poss.x() +x;
    int yy = poss.y() +y;
    QScreen *screen=QGuiApplication::primaryScreen();
    QCursor *cursor= new QCursor();
    cursor->setPos(screen , xx,yy);
}

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
                                                 ("Enter the password:"), QLineEdit::Normal,
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


bool rightClick(bool clickHeld) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;

    if (right==false && clickHeld==true) {
        right=clickHeld;
        input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    } else if(right==true && clickHeld==false) {
        right=clickHeld;
        input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    }
    SendInput(1, &input, sizeof(INPUT));
    return true;
}

bool leftClick(bool clickHeld) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;

    if (clickHeld == true && left==false) {
        left=clickHeld;
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    } else if(left==true && clickHeld==false){
        left=clickHeld;
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    }
    SendInput(1, &input, sizeof(INPUT));
    return true;
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
                    QMessageBox::information(nullptr, "Information", "Connected");
                    connected=true;}

            }
            if(s.startsWith("Glove Thingi values ") && devices.contains(nam) && sender.toString()==devices[nam][1] && connected==true){

                // qCritical("val.toUtf8()");//qCritical(QString::number(clock()-start_time,'f',7).toUtf8());
                QString val=s.slice(20);
                // qCritical(val.toUtf8());
                QList<float> l;
                foreach(QString num,val.split(" ")){
                    l.append(num.toFloat());
                }
//0ax 1ay 2az 3gx 4gy 5gz 6mx 7my 8mz 9time
                double currentTime = l[9];
                if(lastTime==0){lastTime=currentTime-100;}
                if(lastTime<currentTime){
                    dt = (currentTime - lastTime) / 1000.0; // Calculate time in seconds
                    // if (dt == 0) dt = 0.001;               // Prevent division by zero
                    lastTime = currentTime;
                    QPoint poss = QCursor::pos();
                    op=pitch, oy=yaw;oor=roll;
                    if(ccc==0){pitch=atan2(-l[0], sqrt(l[1] * l[1] + l[2] * l[2])) * 180.0 / PI;}
                    pitch = pitchh.Kalman_filter(pitch, l[4], atan2(-l[0], sqrt(l[1] * l[1] + l[2] * l[2])) * 180.0 / PI);
                    meow=(atan2(l[7],l[6]));
                    if (meow < 0) meow += 2 * PI;
                    if (meow > 2 * PI) meow -= 2 * PI;
                    // float reo=(atan2(l[6],l[8]));
                    // if (reo < 0) reo += 2 * PI;
                    // if (reo > 2 * PI) reo -= 2 * PI;
                    // qDebug(QString::number(meow).toUtf8()+"        "+QString::number(reo,'f',15).toUtf8()+" ");
                    yaw = yaww.Kalman_filter(yaw, l[5],l[5]*dt +yaw);//meow*180/PI);
                    // roll = Kalman_filter(roll, l[3],atan2(l[1], l[2]) * 180.0 / PI);
                    int i=24;
                    // --- YAW CALCULATION USING MAGNETOMETER ---
                    // pitch=atan2(l[6],l[8])*1000;
                    // yaw = atan2(l[7], l[6]); // Calculate yaw from magnetometer readings
                    // float declinationAngle = -0.1783; // Declination in radians for -10° 13'
                    // yaw += declinationAngle;          // Adjust for magnetic declination

                    // Normalize yaw to 0-360 degrees
                    // if (pitch < 0) pitch += 2 * PI;
                    // if (pitch > 2 * PI) pitch -= 2 * PI;
                    // yaw = yaw * 180.0 / PI; // Convert yaw to degrees
                    qDebug(s.toUtf8());
                    // qDebug(QString::number(pitch,'f',15).toUtf8()+" "+QString::number(yaw,'f',15).toUtf8()+" "+QString::number(yaw,'f',15).toUtf8()+" "+QString::number(l[9],'f',7).toUtf8());
                    // qDebug();
                    // qDebug();
                    // qDebug(QString::number(l[9],'f',7).toUtf8());

                    //ROLLLLLL
                    double accRoll = atan2(l[1], l[2]) * 180.0 / PI;
                    if (accRoll < 0) accRoll += 360;
                    if (accRoll > 360) accRoll -=360;
                    if(ccc==0){roll=accRoll;}
                    //double gyroRollRate = l[3]; // Rad/s or Deg/s around X
                    // double dt = 0.01; // Time step in seconds
                    float cosa=cos(meow), cosb=cos(PI*pitch/180),cosc=cos(PI*roll/180);
                    float sina=sin(meow), sinb=sin(PI*pitch/180),sinc=sin(PI*roll/180);
                    // Complementary filter: 98% Gyro + 2% Accel
                    roll = rolll.Kalman_filter(roll,l[3],accRoll);//0.98 * (roll + l[3] * dt) + (1.0 - 0.98) * accRoll;
                    // double rollDeg = qRadiansToDegrees(roll);
                    // qCritical(QString::number(roll).toUtf8());
                    if(ccc<170){ccc++;}
                    if(ccc>30 && ccc<170){
                        ax=l[0]*cosa*cosb+(cosa*sinb*sinc-sina*cosc)*l[1]+(cosa*sinb*cosc+sina*sinc)*l[2];
                        ay=l[0]*sina*cosb+(sina*sinb*sinc+cosa*cosc)*l[1]+(sina*sinb*cosc-cosa*sinc)*l[2];
                        az=-l[0]*sinb+l[1]*cosb*sinc+cosb*cosc*l[2];
                        amx+=ax;amz+=az;amy+=ay;}
                    else if(ccc>=170){

                        ax=l[0]*cosa*cosb+(cosa*sinb*sinc-sina*cosc)*l[1]+(cosa*sinb*cosc+sina*sinc)*l[2]-amx/139;
                        ay=l[0]*sina*cosb+(sina*sinb*sinc+cosa*cosc)*l[1]+(sina*sinb*cosc-cosa*sinc)*l[2]-amy/139;
                        az=-l[0]*sinb+l[1]*cosb*sinc+cosb*cosc*l[2]-amz/139;
                        // vx=vx/1.1+(double)((int)(ax*100))/100*dt;
                        vx+=ax*dt;
                        if((int)(ax*100)==0){vx=0;}
                    dx+=vx*dt;
                        vy+=ay*dt;
                    if((int)(ay*100)==0){vy=0;}
                    // vy=vy/1.1+(double)((int)(ay*100))/100*dt;
                    dy+=vy*dt;
                        vz+=az*dt;
                    if((int)(az*100)==0){vz=0;}
                    // vz=vz/1.1+(double)((int)(az*100))/100*dt;
                    dz+=vz*dt;}
                    qCritical(QString::number(dx).toUtf8()+ " "+QString::number(dy).toUtf8()+ " "+QString::number(dz).toUtf8());
                    qCritical("v: "+QString::number(vx).toUtf8()+ " "+QString::number(vy).toUtf8()+ " "+QString::number(vz).toUtf8());
                    qCritical("a: "+QString::number(ax).toUtf8()+ " "+QString::number(ay).toUtf8()+ " "+QString::number(az).toUtf8());
                    cursor((yaw-oy)*17,((+op-pitch)*17));

                    /*
                    while(i<s.length() && s[i]!=' '){
                        name=name.append(s[i++]);
                    }
                    i+=5;
                    nam=name;*/
                    rightClick(l[10]);
                    leftClick(l[11]);
                }
            }
        }
    }

};

void ping(){
    QUdpSocket udpSocket;
    QByteArray datagram = "Glove Thingi ping";
    QLayoutItem* child;
    while(l2->count()!=0)
    {
        child = l2->takeAt(0);
        delete child;
    }
    devices.clear();
    QList<QNetworkInterface> is = QNetworkInterface::allInterfaces();
    // QNetworkInterface ai;
    quint16 destinationPort = 5555;
    for(const QNetworkInterface& i:is){
        if(i.flags().testFlag(QNetworkInterface::IsUp) && !i.flags().testFlag(QNetworkInterface::IsLoopBack)){
            for (const QNetworkAddressEntry &entry : i.addressEntries()) {
                // Check if the address is IPv4 before getting the broadcast address
                if (entry.ip().protocol() == QHostAddress::IPv4Protocol && !entry.broadcast().isNull()) {
                    // QHostAddress broadcastAddress = entry.broadcast();
                    // qCritical(i.name().toUtf8());
                    udpSocket.writeDatagram(datagram.data(), datagram.size(), entry.broadcast(), destinationPort);
                }
            }
            // ai = i;
            // break;
        }
    }
    // QHostAddress destinationAddress = ai.addressEntries().first().broadcast();//QHostAddress::Broadcast;
    // qCritical(destinationAddress.toString().toUtf8());
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


// void values(){
//     int xx = rand() % 1366;
//     int yy = rand() % 768;
//     cursor(xx,yy);
// }




// int main(int argc, char *argv[])
// {
//     QApplication a(argc, argv);
//     MainWindow w;
//     bool nn = true;
//     // while(nn == true){
//     //     values();
//     // }

//     leftClick(true);

//     QTimer::singleShot(2000, []() {
//         rightClick(false);
//         qDebug() << "Simulating click-and-hold complete";
//     });

//     w.show();
//     return a.exec();
// }
