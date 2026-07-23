CXX := g++
CXXFLAGS := -std=c++17 -g -fPIC
LDFLAGS :=
SRC := main.cpp src/token.cpp src/error.cpp
OBJ := $(SRC:.cpp=.o)
run: $(OBJ)
	$(CXX) $(CXXFLAGS) -o minipar $(OBJ) $(LDFLAGS)
-include $(OBJ:.o=.d)
clean:
	rm -f $(OBJ) minipar
.PHONY: run clean
