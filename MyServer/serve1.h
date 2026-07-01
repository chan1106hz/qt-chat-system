#ifndef SERVE1_H
#define SERVE1_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QThread>
#include <QList>

QT_BEGIN_NAMESPACE
namespace Ui { class Serve1; }
QT_END_NAMESPACE

class ClientHandler;

// ============ 主服务器窗口（主线程：UI + 监听 + 消息路由） ============
class Serve1 : public QWidget
{
    Q_OBJECT

public:
    Serve1(QWidget *parent = nullptr);
    ~Serve1();

private slots:
    void on_pushButton_clicked();      // 启动服务器
    void on_pushButton_2_clicked();    // 发送（群发）
    void onNewConnection();            // 有新客户端连接
    void routeMessage(const QString &msg, ClientHandler *sender);  // 路由消息
    void onHandlerDisconnected(ClientHandler *handler);            // 客户端断开

private:
    Ui::Serve1 *ui;
    QTcpServer *m_server;
    // username → 对应的 ClientHandler（只在主线程访问，无需加锁）
    QMap<QString, ClientHandler*> m_userMap;
    // 所有工作线程列表（用于清理）
    QList<QThread*> m_threads;
};

// ============ 客户端处理器（每个客户端一个线程） ============
class ClientHandler : public QObject
{
    Q_OBJECT
public:
    explicit ClientHandler(QTcpSocket *socket, QObject *parent = nullptr);
    ~ClientHandler();

    QString username() const { return m_username; }
    void setUsername(const QString &name) { m_username = name; }

signals:
    // 收到消息 → 发给主线程路由
    void messageReceived(const QString &msg, ClientHandler *sender);
    // 客户端断开 → 通知主线程清理
    void handlerDisconnected(ClientHandler *handler);

public slots:
    void onReadyRead();               // 读取客户端发来的数据
    void onDisconnected();            // 客户端断开连接
    void writeMessage(const QString &msg);  // 向该客户端发送消息

private:
    QTcpSocket *m_socket;
    QString m_username;
};

#endif // SERVE1_H
