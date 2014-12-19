#include <boost/phoenix.hpp>
#include <boost/fusion/adapted/std_pair.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace spirit = boost::spirit;
namespace qi = spirit::qi;
namespace ascii = spirit::ascii;
using boost::property_tree::ptree;

namespace boost {
	namespace spirit {
		namespace traits
		{
			// in this case we just expose the embedded 'int' as the attribute instance
			// to use, allowing to leave the function 'post()' empty
			template <typename Type>
			struct assign_to_attribute_from_value < ptree, Type>
			{
				typedef ptree attr_type;

				static void call(const Type& val, attr_type& attr) { attr.put_value(val); }
			};

			template <typename Type>
			struct assign_to_attribute_from_value < std::pair<std::string, ptree>, Type >
			{
				typedef std::pair<std::string, ptree> attr_type;

				static void call(const Type& val, attr_type& attr) { attr.second = val; }
			};
		}
	}
}

template<typename Iterator>
struct cfg_grammar : public qi::grammar<Iterator, ptree(), ascii::space_type>
{
	cfg_grammar()
		: cfg_grammar::base_type(key_value_pairs)
	{
		namespace phx = boost::phoenix;
		using phx::bind;
		using phx::val;
		using spirit::_1;
		using spirit::_2;
		using spirit::_3;
		using spirit::_4;

		key_value_pairs %= comments >> *(string_val >> comments);
		string_val %= key >> qi::lit(':') >> value;
		key %= *(ascii::char_ - qi::lit(' ') - qi::lit(':') - qi::lit('\n'));
		value %= qi::as<ptree>()[number] | qi::as<ptree>()[string] | object | list;
		string %= qi::lit('"') >> *(ascii::char_ - qi::lit('"')) >> qi::lit('"');
		number %= qi::int_;
		object %= qi::lit('{') >> key_value_pairs >> qi::lit('}');
		list %= qi::lit('{') >> *qi::as<std::pair<std::string, ptree>>()[value] >> qi::lit('}');
		comments %= *comment;
		comment %= qi::lexeme[qi::lit('#') >> *(qi::char_ - qi::lit('\n')) >> qi::lit('\n')];

		qi::on_error<qi::fail>
			(
			key_value_pairs
			, std::cout
			<< val("Error! Expecting ")
			<< _4                               // what failed?
			<< val(" here: \"")
			<< phx::construct<std::string>(_3, _2)   // iterators to error-pos, end
			<< val("\"")
			<< std::endl
			);
	}



	qi::rule<Iterator, ptree(), ascii::space_type> key_value_pairs;
	qi::rule<Iterator, std::pair<std::string, ptree>(), ascii::space_type> string_val;
	qi::rule<Iterator, std::string()> key;
	qi::rule<Iterator, ptree(), ascii::space_type> value;
	qi::rule<Iterator, std::string()> string;
	qi::rule<Iterator, int()> number;
	qi::rule<Iterator, ptree(), ascii::space_type> object;
	qi::rule<Iterator, ptree(), ascii::space_type> list;
	qi::rule<Iterator, void(), ascii::space_type> comments;
	qi::rule<Iterator, void(), ascii::space_type> comment;
};

ptree parse_cfg(std::string filecontent)
{
	typedef std::string::const_iterator Iterator;
	ptree result;

	Iterator first = filecontent.begin();
	Iterator last = filecontent.end();

	std::cout << std::string(first, last) << std::endl;

	qi::phrase_parse(first, last, cfg_grammar<Iterator>(), ascii::space, result);

	boost::property_tree::json_parser::write_json(std::cout, result);
	return result;
}
