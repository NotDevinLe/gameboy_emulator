CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude

# SDL2 include paths (for macOS Homebrew)
UNAME_S := $(shell uname -s 2>/dev/null || echo "Unknown")
ifeq ($(UNAME_S),Darwin)
    # Try Homebrew paths first, then fallback to /usr/local
    ifeq ($(shell test -d /opt/homebrew/include/SDL2 && echo "yes"),yes)
        CXXFLAGS += -I/opt/homebrew/include
        LDFLAGS := -L/opt/homebrew/lib
    else ifeq ($(shell test -d /usr/local/include/SDL2 && echo "yes"),yes)
        CXXFLAGS += -I/usr/local/include
        LDFLAGS := -L/usr/local/lib
    endif
endif

# SDL2 libraries
LDFLAGS += -lSDL2 -lSDL2_ttf

SRC_DIR  := lib
EMU_DIR  := emulator

SRCS := $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(EMU_DIR)/*.cpp)
OBJS := $(SRCS:.cpp=.o)

BIN  := gbemu
TEST_BIN := test_runner

.PHONY: all clean test

all: $(BIN)

test: $(TEST_BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

$(TEST_BIN): test_runner.cpp $(filter-out emulator/main.o,$(OBJS))
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ test_runner.cpp $(filter-out emulator/main.o,$(OBJS))

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(BIN) $(TEST_BIN)

