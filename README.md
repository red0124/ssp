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
        std::cout << p.error_msg() << std::endl;
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
 * Easy to use
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

Run tests (optional):
```
$ make test
```

# Usage

## Error handling

Detailed error messages can be accessed via the **error_msg** method, and to   
enable them the error mode has to be changed to **error_mode::String** using   
the **set_error_mode** method:  
```cpp
void parser::set_error_mode(ss::error_mode);
const std::string& parser::error_msg();
bool parser::valid();
```
Error messages can always be disabled by setting the error mode to  
**error_mode::Bool**. An error can be detected using the **valid** method which   
will return **false** if the file could not be opened, or if the conversion   
could not be made (invalid types, invalid number of columns, ...). 

## Conversions
The above example will be used to show some of the features of the library.  
As seen above, the **get_next** method returns a tuple of objects specified  
inside the template type list.   

We could pass **void** as the second template parameter to ignore the second (age)  
column in our csv, a tuple of only 2 parameters will be retuned:  

```cpp
// returns std::tuple<std::string, double>
auto [name, grade] = p.get_next<std::string, void, double>();
```

If a conversion could not be applied, the method would return a tuple of  
default constructed objects, and **valid** would return **false** , for example  
if the third (grade) column in our csv could not be converted to a double  
the conversion would fail. **std::optional<double>** could be passed if we  
wanted the conversion to proceed in the case of a failure:   

```cpp
// returns std::tuple<std::string, int, std::optional<double>>
auto [name, age, grade] = p.get_next<std::string, int, std::optional<double>();
if(grade) {
    // do something with grade
}
```

Similar to **std::optional**, **std::variant** could be used to try other   
conversions if the previous failed _(note: conversion to **std::string** will   
always pass)_:   

```cpp
// returns std::tuple<std::string, int, std::variant<double, char>>
auto [name, age, grade] = 
    p.get_next<std::string, int, std::variant<double, char>();
if(std::holds_alternative<double>(grade)) {
    // grade set as double
} else if(std::holds_alternative<char>(grade)) {
    // grade set as char
}
```

## Restrictions

Custom **restrictions** can be used to narrow down the conversions of unwanted  
values. **ss::ir** (in range) and **ss::ne** (none empty) are one of those:   

```cpp
// returns std::tuple<std::string, int, double>
// ss::ne makes sure that the name is not empty
// ss::ir makes sure that the grade will be in range [0, 10]
auto [name, age, grade] = 
    p.get_next<ss::ne<std::string>, int, ss::ir<double, 0, 10>>();
```

If the restrictions are not met, the conversion will fail.  
Other predefined restrictions are **ss::ax** (all except), **ss::nx** (none except)   
and **ss::oor** (out of range): 

```cpp
// all ints exept 10 and 20
ss::ax<int, 10, 20>
// only 10 and 20
ss::nx<int, 10, 20>
// all values except the range [0, 10]
ss::oor<int, 0, 10>
```

To define a restriction, a class/struct needs to be made which has a   
**ss_valid** method which returns a **bool** and accepts one object. The type of the  
conversion will be the same as the type of the passed object within **ss_valid**   
and not the restriction itself. Optionally, an **error** method can be made to   
describe the invalid conversion.  

```cpp
template <typename T>
struct even {
    bool ss_valid(const T& value) const {
        return value % 2 == 0;
    }

    const char* error() const {
        return "number not even";
    }
};

// ...
// only even numbers will pass
// returns std::tuple<std::string, int>
auto [name, age] = p.get_next<std::string, even<int>, void>();

```
