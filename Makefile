CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -g -Iinclude
LDFLAGS := -pthread

SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:.cpp=.o)
TARGET := os_course_design

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.cpp include/os_project.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
