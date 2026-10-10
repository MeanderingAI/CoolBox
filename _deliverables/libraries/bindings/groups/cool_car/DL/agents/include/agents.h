#ifndef ML_DEEP_LEARNING_AGENTS_H
#define ML_DEEP_LEARNING_AGENTS_H

/// @file agents.h
/// @brief Umbrella header for the LLM-agent and interactive-learning papers.
///
/// Each component cites the paper it implements; the machine-readable registry
/// lives in agent_citations.h and the BibTeX entries in
/// _internal_documents/engineering_documention/bib/agent_papers.bib.
///
/// - trace_opto.h   Cheng et al., Trace / OPTO (arXiv:2406.16218)
/// - reflexion.h    Shinn et al., Reflexion (arXiv:2303.11366)
/// - bapo.h         Schnabel et al., Lost in Transmission (arXiv:2505.08140)
/// - steerability.h Chang et al., Steerability Evaluation (arXiv:2505.23816)
/// - llf_helix.h    Xu et al., Learning from Language Feedback (arXiv:2506.10341)

#include "agent_citations.h"
#include "bapo.h"
#include "llf_helix.h"
#include "reflexion.h"
#include "steerability.h"
#include "trace_opto.h"

#endif // ML_DEEP_LEARNING_AGENTS_H
