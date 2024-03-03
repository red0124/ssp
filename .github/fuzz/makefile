EXE=ssp_fuzz

all:
	clang++ ${CXXFLAGS} ssp_fuzz.cpp -fsanitize=fuzzer -std=c++17 -o ${EXE}

run:
	./${EXE} -max_total_time=900
