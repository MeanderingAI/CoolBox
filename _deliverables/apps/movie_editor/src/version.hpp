#ifndef COOLBOX_APPS_MOVIE_EDITOR_VERSION_HPP
#define COOLBOX_APPS_MOVIE_EDITOR_VERSION_HPP

#include <string>

#ifndef MOVIE_EDITOR_VERSION
#define MOVIE_EDITOR_VERSION "0.0.0-dev"
#endif
#ifndef MOVIE_EDITOR_GIT_HASH
#define MOVIE_EDITOR_GIT_HASH "unknown"
#endif

namespace movie_editor_app {

// "<version> (<short git hash>)", e.g. "0.1.0 (a1b2c3d)". Both halves are
// baked in at build time by CMakeLists.txt (git rev-parse --short HEAD).
inline std::string version_string() {
    return std::string(MOVIE_EDITOR_VERSION) + " (" + MOVIE_EDITOR_GIT_HASH + ")";
}

} // namespace movie_editor_app

#endif // COOLBOX_APPS_MOVIE_EDITOR_VERSION_HPP
