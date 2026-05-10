/* encode_ini_file.cpp
 *
 * Author: Fabian Meyer
 * Created On: 14 Nov 2020
 */

#include <inicpp.h>
#include <iostream>

int main()
{
    ini::IniFile inif;

    inif["Foo"]["hello"] = "world";
    inif["Foo"]["float"] = 1.02f;
    inif["Foo"]["int"] = 123;
    inif["Another"]["char"] = 'q';
    inif["Another"]["bool"] = true;

    std::string content = inif.encode();

    std::cout << "Encoded ini file" << std::endl;
    std::cout << content << std::endl;

    return 0;
}