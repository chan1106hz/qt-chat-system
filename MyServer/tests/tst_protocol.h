#ifndef TST_PROTOCOL_H
#define TST_PROTOCOL_H

#include <QObject>
#include <QString>

// ============================================================
// 协议解析测试 —— 对应服务端 routeMessage() 中的字符串处理逻辑
// ============================================================

class TestProtocol : public QObject
{
    Q_OBJECT

private slots:
    // ============ R| 注册协议 ============
    void test_register_basic();           // R|用户名 → 提取用户名
    void test_register_chinese_name();    // R|婵婵
    void test_register_empty_name();      // R|  → 空用户名

    // ============ S| 私聊协议 ============
    void test_unicast_basic();            // S|小红|你好 → 解析目标+消息
    void test_unicast_pipe_in_message();  // S|小红|a|b|c → 消息不被截断
    void test_unicast_section_vs_split(); // section() 和 split() 的区别
    void test_unicast_empty_target();     // S||消息 → 目标为空
    void test_unicast_no_pipe();          // S| → 协议不完整

    // ============ T| 群聊协议 ============
    void test_broadcast_basic();          // T|大家好 → 提取消息
    void test_broadcast_empty_message();  // T| → 仅分隔符
    void test_broadcast_chinese();        // T|大家周末愉快

    // ============ 协议识别 ============
    void test_type_detection_R();         // 首字符 R → 注册
    void test_type_detection_S();         // 首字符 S → 私聊
    void test_type_detection_T();         // 首字符 T → 群聊
    void test_type_detection_unknown();   // 首字符 X → 未知
    void test_type_empty_string();        // 空字符串

    // ============ 边界情况 ============
    void test_long_username();            // 超长用户名
    void test_long_message();             // 超长消息内容
    void test_special_characters();       // 特殊字符 !@#$%
};

#endif // TST_PROTOCOL_H
