#!/bin/python3

headers_dir = 'include/ss/'
headers = ['type_traits.hpp',
           'exception.hpp',
           'function_traits.hpp',
           'restrictions.hpp',
           'common.hpp',
           'setup.hpp',
           'splitter.hpp',
           'extract.hpp',
           'converter.hpp',
           'parser.hpp']

combined_file = []
includes = []

for header in headers:
    with open(headers_dir + header) as f:
        for line in f.read().splitlines():
            if '#include "' in line or '#include <fast_float' in line:
                continue

            if '#include <' in line:
                includes.append(line)
                continue

            if '#pragma once' not in line:
                combined_file.append(line)

includes = sorted(set(includes))

print('\n'.join(includes))
print('#define SSP_DISABLE_FAST_FLOAT')
print('\n'.join(combined_file))
