
#include "webqq_hash.hpp"
#include "webqq_calljs.hpp"
#include "webqq_impl.hpp"

namespace{
	std::string hash_func_content;
}

namespace webqq {
namespace qqimpl {
namespace detail {

std::string hash_func(std::string x, std::string K)
{
	return call_js_helper_function_in_buffer(hash_func_content.c_str(), hash_func_content.length(), "hash.js", "hash", { x, K});
}

static void extract_hash_function(boost::system::error_code& ec, std::string);

void get_hash_file_op::operator()(boost::system::error_code ec, std::size_t bytes_transfered)
{
	std::string js_content;
	BOOST_ASIO_CORO_REENTER(this)
	{
		m_stream = std::make_shared<avhttp::http_stream>(m_webqq->get_ioservice());

		m_buffer = std::make_shared<boost::asio::streambuf>();

		BOOST_ASIO_CORO_YIELD avhttp::async_read_body(*m_stream, "http://pub.idqqimg.com/smartqq/js/mq.js", *m_buffer, *this);

		js_content.resize(bytes_transfered);

		m_buffer->sgetn(&js_content[0], bytes_transfered);

		// ok 提取 hash
		extract_hash_function(ec, js_content);

		m_handler(boost::system::error_code());
	}
}

static void extract_hash_function(boost::system::error_code& ec,  std::string md_js)
{
	// 首先，
	// 好了， 现在开始寻找 hash 函数

	auto getGroupList_pos = md_js.find("getGroupList=");

	auto x_hash_assigment_pos = md_js.find("x.hash=", getGroupList_pos);

	// 找到 x.hash= 后面的函数名字

	auto hash_line = md_js.substr(x_hash_assigment_pos, 12);

	boost::smatch what;
	boost::regex_search(hash_line, what, boost::regex(R"rawstring(x.hash=([^\(]+)\()rawstring"));

	std::string js_hash_function_name = what[1].str();

// 	start = what[0].second;
	boost::regex regex_used_to_find_hash_location;
	regex_used_to_find_hash_location.set_expression(boost::str(boost::format("[ ,\t]+%s[ ]*=[ ]*function") % js_hash_function_name));

	std::string::const_iterator start= md_js.begin(), end = md_js.end();

	boost::regex_search(start, end, what, regex_used_to_find_hash_location);

	std::string::const_iterator finded_location = what[0].second;

	// 现在 ， finded_location 指向应该是 ,u=function( 这个地方，
	// 开始提取 javascript 代码

	if (*finded_location==',')
		finded_location++;

	hash_func_content.reserve(500);

	hash_func_content="function hash";

	// 向前遍历，会依次看到 u = f u n c t i o n ( 参数 ) {
	// 看到 { 后开始计算

	do{
		hash_func_content.push_back(*finded_location);
		finded_location++;
	}while(*finded_location!='{');

	hash_func_content += "\n\{";

	int indent_code_block = 1;

	finded_location++;

	while(finded_location != md_js.cend())
	{
		switch(*finded_location)
		{
			case '{':
				indent_code_block ++;
				hash_func_content.push_back(*finded_location);
				break;
			case '}':
				indent_code_block --;
				hash_func_content.push_back(*finded_location);
				break;
			default:
				hash_func_content.push_back(*finded_location);
		}
		if(indent_code_block==0)
			break;
		finded_location ++;
	}
}


} // namespace detail
} // namespace qqimpl
} // namespace webqq
