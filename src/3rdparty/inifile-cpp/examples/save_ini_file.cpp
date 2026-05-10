/* save_ini_file.cpp
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
        std::cerr << "usage: save_ini_file [FILE_PATh]" << std::endl;
        return 1;
    }

    std::string path = argv[1];

    ini::IniFile inif;

    inif["Foo"]["hello"] = "world";
    inif["Foo"]["float"] = 1.02f;
    inif["Foo"]["int"] = 123;
    inif["Another"]["char"] = 'q';
    inif["Another"]["bool"] = true;

    inif.save(path);

    std::cout << "Saved ini file." << std::endl;

    return 0;
}