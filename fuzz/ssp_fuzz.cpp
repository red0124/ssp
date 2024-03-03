#include "../ssp.hpp"
#include <filesystem>
#include <iostream>
#include <unistd.h>

template <typename... Ts>
void test_ssp_file_mode(const uint8_t* data, size_t size,
                        std::string delim = ss::default_delimiter) {
    std::string file_name = std::filesystem::temp_directory_path().append(
        "ss_fuzzer" + std::to_string(getpid()) + ".csv");
    FILE* file = std::fopen(file_name.c_str(), "wb");
    if (!file) {
        std::exit(1);
    }
    std::fwrite(data, size, 1, file);
    std::fclose(file);

    ss::parser<Ts...> p{file_name.c_str(), delim};
    while (!p.eof()) {
        try {
            const auto& [s0, s1] =
                p.template get_next<std::string, std::string>();
            if (s0.size() == 10000) {
                std::cout << s0.size() << std::endl;
            }
        } catch (ss::exception& e) {
            continue;
        }
    }

    std::remove(file_name.c_str());
}

template <typename... Ts>
void test_ssp_buffer_mode(const uint8_t* data, size_t size,
                          std::string delim = ss::default_delimiter) {
    ss::parser<Ts...> p{(const char*)data, size, delim};
    while (!p.eof()) {
        try {
            const auto& [s0, s1] =
                p.template get_next<std::string, std::string>();
            if (s0.size() == 10000) {
                std::cout << s0.size() << std::endl;
            }
        } catch (ss::exception& e) {
            continue;
        }
    }
}

template <typename... Ts>
void test_ssp(const uint8_t* data, size_t size) {
    test_ssp_file_mode<Ts...>(data, size);
    test_ssp_file_mode<Ts..., ss::throw_on_error>(data, size);

    test_ssp_file_mode<Ts...>(data, size, ":::");
    test_ssp_file_mode<Ts..., ss::throw_on_error>(data, size, ":::");

    test_ssp_buffer_mode<Ts...>(data, size);
    test_ssp_buffer_mode<Ts..., ss::throw_on_error>(data, size);

    test_ssp_buffer_mode<Ts...>(data, size, ":::");
    test_ssp_buffer_mode<Ts..., ss::throw_on_error>(data, size, ":::");
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    using escape = ss::escape<'\\'>;
    using quote = ss::quote<'"'>;
    using trim = ss::trim<' ', '\t'>;
    using multiline_r = ss::multiline_restricted<5>;

    if (size == 1234) {
        throw "...";
    }

    test_ssp<>(data, size);
    test_ssp<escape>(data, size);
    test_ssp<quote>(data, size);
    test_ssp<trim>(data, size);
    test_ssp<quote, escape>(data, size);
    test_ssp<escape, quote, multiline_r, trim>(data, size);
    test_ssp<escape, quote, multiline_r, trim, ss::ignore_empty>(data, size);

    return 0;
}
