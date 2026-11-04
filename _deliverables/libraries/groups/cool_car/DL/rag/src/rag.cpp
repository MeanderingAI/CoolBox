#include "rag.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>

namespace ml {
namespace deep_learning {
namespace rag {
namespace {

uint64_t fnv1a(const std::string& value) {
    uint64_t hash = 1469598103934665603ULL;
    for (unsigned char ch : value) {
        hash ^= ch;
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::vector<std::string> tokens(const std::string& text) {
    std::vector<std::string> result;
    std::string token;
    for (unsigned char ch : text) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
            token.push_back(static_cast<char>(ch));
        } else if (ch >= 'A' && ch <= 'Z') {
            token.push_back(static_cast<char>(ch - 'A' + 'a'));
        } else if (!token.empty()) {
            result.push_back(token);
            token.clear();
        }
    }
    if (!token.empty()) {
        result.push_back(token);
    }
    return result;
}

void normalize(std::vector<double>& values) {
    double squared_norm = 0.0;
    for (double value : values) {
        squared_norm += value * value;
    }
    if (squared_norm == 0.0) {
        return;
    }
    const double scale = 1.0 / std::sqrt(squared_norm);
    for (double& value : values) {
        value *= scale;
    }
}

double cosine_similarity(const std::vector<double>& left,
                         const std::vector<double>& right) {
    double dot = 0.0;
    double left_norm = 0.0;
    double right_norm = 0.0;
    for (size_t i = 0; i < left.size(); ++i) {
        dot += left[i] * right[i];
        left_norm += left[i] * left[i];
        right_norm += right[i] * right[i];
    }
    if (left_norm == 0.0 || right_norm == 0.0) {
        return 0.0;
    }
    return dot / std::sqrt(left_norm * right_norm);
}

void validate_score(double value, const char* name) {
    if (!std::isfinite(value) || value < -1.0 || value > 1.0) {
        throw std::invalid_argument(std::string(name) +
                                    " must be finite and between -1 and 1");
    }
}

void validate_nonempty_query(const std::string& query) {
    if (query.empty()) {
        throw std::invalid_argument("Query must not be empty");
    }
}

std::string build_context_prompt(const std::string& system_prompt,
                                 const std::string& query,
                                 const std::vector<SearchResult>& sources) {
    std::ostringstream prompt;
    prompt << system_prompt << "\n\nContext:\n";
    if (sources.empty()) {
        prompt << "(no relevant context)\n";
    } else {
        for (size_t i = 0; i < sources.size(); ++i) {
            prompt << '[' << (i + 1) << "] " << sources[i].document.text << '\n';
        }
    }
    prompt << "\nQuestion: " << query << "\nAnswer:";
    return prompt.str();
}

void merge_results(std::vector<SearchResult>& destination,
                   const std::vector<SearchResult>& incoming,
                   size_t maximum_sources) {
    for (const SearchResult& result : incoming) {
        auto existing = std::find_if(
            destination.begin(), destination.end(),
            [&](const SearchResult& value) {
                return value.document.id == result.document.id;
            });
        if (existing == destination.end()) {
            destination.push_back(result);
        } else if (result.score > existing->score) {
            *existing = result;
        }
    }
    std::stable_sort(destination.begin(), destination.end(),
                     [](const SearchResult& left, const SearchResult& right) {
                         return left.score > right.score;
                     });
    if (destination.size() > maximum_sources) {
        destination.resize(maximum_sources);
    }
}

} // namespace

HashingEmbedder::HashingEmbedder(size_t dimensions) : dimensions_(dimensions) {
    if (dimensions_ == 0) {
        throw std::invalid_argument("Embedding dimensions must be greater than zero");
    }
}

std::vector<double> HashingEmbedder::embed(const std::string& text) const {
    std::vector<double> embedding(dimensions_, 0.0);
    for (const std::string& token : tokens(text)) {
        const uint64_t hash = fnv1a(token);
        const size_t index = static_cast<size_t>(hash % dimensions_);
        const double sign = ((hash >> 63U) == 0U) ? 1.0 : -1.0;
        embedding[index] += sign;
    }
    normalize(embedding);
    return embedding;
}

VectorStore::VectorStore(size_t dimensions) : dimensions_(dimensions) {
    if (dimensions_ == 0) {
        throw std::invalid_argument("Embedding dimensions must be greater than zero");
    }
}

void VectorStore::validate_embedding(const std::vector<double>& embedding) const {
    if (embedding.size() != dimensions_) {
        throw std::invalid_argument("Embedding size does not match vector store dimensions");
    }
    for (double value : embedding) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("Embedding values must be finite");
        }
    }
}

void VectorStore::add(Document document, std::vector<double> embedding) {
    if (document.id.empty()) {
        throw std::invalid_argument("Document id must not be empty");
    }
    if (document.text.empty()) {
        throw std::invalid_argument("Document text must not be empty");
    }
    validate_embedding(embedding);
    auto existing = std::find_if(entries_.begin(), entries_.end(),
                                 [&](const Entry& entry) {
                                     return entry.document.id == document.id;
                                 });
    if (existing != entries_.end()) {
        existing->document = std::move(document);
        existing->embedding = std::move(embedding);
        return;
    }
    entries_.push_back({std::move(document), std::move(embedding)});
}

std::vector<SearchResult> VectorStore::search(
    const std::vector<double>& query_embedding,
    size_t top_k,
    double minimum_score) const {
    validate_embedding(query_embedding);
    validate_score(minimum_score, "Minimum score");
    std::vector<SearchResult> results;
    results.reserve(entries_.size());
    for (const Entry& entry : entries_) {
        const double score = cosine_similarity(query_embedding, entry.embedding);
        if (score >= minimum_score) {
            results.push_back({entry.document, score});
        }
    }
    std::stable_sort(results.begin(), results.end(),
                     [](const SearchResult& left, const SearchResult& right) {
                         return left.score > right.score;
                     });
    if (results.size() > top_k) {
        results.resize(top_k);
    }
    return results;
}

RagPipeline::RagPipeline(EmbeddingFunction embedder,
                         GeneratorFunction generator,
                         size_t embedding_dimensions,
                         RagConfig config)
    : embedder_(std::move(embedder)),
      generator_(std::move(generator)),
      store_(embedding_dimensions),
      config_(std::move(config)) {
    if (!embedder_) {
        throw std::invalid_argument("Embedder callback must be provided");
    }
    if (!generator_) {
        throw std::invalid_argument("Generator callback must be provided");
    }
    if (config_.top_k == 0) {
        throw std::invalid_argument("top_k must be greater than zero");
    }
    validate_score(config_.minimum_score, "Minimum score");
}

void RagPipeline::add_document(Document document) {
    store_.add(document, embedder_(document.text));
}

void RagPipeline::add_documents(const std::vector<Document>& documents) {
    for (const Document& document : documents) {
        add_document(document);
    }
}

std::vector<SearchResult> RagPipeline::retrieve(const std::string& query) const {
    validate_nonempty_query(query);
    return store_.search(embedder_(query), config_.top_k, config_.minimum_score);
}

std::string RagPipeline::build_prompt(
    const std::string& query,
    const std::vector<SearchResult>& sources) const {
    validate_nonempty_query(query);
    return build_context_prompt(config_.system_prompt, query, sources);
}

RagResponse RagPipeline::answer(const std::string& query) const {
    RagResponse response;
    response.sources = retrieve(query);
    response.prompt = build_prompt(query, response.sources);
    response.answer = generator_(response.prompt);
    return response;
}

KnowledgeGraph::KnowledgeGraph(size_t embedding_dimensions)
    : dimensions_(embedding_dimensions) {
    if (dimensions_ == 0) {
        throw std::invalid_argument("Embedding dimensions must be greater than zero");
    }
}

void KnowledgeGraph::validate_embedding(
    const std::vector<double>& embedding) const {
    if (embedding.size() != dimensions_) {
        throw std::invalid_argument("Embedding size does not match graph dimensions");
    }
    for (double value : embedding) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("Embedding values must be finite");
        }
    }
}

void KnowledgeGraph::add_node(Document document, std::vector<double> embedding) {
    if (document.id.empty() || document.text.empty()) {
        throw std::invalid_argument("Graph documents require non-empty id and text");
    }
    validate_embedding(embedding);
    const std::string id = document.id;
    nodes_[id] = {std::move(document), std::move(embedding)};
}

void KnowledgeGraph::add_edge(GraphEdge edge, bool bidirectional) {
    if (!contains(edge.source_id) || !contains(edge.target_id)) {
        throw std::invalid_argument("Graph edge endpoints must exist");
    }
    if (edge.relation.empty()) {
        throw std::invalid_argument("Graph edge relation must not be empty");
    }
    if (!std::isfinite(edge.weight) || edge.weight <= 0.0) {
        throw std::invalid_argument("Graph edge weight must be finite and positive");
    }
    adjacency_[edge.source_id].push_back(edge);
    ++edge_count_;
    if (bidirectional && edge.source_id != edge.target_id) {
        GraphEdge reverse{edge.target_id, edge.source_id, edge.relation, edge.weight};
        adjacency_[reverse.source_id].push_back(std::move(reverse));
        ++edge_count_;
    }
}

bool KnowledgeGraph::contains(const std::string& document_id) const {
    return nodes_.find(document_id) != nodes_.end();
}

std::vector<GraphEdge> KnowledgeGraph::edges_from(
    const std::string& document_id) const {
    const auto found = adjacency_.find(document_id);
    return found == adjacency_.end() ? std::vector<GraphEdge>() : found->second;
}

std::vector<GraphSearchResult> KnowledgeGraph::search(
    const std::vector<double>& query_embedding,
    size_t top_k,
    size_t max_hops,
    double semantic_weight,
    double graph_weight) const {
    validate_embedding(query_embedding);
    if (top_k == 0) {
        return {};
    }
    if (!std::isfinite(semantic_weight) || !std::isfinite(graph_weight) ||
        semantic_weight < 0.0 || graph_weight < 0.0 ||
        semantic_weight + graph_weight <= 0.0) {
        throw std::invalid_argument("GraphRAG weights must be finite and non-negative");
    }
    const double total_weight = semantic_weight + graph_weight;
    semantic_weight /= total_weight;
    graph_weight /= total_weight;

    struct State {
        std::string id;
        double propagated_score;
        size_t hops;
        std::vector<std::string> path;
    };

    std::vector<std::pair<std::string, double>> semantic_scores;
    semantic_scores.reserve(nodes_.size());
    for (const auto& pair : nodes_) {
        semantic_scores.push_back(
            {pair.first, cosine_similarity(query_embedding, pair.second.embedding)});
    }
    std::stable_sort(semantic_scores.begin(), semantic_scores.end(),
                     [](const auto& left, const auto& right) {
                         return left.second > right.second;
                     });

    std::unordered_map<std::string, double> best_graph_score;
    std::unordered_map<std::string, std::vector<std::string>> best_path;
    std::queue<State> frontier;
    const size_t seed_count = std::min(top_k, semantic_scores.size());
    for (size_t i = 0; i < seed_count; ++i) {
        frontier.push({semantic_scores[i].first, semantic_scores[i].second, 0,
                       {semantic_scores[i].first}});
        best_graph_score[semantic_scores[i].first] = semantic_scores[i].second;
        best_path[semantic_scores[i].first] = {semantic_scores[i].first};
    }

    while (!frontier.empty()) {
        State state = std::move(frontier.front());
        frontier.pop();
        if (state.hops >= max_hops) {
            continue;
        }
        for (const GraphEdge& edge : edges_from(state.id)) {
            const double propagated =
                state.propagated_score * edge.weight /
                static_cast<double>(state.hops + 2);
            const auto prior = best_graph_score.find(edge.target_id);
            if (prior != best_graph_score.end() && prior->second >= propagated) {
                continue;
            }
            std::vector<std::string> path = state.path;
            path.push_back(edge.target_id);
            best_graph_score[edge.target_id] = propagated;
            best_path[edge.target_id] = path;
            frontier.push(
                {edge.target_id, propagated, state.hops + 1, std::move(path)});
        }
    }

    std::unordered_map<std::string, double> semantic_by_id;
    for (const auto& score : semantic_scores) {
        semantic_by_id[score.first] = score.second;
    }
    std::vector<GraphSearchResult> results;
    results.reserve(nodes_.size());
    for (const auto& pair : nodes_) {
        const double graph_score =
            best_graph_score.find(pair.first) == best_graph_score.end()
                ? 0.0
                : best_graph_score[pair.first];
        const double combined =
            semantic_weight * semantic_by_id[pair.first] + graph_weight * graph_score;
        const auto path = best_path.find(pair.first);
        results.push_back({pair.second.document, combined,
                           path == best_path.end()
                               ? std::vector<std::string>{pair.first}
                               : path->second});
    }
    std::stable_sort(results.begin(), results.end(),
                     [](const GraphSearchResult& left,
                        const GraphSearchResult& right) {
                         return left.score > right.score;
                     });
    if (results.size() > top_k) {
        results.resize(top_k);
    }
    return results;
}

GraphRagPipeline::GraphRagPipeline(EmbeddingFunction embedder,
                                   GeneratorFunction generator,
                                   size_t embedding_dimensions,
                                   GraphRagConfig config)
    : embedder_(std::move(embedder)),
      generator_(std::move(generator)),
      graph_(embedding_dimensions),
      config_(std::move(config)) {
    if (!embedder_ || !generator_) {
        throw std::invalid_argument("GraphRAG requires embedder and generator callbacks");
    }
    if (config_.top_k == 0) {
        throw std::invalid_argument("GraphRAG top_k must be greater than zero");
    }
}

void GraphRagPipeline::add_document(Document document) {
    graph_.add_node(document, embedder_(document.text));
}

void GraphRagPipeline::add_documents(const std::vector<Document>& documents) {
    for (const Document& document : documents) {
        add_document(document);
    }
}

void GraphRagPipeline::add_relationship(GraphEdge edge, bool bidirectional) {
    graph_.add_edge(std::move(edge), bidirectional);
}

std::vector<GraphSearchResult> GraphRagPipeline::retrieve(
    const std::string& query) const {
    validate_nonempty_query(query);
    return graph_.search(embedder_(query), config_.top_k, config_.max_hops,
                         config_.semantic_weight, config_.graph_weight);
}

GraphRagResponse GraphRagPipeline::answer(const std::string& query) const {
    GraphRagResponse response;
    response.sources = retrieve(query);
    std::ostringstream prompt;
    prompt << config_.system_prompt << "\n\nGraph context:\n";
    for (size_t i = 0; i < response.sources.size(); ++i) {
        prompt << '[' << (i + 1) << "] " << response.sources[i].document.text
               << "\n    path: ";
        for (size_t j = 0; j < response.sources[i].path.size(); ++j) {
            if (j != 0) {
                prompt << " -> ";
            }
            prompt << response.sources[i].path[j];
        }
        prompt << '\n';
    }
    prompt << "\nQuestion: " << query << "\nAnswer:";
    response.prompt = prompt.str();
    response.answer = generator_(response.prompt);
    return response;
}

CorrectiveRagPipeline::CorrectiveRagPipeline(
    RetrieverFunction retriever,
    RelevanceEvaluator evaluator,
    GeneratorFunction generator,
    CorrectiveRagConfig config,
    RetrieverFunction fallback_retriever)
    : retriever_(std::move(retriever)),
      evaluator_(std::move(evaluator)),
      generator_(std::move(generator)),
      config_(std::move(config)),
      fallback_retriever_(std::move(fallback_retriever)) {
    if (!retriever_ || !evaluator_ || !generator_) {
        throw std::invalid_argument(
            "Corrective RAG requires retriever, evaluator, and generator callbacks");
    }
    validate_score(config_.relevance_threshold, "Relevance threshold");
    if (config_.minimum_relevant_sources == 0 || config_.maximum_sources == 0) {
        throw std::invalid_argument("Corrective RAG source limits must be positive");
    }
}

CorrectiveRagResponse CorrectiveRagPipeline::answer(
    const std::string& query) const {
    validate_nonempty_query(query);
    CorrectiveRagResponse response;
    const std::vector<SearchResult> initial = retriever_(query);
    for (SearchResult result : initial) {
        const double relevance = evaluator_(query, result);
        validate_score(relevance, "Evaluator relevance");
        result.score = relevance;
        if (relevance >= config_.relevance_threshold) {
            response.sources.push_back(std::move(result));
        } else {
            response.rejected_sources.push_back(std::move(result));
        }
    }
    if (response.sources.size() < config_.minimum_relevant_sources &&
        fallback_retriever_) {
        response.used_fallback = true;
        std::vector<SearchResult> verified_fallback;
        for (SearchResult result : fallback_retriever_(query)) {
            const double relevance = evaluator_(query, result);
            validate_score(relevance, "Evaluator relevance");
            result.score = relevance;
            if (relevance >= config_.relevance_threshold) {
                verified_fallback.push_back(std::move(result));
            } else {
                response.rejected_sources.push_back(std::move(result));
            }
        }
        merge_results(response.sources, verified_fallback, config_.maximum_sources);
    }
    merge_results(response.sources, {}, config_.maximum_sources);
    response.prompt =
        build_context_prompt(config_.system_prompt, query, response.sources);
    response.answer = generator_(response.prompt);
    return response;
}

HydePipeline::HydePipeline(EmbeddingFunction embedder,
                           GeneratorFunction hypothesis_generator,
                           GeneratorFunction answer_generator,
                           size_t embedding_dimensions,
                           HydeConfig config)
    : embedder_(std::move(embedder)),
      hypothesis_generator_(std::move(hypothesis_generator)),
      answer_generator_(std::move(answer_generator)),
      store_(embedding_dimensions),
      config_(std::move(config)) {
    if (!embedder_ || !hypothesis_generator_ || !answer_generator_) {
        throw std::invalid_argument(
            "HyDE requires embedder, hypothesis, and answer callbacks");
    }
    if (config_.top_k == 0) {
        throw std::invalid_argument("HyDE top_k must be greater than zero");
    }
    validate_score(config_.minimum_score, "Minimum score");
}

void HydePipeline::add_document(Document document) {
    store_.add(document, embedder_(document.text));
}

void HydePipeline::add_documents(const std::vector<Document>& documents) {
    for (const Document& document : documents) {
        add_document(document);
    }
}

std::vector<SearchResult> HydePipeline::retrieve(
    const std::string& query,
    std::string* hypothetical_document) const {
    validate_nonempty_query(query);
    const std::string hypothesis =
        hypothesis_generator_(config_.hypothesis_prompt + query);
    if (hypothesis.empty()) {
        throw std::runtime_error("HyDE hypothesis generator returned empty text");
    }
    if (hypothetical_document) {
        *hypothetical_document = hypothesis;
    }
    return store_.search(embedder_(hypothesis), config_.top_k,
                         config_.minimum_score);
}

HydeResponse HydePipeline::answer(const std::string& query) const {
    HydeResponse result;
    result.response.sources = retrieve(query, &result.hypothetical_document);
    result.response.prompt =
        build_context_prompt(config_.system_prompt, query, result.response.sources);
    result.response.answer = answer_generator_(result.response.prompt);
    return result;
}

AgenticRagPipeline::AgenticRagPipeline(AgentPlanner planner,
                                       GeneratorFunction generator,
                                       AgentCritic critic,
                                       AgenticRagConfig config)
    : planner_(std::move(planner)),
      generator_(std::move(generator)),
      critic_(std::move(critic)),
      config_(std::move(config)) {
    if (!planner_ || !generator_) {
        throw std::invalid_argument("Agentic RAG requires planner and generator callbacks");
    }
    if (config_.max_steps == 0 || config_.maximum_sources == 0) {
        throw std::invalid_argument("Agentic RAG limits must be positive");
    }
}

void AgenticRagPipeline::add_tool(std::string name, RetrieverFunction tool) {
    if (name.empty() || !tool) {
        throw std::invalid_argument("Agentic RAG tools require name and callback");
    }
    tools_[std::move(name)] = std::move(tool);
}

bool AgenticRagPipeline::has_tool(const std::string& name) const {
    return tools_.find(name) != tools_.end();
}

AgenticRagResponse AgenticRagPipeline::answer(const std::string& query) const {
    validate_nonempty_query(query);
    AgenticRagResponse response;
    bool requested_answer = false;
    for (size_t iteration = 0; iteration < config_.max_steps; ++iteration) {
        const AgentDecision decision = planner_(query, response.trace);
        AgentStep step;
        step.iteration = iteration;
        step.action = decision.action;
        step.tool = decision.tool;
        step.input = decision.input.empty() ? query : decision.input;

        if (decision.action == AgentAction::kSearch) {
            const auto tool = tools_.find(decision.tool);
            if (tool == tools_.end()) {
                throw std::invalid_argument("Agent requested unknown tool: " +
                                            decision.tool);
            }
            const std::vector<SearchResult> found = tool->second(step.input);
            merge_results(response.sources, found, config_.maximum_sources);
            step.observation =
                "Retrieved " + std::to_string(found.size()) + " source(s)";
            response.trace.push_back(std::move(step));
            continue;
        }
        if (decision.action == AgentAction::kAnswer) {
            step.observation = "Planner requested synthesis";
            response.trace.push_back(std::move(step));
            requested_answer = true;
        } else {
            step.observation = "Planner stopped";
            response.trace.push_back(std::move(step));
        }
        break;
    }

    if (!requested_answer && response.sources.empty()) {
        throw std::runtime_error(
            "Agent stopped without requesting an answer or collecting evidence");
    }
    const std::string prompt =
        build_context_prompt(config_.system_prompt, query, response.sources);
    response.answer = generator_(prompt);
    if (critic_) {
        response.final_critique = critic_(query, response.answer, response.sources);
        if (!response.final_critique.empty()) {
            response.answer = generator_(
                prompt + "\n\nCritique of the draft:\n" +
                response.final_critique + "\nRevised answer:");
        }
    }
    return response;
}

ModularRagPipeline::ModularRagPipeline(RagRouter router)
    : router_(std::move(router)) {
    if (!router_) {
        throw std::invalid_argument("Modular RAG requires a router callback");
    }
}

void ModularRagPipeline::add_module(std::string name, RagModule module) {
    if (name.empty() || !module) {
        throw std::invalid_argument("RAG modules require name and callback");
    }
    modules_[std::move(name)] = std::move(module);
}

bool ModularRagPipeline::has_module(const std::string& name) const {
    return modules_.find(name) != modules_.end();
}

std::vector<std::string> ModularRagPipeline::modules() const {
    std::vector<std::string> names;
    names.reserve(modules_.size());
    for (const auto& module : modules_) {
        names.push_back(module.first);
    }
    return names;
}

RoutedRagResponse ModularRagPipeline::answer(const std::string& query) const {
    validate_nonempty_query(query);
    RoutedRagResponse result;
    result.route = router_(query);
    const auto module = modules_.find(result.route);
    if (module == modules_.end()) {
        throw std::invalid_argument("Router selected unknown RAG module: " +
                                    result.route);
    }
    result.response = module->second(query);
    return result;
}

QueryComplexityRouter::QueryComplexityRouter(size_t graph_token_threshold,
                                             size_t agentic_token_threshold)
    : graph_token_threshold_(graph_token_threshold),
      agentic_token_threshold_(agentic_token_threshold) {
    if (graph_token_threshold_ == 0 ||
        agentic_token_threshold_ <= graph_token_threshold_) {
        throw std::invalid_argument(
            "Agentic threshold must be greater than a positive graph threshold");
    }
}

std::string QueryComplexityRouter::route(const std::string& query) const {
    validate_nonempty_query(query);
    const size_t token_count = tokens(query).size();
    if (token_count >= agentic_token_threshold_ ||
        query.find("compare") != std::string::npos ||
        query.find("investigate") != std::string::npos) {
        return "agentic";
    }
    if (token_count >= graph_token_threshold_ ||
        query.find("relationship") != std::string::npos ||
        query.find("connected") != std::string::npos) {
        return "graph";
    }
    return "vector";
}

} // namespace rag
} // namespace deep_learning
} // namespace ml
