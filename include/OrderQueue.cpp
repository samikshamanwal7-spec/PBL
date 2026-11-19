#include "OrderQueue.h"
#include "Portfolio.h"

bool BuyOrder::execute(Portfolio& portfolio, double marketPrice, std::string& message) {
    return portfolio.buy(symbol, quantity, marketPrice, message);
}

bool SellOrder::execute(Portfolio& portfolio, double marketPrice, std::string& message) {
    return portfolio.sell(symbol, quantity, marketPrice, message);
}
