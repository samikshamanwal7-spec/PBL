#include <iostream>
#include <string>
#include <cstdlib>
#include <filesystem>

#include "web/WebApi.h"

// ============================================================
// web_main.cpp -- entry point of the WEB front end.
//
// Starts a local web server on 127.0.0.1 (this computer only; no
// internet needed) and serves web/index.html. The console version
// (src/main.cpp) is unchanged and still works as a fallback.
// ============================================================
int main() {
    std::filesystem::create_directories("data"); // no-op if it already exists

    WebApi api;
    HttpServer server([&api](const HttpRequest& r) { return api.handle(r); });

    int port = server.listenOn(8080);
    if (port < 0) {
        std::cerr << "Could not open a local port (8080-8089 are all busy).\n";
        return 1;
    }

    std::string url = "http://127.0.0.1:" + std::to_string(port);
    std::cout << "Virtual Stock Market Learning Platform (web)\n"
              << "Running offline at " << url << "\n"
              << "Open that address in your browser. Press Ctrl+C here to stop.\n";
#ifdef _WIN32
    std::system(("start \"\" " + url).c_str()); // open the default browser
#endif
    server.run();
    return 0;
}
