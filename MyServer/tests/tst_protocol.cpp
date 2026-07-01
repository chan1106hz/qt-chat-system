#include "tst_protocol.h"
#include <QtTest>

// ============================================================
// 辅助函数：与服务端 routeMessage() 保持完全一致的解析逻辑
// ============================================================

// 判断消息类型（对应服务端的 startsWith 判断）
static QString detectType(const QString &msg)
{
    if (msg.isEmpty()) return "empty";
    if (msg.startsWith("R|")) return "register";
    if (msg.startsWith("S|")) return "unicast";
    if (msg.startsWith("T|")) return "broadcast";
    return "unknown";
}

// 解析 R| 帧 —— 提取用户名
static QString parseRegister(const QString &msg)
{
    return msg.mid(2);  // 去掉 "R|"
}

// 解析 S| 帧 —— 返回 {目标用户名, 消息内容}
static QPair<QString,QString> parseUnicast(const QString &msg)
{
    QString target  = msg.section('|', 1, 1);   // 第 1 个 | 到第 2 个 | 之间
    QString content = msg.section('|', 2);      // 第 2 个 | 之后全部
    return qMakePair(target, content);
}

// 解析 T| 帧 —— 提取消息内容
static QString parseBroadcast(const QString &msg)
{
    return msg.mid(2);  // 去掉 "T|"
}

// split 方式解析（旧版，有 Bug —— 消息含 | 会被截断）
static QPair<QString,QString> parseUnicast_split(const QString &msg)
{
    QStringList parts = msg.split('|');
    if (parts.size() < 3) return qMakePair(QString(), QString());
    return qMakePair(parts.at(1), parts.at(2));
}

// ============================================================
// R| 注册协议测试
// ============================================================

void TestProtocol::test_register_basic()
{
    QCOMPARE(parseRegister("R|xiaoming"), QString("xiaoming"));
}

void TestProtocol::test_register_chinese_name()
{
    QCOMPARE(parseRegister("R|婵婵"), QString("婵婵"));
}

void TestProtocol::test_register_empty_name()
{
    QCOMPARE(parseRegister("R|"), QString(""));
}

// ============================================================
// S| 私聊协议测试
// ============================================================

void TestProtocol::test_unicast_basic()
{
    QPair<QString,QString> r = parseUnicast("S|小红|你好");
    QCOMPARE(r.first,  QString("小红"));
    QCOMPARE(r.second, QString("你好"));
}

void TestProtocol::test_unicast_pipe_in_message()
{
    // 关键测试：消息内容含 | 不应被截断
    QPair<QString,QString> r = parseUnicast("S|小红|a|b|c");
    QCOMPARE(r.first,  QString("小红"));
    QCOMPARE(r.second, QString("a|b|c"));   // ✅ 完整保留
}

void TestProtocol::test_unicast_section_vs_split()
{
    // 对比 split() 和 section() 的结果差异
    QString msg = "S|小红|a|b|c";

    // split() 方式 —— 会截断（旧版 Bug）
    QPair<QString,QString> r1 = parseUnicast_split(msg);
    QCOMPARE(r1.first,  QString("小红"));
    QCOMPARE(r1.second, QString("a"));      // ❌ 只拿到 a

    // section() 方式 —— 正确保留
    QPair<QString,QString> r2 = parseUnicast(msg);
    QCOMPARE(r2.first,  QString("小红"));
    QCOMPARE(r2.second, QString("a|b|c"));  // ✅ 完整保留
}

void TestProtocol::test_unicast_empty_target()
{
    QPair<QString,QString> r = parseUnicast("S||你好");
    QCOMPARE(r.first,  QString(""));         // 目标为空
    QCOMPARE(r.second, QString("你好"));     // 消息正常
}

void TestProtocol::test_unicast_no_pipe()
{
    QPair<QString,QString> r = parseUnicast("S|");
    QCOMPARE(r.first,  QString(""));
    QCOMPARE(r.second, QString(""));
}

// ============================================================
// T| 群聊协议测试
// ============================================================

void TestProtocol::test_broadcast_basic()
{
    QCOMPARE(parseBroadcast("T|大家好"), QString("大家好"));
}

void TestProtocol::test_broadcast_empty_message()
{
    QCOMPARE(parseBroadcast("T|"), QString(""));
}

void TestProtocol::test_broadcast_chinese()
{
    QCOMPARE(parseBroadcast("T|大家周末愉快！"), QString("大家周末愉快！"));
}

// ============================================================
// 协议类型识别
// ============================================================

void TestProtocol::test_type_detection_R()
{
    QCOMPARE(detectType("R|小明"), QString("register"));
}

void TestProtocol::test_type_detection_S()
{
    QCOMPARE(detectType("S|小红|你好"), QString("unicast"));
}

void TestProtocol::test_type_detection_T()
{
    QCOMPARE(detectType("T|大家好"), QString("broadcast"));
}

void TestProtocol::test_type_detection_unknown()
{
    QCOMPARE(detectType("X|data"), QString("unknown"));
}

void TestProtocol::test_type_empty_string()
{
    QCOMPARE(detectType(""), QString("empty"));
}

// ============================================================
// 边界情况
// ============================================================

void TestProtocol::test_long_username()
{
    QString longName(500, 'a');  // 500 个字符的用户名
    QString msg = QString("R|%1").arg(longName);
    QCOMPARE(parseRegister(msg), longName);
}

void TestProtocol::test_long_message()
{
    QString longMsg(2000, 'x');
    QString msg = QString("T|%1").arg(longMsg);
    QCOMPARE(parseBroadcast(msg), longMsg);
}

void TestProtocol::test_special_characters()
{
    QPair<QString,QString> r = parseUnicast("S|user@#$|msg!@#$%^&*()");
    QCOMPARE(r.first,  QString("user@#$"));
    QCOMPARE(r.second, QString("msg!@#$%^&*()"));
}
