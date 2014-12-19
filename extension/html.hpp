
#pragma once

template<typename string>
std::string html_unescape_char(string escaped)
{
	//	boost::replace_all( htmlcharseq, "&nbsp;", " " );
	if (escaped[0] == '#'){
		// &#XX; 这样的模式
		// strtol
		std::wstring unistr;
		unistr.append(1, strtol(&escaped[1], NULL, 10));

		return boost::locale::conv::utf_to_utf<char>(unistr);

	}else{
		if (escaped == "nbsp")
			return " ";
		if (escaped == "mdash")
			return "-";
		if (escaped == "quot")
			return "\"";
		if (escaped == "amp")
			return "&";
		if (escaped == "lt")
			return "<";
		if (escaped == "gt")
			return ">";
#if !defined(_MSC_VER)
		// 处理
		if (escaped== "ndash")
			return "–";
		if (escaped == "euro")
			return "€";
		if (escaped == "sup1")
			return "¹";
		if (escaped == "sup2")
			return "²";
		if (escaped == "sup3")
			return "³";
		if (escaped == "iexcl")
			return "¡";
		if (escaped == "cent")
			return "¢";
		if (escaped == "pound")
			return "£";
		if (escaped == "curren")
			return "¤";
		if (escaped == "yen")
			return "¥";
		if (escaped == "brvbar")
			return "¦";
		if (escaped == "sect")
			return "§";
		if (escaped == "uml")
			return "¨";
		if (escaped == "copy")
			return "©";
		if (escaped == "ordf")
			return "ª";
		if (escaped == "not")
			return "¬";
		if (escaped == "reg")
			return "®";
		if (escaped == "macr")
			return "¯";
		if (escaped == "deg")
			return "°";
		if (escaped == "plusmn")
			return "±";
		if (escaped == "acute")
			return "´";
		if (escaped == "micro")
			return "µ";
		if (escaped == "para")
			return "¶";
		if (escaped == "middot")
			return "·";
		if (escaped == "cedil")
			return "¸";

		if (escaped == "ordm")
			return "º";
		if (escaped == "raquo")
			return "»";
		if (escaped == "frac14")
			return "¼";
		if (escaped == "frac12")
			return "½";
		if (escaped == "frac34")
			return "¾";
		if (escaped == "iquest")
			return "¿";
		if (escaped == "Agrave")
			return "À";
		if (escaped == "Aacute")
			return "Á";
		if (escaped == "Acirc")
			return "Â";
		if (escaped == "Atilde")
			return "Ã";
		if (escaped == "Auml")
			return "Ä";
		if (escaped == "Aring")
			return "Å";
		if (escaped == "AElig")
			return "Æ";
		if (escaped == "Ccedil")
			return "Ç";
		if (escaped == "Egrave")
			return "È";
		if (escaped == "Eacute")
			return "É";
		if (escaped == "Ecirc")
			return "Ê";
		if (escaped == "Euml")
			return "Ë";
		if (escaped == "Igrave")
			return "Ì";
		if (escaped == "Iacute")
			return "Í";
		if (escaped == "Icirc")
			return "Î";
		if (escaped == "Iuml")
			return "Ï";
		if (escaped == "ETH")
			return "Ð";
		if (escaped == "Ntilde")
			return "Ñ";
		if (escaped == "Ograve")
			return "Ò";
		if (escaped == "Oacute")
			return "Ó";
		if (escaped == "Ocirc")
			return "Ô";
		if (escaped == "Otilde")
			return "Õ";
		if (escaped == "Ouml")
			return "Ö";
		if (escaped == "times")
			return "×";
		if (escaped == "Oslash")
			return "Ø";
		if (escaped == "Ugrave")
			return "Ù";
		if (escaped == "Uacute")
			return "Ú";
		if (escaped == "Ucirc")
			return "Û";
		if (escaped == "Uuml")
			return "Ü";
		if (escaped == "Yacute")
			return "Ý";
		if (escaped == "THORN")
			return "Þ";
		if (escaped == "szlig")
			return "ß";
		if (escaped == "agrave")
			return "à";
		if (escaped == "aacute")
			return "á";
		if (escaped == "acirc")
			return "â";
		if (escaped == "atilde")
			return "ã";
		if (escaped == "auml")
			return "ä";
		if (escaped == "aring")
			return "å";
		if (escaped == "aelig")
			return "æ";
		if (escaped == "ccedil")
			return "ç";
		if (escaped == "egrave")
			return "è";
		if (escaped == "eacute")
			return "é";
		if (escaped == "ecirc")
			return "ê";
		if (escaped == "euml")
			return "ë";
		if (escaped == "igrave")
			return "ì";
		if (escaped == "iacute")
			return "í";
		if (escaped == "agrave")
			return "í";
		if (escaped == "icirc")
			return "î";
		if (escaped == "iuml")
			return "ï";
		if (escaped == "eth")
			return "ð";
		if (escaped == "ntilde")
			return "ñ";
		if (escaped == "ograve")
			return "ò";
		if (escaped == "oacute")
			return "ó";
		if (escaped == "ocirc")
			return "ô";
		if (escaped == "otilde")
			return "õ";
		if (escaped == "ouml")
			return "ö";
		if (escaped == "divide")
			return "÷";
		if (escaped == "oslash")
			return "ø";
		if (escaped == "ugrave")
			return "ù";
		if (escaped == "uacute")
			return "ú";
		if (escaped == "ucirc")
			return "û";
		if (escaped == "uuml")
			return "ü";
		if (escaped == "yacute")
			return "ý";
		if (escaped == "thorn")
			return "þ";
#endif
	}
	return "";
}

template<typename string>
std::string html_unescape(string htmlcharseq)
{
	string retstr;
	std::string::iterator chariter = htmlcharseq.begin();

	bool unes = false;

	while (chariter != htmlcharseq.end()){
		if (unes){
			// 找到 ;
			std::string::iterator es_end =  std::find(chariter, htmlcharseq.end(), ';');
			// 把 ; 之前的给组合一下.
			if (es_end != htmlcharseq.end()){
				std::string es_seq(chariter, es_end);

				retstr.append( html_unescape_char(es_seq) );


				chariter = es_end + 1;
				unes = false;
			}else return "解码错误";

		}else{
			if (*chariter == '&'){
				unes = true;
			}else{
				retstr.append(1, *chariter);
			}
			++ chariter;
		}
	}

	return retstr;
}
