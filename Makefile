CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude
LDFLAGS  :=

SRC_DIR  := lib
EMU_DIR  := emulator

SRCS := $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(EMU_DIR)/*.cpp)
OBJS := $(SRCS:.cpp=.o)

BIN  := gbemu

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(BIN)

