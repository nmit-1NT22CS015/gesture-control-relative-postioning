#include <QtCore>

class Thread : public QThread
{
private:
    void run()
    {
        qDebug()<<"From worker thread: "<<currentThreadId();
    }
};
