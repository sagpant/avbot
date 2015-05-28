
#include "webqq_hash.hpp"
#include "webqq_impl.hpp"

namespace{
	std::string hash_func_content;
}

namespace webqq {
namespace qqimpl {
namespace detail {

	std::string hash_p(const char* x, const char* K)
	{
		char a[4] = { 0 };
		int i;
#ifdef WIN32
		unsigned __int64 uin_n = _strtoui64(x, NULL, 10);
#else
		unsigned long long uin_n = strtoull(x, NULL, 10);
#endif
		for (i = 0; i < strlen(K); i++)
			a[i % 4] ^= K[i];
		char* j[] = { "EC", "OK" };
		char d[4];
		d[0] = (uin_n >> 24 & 255) ^ j[0][0];
		d[1] = (uin_n >> 16 & 255) ^ j[0][1];
		d[2] = (uin_n >> 8 & 255) ^ j[1][0];
		d[3] = (uin_n & 255) ^ j[1][1];
		char j2[8];
		for (i = 0; i < 8; i++)
			j2[i] = i % 2 == 0 ? a[i >> 1] : d[i >> 1];
		char a2[] = "0123456789ABCDEF";
		char d2[17] = { 0 };
		for (i = 0; i < 8; i++) {
			d2[i * 2] = a2[j2[i] >> 4 & 15];
			d2[i * 2 + 1] = a2[j2[i] & 15];
		}
		return d2;
	}

	std::string hash_auto(std::string x, std::string K)
	{
		return hash_p(x.c_str(), K.c_str());
	}

std::string hash_func(std::string x, std::string K)
{
	if (hash_func_content == R"javascript(function hash(x,K)
{x+="";for(var N=[],T=0;T<K.length;T++)N[T%4]^=K.charCodeAt(T);var U=["EC","OK"],V=[];V[0]=x>>24&255^U[0].charCodeAt(0);V[1]=x>>16&255^U[0].charCodeAt(1);V[2]=x>>8&255^U[1].charCodeAt(0);V[3]=x&255^U[1].charCodeAt(1);
U=[];for(T=0;T<8;T++)U[T]=T%2==0?N[T>>1]:V[T>>1];N=["0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F"];V="";for(T=0;T<U.length;T++){V+=N[U[T]>>4&15];V+=N[U[T]&15]}return V})javascript")
	{
		return hash_p(x.c_str(), K.c_str());
	}

	return hash_auto(x, K);
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

	hash_func_content += "\n{";

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
