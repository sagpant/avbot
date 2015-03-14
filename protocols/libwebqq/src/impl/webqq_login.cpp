
#include "webqq_login.hpp"
#include "tea.hpp"

#include "webqq_calljs.hpp"

namespace webqq {
namespace qqimpl {
namespace detail {

inline std::string util_md5_digest_str(const std::string & data)
{
	boost::hashes::md5::digest_type md5sum ;
	md5sum = boost::hashes::compute_digest<boost::hashes::md5>(data);
	return md5sum.str();
}

inline std::string util_md5_digest_raw(const std::string & data)
{
	boost::hashes::md5::digest_type md5sum ;
	md5sum = boost::hashes::compute_digest<boost::hashes::md5>(data);
	return std::string(reinterpret_cast<const char*>(md5sum.c_array()), md5sum.static_size);
}

inline static std::string RSA_public_encrypt(RSA* rsa, const std::string& from)
{
    std::string result;
    const int keysize = RSA_size(rsa);
    std::vector<unsigned char> block(keysize);
    const int chunksize = keysize  - RSA_PKCS1_PADDING_SIZE;
    int inputlen = from.length();

    for(int i = 0 ; i < inputlen; i+= chunksize)
    {
        auto resultsize = RSA_public_encrypt(std::min(chunksize, inputlen - i), (uint8_t*) &from[i],  &block[0], (RSA*) rsa, RSA_PKCS1_PADDING);
        result.append((char*)block.data(), resultsize);
    }
    return result;
}


/**
 * @brief 成加密用的 RSA 公钥
 *
 * @return RSA*
 */
inline static std::shared_ptr<RSA> new_tecent_RSA_pubkey()
{
	// 生成加密用的 RSA 公钥
	RSA* rsa = RSA_new();

	BIGNUM* rsa_n = BN_new();
	BIGNUM* rsa_e = BN_new();

	BN_hex2bn(&rsa_n, "F20CE00BAE5361F8FA3AE9CEFA495362FF7DA1BA628F64A347F0A8C012BF0B254A30CD92ABFFE7A6EE0DC424CB6166F8819EFA5BCCB20EDFB4AD02E412CCF579B1CA711D55B8B0B3AEB60153D5E0693A2A86F3167D7847A0CB8B00004716A9095D9BADC977CBB804DBDCBA6029A9710869A453F27DFDDF83C016D928B3CBF4C7");
	BN_set_word(rsa_e, 3);

	if (rsa->n)
		BN_free(rsa->n);
	rsa->n = rsa_n;
	if (rsa->e)
		BN_free(rsa->e);
	rsa->e = rsa_e;

	return std::shared_ptr<RSA>(rsa, RSA_free);
}


inline std::string uin_decode(const std::string &uin)
{
	int i;
	int uin_byte_length;
	char _uin[9] = {0};

	/* Calculate the length of uin (it must be 8?) */
	uin_byte_length = uin.length() / 4;

	/**
	 * Ok, parse uin from string format.
	 * "\x00\x00\x00\x00\x54\xb3\x3c\x53" -> {0,0,0,0,54,b3,3c,53}
	 */
	for( i = 0; i < uin_byte_length ; i++ ) {
		char u[5] = {0};
		char tmp;
		strncpy( u, & uin [  i * 4 + 2 ] , 2 );

		errno = 0;
		tmp = strtol( u, NULL, 16 );

		if( errno ) {
			return NULL;
		}

		_uin[i] = tmp;
	}
	return std::string(_uin, 8);
}

static std::string hexstring_to_bin(std::string md5string)
{
	typedef boost::archive::iterators::transform_width<
		boost::bin_from_hex<std::string::iterator>,
		8, 4, uint8_t> bin_from_hex_iterator;

	return std::string(bin_from_hex_iterator(md5string.begin()),
		bin_from_hex_iterator(md5string.end()));
}

static bool is_md5(std::string s)
{
	if (s.length() != 32)
		return false;
	// check for non hex code
	if (std::find_if_not(s.begin(), s.end(), boost::is_any_of("0123456789abcdefABCDEF")) != s.end())
		return false;
	return true;
}

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
	return call_js_helper_function("qrc://js/encrypt.js", "encryption", { pwd, boost::replace_all_copy(salt, "\\", "-"), vc});

	std::string md5pwd = pwd;
	if (!is_md5(pwd))
	{
		md5pwd = boost::to_upper_copy(util_md5_digest_str(pwd));
	}

	auto h1 = hexstring_to_bin(md5pwd);
	auto s2 = util_md5_digest_raw(h1 + uin_decode(salt));

	TEA tea(s2);

	auto tx_pubkey = new_tecent_RSA_pubkey();

	auto rsaH1 = tea.strToBytes(RSA_public_encrypt(tx_pubkey.get(), h1));

	std::string rsaH1Len, vcodeLen;
	{
		char _rsaH1Len[20] = {0};
		std::snprintf(_rsaH1Len, sizeof _rsaH1Len, "%X", (unsigned int) rsaH1.length() /2);
		rsaH1Len = _rsaH1Len;
		char _vcodeLen[20] = {0};
		std::snprintf(_vcodeLen, sizeof _vcodeLen, "000%X", (unsigned int) vc.length());
		vcodeLen = _vcodeLen;
	}

	while (rsaH1Len.length() < 4) {
		rsaH1Len = "0" + rsaH1Len;
	}


	auto hexVcode = tea.strToBytes(boost::to_upper_copy(vc));

	auto saltPwd = tea.enAsBase64(rsaH1Len + rsaH1 + tea.strToBytes(uin_decode(salt)) + vcodeLen + hexVcode);

	std::string saltPwd_replaced;

	std::transform(saltPwd.begin(), saltPwd.end(), std::back_inserter(saltPwd_replaced),[](const char c)
	{
		switch(c)
		{
			case '/':
				return '-';
			case '+':
				return '*';
			case '=':
				return '_';
			default:
				return c;
		}
	});

	return saltPwd_replaced;
}

}
}
}

