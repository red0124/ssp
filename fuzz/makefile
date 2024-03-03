BIN=ssp_fuzz
OPT_BIN=opt_ssp_fuzz
SRC=ssp_fuzz.cpp

all:
	${CXX} ${SRC} -fsanitize=fuzzer -std=c++17 -o ${BIN}
	${CXX} ${SRC} -fsanitize=fuzzer -std=c++17 -O2 -o ${OPT_BIN}

run:
	./${BIN} -max_total_time=300
	./${OPT_BIN} -max_total_time=300
