#include "client1.h"
#include "ui_client1.h"
#include <QMessageBox>

// ==================== SocketWorker（在 worker 线程中运行）====================

SocketWorker::SocketWorker(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
{
}

SocketWorker::~SocketWorker()
{
    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}

void SocketWorker::connectToHost(const QString &host, quint16 port)
{
    // 在线程中创建 socket（确保 socket 属于此线程）
    if (!m_socket) {
        m_socket = new QTcpSocket();
        connect(m_socket, &QTcpSocket::readyRead,
                this, &SocketWorker::onReadyRead);
        connect(m_socket, &QTcpSocket::connected,
                this, &SocketWorker::socketConnected);
        connect(m_socket, &QTcpSocket::disconnected,
                this, &SocketWorker::socketDisconnected);
        connect(m_socket,
                QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
                this, [this](QAbstractSocket::SocketError) {
                    emit socketError(m_socket->errorString());
                });
    }
    m_socket->connectToHost(host, port);
}

void SocketWorker::disconnectFromHost()
{
    if (m_socket && m_socket->isOpen()) {
        m_socket->disconnectFromHost();
    }
}

void SocketWorker::sendMessage(const QString &msg)
{
    if (m_socket && m_socket->isOpen()) {
        m_socket->write(msg.toUtf8());
    }
}

void SocketWorker::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    emit dataReceived(data);
}

// ==================== Client1（UI 线程）====================

Client1::Client1(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Client1)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
    , m_connected(false)
{
    ui->setupUi(this);

    // 1. 创建 worker 线程和工作对象
    m_workerThread = new QThread(this);
    m_worker = new SocketWorker();   // 无 parent，之后 moveToThread
    m_worker->moveToThread(m_workerThread);

    // 2. 连接信号/槽（跨线程使用 QueuedConnection）
    connect(m_worker, &SocketWorker::dataReceived,
            this, &Client1::onDataReceived,
            Qt::QueuedConnection);
    connect(m_worker, &SocketWorker::socketConnected,
            this, &Client1::onConnected,
            Qt::QueuedConnection);
    connect(m_worker, &SocketWorker::socketDisconnected,
            this, &Client1::onDisconnected,
            Qt::QueuedConnection);
    connect(m_worker, &SocketWorker::socketError,
            this, &Client1::onError,
            Qt::QueuedConnection);

    // 3. 线程结束时清理 worker
    connect(m_workerThread, &QThread::finished,
            m_worker, &QObject::deleteLater);

    // 4. 启动工作线程的事件循环
    m_workerThread->start();
}

Client1::~Client1()
{
    // 断开连接
    if (m_connected) {
        QMetaObject::invokeMethod(m_worker, "disconnectFromHost",
                                  Qt::QueuedConnection);
    }
    // 退出并等待工作线程
    m_workerThread->quit();
    m_workerThread->wait(3000);
    delete ui;
}

// 连接到服务器
void Client1::on_pushButton_clicked()
{
    if (m_connected) {
        ui->textEdit->append("[Client] 已经连接到服务器了");
        return;
    }

    QString host = ui->lineEdit->text();
    quint16 port = ui->lineEdit_2->text().toInt();

    // 通过信号驱动 worker 线程去连接
    QMetaObject::invokeMethod(m_worker, "connectToHost",
                              Qt::QueuedConnection,
                              Q_ARG(QString, host),
                              Q_ARG(quint16, port));
}

// 断开连接
void Client1::on_pushButton_2_clicked()
{
    QMetaObject::invokeMethod(m_worker, "disconnectFromHost",
                              Qt::QueuedConnection);
}

// 发送消息
void Client1::on_pushButton_3_clicked()
{
    if (!m_connected) {
        QMessageBox::warning(this, "未连接", "请先连接到服务器！");
        return;
    }

    QString formatted = formatMessage();
    if (formatted.isEmpty()) return;

    // 通过信号驱动 worker 线程去发送
    QMetaObject::invokeMethod(m_worker, "sendMessage",
                              Qt::QueuedConnection,
                              Q_ARG(QString, formatted));

    // 清空输入区
    ui->textEdit_2->clear();
}

// 消息格式化：支持三种输入方式
//   1. S|用户名|消息  → 直接发送（私聊）
//   2. T|消息         → 直接发送（群聊）
//   3. @用户名 消息    → 自动转 S|用户名|消息（私聊）
//   4. 其他           → 自动转 T|消息（群聊）
QString Client1::formatMessage()
{
    QString raw = ui->textEdit_2->toPlainText().trimmed();
    if (raw.isEmpty()) return QString();

    // 已是完整协议格式，原样发送
    if (raw.startsWith("S|") && raw.count('|') >= 2) {
        return raw;   // S|用户名|消息
    }
    if (raw.startsWith("T|")) {
        return raw;   // T|消息
    }

    // @用户名 消息 → 转 S|用户名|消息
    if (raw.startsWith('@')) {
        int spaceIdx = raw.indexOf(' ');
        if (spaceIdx > 1) {
            QString targetName = raw.mid(1, spaceIdx - 1);  // 提取用户名
            QString content = raw.mid(spaceIdx + 1);         // 提取消息
            if (!targetName.isEmpty() && !content.isEmpty()) {
                return QString("S|%1|%2").arg(targetName).arg(content);
            }
        }
    }

    // 默认群发
    return QString("T|%1").arg(raw);
}

// 接收数据显示
void Client1::onDataReceived(const QByteArray &data)
{
    QString msg = QString::fromUtf8(data);
    ui->textEdit->append(msg);
}

// 连接成功
void Client1::onConnected()
{
    m_connected = true;
    ui->textEdit->append("[Client] ✓ 已连接到服务器");

    // 发送注册消息: R|用户名
    QString username = ui->lineEdit_3->text();
    if (!username.isEmpty()) {
        QString login = QString("R|%1").arg(username);
        QMetaObject::invokeMethod(m_worker, "sendMessage",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, login));
        // 窗口标题显示用户名
        setWindowTitle(username);
    }
}

// 断开连接
void Client1::onDisconnected()
{
    m_connected = false;
    ui->textEdit->append("[Client] 已断开连接");
}

// 错误处理
void Client1::onError(const QString &errorMsg)
{
    ui->textEdit->append(QString("[Client] 错误: %1").arg(errorMsg));
}
