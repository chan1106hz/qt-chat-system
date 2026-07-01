#ifndef TST_FORMAT_H
#define TST_FORMAT_H

#include <QObject>
#include <QString>

// ============================================================
// 客户端消息格式化测试 —— 对应 Client1::formatMessage() 逻辑
// ============================================================

class TestFormat : public QObject
{
    Q_OBJECT

private slots:
    // ============ 直接协议格式 → 原样发送 ============
    void test_direct_unicast();          // 输入 S|小红|你好 → 原样
    void test_direct_broadcast();        // 输入 T|大家好 → 原样

    // ============ 简写格式 → 自动转换 ============
    void test_short_unicast();           // @小红 你好 → S|小红|你好
    void test_short_unicast_long();      // @多多 吃饭了吗 → S|多多|吃饭了吗
    void test_short_no_space();          // @小红你好（无空格）→ 走群聊
    void test_short_at_only();           // @ → 走群聊
    void test_short_no_content();        // @小红 （有空格但无消息）→ 走群聊

    // ============ 普通文本 → 自动转群聊 ============
    void test_plain_to_broadcast();      // 你好 → T|你好
    void test_plain_chinese();           // 大家周末愉快 → T|大家周末愉快
    void test_empty_input();             // 空字符串 → 不发送

    // ============ 边界情况 ============
    void test_double_at();               // @@小红 在吗 → 不以正常 S 开头 → 群聊
    void test_at_in_middle();            // 你好@小红 → 不走私聊 → 群聊
    void test_whitespace_only();         // 只有空格 → trim 后为空
    void test_unicast_with_pipe();       // S|name|msg|extra → 原样发送（已有 ≥2 个|）
};

#endif // TST_FORMAT_H
