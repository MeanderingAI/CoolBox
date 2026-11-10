#ifndef ML_DEEP_LEARNING_RAG_H
#define ML_DEEP_LEARNING_RAG_H

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace ml {
namespace deep_learning {
namespace rag {

struct Document {
    std::string id;
    std::string text;
    std::map<std::string, std::string> metadata;
};

struct SearchResult {
    Document document;
    double score = 0.0;
};

using EmbeddingFunction = std::function<std::vector<double>(const std::string&)>;
using GeneratorFunction = std::function<std::string(const std::string&)>;

class HashingEmbedder {
public:
    explicit HashingEmbedder(size_t dimensions = 384);

    size_t dimensions() const { return dimensions_; }
    std::vector<double> embed(const std::string& text) const;

private:
    size_t dimensions_;
};

class VectorStore {
public:
    explicit VectorStore(size_t dimensions);

    size_t dimensions() const { return dimensions_; }
    size_t size() const { return entries_.size(); }
    void clear() { entries_.clear(); }

    void add(Document document, std::vector<double> embedding);
    std::vector<SearchResult> search(const std::vector<double>& query_embedding,
                                     size_t top_k = 4,
                                     double minimum_score = -1.0) const;

private:
    struct Entry {
        Document document;
        std::vector<double> embedding;
    };

    void validate_embedding(const std::vector<double>& embedding) const;

    size_t dimensions_;
    std::vector<Entry> entries_;
};

struct RagConfig {
    size_t top_k = 4;
    double minimum_score = -1.0;
    std::string system_prompt =
        "Answer the question using only the supplied context. "
        "If the context is insufficient, say so.";
};

struct RagResponse {
    std::string answer;
    std::string prompt;
    std::vector<SearchResult> sources;
};

class RagPipeline {
public:
    RagPipeline(EmbeddingFunction embedder,
                GeneratorFunction generator,
                size_t embedding_dimensions,
                RagConfig config = RagConfig());

    void add_document(Document document);
    void add_documents(const std::vector<Document>& documents);
    std::vector<SearchResult> retrieve(const std::string& query) const;
    std::string build_prompt(const std::string& query,
                             const std::vector<SearchResult>& sources) const;
    RagResponse answer(const std::string& query) const;

    VectorStore& store() { return store_; }
    const VectorStore& store() const { return store_; }
    const RagConfig& config() const { return config_; }

private:
    EmbeddingFunction embedder_;
    GeneratorFunction generator_;
    VectorStore store_;
    RagConfig config_;
};

struct GraphEdge {
    std::string source_id;
    std::string target_id;
    std::string relation;
    double weight = 1.0;
};

struct GraphSearchResult {
    Document document;
    double score = 0.0;
    std::vector<std::string> path;
};

class KnowledgeGraph {
public:
    explicit KnowledgeGraph(size_t embedding_dimensions);

    size_t dimensions() const { return dimensions_; }
    size_t node_count() const { return nodes_.size(); }
    size_t edge_count() const { return edge_count_; }

    void add_node(Document document, std::vector<double> embedding);
    void add_edge(GraphEdge edge, bool bidirectional = true);
    bool contains(const std::string& document_id) const;
    std::vector<GraphEdge> edges_from(const std::string& document_id) const;
    std::vector<GraphSearchResult> search(
        const std::vector<double>& query_embedding,
        size_t top_k = 4,
        size_t max_hops = 2,
        double semantic_weight = 0.7,
        double graph_weight = 0.3) const;

private:
    struct Node {
        Document document;
        std::vector<double> embedding;
    };

    void validate_embedding(const std::vector<double>& embedding) const;

    size_t dimensions_;
    size_t edge_count_ = 0;
    std::unordered_map<std::string, Node> nodes_;
    std::unordered_map<std::string, std::vector<GraphEdge>> adjacency_;
};

struct GraphRagConfig {
    size_t top_k = 4;
    size_t max_hops = 2;
    double semantic_weight = 0.7;
    double graph_weight = 0.3;
    std::string system_prompt =
        "Answer using the supplied graph context and cite relevant source numbers.";
};

struct GraphRagResponse {
    std::string answer;
    std::string prompt;
    std::vector<GraphSearchResult> sources;
};

class GraphRagPipeline {
public:
    GraphRagPipeline(EmbeddingFunction embedder,
                     GeneratorFunction generator,
                     size_t embedding_dimensions,
                     GraphRagConfig config = GraphRagConfig());

    void add_document(Document document);
    void add_documents(const std::vector<Document>& documents);
    void add_relationship(GraphEdge edge, bool bidirectional = true);
    std::vector<GraphSearchResult> retrieve(const std::string& query) const;
    GraphRagResponse answer(const std::string& query) const;

    KnowledgeGraph& graph() { return graph_; }
    const KnowledgeGraph& graph() const { return graph_; }

private:
    EmbeddingFunction embedder_;
    GeneratorFunction generator_;
    KnowledgeGraph graph_;
    GraphRagConfig config_;
};

using RetrieverFunction =
    std::function<std::vector<SearchResult>(const std::string&)>;
using RelevanceEvaluator =
    std::function<double(const std::string&, const SearchResult&)>;

struct CorrectiveRagConfig {
    double relevance_threshold = 0.5;
    size_t minimum_relevant_sources = 1;
    size_t maximum_sources = 4;
    std::string system_prompt =
        "Answer only from verified context. State when evidence is insufficient.";
};

struct CorrectiveRagResponse {
    std::string answer;
    std::string prompt;
    std::vector<SearchResult> sources;
    std::vector<SearchResult> rejected_sources;
    bool used_fallback = false;
};

class CorrectiveRagPipeline {
public:
    CorrectiveRagPipeline(RetrieverFunction retriever,
                          RelevanceEvaluator evaluator,
                          GeneratorFunction generator,
                          CorrectiveRagConfig config = CorrectiveRagConfig(),
                          RetrieverFunction fallback_retriever = RetrieverFunction());

    CorrectiveRagResponse answer(const std::string& query) const;

private:
    RetrieverFunction retriever_;
    RelevanceEvaluator evaluator_;
    GeneratorFunction generator_;
    CorrectiveRagConfig config_;
    RetrieverFunction fallback_retriever_;
};

struct HydeConfig {
    size_t top_k = 4;
    double minimum_score = -1.0;
    std::string hypothesis_prompt =
        "Write a concise hypothetical document that would answer this question:\n";
    std::string system_prompt =
        "Answer using the real retrieved sources, not the hypothetical document.";
};

struct HydeResponse {
    std::string hypothetical_document;
    RagResponse response;
};

class HydePipeline {
public:
    HydePipeline(EmbeddingFunction embedder,
                 GeneratorFunction hypothesis_generator,
                 GeneratorFunction answer_generator,
                 size_t embedding_dimensions,
                 HydeConfig config = HydeConfig());

    void add_document(Document document);
    void add_documents(const std::vector<Document>& documents);
    std::vector<SearchResult> retrieve(const std::string& query,
                                       std::string* hypothetical_document = nullptr) const;
    HydeResponse answer(const std::string& query) const;

private:
    EmbeddingFunction embedder_;
    GeneratorFunction hypothesis_generator_;
    GeneratorFunction answer_generator_;
    VectorStore store_;
    HydeConfig config_;
};

enum class AgentAction {
    kSearch,
    kAnswer,
    kStop
};

struct AgentStep {
    size_t iteration = 0;
    AgentAction action = AgentAction::kSearch;
    std::string tool;
    std::string input;
    std::string observation;
};

struct AgentDecision {
    AgentAction action = AgentAction::kSearch;
    std::string tool;
    std::string input;
};

using AgentPlanner =
    std::function<AgentDecision(const std::string&, const std::vector<AgentStep>&)>;
using AgentCritic = std::function<std::string(
    const std::string&, const std::string&, const std::vector<SearchResult>&)>;

struct AgenticRagConfig {
    size_t max_steps = 6;
    size_t maximum_sources = 8;
    std::string system_prompt =
        "Synthesize an answer from the evidence collected by the retrieval agent.";
};

struct AgenticRagResponse {
    std::string answer;
    std::vector<SearchResult> sources;
    std::vector<AgentStep> trace;
    std::string final_critique;
};

class AgenticRagPipeline {
public:
    AgenticRagPipeline(AgentPlanner planner,
                       GeneratorFunction generator,
                       AgentCritic critic = AgentCritic(),
                       AgenticRagConfig config = AgenticRagConfig());

    void add_tool(std::string name, RetrieverFunction tool);
    bool has_tool(const std::string& name) const;
    AgenticRagResponse answer(const std::string& query) const;

private:
    AgentPlanner planner_;
    GeneratorFunction generator_;
    AgentCritic critic_;
    AgenticRagConfig config_;
    std::map<std::string, RetrieverFunction> tools_;
};

using RagModule = std::function<RagResponse(const std::string&)>;
using RagRouter = std::function<std::string(const std::string&)>;

struct RoutedRagResponse {
    std::string route;
    RagResponse response;
};

class ModularRagPipeline {
public:
    explicit ModularRagPipeline(RagRouter router);

    void add_module(std::string name, RagModule module);
    bool has_module(const std::string& name) const;
    std::vector<std::string> modules() const;
    RoutedRagResponse answer(const std::string& query) const;

private:
    RagRouter router_;
    std::map<std::string, RagModule> modules_;
};

class QueryComplexityRouter {
public:
    QueryComplexityRouter(size_t graph_token_threshold = 12,
                          size_t agentic_token_threshold = 24);

    std::string route(const std::string& query) const;

private:
    size_t graph_token_threshold_;
    size_t agentic_token_threshold_;
};

} // namespace rag
} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_RAG_H
