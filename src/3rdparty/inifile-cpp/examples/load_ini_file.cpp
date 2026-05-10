/* load_ini_file.cpp
 *
 * Author: Fabian Meyer
 * Created On: 14 Nov 2020
 */

#include <inicpp.h>
#include <iostream>

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        std::cerr << "usage: load_ini_file [FILE_PATh]" << std::endl;
        return 1;
    }

    std::string path = argv[1];

    // load the file
    ini::IniFile inif;
    inif.load(path);

    // show the parsed contents of the ini file
    std::cout << "Parsed ini contents" << std::endl;
    std::cout << "Has " << inif.size() << " sections" << std::endl;
    for(const auto &sectionPair : inif)
    {
        const std::string &sectionName = sectionPair.first;
        const ini::IniSection &section = sectionPair.second;
        std::cout << "Section '" << sectionName << "' has " << section.size() << " fields" << std::endl;

        for(const auto &fieldPair : sectionPair.second)
        {
            const std::string &fieldName = fieldPair.first;
            const ini::IniField &field = fieldPair.second;
            std::cout << "  Field '" << fieldName << "' Value '" << field.as<std::string>() << "'" << std::endl;
        }
    }

    return 0;
}