# Static split parser

A header only "csv" parser which is fast and versatile with modern c++ api. Requires compiler with c++17 support. 

Conversion for numeric values taken from [oliver schönrock](https://gist.github.com/oschonrock/67fc870ba067ebf0f369897a9d52c2dd) .   
Function traits taken from [qt-creator](https://code.woboq.org/qt5/qt-creator/src/libs/utils/functiontraits.h.html) .   

Example, lets say we have a csv file containing students in the 
following format <name,age,grade>:

```
James Bailey,65,2.5
Brian S. Wolfe,40,11.9
Nathan Fielder,37,Really good grades
Bill (Heath) Gates,65,3.3
```
```
#include <iostream>
#include <ss/parser.hpp>

int main() {
    ss::parser p{"students.csv", ","};
    if (!p.valid()) {
        exit(exit_failure);
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

And if we execute the program we get the following output:

```
$ ./program
James Bailey 65 2.5
Brian S. Wolfe 40 11.9
Bill (Heath) Gates 65 3.3
```

...
