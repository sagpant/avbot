#pragma once
#include <string>
#include <algorithm>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/type_traits.hpp>
#include <boost/version.hpp>

//重载掉有bug的boost方法
namespace boost { namespace property_tree {
#if BOOST_VERSION >= 105500
	namespace json_parser
#else
	namespace info_parser
#endif
{
    // Create necessary escape sequences from illegal characters
    inline std::string create_escapes(const std::basic_string<char> &s)
    {
        std::basic_string<char> result;
        std::basic_string<char>::const_iterator b = s.begin();
        std::basic_string<char>::const_iterator e = s.end();
        while (b != e)
        {
            // This assumes an ASCII superset. But so does everything in PTree.
            // We escape everything outside ASCII, because this code can't
            // handle high unicode characters.
            if (static_cast<unsigned char>(*b) == 0x20 || static_cast<unsigned char>(*b) == 0x21 || (static_cast<unsigned char>(*b) >= 0x23 && static_cast<unsigned char>(*b) <= 0x2E) ||
                (static_cast<unsigned char>(*b) >= 0x30 && static_cast<unsigned char>(*b) <= 0x5B) || (static_cast<unsigned char>(*b) >= 0x5D && static_cast<unsigned char>(*b) <= 0xFF)  //it fails here because char are signed
                || (*b >= -0x80 && *b < 0 ) ) // this will pass UTF-8 signed chars
                result += *b;
            else if (*b == char('\b')) result += char('\\'), result += char('b');
            else if (*b == char('\f')) result += char('\\'), result += char('f');
            else if (*b == char('\n')) result += char('\\'), result += char('n');
            else if (*b == char('\r')) result += char('\\'), result += char('r');
            else if (*b == char('/')) result += char('\\'), result += char('/');
            else if (*b == char('"'))  result += char('\\'), result += char('"');
            else if (*b == char('\\')) result += char('\\'), result += char('\\');
            else
            {
                const char *hexdigits = "0123456789ABCDEF";
                typedef make_unsigned<char>::type UCh;
                unsigned long u = (std::min)(static_cast<unsigned long>(
                                                 static_cast<UCh>(*b)),
                                             0xFFFFul);
                int d1 = u / 4096; u -= d1 * 4096;
                int d2 = u / 256; u -= d2 * 256;
                int d3 = u / 16; u -= d3 * 16;
                int d4 = u;
                result += char('\\'); result += char('u');
                result += char(hexdigits[d1]); result += char(hexdigits[d2]);
                result += char(hexdigits[d3]); result += char(hexdigits[d4]);
            }
            ++b;
        }
        return result;
    }
} } }
