#!/usr/bin/env sh

echo yes | cpan DateTime Capture::Tiny

wget -qO- https://github.com/linux-test-project/lcov/releases/download/v2.0/lcov-2.0.tar.gz | tar xvz
(cd lcov && make install)
lcov --version
