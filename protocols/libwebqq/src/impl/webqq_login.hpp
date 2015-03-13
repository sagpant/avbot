
/*
 * Copyright (C) 2012 - 2014  微蔡 <microcai@fedoraproject.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <iostream>

#include <boost/make_shared.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/json_parser.hpp>
namespace js = boost::property_tree::json_parser;
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

#include <avhttp.hpp>
#include <avhttp/async_read_body.hpp>

#include <boost/hash.hpp>

#include "boost/timedcall.hpp"
#include "boost/urlencode.hpp"
#include "boost/stringencodings.hpp"
#include "boost/bin_from_hex.hpp"

#include "webqq_impl.hpp"

#include "constant.hpp"

#include "webqq_status.hpp"
#include "webqq_group_qqnumber.hpp"
#include "webqq_group_list.hpp"
#include "webqq_buddy_info.hpp"
#include "webqq_vfwebqq.hpp"

#include "tea.hpp"

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

inline std::string generate_clientid()
{
	srand( time( NULL ) );
	return boost::str( boost::format( "%d%d%d" ) % (rand() % 90 + 10) % (rand() % 90 + 10) % (rand() % 90 + 10) );
}

// qq 登录办法-验证码登录
template<class Handler>
class SYMBOL_HIDDEN login_vc_op : boost::asio::coroutine{
public:
	login_vc_op(std::shared_ptr<qqimpl::WebQQ> webqq, std::string _vccode, Handler handler)
		: m_webqq(webqq), vccode(_vccode), m_handler(handler)
	{
		std::string md5 = webqq_password_encode(m_webqq->m_passwd, vccode, m_webqq->m_verifycode.uin);

		// do login !
		std::string url = boost::str(
							  boost::format(
								  "%s/login?u=%s&p=%s&verifycode=%s&"
								  "webqq_type=%d&remember_uin=1&aid=%s&login2qq=1&"
								  "&u1=http%%3A%%2F%%2Fweb2.qq.com%%2Floginproxy.html%%3Flogin2qq%%3D1%%26webqq_type%%3D10"
								  "&h=1&ptredirect=0&"
								  "ptlang=2052&from_ui=1&daid=164&pttype=1&dumy=&fp=loginerroralert&"
								  "action=0-7-26321&mibao_css=m_webqq&t=1&g=1"
								  "&js_type=0&js_ver=10114"
								  "&login_sig=%s"
								  "&pt_uistyle=5"
								  "&pt_randsalt=0"
								  "&pt_vcode_v1=0"
								  "&pt_verifysession_v1=%s")
							  % LWQQ_URL_LOGIN_HOST
							  % m_webqq->m_qqnum
							  % md5
							  % vccode
							  % m_webqq->m_status
							  % APPID
							  % m_webqq->m_login_sig
							  % m_webqq->pt_verifysession
						  );

		m_webqq->logger.dbg() << "login url is : " << url;

		m_stream = std::make_shared<avhttp::http_stream>(boost::ref(m_webqq->get_ioservice()));
		m_webqq->m_cookie_mgr.get_cookie(url, *m_stream);
		m_stream->request_options(
			avhttp::request_opts()
			(avhttp::http_options::connection, "close")
		);

		m_buffer = std::make_shared<boost::asio::streambuf>();

		avhttp::async_read_body(*m_stream, url, *m_buffer, *this);
	}

	// 在这里实现　QQ 的登录.
	void operator()(boost::system::error_code ec, std::size_t bytes_transfered=0)
	{
		BOOST_ASIO_CORO_REENTER(this)
		{
			if( ( check_login( ec, bytes_transfered ) == 0 ) && ( m_webqq->m_status == LWQQ_STATUS_ONLINE ) )
			{
				m_webqq->logger.info() <<  "redirecting to " << m_next_url;

				// 再次　login
				m_stream = std::make_shared<avhttp::http_stream>(boost::ref(m_webqq->get_ioservice()));
				m_webqq->m_cookie_mgr.get_cookie(m_next_url, *m_stream);
				m_stream->request_options(
					avhttp::request_opts()
					(avhttp::http_options::connection, "close")
					(avhttp::http_options::referer, "https://ui.ptlogin2.qq.com/cgi-bin/login?daid=164&target=self&style=16&mibao_css=m_webqq&appid=501004106&enable_qlogin=0&no_verifyimg=1&s_url=http%3A%2F%2Fw.qq.com%2Floginproxy.html&f_url=loginerroralert&strong_login=1&login_state=10&t=20131024001")
					(avhttp::http_options::accept, "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8")
					(avhttp::http_options::user_agent, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/29.0.1547.32 Safari/537.36")
				);

				m_stream->max_redirects(0);

				m_buffer = std::make_shared<boost::asio::streambuf>();

				BOOST_ASIO_CORO_YIELD avhttp::async_read_body(
					* m_stream, m_next_url, *m_buffer,	*this);

				m_webqq->m_cookie_mgr.save_cookie(*m_stream);

				m_buffer = std::make_shared<boost::asio::streambuf>();

				m_webqq->m_cookie_mgr.get_cookie(m_stream->location(), *m_stream);

				if (ec == avhttp::errc::found){
					m_stream->request_options(
						avhttp::request_opts()
						(avhttp::http_options::connection, "close")
						(avhttp::http_options::accept, "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8")
						(avhttp::http_options::user_agent, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/29.0.1547.32 Safari/537.36")
					);

					m_webqq->logger.info() <<  "redirected again to " <<  m_stream->location();
					BOOST_ASIO_CORO_YIELD avhttp::async_read_body(*m_stream, m_stream->location(), *m_buffer, *this);
					m_webqq->m_cookie_mgr.save_cookie(*m_stream);
				}

				m_webqq->logger.info() <<  "redirecting success!!";

				if (m_webqq->m_clientid.empty())
				{
					m_webqq->m_clientid = generate_clientid();
					m_webqq->m_cookie_mgr.save_cookie(
						"psession.qq.com", "/", "clientid", m_webqq->m_clientid, "session"
					);
				}
				//change status,  this is the last step for login
				// 设定在线状态.

				//  get vfwebqq
				BOOST_ASIO_CORO_YIELD async_update_vfwebqq(m_webqq, std::bind<void>(*this, std::placeholders::_1, 0));

				m_webqq->logger.info() <<  "changing status...";

				BOOST_ASIO_CORO_YIELD async_change_status(
					m_webqq, LWQQ_STATUS_ONLINE,
					std::bind<void>(*this, std::placeholders::_1, 0)
				);

				if(ec)
				{
					m_webqq->logger.err() <<  "change online status failed!";

					// 修改在线状态失败!
					m_webqq->get_ioservice().post(boost::asio::detail::bind_handler(m_handler, ec));
					return;
				}

				m_webqq->logger.info() <<  "status => online";

				// 先是 刷新 buddy 列表
				m_webqq->logger.info() <<  "fetching buddy list";
				BOOST_ASIO_CORO_YIELD async_update_buddy_list(m_webqq, *this);
				m_webqq->logger.info() <<  "fetching buddy complete";

				if (!m_webqq->m_fetch_groups)
				{
					m_webqq->logger.info() <<  "group fetching disable.";
					return m_webqq->get_ioservice().post(
						boost::asio::detail::bind_handler(m_handler, ec)
					);
				}
				//polling group list
				i = 0;

				// 重试五次，每次延时，如果还失败， 只能说登录失败.
				do{
					BOOST_ASIO_CORO_YIELD update_group_list(m_webqq, std::bind<void>(*this, std::placeholders::_1, 0));
					if ( ec )
					{
						m_webqq->logger.warn() << "刷新群列表失败，第 " << i << " 次重试中(共五次)...";

						BOOST_ASIO_CORO_YIELD boost::delayedcallsec(
							m_webqq->get_ioservice(), i*20+50,
							boost::asio::detail::bind_handler(*this, ec, 0)
						);

					}
				}while (i++ < 5 && ec);
				if (ec){
					// 刷群列表失败，登录也就失败了.
					return m_webqq->get_ioservice().post(
						boost::asio::detail::bind_handler(m_handler, ec)
					);
				}

				m_webqq->logger.info() <<  "group fetching numbers......";

				// 接着是刷新群成员列表.
				for (iter = m_webqq->m_groups.begin(); iter != m_webqq->m_groups.end(); ++iter)
				{
					BOOST_ASIO_CORO_YIELD
						m_webqq->update_group_member(iter->second , std::bind<void>(*this, std::placeholders::_1, 0));

					BOOST_ASIO_CORO_YIELD
						boost::delayedcallms(m_webqq->get_ioservice(), 130, std::bind<void>(*this, ec, 0));
				}

				m_webqq->logger.info() <<  "group numbers fetched";
			}
			return m_webqq->get_ioservice().post(
				boost::asio::detail::bind_handler(m_handler, ec)
			);
		}
	}

private:
	bool is_md5(std::string s)
	{
		if (s.length() != 32)
			return false;
		// check for non hex code
		if (std::find_if_not(s.begin(), s.end(), boost::is_any_of("0123456789abcdefABCDEF")) != s.end())
			return false;
		return true;
	}

	std::string hexstring_to_bin(std::string md5string)
	{
		typedef boost::archive::iterators::transform_width<
			boost::bin_from_hex<std::string::iterator>,
			8, 4, uint8_t> bin_from_hex_iterator;

		return std::string(bin_from_hex_iterator(md5string.begin()),
			bin_from_hex_iterator(md5string.end()));
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
	std::string webqq_password_encode(const std::string & pwd, const std::string & vc, const std::string & salt)
	{
		std::string md5pwd = pwd;
		if (!is_md5(pwd))
		{
			md5pwd = util_md5_digest_str(pwd);
		}

		auto h1 = hexstring_to_bin(md5pwd);
		auto s2 = util_md5_digest_str(h1 + salt);


		auto tx_pubkey = new_tecent_RSA_pubkey();

		auto rsaH1 = RSA_public_encrypt(tx_pubkey.get(), h1);

		std::string rsaH1Len, vcodeLen;
		{
			char _rsaH1Len[20] = {0};
			std::snprintf(_rsaH1Len, sizeof _rsaH1Len, "%X", (unsigned int) rsaH1.length());
			rsaH1Len = _rsaH1Len;
			char _vcodeLen[20] = {0};
			std::snprintf(_vcodeLen, sizeof _vcodeLen, "000%X", (unsigned int) vc.length());
			vcodeLen = _vcodeLen;
		}

		while (rsaH1Len.length() < 4) {
            rsaH1Len = "0" + rsaH1Len;
        }

        TEA tea(s2);

 		auto hexVcode = tea.strToBytes(boost::to_upper_copy(vc));

		auto saltPwd = tea.enAsBase64(rsaH1Len + rsaH1 + tea.strToBytes(salt) + vcodeLen + hexVcode);

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

private:
	int check_login(boost::system::error_code & ec, std::size_t bytes_transfered)
	{
		std::string response;
		response.resize(bytes_transfered);
		m_buffer->sgetn(&response[0], bytes_transfered);

		m_webqq->logger.dbg() << utf8_to_local_encode(response);

		int status;
		boost::cmatch what;
		boost::regex ex("ptuiCB\\('([0-9]+)',[ ]?'([0-9])',[ ]?'([^']*)',[ ]?'([0-9])',[ ]?'([^']*)',[ ]?'([^']*)'[ ]*\\);");

		if(boost::regex_search(response.c_str(), what, ex))
		{
			status = boost::lexical_cast<int>(what[1]);
			m_webqq->m_nick = what[6];
			m_next_url = what[3];
		}else
			status = 9;

		if ( (status >= 0 && status <= 8) || status == error::login_failed_blocked_account)
		{
			ec = error::make_error_code(static_cast<error::errc_t>(status));
		}else{
			ec = error::login_failed_other;
		}

		if (!ec){
			m_webqq->m_status = LWQQ_STATUS_ONLINE;
			m_webqq->m_cookie_mgr.save_cookie(*m_stream);
			m_webqq->logger.info() <<  "login success!";
		}else{
			status = LWQQ_STATUS_OFFLINE;
			m_webqq->logger.info() <<  "login failed!!!!  " <<  ec.message();
		}

		return status;
	}

private:
	std::shared_ptr<qqimpl::WebQQ> m_webqq;
	Handler m_handler;
	std::string m_next_url;

	read_streamptr m_stream;
	std::shared_ptr<boost::asio::streambuf> m_buffer;
	std::string vccode;

private:
	int i;
	grouplist::iterator iter;
};

template<class Handler>
login_vc_op<Handler> make_async_login_vc_op(std::shared_ptr<qqimpl::WebQQ> webqq, std::string _vccode, Handler handler)
{
	return login_vc_op<Handler>(webqq, _vccode, handler);
}

} // namespace detail


template<class Handler>
void async_login(std::shared_ptr<qqimpl::WebQQ> webqq, std::string _vccode, Handler handler)
{
	detail::make_async_login_vc_op(webqq, _vccode, handler);
}

} // namespace qqimpl
} // namespace webqq
