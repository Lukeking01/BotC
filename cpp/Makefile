# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Iinclude \
           $(shell pkg-config --cflags Qt6Widgets) -j4
LDFLAGS = $(shell pkg-config --libs Qt6Widgets)

# Source files
SRC = main.cpp storyteller.cpp
OBJ = $(SRC:.cpp=.o)

# Executable
TARGET = storyteller

# Default target
all: $(TARGET)

# Link object files into executable
$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

# Compile source files
%.o: %.cpp storyteller.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJ) $(TARGET)
