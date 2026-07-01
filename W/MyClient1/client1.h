#ifndef CLIENT1_H
#define CLIENT1_H

#include <QWidget>
#include <QTcpSocket>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui { class Client1; }
QT_END_NAMESPACE

// ============ Socket 工作对象（运行在独立线程中）============
class SocketWorker : public QObject
{
    Q_OBJECT
public:
    explicit SocketWorker(QObject *parent = nullptr);
    ~SocketWorker();

public slots:
    void connectToHost(const QString &host, quint16 port);
    void disconnectFromHost();
    void sendMessage(const QString &msg);   // 写数据（在 worker 线程执行）
    void onReadyRead();                     // 读数据（在 worker 线程执行）

signals:
    void dataReceived(const QByteArray &data);  // 读取到的数据 → UI 线程
    void socketConnected();
    void socketDisconnected();
    void socketError(const QString &errorMsg);

private:
    QTcpSocket *m_socket;
};

// ============ 主窗口（UI 线程）============
class Client1 : public QWidget
{
    Q_OBJECT

public:
    Client1(QWidget *parent = nullptr);
    ~Client1();

private slots:
    void on_pushButton_clicked();       // 连接服务器
    void on_pushButton_2_clicked();     // 断开连接
    void on_pushButton_3_clicked();     // 发送消息
    void onDataReceived(const QByteArray &data);  // 接收消息显示
    void onConnected();
    void onDisconnected();
    void onError(const QString &errorMsg);

private:
    QString formatMessage();  // 根据消息内容自动格式化（@name → S|, 否则 → T|）

    Ui::Client1 *ui;
    QThread *m_workerThread;
    SocketWorker *m_worker;
    bool m_connected;
};

#endif // CLIENT1_H
