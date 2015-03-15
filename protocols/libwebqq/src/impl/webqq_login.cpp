
#include "webqq_login.hpp"
#include "webqq_calljs.hpp"

namespace webqq {
namespace qqimpl {
namespace detail {


/**
* I hacked the javascript file named comm.js, which received from tencent
* server, and find that fuck tencent has changed encryption algorithm
* for password in webqq3 . The new algorithm is below(descripted with javascript):
* var M=C.p.value; // M is the qq password
* var I=hexchar2bin(md5(M)); // Make a md5 digest
* var H=md5(I+pt.uin); // Make md5 with I and uin(see below)
* var G=md5(H+C.verifycode.value.toUpperCase());
*
* @param pwd User's password
* @param vc Verify Code. e.g. "!M6C"
* @param salt A string like "\x00\x00\x00\x00\x54\xb3\x3c\x53", NB: it
*        must contain 8 hexadecimal number, in this example, it equaled
*        to "0x0,0x0,0x0,0x0,0x54,0xb3,0x3c,0x53"
*
* @return Encoded password
*/
std::string webqq_password_encode(const std::string& pwd, const std::string& vc, const std::string& salt)
{
	return call_js_helper_function(":/js/encrypt.js", "encryption", { pwd, boost::replace_all_copy(salt, "\\", "-"), vc});
}

}
}
}

