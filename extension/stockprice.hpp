/*
 * Copyright (C) 2013  microcai <microcai@fedoraproject.org>
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

#ifndef __STOCK_HPP__
#define __STOCK_HPP__

#include <boost/locale.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>

#include <avhttp.hpp>
#include <avhttp/async_read_body.hpp>

#include "extension.hpp"

// 数据类型转换.
#define TYPE_CONVERT(val, type) do { \
	try { val = boost::lexical_cast<type> (str); } \
	catch (...) { val = 0.0f; } } while (false)

namespace stock {

// 个股当前数据.
typedef struct stock_data
{
    std::string stock_id;	// 股票代码.
    std::string stock_name; // 股票名称.
    int stock_state;		// 涨还是跌, 涨为1, 跌为-1, 平为0.
    double current_price;	// 当前价.
    double head_price[10];  // 最新买卖盘口, 5个卖盘, 5个买盘.
    double head_number[10]; // 最新买卖盘口数量, 同上.
    double outamt;	// 外盘.
    double inamt;	// 内盘.
    double before_close_price;	// 昨收盘.
    double current_open_price;	// 今开盘.
    double best_high_price;		// 最高价.
    double best_low_price;	// 最低价.
    double amplitude;	    // 振　幅.
    boost::int64_t volume;	// 成交量.
    boost::int64_t amount;	// 成交额.
    double total_market_value;  // 总市值.
    double pearnings;	        // 市盈率.
    double rising_limit;	    // 涨停价.
    double falling_limit;       // 跌停价.
    double vol_ratio;	// 量　比.
    double bs_ratio;	// 委　比.
    double bs_diff;		// 委　差.
    double pnetasset;	// 市净率.

    stock_data()
        : stock_id("")
        , stock_name("")
        , stock_state(0)
        , current_price(0.0f)
        , outamt(0.0f)
        , inamt(0.0f)
        , before_close_price(0.0f)
        , best_high_price(0.0f)
        , best_low_price(0.0f)
        , amplitude(0.0f)
        , volume(0)
        , amount(0)
        , total_market_value(0.0f)
        , pearnings(0.0f)
        , rising_limit(0.0f)
        , falling_limit(0.0f)
        , vol_ratio(0.0f)
        , bs_ratio(0.0f)
        , bs_diff(0.0f)
        , pnetasset(0.0f)
    {
        memset(head_price, 0, sizeof(head_price));
        memset(head_number, 0, sizeof(head_number));
    }

} stock_data;

// 大盘当前数据.
typedef struct stock_public
{
    std::string stock_name;	    // 上证或深证.
    std::string stock_id;	    // id.
    double current_price;	    // 当前价.
    double before_close_price;	// 昨收盘.
    double current_open_price;  // 今开盘.
    double best_high_price;		// 最高价.
    double best_low_price;	    // 最低价.
    double amplitude;	        // 振　幅.
    boost::int64_t turnover;	// 成交量.
    boost::int64_t business;	// 成交额.
    double rise;	// 上 涨.
    double fair;	// 持 平.
    double fell;	// 下 跌.

    stock_public()
        : stock_name("")
        , stock_id("")
        , current_price(0.0f)
        , before_close_price(0.0f)
        , current_open_price(0.0f)
        , best_high_price(0.0f)
        , best_low_price(0.0f)
        , amplitude(0.0f)
        , turnover(0)
        , business(0)
        , rise(0.0f)
        , fair(0.0f)
        , fell(0.0f)
    {}

} stock_public;

std::string to_price(double price)
{
	std::string ret = "元 ";
	if (price > 100000000.0f) {
		price /= 100000000.0f;
		ret = "亿元 ";
	} else if (price > 10000.0f) {
		price /= 10000.0f;
		ret = "万元 ";
	}
	ret = boost::str(boost::format("%0.2f%s") % price % ret);
	return ret;
}

// 分析个股数据.
bool parser_stock_data(std::string &data, stock_data &sd)
{
	boost::regex ex;
	boost::smatch what;
	std::string s(data);
	std::string::const_iterator start, end;
	int count = 0;

	start = s.begin();
	end = s.end();
	ex.assign("([^,;\"\']+)");

	while (boost::regex_search(start, end, what, ex, boost::match_default)) {
		if (what[1].first == what[1].second)
			break;
		int size = what.size();
		std::string str;
		for (int i = 1; i < size; i++) {
			str = std::string(what[i]);
			if (count == 1) {				// NAME.
				sd.stock_name = boost::locale::conv::between(boost::trim_copy(str) , "UTF-8", "gbk");
			} else if (count == 2) {		// 今.
				TYPE_CONVERT(sd.current_open_price, double);
			} else if (count == 3) {		// 昨.
				TYPE_CONVERT(sd.before_close_price, double);
			} else if (count == 4) {		// 当.
				TYPE_CONVERT(sd.current_price, double);
			} else if (count == 5) {		// 高.
				TYPE_CONVERT(sd.best_high_price, double);
			} else if (count == 6) {		// 低.
				TYPE_CONVERT(sd.best_low_price, double);
			} else if (count == 9) {		// 量.
				TYPE_CONVERT(sd.volume, boost::int64_t);
			} else if (count == 10) {		// 额.
				TYPE_CONVERT(sd.amount, boost::int64_t);
			} else if (count == 11) {		// m1...m5
				TYPE_CONVERT(sd.head_number[0], double);
			} else if (count == 12) {		// m1p...m5p
				TYPE_CONVERT(sd.head_price[0], double);
			} else if (count == 13) {		// m1...m5
				TYPE_CONVERT(sd.head_number[1], double);
			} else if (count == 14) {		// m1p...m5p
				TYPE_CONVERT(sd.head_price[1], double);
			} else if (count == 15) {		// m1...m5
				TYPE_CONVERT(sd.head_number[2], double);
			} else if (count == 16) {		// m1p...m5p
				TYPE_CONVERT(sd.head_price[2], double);
			} else if (count == 17) {		// m1...m5
				TYPE_CONVERT(sd.head_number[3], double);
			} else if (count == 18) {		// m1p...m5p
				TYPE_CONVERT(sd.head_price[3], double);
			} else if (count == 19) {		// m1...m5
				TYPE_CONVERT(sd.head_number[4], double);
			} else if (count == 20) {		// m1p...m5p
				TYPE_CONVERT(sd.head_price[4], double);
			} else if (count == 21) {		// m1...m5
				TYPE_CONVERT(sd.head_number[5], double);
			} else if (count == 22) {		// m1p...m5p
				TYPE_CONVERT(sd.head_price[5], double);
			} else if (count == 23) {		// m1...m5
				TYPE_CONVERT(sd.head_number[6], double);
			} else if (count == 24) {		// m1p...m5p
				TYPE_CONVERT(sd.head_price[6], double);
			} else if (count == 25) {		// m1...m5
				TYPE_CONVERT(sd.head_number[7], double);
			} else if (count == 26) {		// m1p...m5p
				TYPE_CONVERT(sd.head_price[7], double);
			} else if (count == 27) {		// m1...m5
				TYPE_CONVERT(sd.head_number[8], double);
			} else if (count == 28) {		// m1p...m5p
				TYPE_CONVERT(sd.head_price[8], double);
			} else if (count == 29) {		// m1...m5
				TYPE_CONVERT(sd.head_number[9], double);
			} else if (count == 30) {		// m1p...m5p
				TYPE_CONVERT(sd.head_price[9], double);
			}
		}
		start = what[0].second;
		count++;
	}

	if (count == 0)
		return false;

	return true;
}

// 分析大盘数据.
bool parser_stock_data_public(std::string &data, stock_public &sp)
{
	boost::regex ex;
	boost::smatch what;
	std::string s(data);
	std::string::const_iterator start, end;
	int count = 0;

	start = s.begin();
	end = s.end();
	ex.assign("([^,;\"\']+)");

	while (boost::regex_search(start, end, what, ex, boost::match_default)) {
		if (what[1].first == what[1].second)
			break;
		int size = what.size();
		std::string str, id;
		for (int i = 1; i < size; i++) {
			str = std::string(what[i]);
			if (count == 1) {				// NAME.
				sp.stock_name = boost::locale::conv::between(boost::trim_copy(str) , "UTF-8", "gbk");
			} else if (count == 2) {		// 今.
				TYPE_CONVERT(sp.current_open_price, double);
			} else if (count == 3) {		// 昨.
				TYPE_CONVERT(sp.before_close_price, double);
			} else if (count == 4) {		// 当.
				TYPE_CONVERT(sp.current_price, double);
			} else if (count == 5) {		// 高.
				TYPE_CONVERT(sp.best_high_price, double);
			} else if (count == 6) {		// 低.
				TYPE_CONVERT(sp.best_low_price, double);
			} else if (count == 9) {		// 量.
				TYPE_CONVERT(sp.turnover, boost::int64_t);
			} else if (count == 10) {		// 额.
				TYPE_CONVERT(sp.business, boost::int64_t);
			}
		}
		start = what[0].second;
		count++;
	}

	if (count == 0)
		return false;

	return true;
}

// 向 http://hq.sinajs.cn/?list=sh000001 查询A股指数.
template<class MsgSender>
struct stock_fetcher_op
{
	stock_fetcher_op(boost::asio::io_service &io, MsgSender s, std::string q)
	  : m_io_service(io)
	  , m_sender(s)
	  , m_stream(new avhttp::http_stream(io))
	  , m_query(q)
	  , m_status(0)
	{
		boost::trim(m_query);
		if (m_query == "上证指数" || m_query == "大盘" || m_query == "") {
			m_query = "000001";
		} else {
			// 检查股票参数是否是数字字符串, 如果不是, 则输出不支持的查询.
			for (std::string::iterator i = m_query.begin();
				i != m_query.end(); i++) {
				if (*i >= '0' && *i <= '9') {
					continue;
				} else {
					m_sender(std::string("avbot暂不支持该查询 " + m_query + " 格式, 请使用股票代码进行查询."));
					return;
				}
			}
		}

		// OK, 开始查询股票.
		std::string url = "http://hq.sinajs.cn/?list=sh" + m_query;
		buf = std::make_shared<boost::asio::streambuf>();
		avhttp::async_read_body(*m_stream, url, *buf, *this);
	}

	void operator()(boost::system::error_code ec, std::size_t bytes_transfered)
	{
		if (!ec || ec == boost::asio::error::eof) {

			std::string jscript;
			jscript.resize(bytes_transfered);
			buf->sgetn(&jscript[0], bytes_transfered);

			if (m_query == "000001") {
				stock_public sh;
				if (parser_stock_data_public(jscript, sh)) {
					double change_rate = ((sh.current_price - sh.before_close_price) / sh.before_close_price) * 100.0f;
					double change = sh.current_price - sh.before_close_price;
					double volume_of_trade = (double)sh.turnover / 100000000.0f;
					std::string amount = to_price(sh.business);
					std::string msg = boost::str(boost::format("%s: %0.2f 开盘价: %0.2f 最高价：%0.2f 最低价：%0.2f 涨跌幅: %0.2f%% 涨跌: %0.2f 成交量: %0.2f亿手 成交额: %s")
						% sh.stock_name % sh.current_price % sh.current_open_price %sh.best_high_price %sh.best_low_price % change_rate % change % volume_of_trade % amount);
					m_sender(msg);
				}
			} else {
				stock_data sd;
				if (parser_stock_data(jscript, sd)) {
					if (sd.stock_name == "") {
						if (m_status != 0) {
							m_sender(std::string("avbot没有查询到 " + m_query + " 相关信息."));
							return;
						}
						m_status++;
						// 上市没有查询到, 从深市查询.
						std::string url = "http://hq.sinajs.cn/?list=sz" + m_query;

						buf = std::make_shared<boost::asio::streambuf>();
						avhttp::async_read_body(*m_stream, url, *buf, *this);
						return;
					}
					double change_rate = ((sd.current_price - sd.before_close_price) / sd.before_close_price) * 100.0f;
					double change = sd.current_price - sd.before_close_price;
					double volume_of_trade = (double)sd.volume / 1000000.0f;
					std::string amount = to_price(sd.amount);
					std::string msg = boost::str(boost::format("%s: %0.2f 开盘价: %0.2f 最高价：%0.2f 最低价：%0.2f 涨跌幅: %0.2f%% 涨跌: %0.2f 成交量: %0.2f万手 成交额: %s")
						% sd.stock_name % sd.current_price % sd.current_open_price %sd.best_high_price %sd.best_low_price % change_rate % change % volume_of_trade % amount);
					m_sender(msg);
				}
			}
		}
	}

	std::shared_ptr<boost::asio::streambuf> buf;
	boost::asio::io_service & m_io_service;
	MsgSender m_sender;
	std::shared_ptr<avhttp::http_stream> m_stream;
	std::string m_query;
	int m_status;
};

template<class MsgSender>
void stock_fetcher(boost::asio::io_service & io_service, MsgSender sender, std::string query)
{
	stock_fetcher_op<MsgSender>(io_service, sender, query);
}

} // namespace stock

template<class MsgSender>
class stockprice
{
private:
	boost::asio::io_service &io_service;
	MsgSender m_sender;

public:
	stockprice(boost::asio::io_service &io, MsgSender sender)
	  : m_sender(sender)
	  , io_service(io)
	{}

	void operator()(const boost::system::error_code &error);
	void operator()(channel_identifier cid, avbotmsg msg, send_avchannel_message_t sender, boost::asio::yield_context yield_context)
	{
		std::string textmsg = msg.to_plain_text();

		boost::cmatch what;
		if (boost::regex_search(textmsg.c_str(), what, boost::regex(".qqbot 股票(.*)")))
		{
			stock::stock_fetcher(io_service, m_sender, std::string(what[1]));
		}
		if (boost::regex_search(textmsg.c_str(), what, boost::regex(".qqbot stock(.*)"))) {
			stock::stock_fetcher(io_service, m_sender, std::string(what[1]));
		}
	}
};

template<class MsgSender>
stockprice<typename boost::remove_reference<MsgSender>::type>
make_stockprice(boost::asio::io_service &io, MsgSender sender)
{
	return  stockprice<typename boost::remove_reference<MsgSender>::type>(io, sender);
}

#endif // __STOCK_HPP__
