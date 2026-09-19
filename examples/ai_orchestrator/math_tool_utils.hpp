#pragma once

#include <optional>
#include <string>

namespace agent_rpc::examples {

std::optional<std::string> extract_math_expression(const std::string& question);

bool is_usable_mcp_tool_result(const std::string& result);

}  // namespace agent_rpc::examples
