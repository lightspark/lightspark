# inifile-cpp
![License](https://img.shields.io/packagist/l/doctrine/orm.svg)
[![CMake](https://github.com/Rookfighter/inifile-cpp/workflows/CMake/badge.svg)](https://github.com/Rookfighter/inifile-cpp/actions/workflows/cmake.yml)
[![codecov](https://codecov.io/gh/Rookfighter/inifile-cpp/graph/badge.svg?token=39FL0C8NRK)](https://codecov.io/gh/Rookfighter/inifile-cpp)

```inifile-cpp``` is a simple and easy to use single header-only ini file en- and decoder for C++.

## Install

Install the headers using the CMake build system:

```sh
cd <path-to-repo>
mkdir build
cd build
cmake ..
make install
```

or simply copy the header file into your project and include it directly.

## Usage

For examples on how to use and extend ```inifile-cpp``` for your custom needs, please have a look at the ```examples/``` directory.

```inifile-cpp``` allows loading data from any ```std::istream``` and requires a
single function call or use the overloaded constructor.

```cpp
#include <inicpp.h>

int main()
{
    // create istream object "is" ...

    // use function
    ini::IniFile myFirstIni;
    myFirstIni.decode(is);

    // or use the constructor
    ini::IniFile mySecondIni(is);
}
```

You can directly load ini-data from files by using the  ```load()``` function. It requires a file path
and automatically parses its contents:

```cpp
#include <inicpp.h>

int main()
{
    // load an ini file
    ini::IniFile myIni;
    myIni.load("some/ini/path");
}
```

You can enable decoding of multi-line values using the  ```setMultiLineValues(true)``` function.  If you do this, field values may be continued on the next line, after indentation.  Each line will be separated by the `\n` character in the final value, and the indentation will be removed.

```cpp
#include <inicpp.h>

int main()
{
    // load an ini file
    ini::IniFile myIni;
    myIni.setMultiLineValues(true);
    myIni.load("some/ini/path");
}
```

When duplicate fields are decoded the previous value is simply overwritten by default. You can disallow duplicate fields from being overwritten by using the ```allowOverwriteDuplicateFields(false)``` function. If you do this, an exception will be thrown if a duplicate field is found inside a section.

```cpp
#include <inicpp.h>

int main()
{
    // load an ini file
    ini::IniFile myIni;
    myIni.allowOverwriteDuplicateFields(false);
    // throws an exception if the ini file has duplicate fields
    myIni.load("some/ini/path");
}
```

Sections and fields can be accessed using the index operator ```[]```.
The values can be converted to various native types:

```cpp
bool myBool = myIni["Foo"]["myBool"].as<bool>();
char myChar = myIni["Foo"]["myChar"].as<char>();
unsigned char myUChar = myIni["Foo"]["myUChar"].as<unsigned char>();
int myInt = myIni["Foo"]["myInt"].as<int>();
unsigned int myUInt = myIni["Foo"]["myUInt"].as<unsigned int>();
long myLong = myIni["Foo"]["myLong"].as<long>();
unsigned long myULong = myIni["Foo"]["myULong"].as<unsigned long>();
float myFloat = myIni["Foo"]["myFloat"].as<float>();
double myDouble = myIni["Foo"]["myDouble"].as<double>();
std::string myStr = myIni["Foo"]["myStr"].as<std::string>();
const char *myStr2 = myIni["Foo"]["myStr"].as<const char*>();
```

Natively supported types are:

* ```bool```
* ```char```
* ```unsigned char```
* ```short```
* ```unsigned short```
* ```int```
* ```unsigned int```
* ```long```
* ```unsigned long```
* ```float```
* ```double```
* ```std::string```
* ```const char *```
* ```std::string_view```

Custom type conversions can be added by implementing specialized template of the ```ini::Convert<T>``` functor (see examples).

Values can be assigned to ini fileds just by using the assignment operator.
The content of the inifile can then be written to any ```std::ostream``` object.

```cpp
#include <inicpp.h>

int main()
{
    // create ostream object "os" ...

    ini::IniFile myIni;

    myIni["Foo"]["myInt"] = 1;
    myIni["Foo"]["myStr"] = "Hello world";
    myIni["Foo"]["myBool"] = true;
    myIni["Bar"]["myDouble"] = 1.2;

    myIni.encode(os);
}
```

You can directly save ini-data to files by using the  ```save()``` function. It requires a file path
and automatically stores the ini file contents:

```cpp
#include <inicpp.h>

int main()
{
    ini::IniFile myIni;

    myIni["Foo"]["myInt"] = 1;
    myIni["Foo"]["myStr"] = "Hello world";
    myIni["Foo"]["myBool"] = true;
    myIni["Bar"]["myDouble"] = 1.2;

    myIni.save("some/ini/path");
}
```

You can define custom type conversions for inifile-cpp which will be automatically used by the assignment operator and the ```as()``` method of ini fields, e.g. you can add support for ```std::vector``` (see also examples):

```cpp
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
```

## Contributing

If you want to contribute new features or bug fixes, simply file a pull request.
Make sure all CI checks pass, otherwise PRs will not be merged.

## License

`inifile-cpp` is licensed under the [MIT license](https://github.com/Rookfighter/inifile-cpp/blob/main/LICENSE.txt)
