
```
   __________ ____ 
  / ___/ ___// __ \
  \__ \\__ \/ /_/ /
 ___/ /__/ / ____/ 
/____/____/_/      
```

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![coverage](https://coveralls.io/repos/github/red0124/ssp/badge.svg?branch=master)](https://coveralls.io/github/red0124/ssp?branch=master)
[![fuzz](https://github.com/red0124/ssp/workflows/fuzz-ci/badge.svg)](https://github.com/red0124/ssp/actions/workflows/fuzz.yml)
[![single-header](https://github.com/red0124/ssp/workflows/single-header-ci/badge.svg)](https://github.com/red0124/ssp/actions/workflows/single-header.yml)
[![ubuntu-latest-gcc](https://github.com/red0124/ssp/workflows/ubuntu-latest-gcc-ci/badge.svg)](https://github.com/red0124/ssp/actions/workflows/ubuntu-latest-gcc.yml)
[![ubuntu-latest-clang](https://github.com/red0124/ssp/workflows/ubuntu-latest-clang-ci/badge.svg)](https://github.com/red0124/ssp/actions/workflows/ubuntu-latest-clang.yml)
[![windows-msys2-gcc](https://github.com/red0124/ssp/workflows/win-msys2-gcc-ci/badge.svg)](https://github.com/red0124/ssp/actions/workflows/win-msys2-gcc.yml)
[![windows-msys2-clang](https://github.com/red0124/ssp/workflows/win-msys2-clang-ci/badge.svg)](https://github.com/red0124/ssp/actions/workflows/win-msys2-clang.yml)
[![windows-msvc](https://github.com/red0124/ssp/workflows/win-msvc-ci/badge.svg)](https://github.com/red0124/ssp/actions/workflows/win-msvc.yml)
[![macos-apple-clang](https://github.com/red0124/ssp/workflows/macos-apple-clang-ci/badge.svg)](https://github.com/red0124/ssp/actions/workflows/macos-apple-clang.yml)

A header only CSV parser which is fast and versatile with modern C++ API. Requires compiler with C++17 support. [Can also be used to efficiently convert strings to specific types.](#the-converter)

Conversion for floating point values invoked using [fast-float](https://github.com/fastfloat/fast_float) . \
Function traits taken from *qt-creator* .

# Example 
Lets say we have a CSV file containing students in a given format (Id,Age,Grade) and we want to parse and print all the valid values:

```shell
$ cat students.csv
James Bailey,65,2.5
Brian S. Wolfe,40,1.9
Bill (Heath) Gates,65,3.3
```
```cpp
#include <iostream>
#include <ss/parser.hpp>

int main() {
    ss::parser<ss::throw_on_error> p{"students.csv"};

    for (const auto& [id, age, grade] : p.iterate<std::string, int, float>()) {
        std::cout << id << ' ' << age << ' ' << grade << std::endl;
    }

    return 0;
}
```
And if we compile and execute the program we get the following output:
```shell
$ ./a.out
James Bailey 65 2.5
Brian S. Wolfe 40 1.9
Bill (Heath) Gates 65 3.3
```
# Features
 * [Works on any type](#custom-conversions)
 * Easy to use
 * Can work without exceptions
 * [Works with headers](#headers)
 * [Works with quotes, escapes and spacings](#setup)
 * [Works with CSV data stored in buffers](#buffer-mode)
 * [Works with values containing new lines](#multiline)
 * [Columns and rows can be ignored](#special-types)
 * [Works with any type of delimiter](#delimiter)
 * Can return whole objects composed of converted values
 * [Error handling can be configured](#error-handling)
 * [Restrictions can be added for each column](#restrictions)
 * [Works with `std::optional` and `std::variant`](#special-types)
 * Works with **`CRLF`** and **`LF`**
 * [Conversions can be chained if invalid](#substitute-conversions)
 * Fast

# Single header

The library can be used with a single header file **`ssp.hpp`**, but it suffers a significant performance loss when converting floating point values since the **`fast_float`** library is not present within the file.

# Installation

```shell
$ git clone https://github.com/red0124/ssp
$ cd ssp
$ cmake --configure .
$ sudo make install
```

*Note, this will also install the fast_float library.*\
The library supports [CMake](#Cmake) and [meson](#Meson) build systems

# Usage

## Headers

The parser can be told to use only certain columns by parsing the header. This can be done with the **`use_fields`** method. It accepts any number of string-like arguments or even an **`std::vector<std::string>`** with the field names. If any of the fields are not found within the header or if any fields are defined multiple times it will result in an error.
```shell
$ cat students_with_header.csv
Id,Age,Grade
James Bailey,65,2.5
Brian S. Wolfe,40,1.9
Bill (Heath) Gates,65,3.3
```
```cpp
// ...
ss::parser<ss::throw_on_error> p{"students_with_header.csv"};
p.use_fields("Id", "Grade");

for(const auto& [id, grade] : p.iterate<std::string, float>()) {
    std::cout << id << ' ' << grade << std::endl;
}
// ...
```
```shell
$ ./a.out
James Bailey 2.5
Brian S. Wolfe 1.9
Bill (Heath) Gates 3.3
```
The header can be ignored using the **`ss::ignore_header`** [setup](#Setup) option or by calling the **`ignore_next`** method after the parser has been constructed. If the header has been ignored calling any method related to header usage will result in a compilation error.
```cpp
ss::parser<ss::ignore_header> p{file_name};
```
The fields with which the parser works with can be modified at any given time. The parser can also check if a field is present within the header by using the **`field_exists`** method.
```cpp
    // ...
    ss::parser<ss::throw_on_error> p{"students_with_header.csv"};
    p.use_fields("Grade");

    const auto& grade = p.get_next<std::string>();
    std::cout << grade << std::endl;

    if (p.field_exists("Id")) {
        p.use_fields("Grade", "Id");
        for (const auto& [grade, id] : p.iterate<float, std::string>()) {
            std::cout << grade << ' ' << id << std::endl;
        }
    }
    // ...
```
```shell
$ ./a.out
2.5
1.9 Brian S. Wolfe
3.3 Bill (Heath) Gates
```
The header is parsed with the same rules as other rows, the only difference is that **`multiline`** will be disabled when parsing the header. To get the data that is
present in the header as a **`std::vector<std::string>`**, the **`header`** method can be used, and to get the header line before it has been parsed, the **`raw_header`** method can be used:
```cpp
    // ...
    ss::parser<ss::throw_on_error> p{"students_with_header.csv"};

    std::cout << p.raw_header() << std::endl;

    for (const auto& field: p.header()) {
        std::cout << "> " << field << std::endl;
    }
    // ...
```
```shell
$ ./a.out
Id,Age,Grade
> Id
> Age
> Grade
```
Methods related to headers can also fail, the error handling of these is done in the same way as for other methods.

## Conversions
An alternate loop to the example above would look like: 
```cpp
// ...
ss::parser p{"students.csv"};

while (!p.eof()) {
    const auto& [id, age, grade] = p.get_next<std::string, int, float>();

    if (p.valid()) {
        std::cout << id << ' ' << age << ' ' << grade << std::endl;
    }
}
// ...
```

The alternate example with exceptions disabled will be used to show some of the features of the library. The **`get_next`** method returns a tuple of objects specified inside the template type list.

If a conversion could not be applied, the method would return a tuple of default constructed objects, and the **`valid`** method would return **`false`**, for example if the third (grade) column in our CSV could not be converted to a float the conversion would fail.

If **`get_next`** is called with a **`tuple`** as template parameter it would behave identically to passing the same tuple parameters to **`get_next`**:
```cpp
using student = std::tuple<std::string, int, float>;

// returns std::tuple<std::string, int, float>
auto [id, age, grade] = p.get_next<student>();
```
*Note, it does not always return the specified tuple since the returned tuples parameters may be altered as explained below (no void, no restrictions, ...)*

Whole objects can be returned using the **`get_object`** function which takes the tuple, created in a similar way as **`get_next`** does it, and creates an object out of it:
```cpp
struct student {
    std::string id;
    int age;
    float grade;
};
```
```cpp
// returns student
auto student = p.get_object<student, std::string, int, float>();
```
This works with any object if the constructor could be invoked using the template arguments given to **`get_object`**:
```cpp
// returns std::vector<std::string> containing 2 elements
auto vec = p.get_object<std::vector<std::string>, std::string, std::string>();
```
An iterator loop as in the first example which returns objects would look like:
```cpp
for (const student& s : p.iterate_object<student, std::string, int, float>()) {
    // ...
}
```
And finally, using something I personally like to do, a struct (class) with a **`tied`** method which returns a tuple of references to to the members of the struct.
```cpp
struct student {
    std::string id;
    int age;
    float grade;

    auto tied() { return std::tie(id, age, grade); }
};
```
The method can be used to compare the object, serialize it, deserialize it, etc. Now **`get_next`** can accept such a struct and deduce the types to which to convert the CSV.
```cpp
// returns student
auto s = p.get_next<student>();
```
This works with the iteration loop too.
*Note, the order in which the members of the tied method are returned must match the order of the elements in the CSV*.

## Buffer mode
The parser also works with buffers containing CSV data instead of files. To parse buffer data with the parser simply create the parser by giving it the buffer, as **`const char*`**, and its size. The initial example using a buffer instead of a file would look similar to this:
```cpp
std::string buffer = "James Bailey,65,2.5\nBrian S. Wolfe,40,1.9\n";

ss::parser<ss::throw_on_error> p{buffer.c_str(), buffer.size()};

for (const auto& [id, age, grade] : p.iterate<std::string, int, float>()) {
    std::cout << id << ' ' << age << ' ' << grade << std::endl;
}

return 0;
```
## Setup
By default, many of the features supported by the parser are disabled. They can be enabled within the template parameters of the parser. For example, to enable quoting and escaping the parser would look like:
```cpp
ss::parser<ss::quote<'"'>, ss::escape<'\\'>> p0{file_name};
```
The order of the defined setup parameters is not important:
```cpp
// equivalent to p0
ss::parser<ss::escape<'\\'>, ss::quote<'"'>> p1{file_name};
```
The setup can also be predefined:
```cpp
using my_setup = ss::setup<ss::escape<'\\'>, ss::quote<'"'>>;
// equivalent to p0 and p1
ss::parser<my_setup> p2{file_name};
```
Invalid setups will be met with **`static_asserts`**.
*Note, most setup parameters defined come with a slight performance loss, so use them only if needed.*

### Delimiter
By default, **`,`** is used as the delimiter, a custom delimiter can be specified as the second constructor parameter.
```cpp
ss::parser p{file_name, "--"};
```
*Note, the delimiter can consist of multiple characters but the parser is slightly faster when using single character delimiters.*

### Empty lines
Empty lines can be ignored by defining **`ss::ignore_empty`** within the setup parameters:
```cpp
ss::parser<ss::ignore_empty> p{file_name};
```
If this setup option is not set then reading an empty line will result in an error (unless only one column is present within the CSV).

### Quoting
Quoting can be enabled by defining **`ss::quote`** within the setup parameters. A single character can be defined as the quoting character, for example to use **`"`** as a quoting character:
```cpp
ss::parser<ss::quote<'"'>> p{file_name};
```
Double quote can be used to escape a quote inside a quoted row.
```
"James ""Bailey""" -> 'James "Bailey"'
```
Unterminated quotes result in an error (if multiline is not enabled).
```
"James Bailey,65,2.5 -> error
```
### Escaping
Escaping can be enabled by defining **`ss::escape`** within the setup parameters. Multiple character can be defined as escaping characters.It simply removes any special meaning of the character behind the escaped character, anything can be escaped. For example to use ``\`` as an escaping character:
```cpp
ss::parser<ss::escape<'\\'>> p{file_name};
```
Double escape can be used to escape an escape.
```
James \\Bailey -> 'James \Bailey'
```
Unterminated escapes result in an error.
```
James Bailey,65,2.5\\0 -> error
```
Its usage has more impact when used with quoting or spacing:
```
"James \"Bailey\"" -> 'James "Bailey"'
```
### Spacing
Spacing can be enabled by defining **`ss::trim`** , **`ss::trim_left`**  or **`ss::trim_right`** within the setup parameters. Multiple character can be defined as spacing characters, for example to use ``' '`` as an spacing character **`ss::trim<' '>`** needs to be defined. It removes any space from both sides of the row. To trim only the right side **`ss::trim_right`** can be used, and intuitively **`ss::trim_left`** to trim only the left side. If **`ss::trim`** is enabled, those lines would have an equivalent output:
```
James Bailey,65,2.5
  James Bailey  ,65,2.5
James Bailey,  65,    2.5   
```
Escaping and quoting can be used to leave the space if needed.
```
" James Bailey " -> ' James Bailey '
  " James Bailey "   -> ' James Bailey '
\ James Bailey\  -> ' James Bailey '
  \ James Bailey\    -> ' James Bailey '
"\ James Bailey\ " -> ' James Bailey '
```

### Multiline
Multiline can be enabled by defining **`ss::multilne`** within the setup parameters. It enables the possibility to have the new line characters within rows. The new line character needs to be either escaped or within quotes so either **`ss::escape`** or **`ss::quote`** need to be enabled. There is a specific problem when using multiline, for example, if a row had an unterminated quote, the parser would assume it to be a new line within the row, so until another quote is found, it will treat it as one line which is fine usually, but it can cause the whole CSV file to be treated as a single line by mistake. To prevent this **`ss::multiline_restricted`** can be used which accepts an unsigned number representing the maximum number of lines which can be allowed as a single multiline. Examples:

```cpp
ss::parser<ss::multiline, ss::quote<'\"'>, ss::escape<'\\'>> p{file_name};
```
```
"James\n\n\nBailey" -> 'James\n\n\nBailey'
James\\n\\n\\nBailey -> 'James\n\n\nBailey'
"James\n\n\n\n\nBailey" -> 'James\n\n\n\n\nBailey'
```
```cpp
ss::parser<ss::multiline_restricted<4>, ss::quote<'\"'>, ss::escape<'\\'>> p{file_name};
```
```
"James\n\n\nBailey" -> 'James\n\n\nBailey'
James\\n\\n\\nBailey -> 'James\n\n\nBailey'
"James\n\n\n\n\nBailey" -> error
```
### Example
An example with a more complicated setup:
```cpp
ss::parser<ss::escape<'\\'>, 
           ss::quote<'"'>,
           ss::trim<' ', '\t'>,
           ss::multiline_restricted<5>> p{file_name};

for (const auto& [id, age, grade] : p.iterate<std::string, int, float>()) {
    if (p.valid()) {
        std::cout << "'" << id << ' ' << age << ' ' << grade << "'\n";
    }
}
```
input:
```
      "James Bailey"   ,  65  ,     2.5\t\t\t
\t \t Brian S. Wolfe, "40" ,  "\1.9"
   "\"Nathan Fielder"""   ,  37  ,   Really good grades
"Bill
\"Heath""
Gates",65,   3.3
```
output:
```
'James Bailey 65 2.5'
'Brian S. Wolfe 40 1.9'
'Bill
"Heath"
Gates 65 3.3'
```
## Special types

Passing **`void`** makes the parser ignore a column. In the initial example **`void`** could be given as the second template parameter to ignore the second (age) column in the CSV, a tuple of only 2 parameters would be retuned:
```cpp
// returns std::tuple<std::string, float>
auto [id, grade] = p.get_next<std::string, void, float>();
```
Works with different types of conversions too:
```cpp
using student = std::tuple<std::string, void, float>;

// returns std::tuple<std::string, float>
auto [id, grade] = p.get_next<student>();
```
Values can also be converted to **`std::string_view`**. It is more efficient then converting values to **`std::string`** but one must be careful with the lifetime of it.
```cpp
// string_view id stays valid until the next line is read
auto [id, age, grade] = p.get_next<std::string_view, int, float>();
```

To ignore a whole row, **`ignore_next`** could be used, returns **`false`** if **`eof`**:
```cpp
bool parser::ignore_next();
```
**`std::optional`** could be passed if we wanted the conversion to proceed in the case of a failure returning **`std::nullopt`** for the specified column:

```cpp
// returns std::tuple<std::string, int, std::optional<float>>
auto [id, age, grade] = p.get_next<std::string, int, std::optional<float>>();
if (grade) {
    std::cout << grade.value() << std::endl;
}
```
Similar to **`std::optional`**, **`std::variant`** could be used to try other conversions if the previous failed _(Note, conversion to std::string will always pass)_:
```cpp
// returns std::tuple<std::string, int, std::variant<float, char>>
auto [id, age, grade] = 
    p.get_next<std::string, int, std::variant<float, char>>();
if (std::holds_alternative<float>(grade)) {
    // grade set as float
} else if (std::holds_alternative<char>(grade)) {
    // grade set as char
}
```
Passing **`char`** and types that are aliases to it such as **`uint8_t`** and **`int8_t`** make the parser interpret the input data as a single character in a similar way to how **`std::cin`** does it. To read numeric values into something like **`uint8_t`** the **`ss::uint8`** and **`ss::int8`** types can be used. These are wrappers around the corresponding char aliases and can be implicitly converted to and from them. When these types are given to the parser he will try to read the given data and store it in the underlying element, but this time as a numeric value instead of a single character.
```cpp
// returns std::tuple<std::string, ss::uint8, float>
auto [id, age, grade] = p.get_next<std::string, ss::uint8, float>();
uint8_t age_copy = age;
```
## Restrictions

Custom **`restrictions`** can be used to narrow down the conversions of unwanted values. **`ss::ir`** (in range) and **`ss::ne`** (none empty) are some of those:
```cpp
// ss::ne makes sure that the id is not empty
// ss::ir makes sure that the grade will be in range [0, 10]
// returns std::tuple<std::string, int, float>
auto [id, age, grade] = 
    p.get_next<ss::ne<std::string>, int, ss::ir<float, 0, 10>>();
```
If the restrictions are not met, the conversion will fail. Other predefined restrictions are **`ss::ax`** (all except), **`ss::nx`** (none except) and **`ss::oor`** (out of range), **`ss::lt`** (less than), ...(see *restrictions.hpp*):
```cpp
// all ints exept 10 and 20
ss::ax<int, 10, 20>
// only 10 and 20
ss::nx<int, 10, 20>
// all values except the range [0, 10]
ss::oor<int, 0, 10>
```
To define a restriction, a class/struct needs to be made which has a **`ss_valid`** method which returns a **`bool`** and accepts one object. The type of the conversion will be the same as the type of the passed object within **`ss_valid`** and not the restriction itself. Optionally, an **`error`** method can be made to describe the invalid conversion.
```cpp
template <typename T>
struct even {
    bool ss_valid(const T& value) const {
        return value % 2 == 0;
    }

    // optional
    const char* error() const {
        return "number not even";
    }
};
```
```cpp
// ...
// only even numbers will pass without invoking error handling
// returns std::tuple<std::string, int>
const auto& [id, age] = p.get_next<std::string, even<int>, void>();
// ...
```
## Custom conversions

Custom types can be used when converting values. A specialization of the **`ss::extract`** function needs to be made and you are good to go. A custom conversion for an enum would look like this:
```cpp
enum class shape { circle, square, rectangle, triangle };

template <>
inline bool ss::extract(const char* begin, const char* end, shape& dst) {
    const static std::unordered_map<std::string, shape>
        shapes{{"circle", shape::circle},
               {"square", shape::square},
               {"rectangle", shape::rectangle},
               {"triangle", shape::triangle}};

    if (auto it = shapes.find(std::string(begin, end)); it != shapes.end()) {
        dst = it->second;
        return true;
    }
    return false;
}
```
The shape enum will be used in an example below. The **`inline`** is there just to prevent multiple definition errors. The function returns **`true`** if the conversion was a success, and **`false`** otherwise. The function uses **`const char*`** begin and end for performance reasons.

## Error handling

By default, the parser handles errors only using the **`valid`** method which would return **`false`** if the file could not be opened, or if the conversion could not be made (invalid types, invalid number of columns, ...).\
The **`eof`** method can be used to detect if the end of the file was reached.

Detailed error messages can be accessed via the **`error_msg`** method, and to enable them **`ss::string_error`** needs to be included in the setup. If **`ss::string_error`** is not defined, the **`error_msg`** method will not be defined either.

The line number can be fetched using the **`line`** method.
The cursor position can be fetched using the **`position`** method.
```cpp
const std::string& parser::error_msg() const;
bool parser::valid() const;
bool parser::eof() const;
size_t parser::line() const;
size_t parser::position() const;

// ...
ss::parser<ss::string_error> parser;
```

The above two methods are preferable if invalid inputs are expected and allows for fast handling, but the parser can also be forced to throw an exception in case of an invalid input using the **`ss::throw_on_error`** setup option. Enabling exceptions also makes the **`valid`** method always return **`true`**.

```cpp
ss::parser<ss::throw_on_error> parser;
```
*Note, enabling this option will also make the parser throw if the constructor fails.*

## Substitute conversions

The parser can also be used to effectively parse files whose rows are not always in the same format (not a classical CSV but still CSV-like). A more complicated example would be the best way to demonstrate such a scenario.\
***Important, substitute conversions do not work when throw_on_error is enabled.***

Supposing we have a file containing different shapes in given formats: 
 * circle RADIUS
 * square SIDE
 * rectangle SIDE_A SIDE_B
 * triangle SIDE_A SIDE_B SIDE_C

```
rectangle 2 3
circle 10
triangle 3 4 5
...
```

The delimiter is " ", and the number of columns varies depending on which shape it is. We are required to read the file and to store information (shape and area) of the shapes into a data structure in the same order as they are in the file. 
```cpp
ss::parser p{"shapes.txt", " "};
if (!p.valid()) {
    exit(EXIT_FAILURE);
}

std::vector<std::pair<shape, double>> shapes;

while (!p.eof()) {
    // non negative double
    using udbl = ss::gte<double, 0>;

    auto [circle_or_square, rectangle, triangle] =
          p.try_next<ss::nx<shape, shape::circle, shape::square>, udbl>()
            .or_else<ss::nx<shape, shape::rectangle>, udbl, udbl>()
            .or_else<ss::nx<shape, shape::triangle>, udbl, udbl, udbl>()
            .values();

    if (!p.valid()) {
        // handle error
        continue;
    }

    if (circle_or_square) {
        auto& [s, x] = circle_or_square.value();
        double area = (s == shape::circle) ? x * x * M_PI : x * x;
        shapes.emplace_back(s, area);
    }

    if (rectangle) {
        auto& [s, a, b] = rectangle.value();
        shapes.emplace_back(s, a * b);
    }

    if (triangle) {
        auto& [s, a, b, c] = triangle.value();
        double sh = (a + b + c) / 2;
        if (sh >= a && sh >= b && sh >= c) {
            double area = sqrt(sh * (sh - a) * (sh - b) * (sh - c));
            shapes.emplace_back(s, area);
        }
    }
}

/* do something with the stored shapes */
/* ... */
```
It is quite hard to make an error this way since most things will be checked at compile time.

The **`try_next`** method works in a similar way as **`get_next`** but returns a **`composite`** which holds a **`tuple`** with an **`optional`** to the **`tuple`** returned by **`get_next`**. This **`composite`** has an **`or_else`** method (looks a bit like **`tl::expected`**) which is able to try additional conversions if the previous failed. **`or_else`** also returns a **`composite`**, but in its tuple is the **`optional`** to the **`tuple`** of the previous conversions and an **`optional`** to the **`tuple`** of the new conversion. (sounds more complicated than it is.

To fetch the **`tuple`** from the **`composite`** the **`values`** method is used. The value of the above used conversion would look something like this:
```cpp
std::tuple<
    std::optional<std::tuple<shape, double>>,
    std::optional<std::tuple<shape, double, double>>,
    std::optional<std::tuple<shape, double, double, double>>
    >
```
Similar to the way that **`get_next`** has a **`get_object`** alternative, **`try_next`** has a **`try_object`** alternative, and **`or_else`** has a **`or_object`** alternative. Also all rules applied to **`get_next`** also work with **`try_next`** , **`or_else`**, and all the other **`composite`** conversions.

Each of those **`composite`** conversions can accept a lambda (or anything callable) as an argument and invoke it in case of a valid conversion. That lambda itself need not have any arguments, but if it does, it must either accept the whole **`tuple`**/object as one argument or all the elements of the tuple separately. If the lambda returns something that can be interpreted as **`false`** the conversion will fail, and the next conversion will try to apply. Rewriting the whole while loop using lambdas would look like this:
```cpp
// non negative double
using udbl = ss::gte<double, 0>;

while (!p.eof()) {
    p.try_next<ss::nx<shape, shape::circle, shape::square>, udbl>(
         [&](const auto& data) {
             const auto& [s, x] = data;
             double area = (s == shape::circle) ? x * x * M_PI : x * x;
             shapes.emplace_back(s, area);
         })
        .or_else<ss::nx<shape, shape::rectangle>, udbl, udbl>(
            [&](shape s, double a, double b) { shapes.emplace_back(s, a * b); })
        .or_else<ss::nx<shape, shape::triangle>, udbl, udbl, udbl>(
            [&](auto s, auto a, auto b, auto c) {
                double sh = (a + b + c) / 2;
                if (sh >= a && sh >= b && sh >= c) {
                    double area = sqrt(sh * (sh - a) * (sh - b) * (sh - c));
                    shapes.emplace_back(s, area);
                }
            })
        .on_error([] {
            // handle error
        });
}
```
It is a bit less readable, but it removes the need to check which conversion was invoked. The **`composite`** also has an **`on_error`** method which accepts a lambda which will be invoked if no previous conversions were successful. The lambda can take no arguments or just one argument, an **`std::string`**, in which the error message is stored if **`string_error`** is enabled:
```cpp
p.try_next<int>()
    .on_error([](const std::string& e) { /* int conversion failed */ })
    .or_object<x, double>()
    .on_error([] { /* int and x conversions failed (all previous failed) */ });
```
*See unit tests for more examples.*

# Rest of the library

First of all, *type_traits.hpp* and *function_traits.hpp* contain many handy traits used in the parser. Most of them are operating on tuples of elements and can be utilized in projects. 

## The converter

**`ss::parser`** is used to manipulate on files. It has a builtin file reader, but the conversions themselves are done using the **`ss::converter`**.

To convert a string the **`convert`** method can be used. It accepts a c-string as input and a delimiter, as **`std::string`**, and retruns a **`tuple`** of objects in the same way **`get_next`** does it for the parser. A whole object can be returned too using the **`convert_object`** method, again in an identical way **`get_object`** doest it for the parser.
```cpp
ss::converter c;

auto [x, y, z] = c.convert<int, double, char>("10::2.2::3", "::");
if (c.valid()) {
    // do something with x y z
}

auto s = c.convert_object<student, std::string, int, double>("name,20,10", ",");
if (c.valid()) {
    // do something with s
}
```
All setup parameters, special types and restrictions work on the converter too.  
Error handling is also identical to error handling of the parser.

The converter has also the ability to just split the line, and depending if either quoting or escaping are enabled it may change the line, rather than creating a copy, for performance reasons. It returns an **`std::vector`** of **`std::pair`**s of pointers, begin and end, each pair representing a split segment (column) of the whole string. The vector can then be used in a overloaded **`convert`** method. This allows the reuse of the same line without splitting it on every conversion.
```cpp
ss::converter c;
auto split_line = c.split("circle 10", " ");
auto [s, r] = c.convert<shape, int>(split_line);
```
Using the converter is also an easy and fast way to convert single values.
```cpp
ss::converter c;
std::string s;
std::cin >> s;
int num = c.convert<int>(s.c_str());
```
The same setup parameters also apply for the converter, tho multiline has not impact on it. Since escaping and quoting potentially modify the content of the given line, a converter which has those setup parameters defined does not have the same convert method, **`the input line cannot be const`**.

# Using as a project dependency

## CMake

If the repository is cloned within the CMake project, it can be added in the following way:
```cmake
add_subdirectory(ssp)
```
Alternatively, it can be fetched from the repository:
```cmake
include(FetchContent)
FetchContent_Declare(
  ssp
  GIT_REPOSITORY https://github.com/red0124/ssp.git
  GIT_TAG origin/master
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(ssp)
```
Either way, after you prepare the target, you just have to invoke it in your project:
```cmake
target_link_libraries(project PUBLIC ssp fast_float)
```
## Meson

Create an *ssp.wrap* file in your *subprojects* directory with the following content:
```wrap
[wrap-git]
url = https://github.com/red0124/ssp
revision = origin/master
```
Then simply fetch the dependency and it is ready to be used:
```meson
ssp_dep = dependency('ssp')
```
