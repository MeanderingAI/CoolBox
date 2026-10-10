#ifndef ML_DEEP_LEARNING_AGENTS_CITATIONS_H
#define ML_DEEP_LEARNING_AGENTS_CITATIONS_H

#include <string>
#include <vector>

namespace ml {
namespace deep_learning {
namespace agents {

/// Bibliographic record for a paper implemented by this module.
struct PaperCitation {
    std::string key;      ///< BibTeX key in _internal_documents/engineering_documention/bib/agent_papers.bib
    std::string title;
    std::string authors;
    std::string venue;
    std::string arxiv_id;
    std::string url;
    std::string component; ///< Header in this module that implements the paper.
};

/// Every paper implemented by DL/agents, in publication order.
const std::vector<PaperCitation>& agent_citations();

/// Returns nullptr when no citation matches @p key.
const PaperCitation* find_citation(const std::string& key);

/// Citations for the paper(s) implemented by a given header, e.g. "bapo.h".
std::vector<PaperCitation> citations_for_component(const std::string& component);

/// Renders the registry as a BibTeX document.
std::string to_bibtex(const std::vector<PaperCitation>& citations);

} // namespace agents
} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_AGENTS_CITATIONS_H
