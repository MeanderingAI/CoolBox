import pytest

from ml_toolbox import ml_core


dl = ml_core.deep_learning


def test_rag_pipeline_retrieves_context_and_calls_generator():
    embedder = dl.rag.HashingEmbedder(128)
    prompts = []

    def generate(prompt):
        prompts.append(prompt)
        return "CoolBox uses native C++ components."

    pipeline = dl.rag.RagPipeline(
        embedder.embed,
        generate,
        embedder.dimensions,
    )
    pipeline.add_document(
        dl.rag.Document(
            "coolbox",
            "CoolBox includes native C++ machine-learning components.",
            {"source": "README"},
        )
    )

    response = pipeline.answer("What does CoolBox include?")

    assert response.answer == "CoolBox uses native C++ components."
    assert response.sources[0].document.id == "coolbox"
    assert "[1] CoolBox includes native C++" in prompts[0]


def test_graphrag_traverses_relationships():
    rag = dl.rag
    embedder = rag.HashingEmbedder(128)
    pipeline = rag.GraphRagPipeline(
        embedder.embed,
        lambda prompt: "graph answer",
        embedder.dimensions,
    )
    pipeline.add_documents(
        [
            rag.Document("alice", "Alice founded Acme robotics"),
            rag.Document("acme", "Acme builds warehouse robots"),
        ]
    )
    pipeline.add_relationship(
        rag.GraphEdge("alice", "acme", "founded")
    )

    response = pipeline.answer("How are Alice and Acme connected?")

    assert response.answer == "graph answer"
    assert response.sources
    assert pipeline.graph.edge_count == 2
    assert "path:" in response.prompt


def test_corrective_rag_filters_and_uses_fallback():
    rag = dl.rag
    config = rag.CorrectiveRagConfig()
    config.relevance_threshold = 0.6

    pipeline = rag.CorrectiveRagPipeline(
        lambda query: [
            rag.SearchResult(rag.Document("bad", "irrelevant"), 0.2)
        ],
        lambda query, result: 0.9 if result.document.id == "good" else 0.1,
        lambda prompt: "corrected",
        config,
        lambda query: [
            rag.SearchResult(rag.Document("good", "verified evidence"), 0.8)
        ],
    )

    response = pipeline.answer("question")

    assert response.answer == "corrected"
    assert response.used_fallback
    assert [item.document.id for item in response.sources] == ["good"]
    assert [item.document.id for item in response.rejected_sources] == ["bad"]


def test_hyde_uses_hypothesis_for_retrieval():
    rag = dl.rag
    embedder = rag.HashingEmbedder(128)
    pipeline = rag.HydePipeline(
        embedder.embed,
        lambda prompt: "photosynthesis converts sunlight into energy",
        lambda prompt: "grounded answer",
        embedder.dimensions,
    )
    pipeline.add_document(
        rag.Document("plants", "Plants use photosynthesis to convert sunlight")
    )

    response = pipeline.answer("How do plants use sunlight?")

    assert "photosynthesis" in response.hypothetical_document
    assert response.response.answer == "grounded answer"
    assert response.response.sources[0].document.id == "plants"


def test_agentic_rag_plans_search_and_revision():
    rag = dl.rag
    generations = []

    def planner(query, trace):
        if not trace:
            return rag.AgentDecision(rag.AgentAction.SEARCH, "docs", query)
        return rag.AgentDecision(rag.AgentAction.ANSWER)

    def generate(prompt):
        generations.append(prompt)
        return "draft" if len(generations) == 1 else "revised"

    pipeline = rag.AgenticRagPipeline(
        planner,
        generate,
        lambda query, answer, sources: "include evidence",
    )
    pipeline.add_tool(
        "docs",
        lambda query: [
            rag.SearchResult(rag.Document("doc", "supporting evidence"), 0.9)
        ],
    )

    response = pipeline.answer("Investigate this claim")

    assert response.answer == "revised"
    assert len(response.trace) == 2
    assert response.sources[0].document.id == "doc"
    assert len(generations) == 2


def test_modular_rag_routes_between_pipelines():
    rag = dl.rag
    embedder = rag.HashingEmbedder(64)
    vector = rag.RagPipeline(
        embedder.embed,
        lambda prompt: "vector",
        embedder.dimensions,
    )
    vector.add_document(rag.Document("doc", "simple context"))
    router = rag.QueryComplexityRouter(4, 8)
    pipeline = rag.AdaptiveRagPipeline(router.route)
    pipeline.add_module("vector", vector.answer)
    pipeline.add_module("graph", vector.answer)
    pipeline.add_module("agentic", vector.answer)

    assert pipeline.answer("simple question").route == "vector"
    assert (
        pipeline.answer("explain the relationship between connected items").route
        == "graph"
    )
    assert (
        pipeline.answer(
            "compare many sources and investigate every conflicting claim"
        ).route
        == "agentic"
    )


def test_scaling_memory_estimate_is_available():
    estimate = dl.scaling.estimate_zero_memory(
        1_000,
        dl.scaling.ZeroStage.PARAMETERS,
        4,
    )

    assert estimate.per_device_bytes == pytest.approx(4_000.0)
    assert estimate.communication_volume_factor == pytest.approx(1.5)


def test_agent_metrics_are_available():
    sample = dl.agents.SteeringSample()
    sample.source = [0.0, 0.0]
    sample.goal = [1.0, 0.0]
    sample.output = [0.5, 0.0]

    result = dl.agents.decompose_steering(sample)

    assert result.miscalibration == pytest.approx(-0.5)
    assert result.side_effect_magnitude == pytest.approx(0.0)


def test_explainability_request_factories_are_available():
    request = dl.explainability.ExplainRequest.for_integrated_gradients(
        target_class=2,
        steps=16,
    )

    assert request.method == dl.explainability.ExplanationMethod.INTEGRATED_GRADIENTS
    assert request.target_class == 2
    assert request.ig_steps == 16
