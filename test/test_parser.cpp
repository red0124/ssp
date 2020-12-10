#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../include/ss/parser.hpp"
#include "doctest.h"
#include <algorithm>
#include <filesystem>

struct unique_file_name {
        const std::string name;

        unique_file_name() : name{std::tmpnam(nullptr)} {
        }

        ~unique_file_name() {
                std::filesystem::remove(name);
        }
};

struct X {
        constexpr static auto delim = ",";
        int i;
        double d;
        std::string s;

        std::string to_string() const {
                return std::to_string(i)
                    .append(delim)
                    .append(std::to_string(d))
                    .append(delim)
                    .append(s);
        }
        auto tied() const {
                return std::tie(i, d, s);
        }
};

template <typename T>
std::enable_if_t<ss::has_m_tied_t<T>, bool> operator==(const T& lhs,
                                                       const T& rhs) {
        return lhs.tied() == rhs.tied();
}

template <typename T>
static void make_and_write(const std::string& file_name,
                           const std::vector<T>& data) {
        std::ofstream out{file_name};
        for (const auto& i : data) {
                out << i.to_string() << std::endl;
        }
}

TEST_CASE("testing parser") {
        unique_file_name f;
        std::vector<X> data = {{1, 2, "x"}, {3, 4, "y"}, {5, 6, "z"}};
        make_and_write(f.name, data);
        {
                ss::parser p{f.name, ","};
                std::vector<X> i;

                while (!p.eof()) {
                        auto a = p.get_next<int, double, std::string>();
                        i.emplace_back(ss::to_struct<X>(a));
                }

                CHECK(std::equal(i.begin(), i.end(), data.begin()));
        }

        {
                ss::parser p{f.name, ","};
                std::vector<X> i;

                p.ignore_next();
                while (!p.eof()) {
                        auto a = p.get_next<int, double, std::string>();
                        i.emplace_back(ss::to_struct<X>(a));
                }

                CHECK(std::equal(i.begin(), i.end(), data.begin() + 1));
        }

        {
                ss::parser p{f.name, ","};
                std::vector<X> i;

                while (!p.eof()) {
                        i.push_back(
                            p.get_struct<X, int, double, std::string>());
                }

                CHECK(std::equal(i.begin(), i.end(), data.begin()));
        }

        {
                ss::parser p{f.name, ","};
                std::vector<X> i;

                while (!p.eof()) {
                        i.push_back(p.get_next<X>());
                }

                CHECK(std::equal(i.begin(), i.end(), data.begin()));
        }

        {
                ss::parser p{f.name, ","};
                std::vector<X> i;

                while (!p.eof()) {
                        auto a = p.get_struct<X, ss::ax<int, 3>, double,
                                              std::string>();
                        if (p.valid()) {
                                i.push_back(a);
                        }
                }
                std::vector<X> expected = {{1, 2, "x"}, {5, 6, "z"}};
                CHECK(std::equal(i.begin(), i.end(), expected.begin()));
        }

        {
                ss::parser p{f.name, ","};
                std::vector<X> i;

                while (!p.eof()) {
                        auto a = p.get_struct<X, ss::nx<int, 3>, double,
                                              std::string>();
                        if (p.valid()) {
                                i.push_back(a);
                        }
                }
                std::vector<X> expected = {{3, 4, "y"}};
                CHECK(std::equal(i.begin(), i.end(), expected.begin()));
        }

        {
                unique_file_name empty_f;
                std::vector<X> empty_data = {};
                make_and_write(empty_f.name, empty_data);

                ss::parser p{empty_f.name, ","};
                std::vector<X> i;

                while (!p.eof()) {
                        i.push_back(p.get_next<X>());
                }
                CHECK(i.empty());
        }
}

size_t move_called = 0;
size_t copy_called = 0;

struct my_string {
        char* data{nullptr};

        my_string() = default;

        ~my_string() {
                delete[] data;
        }

        my_string(const my_string&) {
                copy_called++;
        }

        my_string(my_string&& other) : data{other.data} {
                move_called++;
                other.data = nullptr;
        }
};

template <>
inline bool ss::extract(const char* begin, const char* end, my_string& s) {
        size_t size = end - begin;
        s.data = new char[size + 1];
        strncpy(s.data, begin, size);
        s.data[size] = '\0';
        return true;
}

struct Y {
        my_string x;
        my_string y;
        my_string z;
        auto tied() {
                return std::tie(x, y, z);
        }
};

TEST_CASE("testing the moving of parsed values") {
        size_t move_called_one_col;

        {
                unique_file_name f;
                std::ofstream out{f.name};
                out << "x" << std::endl;

                ss::parser p{f.name, ","};
                auto x = p.get_next<my_string>();
                CHECK(copy_called == 0);
                CHECK(move_called < 3);
                move_called_one_col = move_called;
                move_called = copy_called = 0;
        }

        unique_file_name f;
        {
                std::ofstream out{f.name};
                out << "a,b,c" << std::endl;
        }

        {

                ss::parser p{f.name, ","};
                auto x = p.get_next<my_string, my_string, my_string>();
                CHECK(copy_called == 0);
                CHECK(move_called == 3 * move_called_one_col);
                move_called = copy_called = 0;
        }

        {
                ss::parser p{f.name, ","};
                auto x = p.get_struct<Y, my_string, my_string, my_string>();
                CHECK(copy_called == 0);
                CHECK(move_called == 6 * move_called_one_col);
                move_called = copy_called = 0;
        }

        {
                ss::parser p{f.name, ","};
                auto x = p.get_next<Y>();
                CHECK(copy_called == 0);
                CHECK(move_called == 6 * move_called_one_col);
                move_called = copy_called = 0;
        }
}
