CXX			:= g++
CXX_FLAGS	:= -Wall -Wextra -std=c++17

SRC			:= src
INCLUDE		:= include

EXECUTABLE	:= passthrough

all: $(EXECUTABLE)

run: clean all
	clear
	./$(EXECUTABLE)

$(EXECUTABLE): $(SRC)/*.cpp
	$(CXX) $(CXX_FLAGS) -I$(INCLUDE) $^ -o $@

clean:
	-rm passthrough