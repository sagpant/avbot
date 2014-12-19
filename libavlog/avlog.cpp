
#include <boost/format.hpp>
#include "avlog.hpp"

std::string avlog::html_escape(std::string txt)
{
	// escape html strings.
	// 将 < > 给转义.
	boost::replace_all(txt, "&", "&amp;");
	boost::replace_all(txt, "<", "&lt;");
	boost::replace_all(txt, ">", "&gt;");
	boost::replace_all(txt, " ", "&nbsp;");
	return txt;
}

bool avlog::add_log(const std::string& groupid, const std::string& msg, long int id)
{
	// 在qq群列表中查找已有的项目, 如果没找到则创建一个新的.
	ofstream_ptr file_ptr;
	loglist::iterator finder = m_group_list.find(groupid);

	if (m_group_list.find(groupid) == m_group_list.end())
	{
		// 创建文件.
		file_ptr = create_file(groupid);

		m_group_list[groupid] = file_ptr;
		finder = m_group_list.find(groupid);
	}

	// 如果没有找到日期对应的文件.
	if (!fs::exists(fs::path(make_filename(make_path(groupid)))))
	{
		// 创建文件.
		file_ptr = create_file(groupid);

		if (finder->second->is_open())
			finder->second->close();	// 关闭原来的文件.

		// 重置为新的文件.
		finder->second = file_ptr;
	}

	// 得到文件指针.
	file_ptr = finder->second;

	// 构造消息, 添加消息时间头.
	std::string data;

	if (id > 0)
	{
		data = boost::str(
			boost::format("<p id=\"%d\"> %s %s </p>\n")
			% id
			% current_time()
			% msg
		);
	}
	else
	{
		data = boost::str(
			boost::format("<p> %s %s </p>\n")
			% current_time()
			% msg
		);
	}

	// 写入聊天消息.
	file_ptr->write(data.c_str(), data.length());
	// 刷新写入缓冲, 实时写到文件.
	file_ptr->flush();

	file_ptr = m_lecture_file;

	if (file_ptr && m_lecture_groupid == groupid)
	{
		// 写入聊天消息.
		file_ptr->write(data.c_str(), data.length());
		// 刷新写入缓冲, 实时写到文件.
		file_ptr->flush();
	}

	return true;
}

avlog::ofstream_ptr avlog::create_file(const std::string& groupid) const
{
	// 生成对应的路径.
	std::string save_path = make_path(groupid);

	// 如果不存在, 则创建目录.
	if (!fs::exists(fs::path(save_path)))
	{
		if (!fs::create_directory(fs::path(save_path)))
		{
			std::cerr << "create directory " << save_path.c_str() << " failed!" << std::endl;
			return ofstream_ptr();
		}
	}

	// 按时间构造文件名.
	save_path = make_filename(save_path);

	// 创建文件.
	ofstream_ptr file_ptr(new std::ofstream(save_path.c_str(),
											fs::exists(save_path) ? std::ofstream::app : std::ofstream::out));

	if (file_ptr->bad() || file_ptr->fail())
	{
		std::cerr << "create file " << save_path.c_str() << " failed!" << std::endl;
		return ofstream_ptr();
	}

	// 写入信息头.
	std::string info = "<head><meta http-equiv=\"Content-Type\" content=\"text/plain; charset=UTF-8\">\n";
	file_ptr->write(info.c_str(), info.length());

	return file_ptr;
}
