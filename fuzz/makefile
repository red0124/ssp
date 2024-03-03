BIN=ssp_fuzz
SRC=ssp_fuzz.cpp

all:
	${CXX} ${SRC} -fsanitize=fuzzer -std=c++17 -o ${BIN}

run:
	./${BIN} -max_total_time=300
