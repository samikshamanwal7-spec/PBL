CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude
SRC := src/main.cpp src/OrderQueue.cpp
TARGET := stock_sim

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
	rm -f data/users.db
