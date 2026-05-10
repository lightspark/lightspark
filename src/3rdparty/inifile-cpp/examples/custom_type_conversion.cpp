
/* decode_ini_file.cpp
 *
 * Author: Fabian Meyer
 * Created On: 14 Nov 2020
 */

#include <inicpp.h>
#include <iostream>

// the conversion functor must live in the "ini" namespace
namespace ini
{
    /** Conversion functor to parse std::vectors from an ini field-
     * The generic template can be passed down to the vector. */
    template<typename T>
    struct Convert<std::vector<T>>
    {
        /** Decodes a std::vector from a string. */
        void decode(const std::string &value, std::vector<T> &result)
        {
            result.clear();

            // variable to store the decoded value of each element
            T decoded;
            // maintain a start and end pos within the string
            size_t startPos = 0;
            size_t endPos = 0;
            size_t cnt;

            while(endPos != std::string::npos)
            {
                if(endPos != 0)
                    startPos = endPos + 1;
                // search for the next comma as separator
                endPos = value.find(',', startPos);

                // if no comma was found use the rest of the string
                // as input
                if(endPos == std::string::npos)
                    cnt = value.size() - startPos;
                else
                    cnt = endPos - startPos;

                std::string tmp = value.substr(startPos, cnt);
                // use the conversion functor for the type contained in
                // the vector, so the vector can use any type that
                // is compatible with inifile-cpp
                Convert<T> conv;
                conv.decode(tmp, decoded);
                result.push_back(decoded);
            }
        }

        /** Encodes a std::vector to a string. */
        void encode(const std::vector<T> &value, std::string &result)
        {
            // variable to store the encoded element value
            std::string encoded;
            // string stream to build the result stream
            std::stringstream ss;
            for(size_t i = 0; i < value.size(); ++i)
            {
                // use the conversion functor for the type contained in
                // the vector, so the vector can use any type that
                // is compatible with inifile-cp
                Convert<T> conv;
                conv.encode(value[i], encoded);
                ss << encoded;

                // if this is not the last element add a comma as separator
                if(i != value.size() - 1)
                    ss << ',';
            }
            // store the created string in the result
            result = ss.str();
        }
    };
}

int main()
{
    // create some ini content that we can parse
    std::string content = "[Foo]\nintList=1,2,3,4,5,6,7,8\ndoubleList=3.4,1.2,2.2,4.7";

    // decode the ini contents
    ini::IniFile inputIni;
    inputIni.decode(content);

    // print the results
    std::cout << "Parsed ini file" << std::endl;
    std::cout << "===============" << std::endl;

    // parse the int list
    std::vector<int> intList = inputIni["Foo"]["intList"].as<std::vector<int>>();
    std::cout << "int list:" << std::endl;
    for(size_t i = 0; i < intList.size(); ++i)
        std::cout << "  " << intList[i] << std::endl;

    // parse the double list
    std::vector<double> doubleList = inputIni["Foo"]["doubleList"].as<std::vector<double>>();
    std::cout << "double list:" << std::endl;
    for(size_t i = 0; i < doubleList.size(); ++i)
        std::cout << "  " << doubleList[i] << std::endl;

    std::cout << std::endl;

    // create another ini file for encoding
    ini::IniFile outputIni;
    outputIni["Bar"]["floatList"] = std::vector<float>{1.0f, 9.3f, 3.256f};
    outputIni["Bar"]["boolList"] = std::vector<bool>{true, false, false, true};

    std::cout << "Encoded ini file" << std::endl;
    std::cout << "================" << std::endl;
    std::cout << outputIni.encode() << std::endl;

    return 0;
}