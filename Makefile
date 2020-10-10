OPTFLAGS=-O3 -fomit-frame-pointer -funroll-loops -fstrict-aliasing -march=native -mtune=native -msse4.2 -mbmi2 -mavx
WARNFLAGS=-Wall -Wextra -Wshadow -Wstrict-aliasing -Wcast-qual -Wcast-align -Wpointer-arith -Wredundant-decls -Wfloat-equal -Wswitch-enum
MISCFLAGS=-fstack-protector -fvisibility=hidden
DEVFLAGS=-Wno-unused-parameter -Wno-unused-variable

ifndef OPTIMIZED
	MISCFLAGS+=-g -DDEBUG $(DEVFLAGS)
endif

CXXFLAGS=-std=gnu++17 -fno-rtti $(OPTFLAGS) $(WARNFLAGS) $(MISCFLAGS)

lutbs-test: lutbs-test.cpp lutbs.hpp
	$(CXX) $(CXXFLAGS) $< -o $@

relbits: relbits.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

#abstest: abstest.cpp lutbs.hpp
#	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: clean bench

clean:
	rm -f lutbs-test core core.*
