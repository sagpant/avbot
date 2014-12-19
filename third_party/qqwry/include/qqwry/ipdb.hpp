// IPLocation.h: interface for the CIPLocation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IPLOCATION_H__80D0E230_4815_4D01_9CCF_4DAF4DE175E8__INCLUDED_)
#define AFX_IPLOCATION_H__80D0E230_4815_4D01_9CCF_4DAF4DE175E8__INCLUDED_

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#include <map>
#include <list>
#include <stdint.h>
#include <cstdlib>

#if __cplusplus >= 201103L
#define _HAVE_CXX11
#else

#ifdef _MSC_VER

#if _MSC_VER >= 1700
#define _HAVE_CXX11
#endif
#endif
#endif

#ifdef _HAVE_CXX11
#include <memory>
#include <functional>
#else
#include <tr1/memory>
#include <tr1/shared_ptr.h>
#include <tr1/functional>
namespace std{ using namespace std::tr1; }
#endif

#ifdef _WIN32

#include <windows.h>
typedef UINT32	uint32_t;
#else
#include <stdint.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <iconv.h>
#endif

namespace QQWry{
namespace detail{

static inline uint32_t to_hostending(uint32_t v)
{
	uint32_t ret;
#if BIGENDIAN
	ret  = (v & 0x000000ff) << 24;
	ret |= (v & 0x0000ff00) << 8 ;
	ret |= (v & 0x00ff0000) >> 8;
	ret |= (v & 0xff000000) >> 24;
#else
	ret = v;
#endif
	return ret;
}

static inline uint32_t  Get3BYTEptr(char const* var_ptr)
{
	uint32_t ret = (* (uint32_t*) var_ptr) & 0xffffff;
	return to_hostending(ret);
}

static inline int IP_IN_regon(uint32_t ip, uint32_t ip1, uint32_t ip2)
{
	if (ip < ip1)
	{
		return -1;
	}
	else if (ip > ip2)
	{
		return 1;
	}
	return 0;
}

#ifndef _WIN32
static inline int code_convert(char* outbuf, size_t outlen, char* inbuf, size_t inlen)
{
	iconv_t cd;
	char** pin = &inbuf;
	char** pout = &outbuf;

	cd = iconv_open("UTF-8", "GBK");
	if (cd == 0)
		return -1;

	memset(outbuf, '\0', outlen);
	if (iconv(cd, pin, &inlen, pout, &outlen) == (size_t) - 1)
	{
		return -1;
	}
	iconv_close(cd);

	return 0;
}

static inline int utf8_gbk(char* outbuf, size_t outlen, char* inbuf, size_t inlen)
{
	iconv_t cd;
	char** pin = &inbuf;
	char** pout = &outbuf;

	cd = iconv_open("GBK", "UTF-8");
	if (cd == 0)
		return -1;

	memset(outbuf, '\0', outlen);
	if (iconv(cd, pin, &inlen, pout, &outlen) == (size_t) - 1)
	{
		return -1;
	}
	iconv_close(cd);

	return 0;
}

#endif

#pragma pack(1)

struct copywritetag{
	uint32_t sign;// "CZIP"
	uint32_t version;//一个和日期有关的值
	uint32_t unknown1;// 0x01
	uint32_t size;// qqwry.rar大小
	uint32_t unknown2;
	uint32_t key;// 解密qqwry.rar前0x200字节所需密钥
	char text[128];//提供商
	char link[128];//网址
};

class  _offset_
{
public:
	char _offset[3];
	inline operator size_t()
	{
		return ((* (uint32_t*) _offset) & 0xFFFFFF);
	}
};


typedef struct
{
	uint32_t ip;
	_offset_ offset;
} RECORD_INDEX;

#pragma pack()

static inline bool searchMatch(unsigned int& i, unsigned int& it, const char* str, const char* cstr)
{
	size_t si, j;
	size_t len_str;

	len_str = strlen(str);

	for (si = i; cstr[si] != 0; si++)
	{
		for (it = si, j = 0; (j < len_str)  && cstr[it] != 0 && ('?' == str[j] || (cstr[it] == str[j])); j++)
			it++;
		if (j >= len_str)
		{
			i = si;
			return true;
		}
	}
	return false;
}



static inline bool match_exp(char str[], char exp[])
{

	if (str[0] == 0)
	{
		int j;
		for (j = 0; exp[j] && exp[j] == '*';)
			j++;
		return exp[j] == 0;
	}
	else if (exp[0] == 0)
		return false;

	unsigned i, j, it, jt;
	for (i = j = 0; str[i] != 0 && exp[j] != 0 && exp[j] != '*'; i++, j++)
	{
		if (exp[j] != '?' && str[i] != exp[j])
			return false;
	}
	if (str[i] == 0)
	{
		for (; exp[j] && exp[j] == '*';)
			j++;
		return exp[j] == 0;
	}
	else if (exp[j] == 0)
		return false;

	std::string lastSubstr;
	for (jt = j; exp[jt] != 0 ; i = it, j = jt)
	{
		for (; exp[j] != 0 && exp[j] == '*';)
			j++;
		if (exp[j] == 0)
			break;
		for (jt = j; exp[jt] != 0 && exp[jt] != '*';)
			jt++;

		std::string tmp(&exp[j], &exp[jt]);

		if (!searchMatch(i, it, tmp.c_str(), str))
			return false;
		lastSubstr = tmp;
	}

	if (exp[j - 1] == '*')
		return true;
	for (i = strlen(str) - lastSubstr.length(), j = 0; j < lastSubstr.length(); i++, j++)
		if (lastSubstr[j] != '?' && str[i] != lastSubstr[j])
			return false;
	return true;
}

} // namespace detail

struct IPLocation
{
	char country[128];
	char area[128];
};

struct IP_regon
{
	in_addr start;
	in_addr end;
	IPLocation	location;
};

class ipdb
{
	struct auto_file_handle
	{
#ifdef _WIN32
		HANDLE m_fd;
#else
		int m_fd;
#endif // _WIN32
		void do_close()
		{
#ifdef _WIN32
			if (m_fd!=INVALID_HANDLE_VALUE)
				CloseHandle(m_fd);
#else
			if (m_fd >=0)
				close(m_fd);
#endif
		}

		~auto_file_handle()
		{
			do_close();
		}

		auto_file_handle()
#ifdef _WIN32
			:m_fd(INVALID_HANDLE_VALUE)
#else
			:m_fd(-1)
#endif
		{
		}

#ifdef _WIN32
		explicit auto_file_handle(HANDLE _fd)
#else
		explicit auto_file_handle(int _fd)
#endif // _WIN32
			: m_fd(_fd)
		{
		}

#ifdef _WIN32
		void operator =(HANDLE _fd)
		{
			do_close();
			m_fd = _fd;
		}
#else
		void operator =(int _fd)
		{
			do_close();
			m_fd = _fd;
		}

#endif // _WIN32

#ifdef _WIN32
		HANDLE
#else
		int
#endif // _WIN32
		get_fd()
		{
			return m_fd;
		}

#if !defined(BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)
		explicit
#endif
		operator bool() const
		{
#ifdef _WIN32
			return m_fd != INVALID_HANDLE_VALUE;
#else
			return m_fd >= 0;
#endif
		}
	};

	enum
	{
		REDIRECT_MODE_1 = 1,
		REDIRECT_MODE_2 = 2
	};

private: // private member
	uint32_t m_filesize;
	const char* m_file;
	const char* m_curptr;
	size_t m_first_record;
	size_t m_last_record;

#ifdef _WIN32
	auto_file_handle m_filemap;
#else
	bool   m_backup_byfile;
#endif // _WIN32

protected:
	IPLocation GetIPLocation(char const* ptr)
	{
		IPLocation ret;
		char const* areaptr = 0;
		switch (ptr[0])
		{
		case REDIRECT_MODE_1:
			return GetIPLocation((char const*)(m_file + detail::Get3BYTEptr(ptr + 1)));
		case REDIRECT_MODE_2:
			areaptr = ptr + 4;
		default:
			strncpy(ret.country, Get_String(ptr), sizeof(ret.country));
			if (!areaptr)
				areaptr = ptr + strlen(strcpy(ret.country, Get_String(ptr))) + 1;
		}
		strncpy(ret.area, Get_String(areaptr), sizeof(ret.area));
		return ret;
	}

	char* Get_String(char const* p)
	{
		switch (p[0])
		{
		case REDIRECT_MODE_1:
			return Get_String(m_file + detail::Get3BYTEptr(p + 1));
			break;
		case REDIRECT_MODE_2:
			return (char*) m_file + detail::Get3BYTEptr(p + 1);
			break;
		default:
			return (char*) p;
		}
	}
	char* GetArea(char* record);
	char* GetCountry(char* record);
	char* read_string(size_t offset);

	bool MatchRecord(char const* pRecord, const char* exp_country, const char* exp_area, std::map<uint32_t, char*>& country_matched, std::map<uint32_t, char*>& area_matched)
	{
		bool match;
		char const* parea = 0;
		char const* country;
		std::map<uint32_t, char*>::iterator it;

		// First , match the country field
		if (exp_country[0] == '*' && exp_country[1] == 0)
		{
			match = true;
		}
		else
		{
			switch (*pRecord)
			{
			case REDIRECT_MODE_1:
				return MatchRecord(m_file + detail::Get3BYTEptr(pRecord + 1) , exp_country, exp_area, country_matched, area_matched);
			case REDIRECT_MODE_2:
				parea = pRecord + 4;

				it = country_matched.find(detail::Get3BYTEptr(pRecord + 1));

				if (it != country_matched.end())
				{
					match = true;
				}
				else
				{
					return false ;
// 				match = match_exp( Get_String( pRecord ) , exp_country);
// 				// update the matched country list
// 				if (match)
// 					country_matched.insert(std::pair<uint32_t,char*>(::Get3BYTE3(pRecord + 1),0));
				}
				break;
			default:
				country = Get_String(pRecord);
				match = detail::match_exp((char*) country, (char*) exp_country);
				if (match)
				{
					country_matched.insert(std::make_pair<uint32_t, char*> (country - m_file , 0));

					parea = pRecord + strlen(country) + 1;

				}
				else
				{
					return false;
				}

			}

		}

		// then , match the area field

		if (exp_area[0] == '*' && exp_area[1] == 0)
		{
			return true;
		}

		switch (*parea)
		{
		case REDIRECT_MODE_2:
		case REDIRECT_MODE_1:
			it = area_matched.find(detail::Get3BYTEptr(parea + 1));

			if (it  != area_matched.end())
			{
				return true;
			}
			else
			{
				match = detail::match_exp(Get_String(parea), (char*) exp_area);
				if (match)
				{
					// update the matched area list
					area_matched.insert(std::make_pair<uint32_t, char*> (detail::Get3BYTEptr(parea + 1), 0));
				}
				else
				{
					return false;
				}
			}
			break;
		default:
			match = detail::match_exp(Get_String(parea), (char*) exp_area);
			// update the matched area list
			if (match)
				area_matched.insert(std::make_pair<uint32_t, char*> ((parea - m_file) & 0xFFFFFF , 0));
			else
				return false;
		}

		return match;
	}
	char const* FindRecord(in_addr ip)
	{
		size_t i = 0;
		size_t l = 0;
		size_t r = (m_last_record - m_first_record) / 7;

		ip.s_addr = ntohl(ip.s_addr);

		detail::RECORD_INDEX* pindex = (detail::RECORD_INDEX*)(m_file + m_first_record);

		for (size_t tryed = 0; tryed < 50; tryed++)
		{
			switch (detail::IP_IN_regon(ip.s_addr, pindex[i].ip, GetDWORD(pindex[i].offset)))
			{
			case 0:
				return m_file + pindex[i].offset;
			case 1:
				l = i;
				i += (r - i) / 2;
				if (l == i)
					return 0;
				break;
			case -1:
				r = i;
				i -= (i - l) / 2;
				break;
			}
		}
		return 0;
	}
protected: //inline functions

	uint32_t inline GetDWORD(size_t offset)
	{
		uint32_t ret;
		ret = * (uint32_t*)(m_file + offset);
		return detail::to_hostending(ret);
	}

	uint32_t inline Get3BYTE3(size_t offset)
	{
		uint32_t ret = 0;
		ret = * (uint32_t*)(m_file + offset);
		ret &= 0xFFFFFF;
		return detail::to_hostending(ret);
	}

public:
	//************************************
	// Method:    GetIPLocation
	// FullName:  CIPLocation::GetIPLocation
	// Access:    public
	// Returns:   IPLocation
	// Parameter: in_addr ip
	//************************************
	IPLocation GetIPLocation(in_addr ip)
	{
		IPLocation l;
		char const* ptr = FindRecord(ip);
		if (!ptr)
		{
			throw std::runtime_error("IP Record Not Found");
		}
#ifndef _WIN32
		IPLocation gbk = GetIPLocation(ptr + 4);

		detail::code_convert(l.country, 128, gbk.country, strlen(gbk.country));
		detail::code_convert(l.area, 128, gbk.area, strlen(gbk.area));
#else
		l = GetIPLocation(ptr + 4);
#endif
		return l;
	}


	std::list<IP_regon> GetIPs(const char* _exp_country, const char* _exp_area)
	{
		detail::RECORD_INDEX* pindex;
		std::list<IP_regon> retips;
		size_t i;
		bool match;
		std::map<uint32_t, char*> country_matched; // matched country
		std::map<uint32_t, char*> area_matched; // matched country


#ifndef _WIN32
		char exp_country[128];
		char exp_area[128];

		detail::utf8_gbk(exp_country, 128, (char*) _exp_country, strlen(_exp_country));
		detail::utf8_gbk(exp_area, 128, (char*) _exp_area, strlen(_exp_area));

#else
#define exp_country _exp_country
#define exp_area _exp_area
#endif

		for (i = m_first_record; i < m_last_record; i += 7)
		{
			pindex = (detail::RECORD_INDEX*)(m_file + i);

			char const* pRecord = m_file + pindex->offset + 4;

			match = MatchRecord(pRecord, exp_country, exp_area, country_matched, area_matched);
			if (match)
			{
				IP_regon inst;
				inst.start.s_addr = htonl(pindex->ip);
				inst.end.s_addr = htonl(GetDWORD(pindex->offset));

#ifndef _WIN32
				IPLocation gbk = GetIPLocation(pRecord);

				detail::code_convert(inst.location.country, 128, gbk.country, strlen(gbk.country));
				detail::code_convert(inst.location.area, 128, gbk.area, strlen(gbk.area));
#else
				inst.location = GetIPLocation(pRecord);
#endif
#ifdef DEBUG

				printf("%s to ", inet_ntoa(inst.start));
				printf("%s ,location : %s %s\n", inet_ntoa(inst.end), inst.location.country, inst.location.area);
#endif
				retips.insert(retips.end(), inst);

			}
		}
		return retips;
	}

public:
	ipdb(const char*	memptr, size_t len)
		: m_file(memptr)
	{
		m_filesize = len;

		m_first_record = GetDWORD(0);
		m_last_record = GetDWORD(4);
		m_curptr = memptr;
#ifndef _WIN32
		m_backup_byfile = false;
#endif
	}


	ipdb(const char* ipDateFile)
	{
		auto_file_handle m_ipfile(
#ifdef _WIN32
			CreateFile(ipDateFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)
#else
			open(ipDateFile, O_RDONLY)
#endif // _WIN32
		);
		
		if (! m_ipfile)
		{
			throw ("File Open Failed!");
		}

#ifdef _WIN32
		m_filesize = GetFileSize(m_ipfile.get_fd(), 0);
#else
		struct stat fst;
		fstat(m_ipfile.get_fd(), &fst);
		m_filesize = fst.st_size;
#endif // _WIN#32
#ifdef _WIN32
		m_filemap = CreateFileMapping((HANDLE) m_ipfile.get_fd(), 0, PAGE_READONLY, 0, m_filesize, 0);

		if (!m_filemap)
		{
			throw ("CreatFileMapping Failed!");
		}

		m_file = (char*) MapViewOfFile(m_filemap.get_fd(), FILE_MAP_READ, 0, 0, m_filesize);
#else
		m_file = (char*)mmap(0, m_filesize, PROT_READ, MAP_PRIVATE, (int)m_ipfile.get_fd(), 0);
#endif

		if (!m_file)
		{
			throw std::runtime_error("Creat File Mapping Failed!");
		}

#ifndef _WIN32
		m_backup_byfile = true;
#endif // _WIN32
		m_curptr = (char*) m_file;
		m_first_record = GetDWORD(0);
		m_last_record = GetDWORD(4);
	}

	~ipdb()
	{
#ifdef _WIN32
		if (m_filemap)
		{
			UnmapViewOfFile((void*) m_file);
		}
#else
		if (m_backup_byfile)
		{
			munmap((void*) m_file, m_filesize);
		}
#endif // _WIN32
	}

protected:

private:

	// 拷贝构造
	ipdb(const ipdb&);

	// 赋值
	ipdb& operator = (const ipdb&);
};

template<class UncompressFunction>
std::string decodeQQWryDat(std::string copywrite_rar, std::string qqwry_rar, UncompressFunction uncompressfunc)
{
	uint32_t key = QQWry::detail::to_hostending(reinterpret_cast<const detail::copywritetag*>(copywrite_rar.data())->key);
	// 解密
	for (int i = 0; i<0x200; i++)
	{
		key *= 0x805;
		key++;
		key &= 0xFF;
		uint32_t v = reinterpret_cast<const uint8_t*>(qqwry_rar.data())[i] ^ key;

		qqwry_rar[i] = v;
	}

	std::string deflated;
	deflated.resize(20 * 1024 * 1024);
	unsigned long deflated_size = deflated.size();
	uncompressfunc(
		reinterpret_cast<unsigned char*>(&deflated[0]),
		&deflated_size,
		(const unsigned char*) qqwry_rar.data(),
		qqwry_rar.size()
	);

	deflated.resize(deflated_size);
	return deflated;
}

} // namespace QQWry

#endif // !defined(AFX_IPLOCATION_H__80D0E230_4815_4D01_9CCF_4DAF4DE175E8__INCLUDED_)
