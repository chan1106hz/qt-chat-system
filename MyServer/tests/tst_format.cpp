#include "tst_format.h"
#include <QtTest>

// ============================================================
// 辅助函数：与 Client1::formatMessage() 保持完全一致的逻辑
// ============================================================

static QString formatMessage(const QString &rawInput)
{
    QString raw = rawInput.trimmed();
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
            QString targetName = raw.mid(1, spaceIdx - 1);
            QString content = raw.mid(spaceIdx + 1);
            if (!targetName.isEmpty() && !content.isEmpty()) {
                return QString("S|%1|%2").arg(targetName).arg(content);
            }
        }
    }

    // 默认群发
    return QString("T|%1").arg(raw);
}

// ============================================================
// 直接协议格式 → 原样发送
// ============================================================

void TestFormat::test_direct_unicast()
{
    // 用户直接输入完整 S| 协议
    QCOMPARE(formatMessage("S|小红|你好"), QString("S|小红|你好"));
}

void TestFormat::test_direct_broadcast()
{
    // 用户直接输入完整 T| 协议
    QCOMPARE(formatMessage("T|大家好"), QString("T|大家好"));
}

// ============================================================
// 简写格式 → 自动转 S| 私聊
// ============================================================

void TestFormat::test_short_unicast()
{
    // @用户名 消息 → S|用户名|消息
    QCOMPARE(formatMessage("@小红 你好"), QString("S|小红|你好"));
}

void TestFormat::test_short_unicast_long()
{
    QCOMPARE(formatMessage("@多多 吃饭了吗"), QString("S|多多|吃饭了吗"));
}

void TestFormat::test_short_no_space()
{
    // @小红你好 → 没有空格 → fallthrough 到群聊
    QCOMPARE(formatMessage("@小红你好"), QString("T|@小红你好"));
}

void TestFormat::test_short_at_only()
{
    // @ → fallthrough
    QCOMPARE(formatMessage("@"), QString("T|@"));
}

void TestFormat::test_short_no_content()
{
    // @小红（有空格但无消息内容）→ fallthrough 到群聊
    QCOMPARE(formatMessage("@小红 "), QString("T|@小红"));
}

// ============================================================
// 普通文本 → 自动转 T| 群发
// ============================================================

void TestFormat::test_plain_to_broadcast()
{
    QCOMPARE(formatMessage("你好"), QString("T|你好"));
}

void TestFormat::test_plain_chinese()
{
    QCOMPARE(formatMessage("大家周末愉快"), QString("T|大家周末愉快"));
}

void TestFormat::test_empty_input()
{
    // 空输入 → 返回空 QString，不发送
    QCOMPARE(formatMessage(""), QString());
    QCOMPARE(formatMessage("   "), QString());   // 仅空格
}

// ============================================================
// 边界情况
// ============================================================

void TestFormat::test_double_at()
{
    // @@小红 在吗 → 空格在 index=4（小红后面），targetName=raw.mid(1,3)="@小红"
    // 实际行为：取出 "@小红" 作为目标用户名
    QString result = formatMessage("@@小红 在吗");
    QCOMPARE(result, QString("S|@小红|在吗"));

    // @@ 在吗 → 空格在 index=2，targetName=raw.mid(1,1)="@"
    // 只有一个 @ 字符被当作用户名
    QString result2 = formatMessage("@@ 在吗");
    QCOMPARE(result2, QString("S|@|在吗"));
}

void TestFormat::test_at_in_middle()
{
    // "你好@小红" → 不以 S|/T|/@ 开头 → 群聊
    QCOMPARE(formatMessage("你好@小红"), QString("T|你好@小红"));
}

void TestFormat::test_whitespace_only()
{
    QCOMPARE(formatMessage("   "), QString());
}

void TestFormat::test_unicast_with_pipe()
{
    // S|name|msg|extra → 已有 ≥2 个 | → 原样发送
    QCOMPARE(formatMessage("S|小红|a|b|c"), QString("S|小红|a|b|c"));
}
