# Static split parser

A header only "csv" parser which is fast and versatile with modern C++ api. Requires compiler with C++17 support. 

Conversion for numeric values taken from [Oliver Schönrock](https://gist.github.com/oschonrock/67fc870ba067ebf0f369897a9d52c2dd) .   
Function traits taken from [qt-creator](https://code.woboq.org/qt5/qt-creator/src/libs/utils/functiontraits.h.html) .   

# Example 
Lets say we have a csv file containing students in the 
following format <name,age,grade>:

```
$ cat students.csv
James Bailey,65,2.5
Brian S. Wolfe,40,11.9
Nathan Fielder,37,Really good grades
Bill (Heath) Gates,65,3.3
```
```cpp
#include <iostream>
#include <ss/parser.hpp>

int main() {
    ss::parser p{"students.csv", ","};
    if (!p.valid()) {
        exit(EXIT_FAILURE);
    }

    while (!p.eof()) {
        auto [name, age, grade] = p.get_next<std::string, int, double>();

        if (p.valid()) {
            std::cout << name << ' ' << age << ' ' << grade << std::endl;
        }
    }

    return 0;
}
```

And if we compile and execute the program we get the following output:

```
$ ./a.out
James Bailey 65 2.5
Brian S. Wolfe 40 11.9
Bill (Heath) Gates 65 3.3
```
# Features
 * Works on any type
 * No exceptions
 * Columns and rows can be ignored
 * Works with any type of delimiter
 * Can return whole objects composed of converted values
 * Descriptive error handling can be enabled
 * Restrictions can be added for each column
 * Works with `std::optional` and `std::variant`
 * Works with **CRLF** and **LF**
 * Conversions can be chained if invalid
 * Fast

# Instalation

```
$ git clone https://github.com/red0124/ssp
$ cd ssp
$ sudo make install
```
# Usage

...
