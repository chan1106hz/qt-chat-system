#include "serve1.h"
#include "ui_serve1.h"
#include <QDebug>
#include <QMessageBox>

// ==================== Serve1 主服务器 ====================

Serve1::Serve1(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Serve1)
    , m_server(nullptr)
{
    ui->setupUi(this);
    m_server = new QTcpServer(this);
}

Serve1::~Serve1()
{
    // 停止监听
    if (m_server && m_server->isListening()) {
        m_server->close();
    }
    // 退出所有工作线程
    for (QThread *th : m_threads) {
        th->quit();
        th->wait(3000);
        delete th;
    }
    delete ui;
}

// 启动服务器
void Serve1::on_pushButton_clicked()
{
    if (m_server->isListening()) {
        ui->textEdit->append("[Server] 服务器已在运行中...");
        return;
    }

    quint16 port = ui->lineEdit->text().toInt();
    if (!m_server->listen(QHostAddress::Any, port)) {
        QMessageBox::warning(this, "启动失败",
                             QString("无法监听端口 %1: %2").arg(port).arg(m_server->errorString()));
        return;
    }

    connect(m_server, &QTcpServer::newConnection, this, &Serve1::onNewConnection);
    ui->textEdit->append(QString("[Server] 服务器已启动，监听端口: %1").arg(port));
}

// 服务器群发按钮
void Serve1::on_pushButton_2_clicked()
{
    QString msg = ui->textEdit->toPlainText();
    if (msg.isEmpty()) return;

    // 遍历所有在线用户，群发消息
    for (auto it = m_userMap.begin(); it != m_userMap.end(); ++it) {
        // 通过信号/槽（跨线程排队连接）安全发送
        QMetaObject::invokeMethod(it.value(), "writeMessage",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, ui->textEdit->toPlainText()));
    }
}

// 有新客户端连接 → 创建独立线程+处理器
void Serve1::onNewConnection()
{
    while (QTcpSocket *clientSocket = m_server->nextPendingConnection()) {
        // 1. 创建工作线程
        QThread *thread = new QThread(this);
        m_threads.append(thread);

        // 2. 将 socket 移到工作线程（必须在 handler 之前，且必须在 thread->start() 之前）
        clientSocket->setParent(nullptr);          // 断开与 QTcpServer 的父子关系
        clientSocket->moveToThread(thread);

        // 3. 创建工作对象并移到工作线程（socket 和 handler 同属一个线程）
        ClientHandler *handler = new ClientHandler(clientSocket);
        handler->moveToThread(thread);

        // 4. 连接信号/槽
        // handler → Serve1: 消息路由（使用 QueuedConnection 保证线程安全）
        connect(handler, &ClientHandler::messageReceived,
                this, &Serve1::routeMessage,
                Qt::QueuedConnection);
        connect(handler, &ClientHandler::handlerDisconnected,
                this, &Serve1::onHandlerDisconnected,
                Qt::QueuedConnection);

        // handler 的清理在 onHandlerDisconnected 中处理
        // （不能在 finished 用 deleteLater，因为那时事件循环已停止）

        // 5. 启动线程（线程进入事件循环）
        thread->start();

        ui->textEdit->append(QString("[Server] 新客户端连接: %1")
                             .arg(clientSocket->peerAddress().toString()));
    }
}

// 消息路由（在主线程中执行，无需加锁）
void Serve1::routeMessage(const QString &msg, ClientHandler *sender)
{
    if (msg.isEmpty()) return;

    // 根据协议首字符判断消息类型
    if (msg.startsWith("R|")) {
        // === 注册/登录 ===
        QString username = msg.mid(2);  // 去掉 "R|"
        if (username.isEmpty()) return;

        // 如果该用户名已存在，踢掉旧连接
        if (m_userMap.contains(username)) {
            ClientHandler *old = m_userMap.value(username);
            QMetaObject::invokeMethod(old, "writeMessage",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, "You have been kicked by another login."));
            // 旧连接稍后会被清理
        }

        // 注册到用户表
        sender->setUsername(username);
        m_userMap.insert(username, sender);

        ui->textEdit->append(QString("[Login] %1 上线啦！").arg(username));

        // 广播上线通知给所有在线用户
        QString notify = username + " 上线啦！";
        for (auto it = m_userMap.begin(); it != m_userMap.end(); ++it) {
            QMetaObject::invokeMethod(it.value(), "writeMessage",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, notify));
        }
    }
    else if (msg.startsWith("S|")) {
        // === 单对单通信: S|目标用户名|消息内容 ===
        // 用 section 取第1、2个 | 之间的内容，支持消息中带 | 字符
        QString targetName = msg.section('|', 1, 1);
        QString content    = msg.section('|', 2);    // 第2个|之后全部

        if (targetName.isEmpty()) return;

        auto it = m_userMap.find(targetName);
        if (it != m_userMap.end()) {
            // 原样转发消息内容（不附加任何前缀）
            QMetaObject::invokeMethod(it.value(), "writeMessage",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, content));
            ui->textEdit->append(QString("[Unicast] %1 → %2: %3")
                                 .arg(sender->username()).arg(targetName).arg(content));
        } else {
            // 目标用户不在线
            QMetaObject::invokeMethod(sender, "writeMessage",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, QString("用户 %1 不在线").arg(targetName)));
        }
    }
    else if (msg.startsWith("T|")) {
        // === 群发通信: T|消息内容 ===
        QString content = msg.mid(2);    // 去掉 "T|"

        for (auto it = m_userMap.begin(); it != m_userMap.end(); ++it) {
            // 跳过发送者自己
            if (it.value() == sender) continue;
            // 原样转发消息内容（不附加任何前缀）
            QMetaObject::invokeMethod(it.value(), "writeMessage",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, content));
        }
        ui->textEdit->append(QString("[Broadcast] %1: %2").arg(sender->username()).arg(content));
    }
    else {
        // 未知格式
        qDebug() << "[Server] 未知消息格式:" << msg;
    }
}

// 客户端断开连接（在主线程中执行）
void Serve1::onHandlerDisconnected(ClientHandler *handler)
{
    QString name = handler->username();
    if (!name.isEmpty()) {
        m_userMap.remove(name);
        ui->textEdit->append(QString("[Logout] %1 下线了").arg(name));

        // 广播下线通知
        QString notify = name + " 下线了！";
        for (auto it = m_userMap.begin(); it != m_userMap.end(); ++it) {
            QMetaObject::invokeMethod(it.value(), "writeMessage",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, notify));
        }
    }

    // 移除对应线程（先让 handler 自毁，再退出线程）
    QThread *thread = handler->thread();
    if (thread && thread != this->thread()) {
        m_threads.removeOne(thread);
        handler->deleteLater();   // 向工作线程投递删除事件（事件循环还在跑）
        thread->quit();           // 退出事件循环（会先处理完 deleteLater 再退出）
        thread->wait(3000);       // 等待线程结束
        delete thread;            // 安全删除线程对象
    }
}

// ==================== ClientHandler 客户端处理器 ====================

ClientHandler::ClientHandler(QTcpSocket *socket, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
{
    // socket 属于工作线程，直接连接信号/槽（同线程，直接连接）
    connect(m_socket, &QTcpSocket::readyRead,
            this, &ClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &ClientHandler::onDisconnected);
}

ClientHandler::~ClientHandler()
{
    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}

// 在 worker 线程中读取数据
void ClientHandler::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    QString msg = QString::fromUtf8(data);

    // 发射信号给主线程处理（QueuedConnection → 主线程的 routeMessage）
    emit messageReceived(msg, this);
}

// 客户端断开
void ClientHandler::onDisconnected()
{
    emit handlerDisconnected(this);
}

// 向客户端发送消息（在 worker 线程中执行）
void ClientHandler::writeMessage(const QString &msg)
{
    if (m_socket && m_socket->isOpen()) {
        m_socket->write(msg.toUtf8());
    }
}
