/*
 * test.cpp
 *
 *  Created on: 2010-1-20
 *      Author: cai
 */
#include <list>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdio.h>


#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#define CAODAN(x) x
#else
#include <windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <wininet.h>
#define CAODAN(x) L##x
#endif

#include <sys/stat.h>
#include <sys/types.h>

#include <cstdio>
#include <string.h>

#ifdef HAVE_ZLIB
#include <zlib.h>
#else
#include "../miniz.c"
#endif

#include "qqwry/ipdb.hpp"

#pragma pack(1)


#pragma pack()

#define DEBUG

#ifdef _WIN32
static std::string internetDownloadFile(std::wstring url)
{
	std::shared_ptr<void> hinet(::InternetOpenW(0, INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0), InternetCloseHandle);
	std::shared_ptr<void> hUrl(
		::InternetOpenUrlW((HINSTANCE)hinet.get(), url.c_str(), 0, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0),
		::InternetCloseHandle
		);

	std::string filecontent;

	while (true)
	{
		// 读取文件
		std::vector<char> buf;

		buf.resize(1024);

		DWORD readed;

		InternetReadFile((HINTERNET)hUrl.get(), &buf[0], buf.size(), &readed);

		buf.resize(readed);

		if (readed == 0)
		{
			// 读取完毕，return
			return filecontent;
		}

		filecontent.append(buf.data(), readed);
	}
}
#else
// linux version
static std::string internetDownloadFile(std::string url)
{
	std::string cmd = std::string("curl ") + "\"" + url + "\"";
	// call wget
	std::shared_ptr<FILE> downloadpipe(popen(cmd.c_str(), "re"), pclose);

	std::string buf;
	buf.resize(20*1024*1024);

	buf.resize(std::fread(&buf[0], 1, buf.size(), downloadpipe.get()));
	return buf;
}
#endif

#if defined(_WIN32) || defined(_WIN64)
inline std::string wstring2string(std::wstring wstr)
{

	// 接着转换到本地编码

	size_t required_buffer_size = WideCharToMultiByte(CP_ACP, 0, &wstr[0], wstr.size(), NULL, 0, NULL, NULL);
	std::vector<char> outstr;
	outstr.resize(required_buffer_size);

	int converted_size = WideCharToMultiByte(CP_ACP, 0, wstr.data(), wstr.size(), &outstr[0], required_buffer_size, NULL, NULL);
	outstr.resize(converted_size);

	return std::string(outstr.data(), outstr.size());
//#else
	// TODO, use with iconv
	//return str;
}
#endif

#ifndef _WIN32

static bool check_exist(std::string filename)
{
	return access(filename.c_str(), O_RDONLY) ==  0;
}
#else
static bool check_exist(std::string filename)
{
	CLSID clsid;
	CLSIDFromProgID(CComBSTR("Scripting.FileSystemObject"), &clsid);
	void *p;
	CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER, __uuidof(IDispatch), &p);
	CComPtr<IDispatch> disp(static_cast<IDispatch*>(p));
	CComDispatchDriver dd(disp);
	CComVariant arg(filename.c_str());
	CComVariant ret(false);
	dd.Invoke1(CComBSTR("FileExists"), &arg, &ret);
	return ret.boolVal!=0;
}
#endif

// search for QQWry.Dat
// first, look for ./QQWry.Dat
// then look for /var/lib/QQWry.Dat
// then look for $EXEPATH/QQWry.Dat
std::string search_qqwrydat(const std::string exepath)
{
	if (check_exist("QQWry.Dat"))
	{
		return  "QQWry.Dat";
	}

	if (check_exist("/var/lib/QQWry.Dat"))
	{
		return "/var/lib/QQWry.Dat";
	}

	// 找 exe 的位置
	if (check_exist(exepath + "QQWry.Dat"))
	{
		return exepath + "QQWry.Dat";
	}
	//	throw std::runtime_error("QQWry.Dat database not found");

	std::cout << "qqwry.dat 文件没找到，下载中......" << std::endl;

	// 解压
	std::string deflated = QQWry::decodeQQWryDat(
		// 下载 copywrite.rar
		internetDownloadFile(CAODAN("http://update.cz88.net/ip/copywrite.rar")),
		// 下载 qqwry.rar
		internetDownloadFile(CAODAN("http://update.cz88.net/ip/qqwry.rar")),
		// 传入 解压函数
		uncompress
	);

	// 解压 qqwry.rar 为 qqwry.dat
	std::ofstream ofile("QQWry.Dat", std::ios::binary);
	ofile.write((const char*) deflated.data(), deflated.size());
	ofile.close();

	if (check_exist("QQWry.Dat"))
		return "QQWry.Dat";
throw  std::runtime_error("not found");
}

#ifdef _WIN32
std::string  get_exe_dir(std::string argv0)
{
	std::vector<char> Filename;
	Filename.resize(_MAX_FNAME);

	GetModuleFileName(NULL, &Filename[0], _MAX_FNAME);

	CLSID clsid;
	CLSIDFromProgID(CComBSTR("Scripting.FileSystemObject"), &clsid);
	void *p;
	CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER, __uuidof(IDispatch), &p);
	CComPtr<IDispatch> disp(static_cast<IDispatch*>(p));
	CComDispatchDriver dd(disp);
	CComVariant arg(Filename.data());
	CComVariant ret("");
	dd.Invoke1(CComBSTR("GetParentFolderName"), &arg, &ret);
	return wstring2string(ret.bstrVal);
}
#else
std::string  get_exe_dir(std::string argv0)
{
	std::string exefile;
	char* res =  realpath(argv0.c_str(),  NULL);
	exefile = res;
	free(res);
	//　处理到　 / 位置
	exefile.resize(exefile.find_last_of('/'));
	return exefile;
}
#endif

int main(int argc,char * argv[])
{
#ifdef _WIN32
	CoInitialize(NULL);
#endif

	std::string ipfile = search_qqwrydat(get_exe_dir(argv[0]));

	QQWry::ipdb iplook(ipfile.c_str());

	//使用 CIPLocation::GetIPLocation 获得对应 ip 地址的地区表示.
	in_addr ip;

	if (argc <= 2)
	{

		ip.s_addr = inet_addr(argc == 2 ? argv[1] : "8.8.8.8");
		if (ip.s_addr && ip.s_addr != (-1)) // 如果命令行没有键入合法的ip地址，就进行地址-》ip的操作
		{
			QQWry::IPLocation iplocation = iplook.GetIPLocation(ip);
			puts(iplocation.country);
			puts(iplocation.area);
		}

		return 0;
	}
	//使用 CIPLocation::GetIPs 获得匹配地址的所有 ip 区间
	std::list<QQWry::IP_regon> ipregon;

	char country[80],area[80];

	if(argc!=3)
		ipregon = iplook.GetIPs("浙江省温州市","*网吧*");
	else
		ipregon = iplook.GetIPs(argv[1],argv[2]);

	std::list<QQWry::IP_regon>::iterator it;

	//在一个循环中打印出来
	for(it=ipregon.begin(); it != ipregon.end() ; it ++)
	{
		char	ipstr[30];
		strcpy(ipstr,inet_ntoa(it->start));

		printf(  "%s to %s :%s %s\n",ipstr, inet_ntoa(it->end), it->location.country,it->location.area);
	}
#ifdef _WIN32
	CoUninitialize();
#endif
	return 0;
}


