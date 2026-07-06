#include "nat_traversal.h"

namespace trekker {
namespace voip {

NatCandidate NatTraversal::best_candidate(const NatCandidate& local, const NatCandidate& reflexive) const {
    // Prefer reflexive/public candidate when available for better reachability.
    if (!reflexive.address.empty() && reflexive.port != 0) {
        return reflexive;
    }
    return local;
}

} // namespace voip
} // namespace trekker
