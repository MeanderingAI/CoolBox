#include "tyst_framework.hpp"

#include "rag.h"

#include <string>
#include <vector>

using namespace ml::deep_learning::rag;

TYST_TEST(RagTest, HashingEmbedderIsDeterministicAndNormalized) {
    HashingEmbedder embedder(64);
    const auto first = embedder.embed("retrieval augmented generation");
    const auto second = embedder.embed("retrieval augmented generation");

    TYST_EXPECT_EQ(first, second);
    double squared_norm = 0.0;
    for (double value : first) {
        squared_norm += value * value;
    }
    TYST_EXPECT_NEAR(squared_norm, 1.0, 1e-12);
}

TYST_TEST(RagTest, VectorStoreRanksRelevantDocuments) {
    HashingEmbedder embedder(128);
    VectorStore store(embedder.dimensions());
    store.add({"rag", "RAG retrieves documents before generation", {}},
              embedder.embed("RAG retrieves documents before generation"));
    store.add({"vision", "Convolutional networks process images", {}},
              embedder.embed("Convolutional networks process images"));

    const auto results = store.search(embedder.embed("documents for retrieval"), 1);
    TYST_ASSERT_EQ(results.size(), 1u);
    TYST_EXPECT_EQ(results[0].document.id, std::string("rag"));
}

TYST_TEST(RagTest, PipelineBuildsGroundedPromptAndReturnsSources) {
    HashingEmbedder embedder(128);
    std::string received_prompt;
    RagPipeline pipeline(
        [&](const std::string& text) { return embedder.embed(text); },
        [&](const std::string& prompt) {
            received_prompt = prompt;
            return std::string("A grounded answer");
        },
        embedder.dimensions());

    pipeline.add_document({"doc-1", "CoolBox includes native C++ components", {}});
    const RagResponse response = pipeline.answer("What does CoolBox include?");

    TYST_EXPECT_EQ(response.answer, std::string("A grounded answer"));
    TYST_ASSERT_EQ(response.sources.size(), 1u);
    TYST_EXPECT_EQ(response.sources[0].document.id, std::string("doc-1"));
    TYST_EXPECT_NE(received_prompt.find("[1] CoolBox includes"), std::string::npos);
    TYST_EXPECT_NE(received_prompt.find("Question: What does CoolBox include?"),
                   std::string::npos);
}

TYST_TEST(RagTest, RejectsInvalidInputs) {
    TYST_EXPECT_THROW(HashingEmbedder(0), std::invalid_argument);
    VectorStore store(2);
    TYST_EXPECT_THROW(store.add({"", "text", {}}, {1.0, 0.0}), std::invalid_argument);
    TYST_EXPECT_THROW(store.add({"id", "text", {}}, {1.0}), std::invalid_argument);
}

TYST_TEST(GraphRagTest, TraversesRelationshipsFromSemanticSeeds) {
    HashingEmbedder embedder(128);
    GraphRagPipeline pipeline(
        [&](const std::string& text) { return embedder.embed(text); },
        [](const std::string&) { return std::string("graph answer"); },
        embedder.dimensions());
    pipeline.add_documents({
        {"alice", "Alice founded Acme robotics", {}},
        {"acme", "Acme builds autonomous warehouse robots", {}},
        {"unrelated", "Ocean temperatures are measured by buoys", {}}
    });
    pipeline.add_relationship(
        {"alice", "acme", "founded", 1.0});

    const auto response = pipeline.answer("Who is connected to Acme robotics?");

    TYST_EXPECT_EQ(response.answer, std::string("graph answer"));
    TYST_ASSERT_FALSE(response.sources.empty());
    TYST_EXPECT_NE(response.prompt.find("path:"), std::string::npos);
    TYST_EXPECT_GT(pipeline.graph().edge_count(), 0u);
}

TYST_TEST(CorrectiveRagTest, FiltersAndFallsBackWhenEvidenceIsWeak) {
    CorrectiveRagConfig config;
    config.relevance_threshold = 0.6;
    CorrectiveRagPipeline pipeline(
        [](const std::string&) {
            return std::vector<SearchResult>{
                {{"bad", "irrelevant result", {}}, 0.2}};
        },
        [](const std::string&, const SearchResult& result) {
            return result.document.id == "good" ? 0.9 : 0.1;
        },
        [](const std::string&) { return std::string("corrected answer"); },
        config,
        [](const std::string&) {
            return std::vector<SearchResult>{
                {{"good", "verified fallback evidence", {}}, 0.8}};
        });

    const auto response = pipeline.answer("verified question");

    TYST_EXPECT_TRUE(response.used_fallback);
    TYST_ASSERT_EQ(response.sources.size(), 1u);
    TYST_EXPECT_EQ(response.sources[0].document.id, std::string("good"));
    TYST_ASSERT_EQ(response.rejected_sources.size(), 1u);
}

TYST_TEST(HydeTest, RetrievesWithHypotheticalDocumentButAnswersFromSources) {
    HashingEmbedder embedder(128);
    HydePipeline pipeline(
        [&](const std::string& text) { return embedder.embed(text); },
        [](const std::string&) {
            return std::string("Photosynthesis converts sunlight into chemical energy");
        },
        [](const std::string& prompt) {
            return prompt.find("real retrieved sources") != std::string::npos
                ? std::string("grounded")
                : std::string("ungrounded");
        },
        embedder.dimensions());
    pipeline.add_document(
        {"photo", "Plants use photosynthesis to convert sunlight into energy", {}});

    const HydeResponse response = pipeline.answer("How do plants use sunlight?");

    TYST_EXPECT_NE(response.hypothetical_document.find("Photosynthesis"),
                   std::string::npos);
    TYST_EXPECT_EQ(response.response.answer, std::string("grounded"));
    TYST_ASSERT_EQ(response.response.sources.size(), 1u);
}

TYST_TEST(AgenticRagTest, PlansSearchThenCritiquesAndRevises) {
    size_t generations = 0;
    AgenticRagPipeline pipeline(
        [](const std::string&, const std::vector<AgentStep>& trace) {
            AgentDecision decision;
            if (trace.empty()) {
                decision.action = AgentAction::kSearch;
                decision.tool = "docs";
                decision.input = "focused query";
            } else {
                decision.action = AgentAction::kAnswer;
            }
            return decision;
        },
        [&](const std::string&) {
            ++generations;
            return generations == 1 ? std::string("draft") : std::string("revised");
        },
        [](const std::string&, const std::string&,
           const std::vector<SearchResult>&) {
            return std::string("add the supporting detail");
        });
    pipeline.add_tool("docs", [](const std::string&) {
        return std::vector<SearchResult>{
            {{"doc", "supporting detail", {}}, 0.9}};
    });

    const auto response = pipeline.answer("Investigate the detail");

    TYST_EXPECT_EQ(response.answer, std::string("revised"));
    TYST_ASSERT_EQ(response.trace.size(), 2u);
    TYST_ASSERT_EQ(response.sources.size(), 1u);
    TYST_EXPECT_EQ(generations, 2u);
}

TYST_TEST(ModularRagTest, RoutesQueriesToRegisteredModules) {
    QueryComplexityRouter router(4, 8);
    ModularRagPipeline pipeline(
        [&](const std::string& query) { return router.route(query); });
    auto module = [](const std::string& query) {
        RagResponse response;
        response.answer = query;
        return response;
    };
    pipeline.add_module("vector", module);
    pipeline.add_module("graph", module);
    pipeline.add_module("agentic", module);

    TYST_EXPECT_EQ(pipeline.answer("simple question").route,
                   std::string("vector"));
    TYST_EXPECT_EQ(
        pipeline.answer("explain the relationship between these connected items").route,
        std::string("graph"));
    TYST_EXPECT_EQ(
        pipeline.answer("compare many sources and investigate every conflicting claim").route,
        std::string("agentic"));
}
