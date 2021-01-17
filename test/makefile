CXX=g++
CXXFLAGS=-Wall -Wextra -std=c++17 -lstdc++fs
TESTS=test_parser test_converter test_extractions test_splitter

all: $(TESTS)

# pattern rule, replacing built-in implicit .cpp-suffix rule
%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

debug: CXXFLAGS += -g
debug: all

clean:
	@$(RM) -fv $(TESTS)
	@$(RM) *.csv

test: 
	@for i in $(TESTS); do \
		./$$i; \
	done

# don't use any implicit rules
.SUFFIXES:
# these rules won't actually build the targets they're named after
.PHONY: all clean run debug
