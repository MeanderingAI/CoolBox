#include "agent_citations.h"

#include <sstream>

namespace ml {
namespace deep_learning {
namespace agents {

const std::vector<PaperCitation>& agent_citations() {
    static const std::vector<PaperCitation> registry = {
        {
            "cheng_trace_2024",
            "Trace is the Next AutoDiff: Generative Optimization with Rich Feedback, Execution Traces, and LLMs",
            "Cheng, Ching-An and Nie, Allen and Swaminathan, Adith",
            "Advances in Neural Information Processing Systems 37 (NeurIPS)",
            "2406.16218",
            "https://arxiv.org/abs/2406.16218",
            "trace_opto.h"
        },
        {
            "schnabel_lost_in_transmission_2025",
            "Lost in Transmission: When and Why LLMs Fail to Reason Globally",
            "Schnabel, Tobias and Tomlinson, Kiran and Swaminathan, Adith and Neville, Jennifer",
            "Advances in Neural Information Processing Systems 38 (NeurIPS), Spotlight",
            "2505.08140",
            "https://arxiv.org/abs/2505.08140",
            "bapo.h"
        },
        {
            "chang_steerability_2025",
            "A Course Correction in Steerability Evaluation: Revealing Miscalibration and Side Effects in LLMs",
            "Chang, Trenton and Schnabel, Tobias and Swaminathan, Adith and Wiens, Jenna",
            "Proceedings of the AAAI Conference on Artificial Intelligence (AAAI)",
            "2505.23816",
            "https://arxiv.org/abs/2505.23816",
            "steerability.h"
        },
        {
            "xu_llf_helix_2025",
            "Formalizing Learning from Language Feedback with Provable Guarantees",
            "Xu, Wanqiao and Nie, Allen and Zheng, Ruijie and Modi, Aditya and Swaminathan, Adith and Cheng, Ching-An",
            "Proceedings of the International Conference on Machine Learning (ICML)",
            "2506.10341",
            "https://arxiv.org/abs/2506.10341",
            "llf_helix.h"
        }
    };
    return registry;
}

const PaperCitation* find_citation(const std::string& key) {
    for (const auto& citation : agent_citations()) {
        if (citation.key == key) {
            return &citation;
        }
    }
    return nullptr;
}

std::vector<PaperCitation> citations_for_component(const std::string& component) {
    std::vector<PaperCitation> matches;
    for (const auto& citation : agent_citations()) {
        if (citation.component == component) {
            matches.push_back(citation);
        }
    }
    return matches;
}

std::string to_bibtex(const std::vector<PaperCitation>& citations) {
    std::ostringstream out;
    for (const auto& citation : citations) {
        out << "@inproceedings{" << citation.key << ",\n"
            << "  title = {" << citation.title << "},\n"
            << "  author = {" << citation.authors << "},\n"
            << "  booktitle = {" << citation.venue << "},\n"
            << "  eprint = {" << citation.arxiv_id << "},\n"
            << "  archivePrefix = {arXiv},\n"
            << "  url = {" << citation.url << "}\n"
            << "}\n\n";
    }
    return out.str();
}

} // namespace agents
} // namespace deep_learning
} // namespace ml
