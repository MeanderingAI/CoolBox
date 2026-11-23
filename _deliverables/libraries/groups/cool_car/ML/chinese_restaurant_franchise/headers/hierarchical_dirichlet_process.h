#ifndef HIERARCHICAL_DIRICHLET_PROCESS_H
#define HIERARCHICAL_DIRICHLET_PROCESS_H

#include "chinese_restaurant_franchise.h"

namespace ml {

/// The collapsed Chinese Restaurant Franchise sampler for HDP allocations.
///
/// The franchise samples restaurant/table/dish assignments, which are the
/// latent cluster allocations of a hierarchical Dirichlet process. It does not
/// sample dish parameters from a base measure or model observation likelihoods.
using HierarchicalDirichletProcess = ChineseRestaurantFranchise;

} // namespace ml

#endif // HIERARCHICAL_DIRICHLET_PROCESS_H
