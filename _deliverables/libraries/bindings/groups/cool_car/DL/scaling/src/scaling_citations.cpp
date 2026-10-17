#include "scaling_citations.h"

#include <sstream>

namespace ml {
namespace deep_learning {
namespace scaling {

const std::vector<PaperCitation>& scaling_citations() {
    static const std::vector<PaperCitation> registry = {
        {
            "rajbhandari_zero_2020",
            "ZeRO: Memory Optimizations Toward Training Trillion Parameter Models",
            "Rajbhandari, Samyam and Rasley, Jeff and Ruwase, Olatunji and He, Yuxiong",
            "SC '20: International Conference for High Performance Computing, Networking, Storage and Analysis",
            "1910.02054",
            "https://arxiv.org/abs/1910.02054",
            "zero.h"
        },
        {
            "tang_one_bit_lamb_2021",
            "1-bit LAMB: Communication Efficient Large-Scale Large-Batch Training with LAMB's Convergence Speed",
            "Tang, Hanlin and Gan, Shaoduo and Awan, Ammar Ahmad and Rajbhandari, Samyam and Li, Conglong and Lian, Xiangru and Liu, Ji and Zhang, Ce and He, Yuxiong",
            "IEEE International Conference on High Performance Computing, Data, and Analytics (HiPC)",
            "2104.06069",
            "https://arxiv.org/abs/2104.06069",
            "one_bit_lamb.h"
        },
        {
            "rajbhandari_zero_infinity_2021",
            "ZeRO-Infinity: Breaking the GPU Memory Wall for Extreme Scale Deep Learning",
            "Rajbhandari, Samyam and Ruwase, Olatunji and Rasley, Jeff and Smith, Shaden and He, Yuxiong",
            "SC '21: International Conference for High Performance Computing, Networking, Storage and Analysis",
            "2104.07857",
            "https://arxiv.org/abs/2104.07857",
            "zero_infinity.h"
        },
        {
            "rajbhandari_deepspeed_moe_2022",
            "DeepSpeed-MoE: Advancing Mixture-of-Experts Inference and Training to Power Next-Generation AI Scale",
            "Rajbhandari, Samyam and Li, Conglong and Yao, Zhewei and Zhang, Minjia and Aminabadi, Reza Yazdani and Awan, Ammar Ahmad and Rasley, Jeff and He, Yuxiong",
            "Proceedings of the 39th International Conference on Machine Learning (ICML)",
            "2201.05596",
            "https://arxiv.org/abs/2201.05596",
            "moe.h"
        },
        {
            "yao_zeroquant_2022",
            "ZeroQuant: Efficient and Affordable Post-Training Quantization for Large-Scale Transformers",
            "Yao, Zhewei and Aminabadi, Reza Yazdani and Zhang, Minjia and Wu, Xiaoxia and Li, Conglong and He, Yuxiong",
            "Advances in Neural Information Processing Systems 35 (NeurIPS)",
            "2206.01861",
            "https://arxiv.org/abs/2206.01861",
            "zero_quant.h"
        },
        {
            "wang_fastpersist_2024",
            "FastPersist: Accelerating Model Checkpointing in Deep Learning",
            "Wang, Guanhua and Ruwase, Olatunji and Xie, Bing and He, Yuxiong",
            "arXiv preprint",
            "2406.13768",
            "https://arxiv.org/abs/2406.13768",
            "fast_persist.h"
        },
        {
            "lian_universal_checkpointing_2024",
            "Universal Checkpointing: A Flexible and Efficient Distributed Checkpointing System for Large-Scale DNN Training with Reconfigurable Parallelism",
            "Lian, Xinyu and Jacobs, Sam Ade and Kurilenko, Lev and Tanaka, Masahiro and Bekman, Stas and Ruwase, Olatunji and Zhang, Minjia",
            "arXiv preprint",
            "2406.18820",
            "https://arxiv.org/abs/2406.18820",
            "universal_checkpoint.h"
        },
        {
            "wang_domino_2024",
            "Domino: Eliminating Communication in LLM Training via Generic Tensor Slicing and Overlapping",
            "Wang, Guanhua and Zhang, Chengming and Shen, Zheyu and Li, Ang and Ruwase, Olatunji",
            "arXiv preprint",
            "2409.15241",
            "https://arxiv.org/abs/2409.15241",
            "domino.h"
        }
    };
    return registry;
}

const PaperCitation* find_citation(const std::string& key) {
    for (const auto& citation : scaling_citations()) {
        if (citation.key == key) {
            return &citation;
        }
    }
    return nullptr;
}

std::vector<PaperCitation> citations_for_component(const std::string& component) {
    std::vector<PaperCitation> matches;
    for (const auto& citation : scaling_citations()) {
        if (citation.component == component) {
            matches.push_back(citation);
        }
    }
    return matches;
}

std::string to_bibtex(const std::vector<PaperCitation>& citations) {
    std::ostringstream out;
    for (const auto& citation : citations) {
        out << "@article{" << citation.key << ",\n"
            << "  title = {" << citation.title << "},\n"
            << "  author = {" << citation.authors << "},\n"
            << "  journal = {" << citation.venue << "},\n"
            << "  eprint = {" << citation.arxiv_id << "},\n"
            << "  archivePrefix = {arXiv},\n"
            << "  url = {" << citation.url << "}\n"
            << "}\n\n";
    }
    return out.str();
}

} // namespace scaling
} // namespace deep_learning
} // namespace ml
