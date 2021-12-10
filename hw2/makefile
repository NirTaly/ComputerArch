# 046267 Computer Architecture - Winter 20/21 - HW #2

# Environment for C++ 
CXX = g++
CXXFLAGS = -std=c++11 -g -Wall -Wextra -pedantic-errors -Werror

# Automatically detect whether the bp is C or C++
SRC = $(wildcard cacheSim.cpp cache.cpp)

OBJ = $(patsubst %.cpp,%.o,$(SRC))

cacheSim: $(OBJ)
	$(CXX) -o $@ $(OBJ)

$(OBJ): %.o: %.cpp
	$(CXX) -c $(CXXFLAGS)  -o $@ $^ -lm


.PHONY: clean
clean:
	rm -f *.o
	rm -f cacheSim
