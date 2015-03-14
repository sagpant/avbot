
#pragma once

#include <string>
#include <vector>

namespace webqq {
namespace qqimpl {

std::string call_js_helper_function(std::string helper_file, std::string function, std::vector<std::string> args);

std::string call_js_helper_function_in_buffer(const char* js_content, int js_content_length, std::string helper_file, std::string function, std::vector< std::string > args);

} // namespace qqimpl
} // namespace webqq
