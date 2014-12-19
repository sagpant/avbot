/**
 * @file
 * @author
 * @date
 * 
 * usage:
 * 	auto_question question;
 * 	auto_question::value_qq_list list;
 * 
 * 	list.push_back("lovey599");
 * 
 * 	questioin.add_to_list(list);
 * 	questioin.on_handle_message(group, qqclient);
 */

#ifndef __QQBOT_AUTO_QUESTION_H__
#define __QQBOT_AUTO_QUESTION_H__

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#include <boost/noncopyable.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

class auto_welcome : public boost::noncopyable
{
public:
	typedef std::vector<std::string> value_qq_list;
	
	auto_welcome(std::string filename = "welcome.txt")
		: _filename(filename)
	{
		try
		{
			load_question();
		}
		catch (const std::exception& ex)
		{
			_welcome_message =  "欢迎加入本群.\n\
								[请在群对应的 log 文件夹下放置一个 welcome.txt 文件存放欢迎内容.]";
			std::cerr << "load questioin error." <<  ex.what() << std::endl;
		}
	}

	void add_to_list(value_qq_list list)
	{
		BOOST_FOREACH(std::string item, list)
		{
			_process_count.push_back(item);
		}
	}
	
	template<class Msgsender>
	void on_handle_message(Msgsender msgsender)
	{
		handle_question(msgsender);
	}
	
protected:
	void load_question()
	{
		std::ifstream file(_filename.c_str());

		std::size_t fsize = boost::filesystem::file_size(_filename);
		if (fsize > 81920){
			throw std::runtime_error("message file to big");
		}
		_welcome_message.resize(fsize);
		file.read(&_welcome_message[0], fsize);
	}
	template<class Msgsender>
	void handle_question(Msgsender msgsender) const
	{
		std::string str_msg_body = build_message();

		BOOST_FOREACH( std::string item, _process_count )
		{
			try
			{
				std::string str_msg = boost::str( boost::format( "%s%s %s\n %s%s" ) % item % "@" % str_msg_body % "@" % item);
				msgsender( str_msg );
			}
			catch( std::exception err )
			{
				std::cerr <<  err.what() <<  std::endl;
			}

		}
	}

	std::string build_message() const
	{
		return _welcome_message;
	}
	
private:
	std::string _filename;
	
	std::string _welcome_message;

	std::vector<std::string> _process_count;
};

#endif
