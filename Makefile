# Social Network Analyzer - build file
# Usage:  make          (build the program ./sna)
#         make test     (build and run all 6 test programs)
#         make asan     (run all tests with AddressSanitizer + UBSan = leak/overflow check)
#         make clean

CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic -g -Isrc
LIB_SRC  = src/Utils.cpp src/Graph.cpp src/Queue.cpp src/Algorithms.cpp src/FileManager.cpp src/Security.cpp
TESTS    = test_graph test_queue test_algorithms test_files test_security test_validation

all: sna

sna: $(LIB_SRC) src/Menu.cpp src/Loading.cpp src/main.cpp
	$(CXX) $(CXXFLAGS) -o sna $(LIB_SRC) src/Menu.cpp src/Loading.cpp src/main.cpp

tests/bin/%: tests/%.cpp tests/TestHelper.h $(LIB_SRC)
	mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIB_SRC)

test: $(addprefix tests/bin/,$(TESTS))
	@for t in $(TESTS); do ./tests/bin/$$t || exit 1; done
	@echo "ALL TEST PROGRAMS PASSED"

asan:
	mkdir -p tests/bin_asan
	@for t in $(TESTS); do \
	    $(CXX) $(CXXFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -o tests/bin_asan/$$t tests/$$t.cpp $(LIB_SRC) || exit 1; \
	    ./tests/bin_asan/$$t || exit 1; \
	done
	@echo "ALL TESTS PASSED UNDER ASAN/UBSAN (no leaks, no memory errors)"

clean:
	rm -rf sna tests/bin tests/bin_asan tmp_*.txt

.PHONY: all test asan clean
