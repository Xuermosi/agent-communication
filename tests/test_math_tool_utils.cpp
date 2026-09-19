#include <gtest/gtest.h>

#include "math_tool_utils.hpp"

namespace {

using agent_rpc::examples::extract_math_expression;
using agent_rpc::examples::is_usable_mcp_tool_result;

TEST(MathToolUtilsTest, ExtractsExpressionsFromChineseMathQuestions) {
    EXPECT_EQ(extract_math_expression("1+7"), "1+7");
    EXPECT_EQ(extract_math_expression("计算 123 乘以 456"), "123*456");
    EXPECT_EQ(extract_math_expression("2 的 10 次方是多少"), "2^10");
}

TEST(MathToolUtilsTest, RejectsMcpPayloadsThatContainToolErrors) {
    EXPECT_FALSE(is_usable_mcp_tool_result(
        R"({"isError":false,"content":[{"type":"text","text":"Error: Expected number"}]})"));
    EXPECT_TRUE(is_usable_mcp_tool_result(
        R"({"isError":false,"content":[{"type":"text","text":"123 * 456 = 56088"}]})"));
}

}  // namespace
