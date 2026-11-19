#ifndef WEB_API_H
#define WEB_API_H

#include <string>
#include <map>
#include <memory>
#include <random>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cctype>
#include <algorithm>

#include "web/HttpServer.h"

// The existing backend modules -- used exactly as the console version uses them.
#include "modules/user_management/User.h"
#include "modules/stock_market/Market.h"
#include "modules/trading_portfolio/Portfolio.h"
#include "modules/transactions_orders/OrderQueue.h"
#include "modules/transactions_orders/TransactionStorage.h"

// ============================================================
// WebApi.h -- Web front end (interface layer)
//
// Plays the same role as the four *Interface.h files, but instead
// of printing to a console it answers browser requests with JSON.
// It only CALLS the backend (AuthManager, Market, Portfolio,
// TransactionStorage); none of the backend logic is changed.
//
//   POST /api/signup   /api/signin   /api/signout
//   GET  /api/market   /api/portfolio   /api/history
//   POST /api/buy      /api/sell        /api/undo   /api/tick
//   GET  /api/related  /api/trend
// ============================================================
class WebApi {
public:
    WebApi() : auth("data/users.db"), rng(std::random_device{}()) {}

    HttpResponse handle(const HttpRequest& req) {
        if (req.method == "GET" && (req.path == "/" || req.path == "/index.html"))
            return servePage();
        if (req.path.rfind("/api/", 0) == 0) return route(req);
        return text(404, "Not found");
    }

private:
    struct Session {
        User user;
        Portfolio portfolio;
        std::string portfolioFile;
        explicit Session(const User& u)
            : user(u), portfolio(u.getCashBalance()),
              portfolioFile(TransactionStorage::pathFor(u.getUsername())) {}
    };

    AuthManager auth;
    Market market;
    std::map<std::string, std::unique_ptr<Session>> sessions;
    std::mt19937_64 rng;

    // ---------- small helpers ----------
    static std::string esc(const std::string& s) {
        std::string o;
        for (unsigned char c : s) {
            switch (c) {
                case '"':  o += "\\\""; break;
                case '\\': o += "\\\\"; break;
                case '\n': o += "\\n";  break;
                case '\r': o += "\\r";  break;
                case '\t': o += "\\t";  break;
                default:
                    if (c < 0x20) { char b[8]; std::snprintf(b, sizeof b, "\\u%04x", c); o += b; }
                    else o += static_cast<char>(c);
            }
        }
        return o;
    }
    static std::string q(const std::string& s) { return "\"" + esc(s) + "\""; }
    static std::string num(double v) {
        char b[64]; std::snprintf(b, sizeof b, "%.4f", v); return b;
    }
    static HttpResponse json(const std::string& body, int status = 200) {
        HttpResponse r; r.status = status; r.body = body; return r;
    }
    static HttpResponse text(int status, const std::string& msg) {
        HttpResponse r; r.status = status;
        r.contentType = "text/plain; charset=utf-8"; r.body = msg; return r;
    }
    static HttpResponse fail(const std::string& msg, int status = 200) {
        return json("{\"ok\":false,\"error\":" + q(msg) + "}", status);
    }
    static std::string param(const HttpRequest& r, const std::string& k) {
        auto it = r.params.find(k); return it == r.params.end() ? "" : it->second;
    }
    static std::string upper(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
        return s;
    }

    HttpResponse servePage() {
        for (const char* p : {"web/index.html", "../web/index.html"}) {
            std::ifstream in(p, std::ios::binary);
            if (in) {
                std::stringstream ss; ss << in.rdbuf();
                HttpResponse r; r.contentType = "text/html; charset=utf-8"; r.body = ss.str();
                return r;
            }
        }
        return text(500, "web/index.html not found. Run the program from the stock_sim folder.");
    }

    std::string newToken() {
        char b[40]; std::snprintf(b, sizeof b, "%016llx%016llx",
            (unsigned long long)rng(), (unsigned long long)rng());
        return b;
    }

    Session* sessionOf(const HttpRequest& r) {
        auto it = sessions.find(param(r, "token"));
        return it == sessions.end() ? nullptr : it->second.get();
    }

    // Saves cash (users.db) and holdings/history (portfolio file), like Save & Exit.
    void save(Session& s) {
        s.user.setCashBalance(s.portfolio.getCash());
        auth.saveBalance(s.user);
        TransactionStorage::save(s.portfolio, s.portfolioFile);
    }

    std::string summaryJson(Session& s) {
        double holdingsValue = 0, unrealized = 0;
        for (auto& [sym, h] : s.portfolio.allHoldings()) {
            if (h.quantity <= 0) continue;
            Stock* st = market.find(sym);
            double price = st ? st->getPrice() : h.avgBuyPrice;
            holdingsValue += price * h.quantity;
            unrealized += s.portfolio.unrealizedPnL(sym, price);
        }
        std::string o = "{";
        o += "\"cash\":" + num(s.portfolio.getCash());
        o += ",\"holdingsValue\":" + num(holdingsValue);
        o += ",\"netWorth\":" + num(s.portfolio.getCash() + holdingsValue);
        o += ",\"realized\":" + num(s.portfolio.getRealizedPnL());
        o += ",\"unrealized\":" + num(unrealized) + "}";
        return o;
    }

    // ---------- routing ----------
    HttpResponse route(const HttpRequest& r) {
        const std::string& p = r.path;

        if (p == "/api/signup" || p == "/api/signin") {
            std::string name = param(r, "username"), pass = param(r, "password");
            User u;
            bool ok = (p == "/api/signup") ? auth.signUp(name, pass, u) : auth.signIn(name, pass, u);
            if (!ok) return fail(p == "/api/signup" ? "Username already exists or fields were empty."
                                                    : "Invalid username or password.");
            for (auto it = sessions.begin(); it != sessions.end();)   // one live session per user
                it = (it->second->user.getUsername() == u.getUsername()) ? sessions.erase(it) : std::next(it);
            auto s = std::make_unique<Session>(u);
            TransactionStorage::load(s->portfolio, s->portfolioFile);  // restore holdings, P&L, history
            std::string token = newToken();
            std::string out = "{\"ok\":true,\"token\":" + q(token) + ",\"username\":" + q(u.getUsername())
                            + ",\"summary\":" + summaryJson(*s) + "}";
            sessions[token] = std::move(s);
            return json(out);
        }

        Session* s = sessionOf(r);
        if (!s) return fail("Not signed in.", 401);

        if (p == "/api/signout") {
            save(*s);
            sessions.erase(param(r, "token"));
            return json("{\"ok\":true}");
        }

        if (p == "/api/market") {
            std::string o = "{\"ok\":true,\"summary\":" + summaryJson(*s) + ",\"stocks\":[";
            bool first = true;
            for (Stock* st : market.allSorted()) {
                if (!first) { o += ","; }
                first = false;
                o += "{\"symbol\":" + q(st->getSymbol()) + ",\"name\":" + q(st->getName())
                   + ",\"sector\":" + q(st->getSector()) + ",\"price\":" + num(st->getPrice())
                   + ",\"change\":" + num(st->getDayChangePercent())
                   + ",\"held\":" + std::to_string(s->portfolio.quantityHeld(st->getSymbol())) + "}";
            }
            return json(o + "]}");
        }

        if (p == "/api/tick") {
            market.tick();
            return json("{\"ok\":true}");
        }

        if (p == "/api/buy" || p == "/api/sell") {
            std::string sym = upper(param(r, "symbol"));
            Stock* st = market.find(sym);
            if (!st) return fail("Unknown symbol.");
            int qty = 0;
            try { qty = std::stoi(param(r, "quantity")); } catch (...) { return fail("Please enter a valid quantity."); }
            if (p == "/api/buy") market.placeOrder(std::make_shared<BuyOrder>(st->getSymbol(), qty));
            else                 market.placeOrder(std::make_shared<SellOrder>(st->getSymbol(), qty));
            auto results = market.processOrders(s->portfolio);
            std::string msg = results.empty() ? "No result." : results.front();
            bool ok = msg.find("REJECTED") == std::string::npos && msg.find("Rejected") == std::string::npos;
            size_t close = msg.find("] ");
            if (msg.rfind("[", 0) == 0 && close != std::string::npos) msg = msg.substr(close + 2);
            if (ok) save(*s); // keep files up to date after every trade
            return json(std::string("{\"ok\":") + (ok ? "true" : "false") + ",\"message\":" + q(msg)
                        + ",\"summary\":" + summaryJson(*s) + "}");
        }

        if (p == "/api/undo") {
            std::string msg;
            bool ok = s->portfolio.undoLast(msg);
            if (ok) save(*s);
            return json(std::string("{\"ok\":") + (ok ? "true" : "false") + ",\"message\":" + q(msg)
                        + ",\"summary\":" + summaryJson(*s) + "}");
        }

        if (p == "/api/portfolio") {
            std::string o = "{\"ok\":true,\"summary\":" + summaryJson(*s) + ",\"holdings\":[";
            bool first = true;
            for (auto& [sym, h] : s->portfolio.allHoldings()) {
                if (h.quantity <= 0) continue;
                Stock* st = market.find(sym);
                double price = st ? st->getPrice() : h.avgBuyPrice;
                if (!first) { o += ","; }
                first = false;
                o += "{\"symbol\":" + q(sym) + ",\"name\":" + q(st ? st->getName() : sym)
                   + ",\"quantity\":" + std::to_string(h.quantity) + ",\"avgCost\":" + num(h.avgBuyPrice)
                   + ",\"price\":" + num(price) + ",\"unrealized\":" + num(s->portfolio.unrealizedPnL(sym, price)) + "}";
            }
            return json(o + "]}");
        }

        if (p == "/api/history") {
            std::string o = "{\"ok\":true,\"transactions\":[";
            bool first = true;
            for (const Transaction& t : s->portfolio.getHistory().recentFirst()) {
                if (!first) { o += ","; }
                first = false;
                o += std::string("{\"type\":") + q(t.type == TxType::BUY ? "BUY" : "SELL")
                   + ",\"symbol\":" + q(t.symbol) + ",\"quantity\":" + std::to_string(t.quantity)
                   + ",\"price\":" + num(t.priceEach) + ",\"pnl\":" + num(t.realizedPnL)
                   + ",\"time\":" + std::to_string(static_cast<long long>(t.timestamp)) + "}";
            }
            return json(o + "]}");
        }

        if (p == "/api/related") {
            Stock* st = market.find(upper(param(r, "symbol")));
            if (!st) return fail("Unknown symbol.");
            std::string o = "{\"ok\":true,\"symbol\":" + q(st->getSymbol()) + ",\"sector\":" + q(st->getSector()) + ",\"related\":[";
            bool first = true;
            for (auto& [sym, hops] : market.getGraph().bfsWithinHops(st->getSymbol(), 2)) {
                Stock* other = market.find(sym);
                if (!other) continue;
                if (!first) { o += ","; }
                first = false;
                o += "{\"symbol\":" + q(sym) + ",\"name\":" + q(other->getName()) + ",\"hops\":" + std::to_string(hops)
                   + ",\"price\":" + num(other->getPrice()) + "}";
            }
            return json(o + "]}");
        }

        if (p == "/api/trend") {
            Stock* st = market.find(upper(param(r, "symbol")));
            if (!st) return fail("Unknown symbol.");
            std::string o = "{\"ok\":true,\"symbol\":" + q(st->getSymbol()) + ",\"prices\":[";
            bool first = true;
            for (double v : st->getHistory().lastN(30)) {
                if (!first) { o += ","; }
                first = false; o += num(v);
            }
            o += "],\"movingAverage\":" + num(st->getHistory().movingAverage(10))
               + ",\"sinceStart\":" + num(st->getHistory().percentChangeSinceStart()) + "}";
            return json(o);
        }

        return text(404, "Unknown API route");
    }
};

#endif // WEB_API_H
