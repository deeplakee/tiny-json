#ifndef TINY_JSON_TEST_H
#define TINY_JSON_TEST_H

#include <cassert>

// 简单的测试框架
#define TEST(name) void test_##name()
#define RUN_TEST(name)                              \
    do {                                            \
        std::cout << "Running " << #name << "... "; \
        test_##name();                              \
        std::cout << "PASSED" << std::endl;         \
    } while (0)

#define ASSERT_EQ(a, b)        assert((a) == (b))
#define ASSERT_TRUE(expr)      assert(expr)
#define ASSERT_FALSE(expr)     assert(!(expr))
#define ASSERT_DOUBLE_EQ(a, b) assert(std::abs((a) - (b)) < 0.000001)
#define ASSERT_THROW(expr, exception_type) \
    do {                                   \
        bool caught = false;               \
        try {                              \
            expr;                          \
        } catch (const exception_type &) { \
            caught = true;                 \
        }                                  \
        assert(caught);                    \
    } while (0)

#endif // TINY_JSON_TEST_H
