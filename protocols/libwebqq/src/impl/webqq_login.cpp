#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <regex>
#include <cstdint>
#include <iterator>
#include <algorithm>

#include <openssl/md5.h>
#include <openssl/rsa.h>
#include <openssl/engine.h>

#include "webqq_login.hpp"

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

	namespace {

		inline void md5(const unsigned char *input, size_t ilen, unsigned char output[16])
		{
			MD5(input, ilen, output);
		}


		/*
		* Encode a buffer into base64 format
		*/
		int base64_encode(unsigned char *dst, size_t *dlen,
			const unsigned char *src, size_t slen)
		{

			static const unsigned char base64_enc_map[64] =
			{
				'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
				'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
				'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
				'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
				'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
				'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
				'8', '9', '+', '/'
			};
			size_t i, n;
			int C1, C2, C3;
			unsigned char *p;

			if (slen == 0)
			{
				*dlen = 0;
				return(0);
			}

			n = (slen << 3) / 6;

			switch ((slen << 3) - (n * 6))
			{
			case  2: n += 3; break;
			case  4: n += 2; break;
			default: break;
			}

			if (*dlen < n + 1)
			{
				*dlen = n + 1;
				return(-1);
			}

			n = (slen / 3) * 3;

			for (i = 0, p = dst; i < n; i += 3)
			{
				C1 = *src++;
				C2 = *src++;
				C3 = *src++;

				*p++ = base64_enc_map[(C1 >> 2) & 0x3F];
				*p++ = base64_enc_map[(((C1 & 3) << 4) + (C2 >> 4)) & 0x3F];
				*p++ = base64_enc_map[(((C2 & 15) << 2) + (C3 >> 6)) & 0x3F];
				*p++ = base64_enc_map[C3 & 0x3F];
			}

			if (i < slen)
			{
				C1 = *src++;
				C2 = ((i + 1) < slen) ? *src++ : 0;

				*p++ = base64_enc_map[(C1 >> 2) & 0x3F];
				*p++ = base64_enc_map[(((C1 & 3) << 4) + (C2 >> 4)) & 0x3F];

				if ((i + 1) < slen)
					*p++ = base64_enc_map[((C2 & 15) << 2) & 0x3F];
				else *p++ = '=';

				*p++ = '=';
			}

			*dlen = p - dst;
			*p = 0;

			return(0);
		}



		static int to_binary(const char * hex_string, unsigned char * output)
		{
			size_t len = strlen(hex_string);
			if (len < 2 || (len % 2) != 0) {
				return -1;
			}

			auto _Hex = [](int ch) -> int {
				if (ch >= '0' && ch <= '9')
					return ch - '0';
				else if (ch >= 'A' && ch <= 'F')
					return (ch - 'A' + 10);
				else if (ch >= 'a' && ch <= 'f')
					return (ch - 'a' + 10);
				return (int)(-1);
			};

			for (size_t i = 0; i < len;) {
				*output = _Hex(hex_string[i++]);
				*output <<= 4;
				*output |= _Hex(hex_string[i++]);
				*output++;
			}
			return 0;
		}

		static void tea_encrypt_block(uint32_t plain[2], uint32_t key[4], uint32_t cipher[2])
		{
			auto _bswap = [](uint32_t x) -> uint32_t {
				return (((x) >> 24) | (((x) >> 8) & 0xff00) | (((x) << 8) & 0xff0000) | ((x) << 24));
			};

			uint32_t delta = 0x9E3779B9, sum = 0;
			uint32_t left = _bswap(plain[0]), right = _bswap(plain[1]);
			uint32_t a = _bswap(key[0]), b = _bswap(key[1]), c = _bswap(key[2]), d = _bswap(key[3]);
			int count = 16;

			while (count--) {
				sum += delta;
				left += ((right << 4) + a) ^ (right + sum) ^ ((right >> 5) + b);
				right += ((left << 4) + c) ^ (left + sum) ^ ((left >> 5) + d);
			}

			cipher[0] = _bswap(left);
			cipher[1] = _bswap(right);
		}

		static int tea_encrypt(const unsigned char * input, size_t sz, unsigned char * key, unsigned char * cipher, size_t * outlen)
		{
			size_t bytes_pad;
			size_t len;
			unsigned char * plain = NULL;
			int isHead = 1;
			uint64_t pre_cipher = 0;
			uint64_t * plain_ptr = NULL, *cipher_ptr = NULL;

			bytes_pad = (sz + 10) % 8;
			if (bytes_pad != 0){
				bytes_pad = 8 - bytes_pad;
			}

			len = sz + 10 + bytes_pad;

			if (!cipher || (*outlen < len)) {
				*outlen = len;
				return -1;
			}

			plain = (unsigned char *)malloc(len);
			memset(plain, 0, len);

			srand((unsigned int)time(NULL));

			plain[0] = (rand() & 0xF8) | bytes_pad;
			for (size_t i = 1; i <= (bytes_pad + 2); i++) {
				plain[i] = rand() & 0xFF;
			}

			memcpy(&plain[bytes_pad + 3], input, sz);

			plain_ptr = (uint64_t *)plain;
			cipher_ptr = (uint64_t *)cipher;

			for (size_t i = 0; i < len / 8; i++) {
				uint64_t tmp;

				if (isHead) {
					plain_ptr[i] ^= 0; //
					isHead = 0;
				}
				else {
					plain_ptr[i] ^= cipher_ptr[i - 1];
				}

				tea_encrypt_block((uint32_t *)&plain_ptr[i], (uint32_t*)key, (uint32_t *)&tmp);

				cipher_ptr[i] = tmp ^ pre_cipher;
				pre_cipher = plain_ptr[i];
			}
			*outlen = len;
			free(plain);

			return 0;
		}

		static void tx_rsa_encrypt(const unsigned char * input, size_t sz, unsigned char output[128])
		{
			static const char N[] = "F20CE00BAE5361F8FA3AE9CEFA495362FF7DA1BA628F64A347F0A8C012BF0B254A30CD92ABFFE7A6EE0DC424CB6166F8819EFA5BCCB20EDFB4AD02E412CCF579B1CA711D55B8B0B3AEB60153D5E0693A2A86F3167D7847A0CB8B00004716A9095D9BADC977CBB804DBDCBA6029A9710869A453F27DFDDF83C016D928B3CBF4C7";
			static const char E[] = "3";

			RSA * rsa;
			BIGNUM * bn, *be;

			rsa = RSA_new();

			bn = BN_new();
			be = BN_new();

			BN_hex2bn(&bn, N);
			BN_hex2bn(&be, E);

			rsa->n = bn;
			rsa->e = be;

			RSA_public_encrypt(sz, input, output, rsa, RSA_PKCS1_PADDING);

			//BN_free(bn);
			//BN_free(be);
			RSA_free(rsa);

			//rsa_context rsa;
			//const char * pers = "rsa_encrypt";
			//entropy_context entropy;
			//ctr_drbg_context ctr_drbg;


			//entropy_init(&entropy);
			//ctr_drbg_init(&ctr_drbg, entropy_func, &entropy, (unsigned char*)pers, strlen(pers));

			//rsa_init(&rsa, RSA_PKCS_V15, 0);
			//mpi_read_string(&rsa.N, 16, N);
			//mpi_read_string(&rsa.E, 16, E);
			//rsa.len = (mpi_msb(&rsa.N) + 7) >> 3;

			//rsa_pkcs1_encrypt(&rsa, ctr_drbg_random, &ctr_drbg, RSA_PUBLIC, sz,input, output);

			//rsa_free(&rsa);
			//ctr_drbg_free(&ctr_drbg);
			//entropy_free(&entropy);

		}

		std::string tx_password_encrypt(const std::string& pwd, const std::string& captcha, const std::string& salt)
		{
			unsigned char md5_pwd[16] = { 0 };
			unsigned char key_md5[16];
			unsigned char key[24] = { 0 };
			unsigned char cipher[512] = { 0 };
			size_t sz = sizeof(cipher); //
			char b64code[1024] = { 0 };
			size_t b64_sz = sizeof(b64code);

			struct
			{
				unsigned short len_rsa_md5_pwd;
				unsigned char rsa_md5_pwd[128];
				unsigned char salt[8]; //
				unsigned short len_captcha; //4
				char captcha[4];
			} plain = { 0 };


			plain.len_rsa_md5_pwd = 0x8000; // 128 
			plain.len_captcha = 0x0400; //4 验证码长度 
			memcpy(plain.captcha, captcha.c_str(), 4);

			md5((unsigned char*)pwd.c_str(), pwd.length(), md5_pwd);
			tx_rsa_encrypt(md5_pwd, 16, plain.rsa_md5_pwd);
			//
			to_binary(salt.c_str(), plain.salt);
			memcpy(key, md5_pwd, 16);
			memcpy(&key[16], plain.salt, 8);

			md5(key, 24, key_md5);

			//plain_hex = "0080" + to_hex_string(rsa_md5_pwd, 128) + salt + "0004" + to_hex_string((unsigned char *)vcode.c_str(),vcode.length());
			tea_encrypt((unsigned char *)&plain, sizeof(plain), key_md5, cipher, &sz);

			base64_encode((unsigned char*)b64code, &b64_sz, cipher, sz);

			std::transform(b64code, b64code + b64_sz, b64code, [](int ch) ->int { if (ch == '/') return '-'; else if (ch == '+') return '*'; else if (ch == '=') return '_'; else return ch; });

			return b64code;
		}	
	}

	std::string webqq_password_encode(const std::string& pwd, const std::string& vc, const std::string& salt)
	{
		return tx_password_encrypt(pwd, vc, boost::replace_all_copy(salt, "\\x", ""));
	}

}
}
}

