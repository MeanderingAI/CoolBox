#include "../../../../_libraries/packages/IO/servlets/headers/http1_servlet.h"
#include <memory>

std::shared_ptr<networking::servlets::HttpServletBase> make_default_servlet(std::shared_ptr<networking::servlets::RequestHandler> handler) {
    return std::make_shared<networking::servlets::Http1Servlet>(handler);
}
