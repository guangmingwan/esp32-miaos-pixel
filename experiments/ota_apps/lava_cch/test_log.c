// ==================== test_log.c ====================
// 单元测试日志系统
// 使用 printf 直接输出到控制台

// 测试计数器
int test_total_count = 0;
int test_pass_count = 0;
int test_fail_count = 0;

// 初始化日志
void test_log_init()
{
    test_total_count = 0;
    test_pass_count = 0;
    test_fail_count = 0;

    printf("========================================\n");
    printf("LavaX 中国象棋 AI 单元测试报告\n");
    printf("========================================\n");
}

// 记录测试开始
void test_log_section(char section_name[])
{
    printf("\n[%s]\n", section_name);
}

// 记录单个测试结果
void test_log_result(char test_name[], int passed, char detail[])
{
    test_total_count = test_total_count + 1;

    if (passed != 0) {
        test_pass_count = test_pass_count + 1;
        printf("  [PASS] %s", test_name);
    } else {
        test_fail_count = test_fail_count + 1;
        printf("  [FAIL] %s", test_name);
    }

    if (strlen(detail) > 0) {
        printf(" - %s", detail);
    }
    printf("\n");
}

// 断言整数相等
int test_assert_int_eq(char test_name[], int expected, int actual)
{
    char detail[64];
    int passed;

    passed = (expected == actual) ? 1 : 0;

    if (passed == 0) {
        sprintf(detail, "expected=%d, actual=%d", expected, actual);
    } else {
        sprintf(detail, "value=%d", actual);
    }

    test_log_result(test_name, passed, detail);
    return passed;
}

// 断言整数在范围内
int test_assert_int_range(char test_name[], int min_val, int max_val, int actual)
{
    char detail[64];
    int passed;

    passed = (actual >= min_val && actual <= max_val) ? 1 : 0;

    if (passed == 0) {
        sprintf(detail, "range=[%d,%d], actual=%d", min_val, max_val, actual);
    } else {
        sprintf(detail, "value=%d (in range)", actual);
    }

    test_log_result(test_name, passed, detail);
    return passed;
}

// 断言条件为真
int test_assert_true(char test_name[], int condition, char detail[])
{
    test_log_result(test_name, condition ? 1 : 0, detail);
    return condition ? 1 : 0;
}

// 输出测试总结
void test_log_summary()
{
    printf("\n========================================\n");
    printf("测试总结: 总计 %d, 通过 %d, 失败 %d\n",
            test_total_count, test_pass_count, test_fail_count);

    if (test_fail_count == 0) {
        printf("状态: 所有测试通过!\n");
    } else {
        printf("状态: 存在失败的测试!\n");
    }
    printf("========================================\n");
}
