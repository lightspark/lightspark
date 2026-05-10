/*
 * inicpp.h
 *
 * Created on: 26 Dec 2015
 *     Author: Fabian Meyer
 *    License: MIT
 */

#ifndef INICPP_H_
#define INICPP_H_

#include <algorithm>
#include <fstream>
#include <istream>
#include <map>
#include <assert.h>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <string>

#ifdef __cpp_lib_string_view // This one is defined in <string> if we have std::string_view
#   include <string_view>
#endif

namespace ini
{
    /************************************************
     * Helper Functions
     ************************************************/

    /** Returns a string of whitespace characters. */
    constexpr const char *whitespaces()
    {
        return " \t\n\r\f\v";
    }

    /** Returns a string of indentation characters. */
    constexpr const char *indents()
    {
        return " \t";
    }

    /** Trims a string in place.
      * @param str string to be trimmed in place */
    inline void trim(std::string &str)
    {
        // first erasing from end should be slighty more efficient
        // because erasing from start potentially moves all chars
        // multiple indices towards the front.

        auto lastpos = str.find_last_not_of(whitespaces());
        if(lastpos == std::string::npos)
        {
            str.clear();
            return;
        }

        str.erase(lastpos + 1);
        str.erase(0, str.find_first_not_of(whitespaces()));
    }

    /************************************************
     * Conversion Functors
     ************************************************/

    inline bool strToLong(const std::string &value, long &result)
    {
        char *endptr;
        // check if decimal
        result = std::strtol(value.c_str(), &endptr, 10);
        if(*endptr == '\0')
            return true;
        // check if octal
        result = std::strtol(value.c_str(), &endptr, 8);
        if(*endptr == '\0')
            return true;
        // check if hex
        result = std::strtol(value.c_str(), &endptr, 16);
        if(*endptr == '\0')
            return true;

        return false;
    }

    inline bool strToULong(const std::string &value, unsigned long &result)
    {
        char *endptr;
        // check if decimal
        result = std::strtoul(value.c_str(), &endptr, 10);
        if(*endptr == '\0')
            return true;
        // check if octal
        result = std::strtoul(value.c_str(), &endptr, 8);
        if(*endptr == '\0')
            return true;
        // check if hex
        result = std::strtoul(value.c_str(), &endptr, 16);
        if(*endptr == '\0')
            return true;

        return false;
    }

    template<typename T>
    struct Convert
    {};

    template<>
    struct Convert<bool>
    {
        void decode(const std::string &value, bool &result)
        {
            std::string str(value);
            std::transform(str.begin(), str.end(), str.begin(), [](const char c){
                return static_cast<char>(::toupper(c));
            });

            if(str == "TRUE")
                result = true;
            else if(str == "FALSE")
                result = false;
            else
                throw std::invalid_argument("field is not a bool");
        }

        void encode(const bool value, std::string &result)
        {
            result = value ? "true" : "false";
        }
    };

    template<>
    struct Convert<char>
    {
        void decode(const std::string &value, char &result)
        {
            assert(value.size() > 0);
            result = value[0];
        }

        void encode(const char value, std::string &result)
        {
            result = value;
        }
    };

    template<>
    struct Convert<unsigned char>
    {
        void decode(const std::string &value, unsigned char &result)
        {
            assert(value.size() > 0);
            result = value[0];
        }

        void encode(const unsigned char value, std::string &result)
        {
            result = value;
        }
    };

    template<>
    struct Convert<short>
    {
        void decode(const std::string &value, short &result)
        {
            long tmp;
            if(!strToLong(value, tmp))
                throw std::invalid_argument("field is not a short");
            result = static_cast<short>(tmp);
        }

        void encode(const short value, std::string &result)
        {
            std::stringstream ss;
            ss << value;
            result = ss.str();
        }
    };

    template<>
    struct Convert<unsigned short>
    {
        void decode(const std::string &value, unsigned short &result)
        {
            unsigned long tmp;
            if(!strToULong(value, tmp))
                throw std::invalid_argument("field is not an unsigned short");
            result = static_cast<unsigned short>(tmp);
        }

        void encode(const unsigned short value, std::string &result)
        {
            std::stringstream ss;
            ss << value;
            result = ss.str();
        }
    };

    template<>
    struct Convert<int>
    {
        void decode(const std::string &value, int &result)
        {
            long tmp;
            if(!strToLong(value, tmp))
                throw std::invalid_argument("field is not an int");
            result = static_cast<int>(tmp);
        }

        void encode(const int value, std::string &result)
        {
            std::stringstream ss;
            ss << value;
            result = ss.str();
        }
    };

    template<>
    struct Convert<unsigned int>
    {
        void decode(const std::string &value, unsigned int &result)
        {
            unsigned long tmp;
            if(!strToULong(value, tmp))
                throw std::invalid_argument("field is not an unsigned int");
            result = static_cast<unsigned int>(tmp);
        }

        void encode(const unsigned int value, std::string &result)
        {
            std::stringstream ss;
            ss << value;
            result = ss.str();
        }
    };

    template<>
    struct Convert<long>
    {
        void decode(const std::string &value, long &result)
        {
            if(!strToLong(value, result))
                throw std::invalid_argument("field is not a long");
        }

        void encode(const long value, std::string &result)
        {
            std::stringstream ss;
            ss << value;
            result = ss.str();
        }
    };

    template<>
    struct Convert<unsigned long>
    {
        void decode(const std::string &value, unsigned long &result)
        {
            if(!strToULong(value, result))
                throw std::invalid_argument("field is not an unsigned long");
        }

        void encode(const unsigned long value, std::string &result)
        {
            std::stringstream ss;
            ss << value;
            result = ss.str();
        }
    };

    template<>
    struct Convert<double>
    {
        void decode(const std::string &value, double &result)
        {
            result = std::stod(value);
        }

        void encode(const double value, std::string &result)
        {
            std::stringstream ss;
            ss << value;
            result = ss.str();
        }
    };

    template<>
    struct Convert<float>
    {
        void decode(const std::string &value, float &result)
        {
            result = std::stof(value);
        }

        void encode(const float value, std::string &result)
        {
            std::stringstream ss;
            ss << value;
            result = ss.str();
        }
    };

    template<>
    struct Convert<std::string>
    {
        void decode(const std::string &value, std::string &result)
        {
            result = value;
        }

        void encode(const std::string &value, std::string &result)
        {
            result = value;
        }
    };

#ifdef __cpp_lib_string_view
    template<>
    struct Convert<std::string_view>
    {
        void decode(const std::string &value, std::string_view &result)
        {
            result = value;
        }

        void encode(const std::string_view value, std::string &result)
        {
            result = value;
        }
    };
#endif

    template<>
    struct Convert<const char*>
    {
        void encode(const char* const &value, std::string &result)
        {
            result = value;
        }

        void decode(const std::string &value, const char* &result)
        {
            result = value.c_str();
        }
    };

    template<>
    struct Convert<char*>
    {
        void encode(const char* const &value, std::string &result)
        {
            result = value;
        }
    };

    template<size_t n>
    struct Convert<char[n]>
    {
        void encode(const char *value, std::string &result)
        {
            result = value;
        }
    };

    class IniField
    {
    private:
        std::string value_;

    public:
        IniField() : value_()
        {}

        IniField(const std::string &value) : value_(value)
        {}
        IniField(const IniField &field) : value_(field.value_)
        {}

        ~IniField()
        {}

        template<typename T>
        T as() const
        {
            Convert<T> conv;
            T result;
            conv.decode(value_, result);
            return result;
        }

        template<typename T>
        IniField &operator=(const T &value)
        {
            Convert<T> conv;
            conv.encode(value, value_);
            return *this;
        }

        IniField &operator=(const IniField &field)
        {
            value_ = field.value_;
            return *this;
        }
    };

    struct StringInsensitiveLess
    {
        bool operator()(std::string lhs, std::string rhs) const
        {
                std::transform(lhs.begin(), lhs.end(), lhs.begin(), [](const char c){
                    return static_cast<char>(::tolower(c));
                });
                std::transform(rhs.begin(), rhs.end(), rhs.begin(), [](const char c){
                    return static_cast<char>(::tolower(c));
                });
                return  lhs < rhs;
        }
    };

    template <typename Comparator>
    class IniSectionBase : public std::map<std::string, IniField, Comparator>
    {
    public:
        IniSectionBase()
        {}
        ~IniSectionBase()
        {}
    };

    using IniSection = IniSectionBase<std::less<std::string>>;
    using IniSectionCaseInsensitive = IniSectionBase<StringInsensitiveLess>;

    template <typename Comparator>
    class IniFileBase : public std::map<std::string, IniSectionBase<Comparator>, Comparator>
    {
    private:
        char fieldSep_ = '=';
        char esc_ = '\\';
        std::vector<std::string> commentPrefixes_ = { "#" , ";" };
        bool multiLineValues_ = false;
        bool overwriteDuplicateFields_ = true;

        void eraseComment(const std::string &commentPrefix,
            std::string &str,
            std::string::size_type startpos = 0)
        {
            size_t prefixpos = str.find(commentPrefix, startpos);
            if(std::string::npos == prefixpos)
                return;
            // Found a comment prefix, is it escaped?
            if(0 != prefixpos && str[prefixpos - 1] == esc_)
            {
                // The comment prefix is escaped, so just delete the escape char
                // and keep erasing after the comment prefix
                str.erase(prefixpos - 1, 1);
                eraseComment(
                    commentPrefix, str, prefixpos - 1 + commentPrefix.size());
            }
            else
            {
                str.erase(prefixpos);
            }
        }

        void eraseComments(std::string &str)
        {
            for(const std::string &commentPrefix : commentPrefixes_)
                eraseComment(commentPrefix, str);
        }

        /** Tries to find a suitable comment prefix for the string data at the given
          * position. Returns commentPrefixes_.end() if not match was found. */
        std::vector<std::string>::const_iterator findCommentPrefix(const std::string &str,
            const std::size_t startpos) const
        {
            // if startpos is invalid simply return "not found"
            if(startpos >= str.size())
                return commentPrefixes_.end();

            for(size_t i = 0; i < commentPrefixes_.size(); ++i)
            {
                const std::string &prefix = commentPrefixes_[i];
                // if this comment prefix is longer than the string view itself
                // then skip
                if(prefix.size() + startpos > str.size())
                    continue;

                bool match = true;
                for(size_t j = 0; j < prefix.size() && match; ++j)
                    match = str[startpos + j] == prefix[j];

                if(match)
                    return commentPrefixes_.begin() + i;
            }

            return commentPrefixes_.end();
        }

        void writeEscaped(std::ostream &os, const std::string &str) const
        {
            for(size_t i = 0; i < str.length(); ++i)
            {
                auto prefixpos = findCommentPrefix(str, i);
                // if no suitable prefix was found at this position
                // then simply write the current character
                if(prefixpos != commentPrefixes_.end())
                {
                    const std::string &prefix = *prefixpos;
                    os.put(esc_);
                    os.write(prefix.c_str(), prefix.size());
                    i += prefix.size() - 1;
                }
                else if (multiLineValues_ && str[i] == '\n')
                    os.write("\n\t", 2);
                else
                    os.put(str[i]);
            }
        }

    public:
        IniFileBase() = default;

        IniFileBase(const char fieldSep, const char comment)
            : fieldSep_(fieldSep), commentPrefixes_(1, std::string(1, comment))
        {}

        IniFileBase(const std::string &filename)
        {
            load(filename);
        }

        IniFileBase(std::istream &is)
        {
            decode(is);
        }

        IniFileBase(const char fieldSep,
            const std::vector<std::string> &commentPrefixes)
            : fieldSep_(fieldSep), commentPrefixes_(commentPrefixes)
        {}

        IniFileBase(const std::string &filename,
            const char fieldSep,
            const std::vector<std::string> &commentPrefixes)
            : fieldSep_(fieldSep), commentPrefixes_(commentPrefixes)
        {
            load(filename);
        }

        IniFileBase(std::istream &is,
            const char fieldSep,
            const std::vector<std::string> &commentPrefixes)
            : fieldSep_(fieldSep), commentPrefixes_(commentPrefixes)
        {
            decode(is);
        }

        ~IniFileBase()
        {}

        /** Sets the separator charactor for fields in the INI file.
          * @param sep separator character to be used. */
        void setFieldSep(const char sep)
        {
            fieldSep_ = sep;
        }

        /** Sets the character that should be interpreted as the start of comments.
          * Default is '#'.
          * Note: If the inifile contains the comment character as data it must be prefixed with
          * the configured escape character.
          * @param comment comment character to be used. */
        void setCommentChar(const char comment)
        {
            commentPrefixes_ = {std::string(1, comment)};
        }

        /** Sets the list of strings that should be interpreted as the start of comments.
          * Default is [ "#" ].
          * Note: If the inifile contains any comment string as data it must be prefixed with
          * the configured escape character.
          * @param commentPrefixes vector of comment prefix strings to be used. */
        void setCommentPrefixes(const std::vector<std::string> &commentPrefixes)
        {
            commentPrefixes_ = commentPrefixes;
        }

        /** Sets the character that should be used to escape comment prefixes.
          * Default is '\'.
          * @param esc escape character to be used. */
        void setEscapeChar(const char esc)
        {
            esc_ = esc;
        }

        /** Sets whether or not to parse multi-line field values.
          * Default is false.
          * @param enable enable or disable? */
        void setMultiLineValues(bool enable)
        {
            multiLineValues_ = enable;
        }

        /** Sets whether or not overwriting duplicate fields is allowed.
          * If overwriting duplicate fields is not allowed,
          * an exception is thrown when a duplicate field is found inside a section.
          * Default is true.
          * @param allowed Is overwriting duplicate fields allowed or not? */
        void allowOverwriteDuplicateFields(bool allowed)
        {
            overwriteDuplicateFields_ = allowed;
        }

        /** Tries to decode a ini file from the given input stream.
          * @param is input stream from which data should be read. */
        void decode(std::istream &is)
        {
            this->clear();
            int lineNo = 0;
            IniSectionBase<Comparator> *currentSection = nullptr;
            std::string mutliLineValueFieldName = "";
            std::string line;
            // iterate file line by line
            while(!is.eof() && !is.fail())
            {
                std::getline(is, line, '\n');
                eraseComments(line);
                bool hasIndent = line.find_first_not_of(indents()) != 0;
                trim(line);
                ++lineNo;

                // skip if line is empty
                if(line.size() == 0)
                    continue;

                if(line[0] == '[')
                {
                    // line is a section
                    // check if the section is also closed on same line
                    std::size_t pos = line.find("]");
                    if(pos == std::string::npos)
                    {
                        std::stringstream ss;
                        ss << "l." << lineNo
                           << ": ini parsing failed, section not closed";
                        throw std::logic_error(ss.str());
                    }
                    // check if the section name is empty
                    if(pos == 1)
                    {
                        std::stringstream ss;
                        ss << "l." << lineNo
                           << ": ini parsing failed, section is empty";
                        throw std::logic_error(ss.str());
                    }

                    // retrieve section name
                    std::string secName = line.substr(1, pos - 1);
                    currentSection = &((*this)[secName]);

                    // clear multiline value field name
                    // a new section means there is no value to continue
                    mutliLineValueFieldName = "";
                }
                else
                {
                    // line is a field definition
                    // check if section was already opened
                    if(currentSection == nullptr)
                    {
                        std::stringstream ss;
                        ss << "l." << lineNo
                           << ": ini parsing failed, field has no section"
                                " or ini file in use by another application";
                        throw std::logic_error(ss.str());
                    }

                    // find key value separator
                    std::size_t pos = line.find(fieldSep_);
                    if (multiLineValues_ && hasIndent && mutliLineValueFieldName != "")
                    {
                        // extend a multi-line value
                        IniField previous_value = (*currentSection)[mutliLineValueFieldName];
                        std::string value = previous_value.as<std::string>() + "\n" + line;
                        (*currentSection)[mutliLineValueFieldName] = value;
                    }
                    else if(pos == std::string::npos)
                    {
                        std::stringstream ss;
                        ss << "l." << lineNo
                           << ": ini parsing failed, no '"
                           << fieldSep_
                           << "' found";
                        if (multiLineValues_)
                            ss << ", and not a multi-line value continuation";
                        throw std::logic_error(ss.str());
                    }
                    else
                    {
                        // retrieve field name and value
                        std::string name = line.substr(0, pos);
                        trim(name);
                        if (!overwriteDuplicateFields_ && currentSection->count(name) != 0)
                        {
                            std::stringstream ss;
                            ss << "l." << lineNo
                               << ": ini parsing failed, duplicate field found";
                            throw std::logic_error(ss.str());
                        }
                        std::string value = line.substr(pos + 1, std::string::npos);
                        trim(value);
                        (*currentSection)[name] = value;
                        // store last field name for potential multi-line values
                        mutliLineValueFieldName = name;
                    }
                }
            }
        }

        /** Tries to decode a ini file from the given input string.
          * @param content string to be decoded. */
        void decode(const std::string &content)
        {
            std::istringstream ss(content);
            decode(ss);
        }

        /** Tries to load and decode a ini file from the file at the given path.
          * @param fileName path to the file that should be loaded. */
        void load(const std::string &fileName)
        {
            std::ifstream is(fileName.c_str());
            decode(is);
        }

        /** Encodes this inifile object and writes the output to the given stream.
          * @param os target stream. */
        void encode(std::ostream &os) const
        {
            // iterate through all sections in this file
            for(const auto &filePair : *this)
            {
                os.put('[');
                writeEscaped(os, filePair.first);
                os.put(']');
                os.put('\n');

                // iterate through all fields in the section
                for(const auto &secPair : filePair.second)
                {
                    writeEscaped(os, secPair.first);
                    os.put(fieldSep_);
                    writeEscaped(os, secPair.second.template as<std::string>());
                    os.put('\n');
                } 

                // Add a newline after each section
                os.put('\n');
            }
        }

        /** Encodes this inifile object as string and returns the result.
          * @return encoded infile string. */
        std::string encode() const
        {
            std::ostringstream ss;
            encode(ss);
            return ss.str();
        }

        /** Saves this inifile object to the file at the given path.
          * @param fileName path to the file where the data should be stored. */
        void save(const std::string &fileName) const
        {
            std::ofstream os(fileName.c_str());
            encode(os);
        }
    };

    using IniFile = IniFileBase<std::less<std::string>>;
    using IniSection = IniSectionBase<std::less<std::string>>;
    using IniFileCaseInsensitive = IniFileBase<StringInsensitiveLess>;
    using IniSectionCaseInsensitive = IniSectionBase<StringInsensitiveLess>;
}

#endif
