/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010-2011  Timon Van Overveldt (timonvo@gmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "swf.h"
#include "backends/urlutils.h"
#include "compat.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/Error.h"
#include "scripting/class.h"
#include <string>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iostream>
#include <fstream>
#ifdef __MINGW32__
#include <malloc.h>
#else
#include <alloca.h>
#endif

using namespace lightspark;

std::list<uint32_t> URLInfo::uriReservedAndHash =
	{';', '/', '?', ':', '@', '&', '=', '+', '$', ',', '#'};
std::list<uint32_t> URLInfo::uriUnescaped = 
	{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B',
	 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3',
	 '4', '5', '6', '7', '8', '9', '-', '_', '.', '!', '~', '*', '\'', '(',
	 ')'};
std::list<uint32_t> URLInfo::uriReservedAndUnescapedAndHash =
	{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B',
	 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3',
	 '4', '5', '6', '7', '8', '9', '-', '_', '.', '!', '~', '*', '\'', '(',
	 ')', ';', '/', '?', ':', '@', '&', '=', '+', '$', ',', '#'};

std::ostream& lightspark::operator<<(std::ostream& s, const URLInfo& u)
{
	s << u.getParsedURL();
	return s;
}

URLInfo::URLInfo(const tiny_string& u)
{
	url = u;

	std::string str = std::string(url.raw_buf());

	valid = false;
	if (u.empty())
		return;

	//Check for :// marking that there is a protocol in this url
	size_t colonPos = str.find("://");
	if(colonPos == std::string::npos)
		invalidReason = MISSING_PROTOCOL;

	std::string protocolStr;
	if (colonPos != std::string::npos) 
	{
		protocolStr = str.substr(0, colonPos);
		std::transform(protocolStr.begin(), protocolStr.end(), protocolStr.begin(), ::tolower);
	}
	protocol = protocolStr;

	size_t hostnamePos = colonPos != std::string::npos ? colonPos+3 : std::string::npos;
	size_t portPos = std::string::npos;
	size_t pathPos = std::string::npos;
	size_t queryPos = std::string::npos;
	size_t fragmentPos = std::string::npos;
	size_t cursor = hostnamePos;

	if(protocol == "file")
	{
		pathPos = cursor;
		if(pathPos == str.length())
			invalidReason = MISSING_PATH;
	}
	else
	{
		//URLEncode spaces by default. This is just a safety check.
		//Full encoding should be performed BEFORE passing the url to URLInfo
		url = encode(u, ENCODE_SPACES);

		pathPos = str.find("/", cursor);
		cursor = pathPos;

		portPos = str.rfind(":", cursor);
		if(portPos < hostnamePos)
			portPos = std::string::npos;

		queryPos = str.find("?", cursor);
		if(queryPos != std::string::npos)
			cursor = queryPos;
		fragmentPos = str.find("#", cursor);

		if(portPos == hostnamePos || pathPos == hostnamePos || queryPos == hostnamePos)
			invalidReason = MISSING_HOSTNAME;

		//Rebuild the work string using the escaped url
		str = std::string(url.raw_buf());
	}

	//Parse the host string
	std::string hostnameStr;
	if (hostnamePos != std::string::npos)
	{
		hostnameStr= str.substr(hostnamePos, std::min(std::min(pathPos, portPos), queryPos)-hostnamePos);
		std::transform(hostnameStr.begin(), hostnameStr.end(), hostnameStr.begin(), ::tolower);
	}
	hostname = hostnameStr;

	port = 0;
	//Check if the port after ':' is not empty
	if(portPos != std::string::npos && portPos != std::min(std::min(str.length()-1, pathPos-1), queryPos-1))
	{
		portPos++;
		std::istringstream i(str.substr(portPos, std::min(pathPos, str.length())-portPos));
		if(!(i >> port))
			invalidReason = INVALID_PORT;
	}

	//Parse the path string
	if(pathPos != std::string::npos)
	{
		std::string pathStr = str.substr(pathPos, std::min(str.length(), queryPos)-pathPos);
		path = tiny_string(pathStr);
		path = normalizePath(path);
		pathStr = std::string(path.raw_buf());

		size_t lastSlash = pathStr.rfind("/");
		if(lastSlash != std::string::npos)
		{
			pathDirectory = decode(pathStr.substr(0, lastSlash+1),URLInfo::ENCODE_URI);
			pathFile = pathStr.substr(lastSlash+1);
		}
		//We only get here when parsing a file://abc URL
		else
		{
			pathDirectory = "";
			pathFile = path;
		}
	}
	else
	{
		path = "/";
		pathDirectory = path;
		pathFile = "";
	}

	//Copy the query string
	if(queryPos != std::string::npos && queryPos < str.length()-1)
		query = str.substr(queryPos+1);
	else
		query = "";

	//Copy the query string
	if(fragmentPos != std::string::npos && fragmentPos < str.length()-1)
		fragment = str.substr(fragmentPos+1);
	else
		fragment = "";

	//Create a new normalized and encoded string representation
	parsedURL = getProtocol();
	parsedURL += "://";
	parsedURL += getHostname();
	if(getPort() > 0)
	{
		parsedURL += ":";
		parsedURL += Integer::toString(getPort());
	}
	parsedURL += getPath();
	if(query != "")
	{
		parsedURL += "?";
		parsedURL += getQuery();
	}
	if(fragment != "")
	{
		parsedURL += "#";
		parsedURL += getFragment();
	}

	valid = true;
}

tiny_string URLInfo::normalizePath(const tiny_string& u)
{
	std::string pathStr(u.raw_buf());

	//Remove double slashes
	size_t doubleSlash = pathStr.find("//");
	while(doubleSlash != std::string::npos)
	{
		pathStr.replace(doubleSlash, 2, "/");
		doubleSlash = pathStr.find("//");
	}

	//Parse all /../
	size_t doubleDot = pathStr.find("/../");
	size_t previousSlash;
	while(doubleDot != std::string::npos)
	{
		//We are at root, .. doesn't mean anything, just erase
		if(doubleDot == 0)
		{
			pathStr.replace(doubleDot, 3, "");
		}
		//Replace .. with one dir up
		else
		{
			previousSlash = pathStr.rfind("/", doubleDot-2);
			pathStr.replace(previousSlash, doubleDot-previousSlash+3, "");
		}
		doubleDot = pathStr.find("/../");
	}

	//Replace last /.. with one dir up
	if(pathStr.length() >= 3 && pathStr.substr(pathStr.length()-3, 3) == "/..")
	{
		previousSlash = pathStr.rfind("/", pathStr.length()-4);
		pathStr.replace(previousSlash, pathStr.length()-previousSlash+2, "/");
	}

	//Eliminate meaningless /./
	size_t singleDot = pathStr.find("/./");
	while(singleDot != std::string::npos)
	{
		pathStr.replace(singleDot, 2, "");
		singleDot = pathStr.find("/./");
	}


	//Remove redundant last dot
	if(pathStr.length() >= 2 && pathStr.substr(pathStr.length()-2, 2) == "/.")
		pathStr.replace(pathStr.length()-1, 1, "");
	if(pathStr.length() == 1 && pathStr[pathStr.length()-1] == '.')
		pathStr.replace(pathStr.length()-1, 1, "");

	return tiny_string(pathStr);
}

const URLInfo URLInfo::goToURL(const tiny_string& u) const
{
	std::string str = u.raw_buf();

	// absolute URL without protocol, Adobe seems to default to "http:"
	if(str.find("//") == 0)
	{
		tiny_string s;
		s = "http:"+str;
		return URLInfo(s);
	}

	//No protocol or hostname but has port, add protocol and hostname
	if(str.size() >= 2 && str[0] == ':' && str[1] >= '0' && str[1] <= '9')
	{
		tiny_string qualified;

		qualified = getProtocol();
		qualified += "://";
		qualified += getHostname();
		qualified += str;
		return URLInfo(qualified);
	}

	//No protocol, treat this as an unqualified URL
	if(str.find("://") == std::string::npos)
	{
		tiny_string qualified;

		qualified = getProtocol();
		qualified += "://";
		qualified += getHostname();
		if(getPort() > 0)
		{
			qualified += ":";
			qualified += Integer::toString(getPort());
		}
		if(str[0] != '/')
			qualified += getPathDirectory();
		qualified += str;
		return URLInfo(qualified);
	}
	else //Protocol specified, treat this as a qualified URL
		return URLInfo(u);
}

bool URLInfo::isSubOf(const URLInfo& url) const
{
	//Check if the beginning of the new pathDirectory equals the old pathDirectory
	if(getProtocol() != url.getProtocol())
		return false;
	else if(getHostname() != url.getHostname())
		return false;
	else if(!isSubPathOf(url))
		return false;
	else
		return true;
}
bool URLInfo::isSubPathOf(const tiny_string& parent, const tiny_string& child)
{
	return child.substr_bytes(0, parent.numBytes()) == parent;
}
bool URLInfo::isSubDomainOf(const tiny_string& parent, const tiny_string& child)
{
	std::string parentStr = std::string(parent.raw_buf());
	std::transform(parentStr.begin(), parentStr.end(), parentStr.begin(), ::tolower);
	std::string childStr = std::string(child.raw_buf());
	std::transform(childStr.begin(), childStr.end(), childStr.begin(), ::tolower);
	return childStr.substr(0, parentStr.length()) == parentStr;
}
bool URLInfo::matchesDomain(const tiny_string& expression, const tiny_string& subject)
{
	std::string expressionStr = std::string(expression.raw_buf());
	std::transform(expressionStr.begin(), expressionStr.end(), expressionStr.begin(), ::tolower);
	std::string subjectStr = std::string(subject.raw_buf());
	std::transform(subjectStr.begin(), subjectStr.end(), subjectStr.begin(), ::tolower);
	//'*' matches everything
	if(expressionStr == "*" || expressionStr == subjectStr)
		return true;
	//'*.somedomain.tld' matches 'somedomain.tld' and every subdomain of 'somedomain.tld'
	else if(expressionStr.substr(0,2) == "*.")
	{
		//Check if subjectStr == 'somedomain.tld'
		if(subjectStr == expressionStr.substr(2, expressionStr.length()-2))
			return true;
		//Check if subjectStr == 'somesubdomain.somedomain.tld'
		else if(subjectStr.length() >= expressionStr.length() && 
				subjectStr.substr(subjectStr.length()-expressionStr.length()+1, expressionStr.length()-1) == 
				expressionStr.substr(1, expressionStr.length()-1))
			return true;
	}

	//No positive matches found, so return false
	return false;
}

bool URLInfo::sameHost(const URLInfo& other) const
{
	return protocol == other.protocol && 
		hostname == other.hostname &&
		port == other.port;
}

tiny_string URLInfo::encode(const tiny_string& u, ENCODING type)
{
	if (type == ENCODE_URI)
		return encodeURI(u, uriReservedAndUnescapedAndHash);
	else if (type == ENCODE_URICOMPONENT)
		return encodeURI(u, uriUnescaped);

	tiny_string str;
	char buf[12];
	
	for(auto i=u.begin();i!=u.end();++i)
	{
		if(type == ENCODE_SPACES)
		{
			if(*i == ' ')
				str += "%20";
			else
				str += *i;
		}
		else {
			//A-Z, a-z or 0-9, all encoding types except encode spaces don't encode these characters
			if((*i >= 0x41 && *i <= 0x5a) || (*i >= 0x61 && *i <= 0x7a) || (*i >= 0x30 && *i <= 0x39))
				str += *i;
			//Additionally ENCODE_FORM doesn't decode: - _ . ~
			else if(type == ENCODE_FORM && 
					(*i == '-' || *i == '_' || *i == '.' || *i == '~'))
				str += *i;
			//ENCODE_FORM encodes spaces as + instead of %20
			else if(type == ENCODE_FORM && *i == ' ')
				str += '+';
			// ENCODE_ESCAPE doesn't encode:
			//@ - _ . * + /
			else if(type == ENCODE_ESCAPE && 
						(*i == '@' || *i == '-' || *i == '_' || *i == '.' ||
							*i == '*' || *i == '+' || *i == '/'))
				str += *i;

			else
			{
				if(*i<256)
					sprintf(buf,"%%%02X",*i);
				else
					sprintf(buf,"%%u%04X",*i);
				str += buf;
			}
		}
	}

	return str;
}

tiny_string URLInfo::encodeURI(const tiny_string& u, const std::list<uint32_t>& unescapedChars) {
	tiny_string res;
	CharIterator c = u.begin();
	CharIterator end = u.end();
	while (c != end)
	{
		if (std::find(unescapedChars.begin(), unescapedChars.end(), *c) == unescapedChars.end())
		{
			if ((*c >= 0xD800) && (*c <= 0xDFFF))
			{
				res += encodeSurrogatePair(c, end);
			}
			else
			{
				res += encodeSingleChar(*c);
			}
		}
		else
		{
			res += *c;
		}
		++c;
	}
	return res;
}

tiny_string URLInfo::encodeSurrogatePair(CharIterator& c, const CharIterator& end)
{
	if ((*c < 0xD800) || (*c >= 0xDC00))
	{
		createError<URIError>(getWorker(),kInvalidURIError, "encodeURI");
		return "";
	}
	uint32_t highSurrogate = *c;
	++c;
	if ((c == end) || ((*c < 0xDC00) || (*c > 0xDFFF)))
	{
		createError<URIError>(getWorker(),kInvalidURIError, "encodeURI");
		return "";
	}
	uint32_t lowSurrogate = *c;
	return encodeSingleChar((highSurrogate - 0xD800)*0x400 +
				(lowSurrogate - 0xDC00) + 0x10000);
}

tiny_string URLInfo::encodeSingleChar(uint32_t codepoint)
{
	char octets[6];
	gint numOctets = g_unichar_to_utf8(codepoint, octets);
	tiny_string encoded;
	for (int i=0; i<numOctets; i++)
	{
		encoded += encodeOctet(octets[i]);
	}

	return encoded;
}

tiny_string URLInfo::encodeOctet(char c) {
	gchar *buf = (gchar *)alloca(6);
	g_snprintf(buf, 6, "%%%.2X", (unsigned char)c);
	buf[5] = '\0';
	return tiny_string(buf, true);
}

tiny_string URLInfo::decode(const std::string& u, ENCODING type)
{
	if (type == ENCODE_URI)
		return decodeURI(u, uriReservedAndHash);
	else if (type == ENCODE_URICOMPONENT)
		return decodeURI(u, {});

	std::string str;
	//The string can only shrink
	str.reserve(u.length());

	std::string stringBuf;
	stringBuf.reserve(3);

	for(size_t i=0;i<u.length();i++)
	{
		if(i > u.length()-3 || u[i] != '%')
			str += u[i];
		else
		{
			stringBuf = u[i];
			stringBuf += u[i+1];
			stringBuf += u[i+2];
			std::transform(stringBuf.begin(), stringBuf.end(), stringBuf.begin(), ::toupper);

			//ENCODE_SPACES only decodes %20 to space
			if(type == ENCODE_SPACES && stringBuf == "%20")
				str += " ";
			//ENCODE_SPACES doesn't decode other characters
			else if(type == ENCODE_SPACES)
			{
				str += stringBuf;
				i+=2;
			}
			//All encoded characters that weren't excluded above are now decoded
			else
			{	
				if(u[i+1] == 'u' && i+6 <= u.length() && 
				   isxdigit(u[i+2]) && isxdigit(u[i+3]) &&
				   isxdigit(u[i+4]) && isxdigit(u[i+5]))
				{
					uint32_t c = (uint32_t)strtoul(u.substr(i+2, 4).c_str(), NULL, 16);
					if (c == 0)
						str.push_back(c);
					else
					{
						tiny_string s=tiny_string::fromChar(c);
						str.append(s.raw_buf());
					}
					i += 5;
					
				}
				else if(isxdigit(u[i+1]) && isxdigit(u[i+2]))
				{
					uint32_t c = (uint32_t)strtoul(u.substr(i+1, 2).c_str(), NULL, 16);
					if (c == 0)
						str.push_back(c);
					else
					{
						tiny_string s=tiny_string::fromChar(c);
						str.append(s.raw_buf());
					}
					i += 2;
				}
				else
				{
					str += u[i];
				}
			}
		}
	}
	return str;
}

tiny_string URLInfo::decodeURI(const tiny_string& u, const std::list<uint32_t>& reservedChars)
{
	tiny_string res;
	CharIterator c = u.begin();
	CharIterator end = u.end();
	while (c != end)
	{
		if (*c == '%')
		{
			CharIterator encodeBegin = c;
			uint32_t decoded = decodeSingleChar(c, end);
			if (std::find(reservedChars.begin(), reservedChars.end(), decoded) == reservedChars.end())
			{
				res += decoded;
			}
			else
			{
				CharIterator it = encodeBegin;
				while (it != c)
				{
					res += *it;
					++it;
				}
			}
		}
		else
		{
			res += *c;
			++c;
		}
	}

	return res;
}

uint32_t URLInfo::decodeSingleChar(CharIterator& c, const CharIterator& end)
{
	uint32_t decoded = decodeSingleEscapeSequence(c, end);
	if ((decoded & 0x80) != 0) {
		decoded = decodeRestOfUTF8Sequence(decoded, c, end);
	}
	return decoded;
}

uint32_t URLInfo::decodeRestOfUTF8Sequence(uint32_t firstOctet, CharIterator& c, const CharIterator& end) {
	unsigned int numOctets = 0;
	uint32_t mask = 0x80;
	while ((firstOctet & mask) != 0) {
		numOctets++;
		mask = mask >> 1;
	}
	if (numOctets <= 1 || numOctets > 4)
	{
		createError<URIError>(getWorker(),kInvalidURIError, "decodeURI");
		return 0;
	}

	char *octets = (char *)alloca(numOctets);
	octets[0] = firstOctet;
	for (unsigned int i=1; i<numOctets; i++) {
		octets[i] = decodeSingleEscapeSequence(c, end);
	}

	if (isSurrogateUTF8Sequence(octets, numOctets))
	{
		LOG(LOG_NOT_IMPLEMENTED, "decodeURI: decoding surrogate pairs");
		return REPLACEMENT_CHARACTER;
	}

	gunichar unichar = g_utf8_get_char_validated(octets, numOctets);
	if ((unichar == (gunichar)-1) || 
	    (unichar == (gunichar)-2) ||
	    (unichar >= 0x10FFFF))
	{
		createError<URIError>(getWorker(),kInvalidURIError, "decodeURI");
		return 0;
	}
	return (uint32_t)unichar;
}

uint32_t URLInfo::decodeSingleEscapeSequence(CharIterator& c, const CharIterator& end)
{
	if (*c != '%')
	{
		createError<URIError>(getWorker(),kInvalidURIError, "decodeURI");
		return 0;
	}
	++c;
	uint32_t h1 = decodeHexDigit(c, end);
	uint32_t h2 = decodeHexDigit(c, end);
	return (h1 << 4) + h2;
}

bool URLInfo::isSurrogateUTF8Sequence(const char *octets, unsigned int numOctets)
{
	// Surrogate code points: 0xD800 - 0xDFFF
	// UTF-8 encoded: 0xED 0xA0 0x80 - 0xED 0xBF 0xBF
	return (numOctets == 3) &&
		((unsigned char)octets[0] == 0xED) &&
		((unsigned char)octets[1] >= 0xA0) &&
		((unsigned char)octets[1] <= 0xBF);
}

uint32_t URLInfo::decodeHexDigit(CharIterator& c, const CharIterator& end)
{
	if (c == end || !isxdigit(*c))
	{
		createError<URIError>(getWorker(),kInvalidURIError, "decodeURI");
		return 0;
	}

	gint h = g_unichar_xdigit_value(*c);
	assert((h >= 0) && (h < 16));
	++c;
	return (uint32_t) h;
}

bool URLInfo::isRTMP() const
{
	return protocol == "rtmp" || protocol == "rtmpe" || protocol == "rtmps" ||
	       protocol == "rtmpt" || protocol == "rtmpte" || protocol == "rtmpts" || protocol == "rtmfp";
}

std::list< std::pair<tiny_string, tiny_string> > URLInfo::getQueryKeyValue() const
{
	std::list< std::pair<tiny_string, tiny_string> > keyvalues;
	std::list<tiny_string> queries = query.split('&');
	std::list<tiny_string>::iterator it;
	for(it=queries.begin(); it!=queries.end(); ++it)
	{
		uint32_t eqpos = it->find("=");
		if(eqpos!=tiny_string::npos && (eqpos+1<it->numChars()))
		{
			tiny_string key=decode(it->substr(0, eqpos), ENCODE_ESCAPE);
			tiny_string value=decode(it->substr(eqpos+1, it->numChars()-eqpos-1), ENCODE_ESCAPE);
			keyvalues.push_back(std::make_pair(key, value));
		}
	}

	return keyvalues;
}
