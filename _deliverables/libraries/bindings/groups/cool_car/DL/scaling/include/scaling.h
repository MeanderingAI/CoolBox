#ifndef ML_DEEP_LEARNING_SCALING_H
#define ML_DEEP_LEARNING_SCALING_H

/// @file scaling.h
/// @brief Umbrella header for the DeepSpeed-family scaling techniques.
///
/// Each component cites the paper it implements; the machine-readable registry
/// lives in scaling_citations.h and the BibTeX entries in
/// _internal_documents/engineering_documention/bib/scaling_papers.bib.
///
/// - zero.h                  Rajbhandari et al., ZeRO (arXiv:1910.02054)
/// - one_bit_lamb.h          Tang et al., 1-bit LAMB (arXiv:2104.06069)
/// - zero_infinity.h         Rajbhandari et al., ZeRO-Infinity (arXiv:2104.07857)
/// - moe.h                   Rajbhandari et al., DeepSpeed-MoE (arXiv:2201.05596)
/// - zero_quant.h            Yao et al., ZeroQuant (arXiv:2206.01861)
/// - fast_persist.h          Wang et al., FastPersist (arXiv:2406.13768)
/// - universal_checkpoint.h  Lian et al., Universal Checkpointing (arXiv:2406.18820)
/// - domino.h                Wang et al., Domino (arXiv:2409.15241)

#include "collective.h"
#include "domino.h"
#include "fast_persist.h"
#include "moe.h"
#include "one_bit_lamb.h"
#include "scaling_citations.h"
#include "universal_checkpoint.h"
#include "zero.h"
#include "zero_infinity.h"
#include "zero_quant.h"

#endif // ML_DEEP_LEARNING_SCALING_H
