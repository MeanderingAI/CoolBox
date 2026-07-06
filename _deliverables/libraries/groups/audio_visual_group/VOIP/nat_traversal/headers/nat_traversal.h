#pragma once

#include <string>

namespace trekker {
namespace voip {

struct NatCandidate {
    std::string address;
    unsigned short port;
    std::string protocol;
};

class NatTraversal {
public:
    NatCandidate best_candidate(const NatCandidate& local, const NatCandidate& reflexive) const;
};

} // namespace voip
} // namespace trekker
