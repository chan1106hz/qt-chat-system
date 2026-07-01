#include <QtTest>
#include "tst_protocol.h"
#include "tst_format.h"

// ============================================================
// 测试入口 —— 依次执行两个测试套件
// 用法:
//   tests.exe                  → 运行全部测试
//   tests.exe -silent          → 只输出摘要
//   tests.exe -o result.xml    → 输出到 XML（CI 集成用）
// ============================================================

int main(int argc, char *argv[])
{
    int failures = 0;

    // 此处用 QTest::qExec 执行每个测试类，
    // 第一个参数传 nullptr 表示不使用自定义 main window

    TestProtocol tp;
    failures += QTest::qExec(&tp, argc, argv);

    TestFormat tf;
    failures += QTest::qExec(&tf, argc, argv);

    return failures > 0 ? 1 : 0;
}
