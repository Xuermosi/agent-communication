#include "math_tool_utils.hpp"

#include <algorithm>
#include <cctype>
#include <regex>

#include <nlohmann/json.hpp>

namespace agent_rpc::examples {
namespace {

void replace_all(std::string& text, const std::string& from, const std::string& to) {
    size_t position = 0;
    while ((position = text.find(from, position)) != std::string::npos) {
        text.replace(position, from.length(), to);
        position += to.length();
    }
}

bool starts_with_error(const std::string& text) {
    return text.rfind("Error:", 0) == 0 || text.rfind("错误:", 0) == 0;
}

}  // namespace

std::optional<std::string> extract_math_expression(const std::string& question) {
    std::string normalized;
    normalized.reserve(question.size());
    for (unsigned char character : question) {
        if (!std::isspace(character)) {
            normalized.push_back(static_cast<char>(character));
        }
    }

    const std::regex power_pattern(
        R"((-?\d+(?:\.\d+)?)(?:的)?(-?\d+(?:\.\d+)?)(?:次方|次幂|幂))");
    normalized = std::regex_replace(normalized, power_pattern, "$1^$2");

    replace_all(normalized, "乘以", "*");
    replace_all(normalized, "乘", "*");
    replace_all(normalized, "×", "*");
    replace_all(normalized, "加上", "+");
    replace_all(normalized, "加", "+");
    replace_all(normalized, "减去", "-");
    replace_all(normalized, "减", "-");
    replace_all(normalized, "除以", "/");
    replace_all(normalized, "除", "/");

    const std::regex expression_pattern(
        R"(-?\d+(?:\.\d+)?(?:[+\-*/^]-?\d+(?:\.\d+)?)+)");
    std::smatch match;
    if (std::regex_search(normalized, match, expression_pattern)) {
        return match.str();
    }

    return std::nullopt;
}

bool is_usable_mcp_tool_result(const std::string& result) {
    try {
        const auto response = nlohmann::json::parse(result);
        if (!response.is_object() || response.value("isError", false)) {
            return false;
        }

        const auto content = response.find("content");
        if (content == response.end() || !content->is_array()) {
            return false;
        }

        for (const auto& item : *content) {
            if (item.value("type", "") == "text" && item.contains("text")) {
                const auto text = item["text"].get<std::string>();
                if (!starts_with_error(text)) {
                    return true;
                }
            }
        }
    } catch (const nlohmann::json::exception&) {
        return false;
    }

    return false;
}

}  // namespace agent_rpc::examples
