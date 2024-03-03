#!/usr/bin/env sh

$CXX $CFLAGS $CXXFLAGS $LIB_FUZZING_ENGINE $SRC/fuzz/ssp_fuzz.cpp
     -I $SRC/include
     -o $OUT/ssp_fuzz
