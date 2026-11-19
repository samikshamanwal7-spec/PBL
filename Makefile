CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -I.
SRC := src/main.cpp modules/transactions_orders/OrderQueue.cpp
TARGET := stock_sim

WEB_TARGET := stock_sim_web
WEB_SRC := src/web_main.cpp modules/transactions_orders/OrderQueue.cpp

.PHONY: all run web run-web clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

web: $(WEB_SRC)
	$(CXX) $(CXXFLAGS) -pthread $(WEB_SRC) -o $(WEB_TARGET)

run-web: web
	./$(WEB_TARGET)

clean:
	rm -f $(WEB_TARGET)
	rm -f $(TARGET)
	rm -f data/users.db
