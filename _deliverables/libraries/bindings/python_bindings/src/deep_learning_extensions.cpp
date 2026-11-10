#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "DL/layers/include/tensor.h"
#include "rag.h"
#include "scaling.h"

namespace py = pybind11;

using ml::deep_learning::Tensor;

namespace {

void bind_rag(py::module_& parent) {
    using namespace ml::deep_learning::rag;

    py::module_ module = parent.def_submodule("rag", "Retrieval-augmented generation");

    py::class_<Document>(module, "Document")
        .def(py::init<>())
        .def(py::init<std::string, std::string, std::map<std::string, std::string>>(),
             py::arg("id"), py::arg("text"),
             py::arg("metadata") = std::map<std::string, std::string>())
        .def_readwrite("id", &Document::id)
        .def_readwrite("text", &Document::text)
        .def_readwrite("metadata", &Document::metadata);

    py::class_<SearchResult>(module, "SearchResult")
        .def(py::init<>())
        .def(py::init<Document, double>(), py::arg("document"), py::arg("score"))
        .def_readonly("document", &SearchResult::document)
        .def_readonly("score", &SearchResult::score);

    py::class_<HashingEmbedder>(module, "HashingEmbedder")
        .def(py::init<size_t>(), py::arg("dimensions") = 384)
        .def_property_readonly("dimensions", &HashingEmbedder::dimensions)
        .def("embed", &HashingEmbedder::embed, py::arg("text"));

    py::class_<VectorStore>(module, "VectorStore")
        .def(py::init<size_t>(), py::arg("dimensions"))
        .def_property_readonly("dimensions", &VectorStore::dimensions)
        .def("__len__", &VectorStore::size)
        .def("clear", &VectorStore::clear)
        .def("add", &VectorStore::add, py::arg("document"), py::arg("embedding"))
        .def("search", &VectorStore::search,
             py::arg("query_embedding"), py::arg("top_k") = 4,
             py::arg("minimum_score") = -1.0);

    py::class_<RagConfig>(module, "RagConfig")
        .def(py::init<>())
        .def_readwrite("top_k", &RagConfig::top_k)
        .def_readwrite("minimum_score", &RagConfig::minimum_score)
        .def_readwrite("system_prompt", &RagConfig::system_prompt);

    py::class_<RagResponse>(module, "RagResponse")
        .def(py::init<>())
        .def_readonly("answer", &RagResponse::answer)
        .def_readonly("prompt", &RagResponse::prompt)
        .def_readonly("sources", &RagResponse::sources);

    py::class_<RagPipeline>(module, "RagPipeline")
        .def(py::init<EmbeddingFunction, GeneratorFunction, size_t, RagConfig>(),
             py::arg("embedder"), py::arg("generator"),
             py::arg("embedding_dimensions"), py::arg("config") = RagConfig())
        .def("add_document", &RagPipeline::add_document, py::arg("document"))
        .def("add_documents", &RagPipeline::add_documents, py::arg("documents"))
        .def("retrieve", &RagPipeline::retrieve, py::arg("query"))
        .def("build_prompt", &RagPipeline::build_prompt,
             py::arg("query"), py::arg("sources"))
        .def("answer", &RagPipeline::answer, py::arg("query"))
        .def_property_readonly(
            "store",
            py::overload_cast<>(&RagPipeline::store),
            py::return_value_policy::reference_internal)
        .def_property_readonly(
            "config", &RagPipeline::config,
            py::return_value_policy::reference_internal);

    py::class_<GraphEdge>(module, "GraphEdge")
        .def(py::init<>())
        .def(py::init<std::string, std::string, std::string, double>(),
             py::arg("source_id"), py::arg("target_id"), py::arg("relation"),
             py::arg("weight") = 1.0)
        .def_readwrite("source_id", &GraphEdge::source_id)
        .def_readwrite("target_id", &GraphEdge::target_id)
        .def_readwrite("relation", &GraphEdge::relation)
        .def_readwrite("weight", &GraphEdge::weight);
    py::class_<GraphSearchResult>(module, "GraphSearchResult")
        .def_readonly("document", &GraphSearchResult::document)
        .def_readonly("score", &GraphSearchResult::score)
        .def_readonly("path", &GraphSearchResult::path);
    py::class_<KnowledgeGraph>(module, "KnowledgeGraph")
        .def(py::init<size_t>(), py::arg("embedding_dimensions"))
        .def_property_readonly("dimensions", &KnowledgeGraph::dimensions)
        .def_property_readonly("node_count", &KnowledgeGraph::node_count)
        .def_property_readonly("edge_count", &KnowledgeGraph::edge_count)
        .def("add_node", &KnowledgeGraph::add_node)
        .def("add_edge", &KnowledgeGraph::add_edge,
             py::arg("edge"), py::arg("bidirectional") = true)
        .def("contains", &KnowledgeGraph::contains)
        .def("edges_from", &KnowledgeGraph::edges_from)
        .def("search", &KnowledgeGraph::search,
             py::arg("query_embedding"), py::arg("top_k") = 4,
             py::arg("max_hops") = 2, py::arg("semantic_weight") = 0.7,
             py::arg("graph_weight") = 0.3);
    py::class_<GraphRagConfig>(module, "GraphRagConfig")
        .def(py::init<>())
        .def_readwrite("top_k", &GraphRagConfig::top_k)
        .def_readwrite("max_hops", &GraphRagConfig::max_hops)
        .def_readwrite("semantic_weight", &GraphRagConfig::semantic_weight)
        .def_readwrite("graph_weight", &GraphRagConfig::graph_weight)
        .def_readwrite("system_prompt", &GraphRagConfig::system_prompt);
    py::class_<GraphRagResponse>(module, "GraphRagResponse")
        .def_readonly("answer", &GraphRagResponse::answer)
        .def_readonly("prompt", &GraphRagResponse::prompt)
        .def_readonly("sources", &GraphRagResponse::sources);
    py::class_<GraphRagPipeline>(module, "GraphRagPipeline")
        .def(py::init<EmbeddingFunction, GeneratorFunction, size_t, GraphRagConfig>(),
             py::arg("embedder"), py::arg("generator"),
             py::arg("embedding_dimensions"),
             py::arg("config") = GraphRagConfig())
        .def("add_document", &GraphRagPipeline::add_document)
        .def("add_documents", &GraphRagPipeline::add_documents)
        .def("add_relationship", &GraphRagPipeline::add_relationship,
             py::arg("edge"), py::arg("bidirectional") = true)
        .def("retrieve", &GraphRagPipeline::retrieve)
        .def("answer", &GraphRagPipeline::answer)
        .def_property_readonly(
            "graph", py::overload_cast<>(&GraphRagPipeline::graph),
            py::return_value_policy::reference_internal);

    py::class_<CorrectiveRagConfig>(module, "CorrectiveRagConfig")
        .def(py::init<>())
        .def_readwrite("relevance_threshold",
                       &CorrectiveRagConfig::relevance_threshold)
        .def_readwrite("minimum_relevant_sources",
                       &CorrectiveRagConfig::minimum_relevant_sources)
        .def_readwrite("maximum_sources",
                       &CorrectiveRagConfig::maximum_sources)
        .def_readwrite("system_prompt", &CorrectiveRagConfig::system_prompt);
    py::class_<CorrectiveRagResponse>(module, "CorrectiveRagResponse")
        .def_readonly("answer", &CorrectiveRagResponse::answer)
        .def_readonly("prompt", &CorrectiveRagResponse::prompt)
        .def_readonly("sources", &CorrectiveRagResponse::sources)
        .def_readonly("rejected_sources",
                      &CorrectiveRagResponse::rejected_sources)
        .def_readonly("used_fallback", &CorrectiveRagResponse::used_fallback);
    py::class_<CorrectiveRagPipeline>(module, "CorrectiveRagPipeline")
        .def(py::init<RetrieverFunction, RelevanceEvaluator, GeneratorFunction,
                      CorrectiveRagConfig, RetrieverFunction>(),
             py::arg("retriever"), py::arg("evaluator"), py::arg("generator"),
             py::arg("config") = CorrectiveRagConfig(),
             py::arg("fallback_retriever") = RetrieverFunction())
        .def("answer", &CorrectiveRagPipeline::answer);

    py::class_<HydeConfig>(module, "HydeConfig")
        .def(py::init<>())
        .def_readwrite("top_k", &HydeConfig::top_k)
        .def_readwrite("minimum_score", &HydeConfig::minimum_score)
        .def_readwrite("hypothesis_prompt", &HydeConfig::hypothesis_prompt)
        .def_readwrite("system_prompt", &HydeConfig::system_prompt);
    py::class_<HydeResponse>(module, "HydeResponse")
        .def_readonly("hypothetical_document",
                      &HydeResponse::hypothetical_document)
        .def_readonly("response", &HydeResponse::response);
    py::class_<HydePipeline>(module, "HydePipeline")
        .def(py::init<EmbeddingFunction, GeneratorFunction, GeneratorFunction,
                      size_t, HydeConfig>(),
             py::arg("embedder"), py::arg("hypothesis_generator"),
             py::arg("answer_generator"), py::arg("embedding_dimensions"),
             py::arg("config") = HydeConfig())
        .def("add_document", &HydePipeline::add_document)
        .def("add_documents", &HydePipeline::add_documents)
        .def("retrieve", [](const HydePipeline& pipeline,
                            const std::string& query) {
            std::string hypothesis;
            auto sources = pipeline.retrieve(query, &hypothesis);
            return py::make_tuple(hypothesis, sources);
        })
        .def("answer", &HydePipeline::answer);

    py::enum_<AgentAction>(module, "AgentAction")
        .value("SEARCH", AgentAction::kSearch)
        .value("ANSWER", AgentAction::kAnswer)
        .value("STOP", AgentAction::kStop);
    py::class_<AgentStep>(module, "AgentStep")
        .def_readonly("iteration", &AgentStep::iteration)
        .def_readonly("action", &AgentStep::action)
        .def_readonly("tool", &AgentStep::tool)
        .def_readonly("input", &AgentStep::input)
        .def_readonly("observation", &AgentStep::observation);
    py::class_<AgentDecision>(module, "AgentDecision")
        .def(py::init<>())
        .def(py::init<AgentAction, std::string, std::string>(),
             py::arg("action"), py::arg("tool") = "",
             py::arg("input") = "")
        .def_readwrite("action", &AgentDecision::action)
        .def_readwrite("tool", &AgentDecision::tool)
        .def_readwrite("input", &AgentDecision::input);
    py::class_<AgenticRagConfig>(module, "AgenticRagConfig")
        .def(py::init<>())
        .def_readwrite("max_steps", &AgenticRagConfig::max_steps)
        .def_readwrite("maximum_sources",
                       &AgenticRagConfig::maximum_sources)
        .def_readwrite("system_prompt", &AgenticRagConfig::system_prompt);
    py::class_<AgenticRagResponse>(module, "AgenticRagResponse")
        .def_readonly("answer", &AgenticRagResponse::answer)
        .def_readonly("sources", &AgenticRagResponse::sources)
        .def_readonly("trace", &AgenticRagResponse::trace)
        .def_readonly("final_critique", &AgenticRagResponse::final_critique);
    py::class_<AgenticRagPipeline>(module, "AgenticRagPipeline")
        .def(py::init<AgentPlanner, GeneratorFunction, AgentCritic,
                      AgenticRagConfig>(),
             py::arg("planner"), py::arg("generator"),
             py::arg("critic") = AgentCritic(),
             py::arg("config") = AgenticRagConfig())
        .def("add_tool", &AgenticRagPipeline::add_tool)
        .def("has_tool", &AgenticRagPipeline::has_tool)
        .def("answer", &AgenticRagPipeline::answer);

    py::class_<RoutedRagResponse>(module, "RoutedRagResponse")
        .def_readonly("route", &RoutedRagResponse::route)
        .def_readonly("response", &RoutedRagResponse::response);
    py::class_<ModularRagPipeline> modular(
        module, "ModularRagPipeline");
    modular
        .def(py::init<RagRouter>(), py::arg("router"))
        .def("add_module", &ModularRagPipeline::add_module)
        .def("has_module", &ModularRagPipeline::has_module)
        .def_property_readonly("modules", &ModularRagPipeline::modules)
        .def("answer", &ModularRagPipeline::answer);
    module.attr("AdaptiveRagPipeline") = modular;

    py::class_<QueryComplexityRouter>(module, "QueryComplexityRouter")
        .def(py::init<size_t, size_t>(),
             py::arg("graph_token_threshold") = 12,
             py::arg("agentic_token_threshold") = 24)
        .def("route", &QueryComplexityRouter::route);
}

void bind_scaling(py::module_& parent) {
    using namespace ml::deep_learning::scaling;

    py::module_ module = parent.def_submodule("scaling", "Large-scale training primitives");

    py::class_<ShardRange>(module, "ShardRange")
        .def(py::init<>())
        .def_readwrite("offset", &ShardRange::offset)
        .def_readwrite("count", &ShardRange::count)
        .def_property_readonly("end", &ShardRange::end)
        .def("contains", &ShardRange::contains);
    module.def("partition_evenly", &partition_evenly);

    py::class_<CollectiveGroup>(module, "CollectiveGroup")
        .def(py::init<size_t>())
        .def_property_readonly("world_size", &CollectiveGroup::world_size)
        .def_property_readonly("bytes_moved", &CollectiveGroup::bytes_moved)
        .def("all_reduce", &CollectiveGroup::all_reduce)
        .def("reduce_scatter", [](CollectiveGroup& group,
                                  const std::vector<std::vector<double>>& buffers) {
            std::vector<std::vector<double>> shards;
            group.reduce_scatter(buffers, shards);
            return shards;
        })
        .def("all_gather", [](CollectiveGroup& group,
                              const std::vector<std::vector<double>>& shards) {
            std::vector<std::vector<double>> gathered;
            group.all_gather(shards, gathered);
            return gathered;
        })
        .def("broadcast", [](CollectiveGroup& group,
                             const std::vector<double>& source, size_t root) {
            std::vector<std::vector<double>> buffers;
            group.broadcast(source, root, buffers);
            return buffers;
        })
        .def("reset_counters", &CollectiveGroup::reset_counters);

    py::enum_<ZeroStage>(module, "ZeroStage")
        .value("DISABLED", ZeroStage::kDisabled)
        .value("OPTIMIZER_STATES", ZeroStage::kOptimizerStates)
        .value("GRADIENTS", ZeroStage::kGradients)
        .value("PARAMETERS", ZeroStage::kParameters);
    py::class_<ZeroMemoryEstimate>(module, "ZeroMemoryEstimate")
        .def(py::init<>())
        .def_readwrite("parameter_bytes", &ZeroMemoryEstimate::parameter_bytes)
        .def_readwrite("gradient_bytes", &ZeroMemoryEstimate::gradient_bytes)
        .def_readwrite("optimizer_bytes", &ZeroMemoryEstimate::optimizer_bytes)
        .def_readwrite("per_device_bytes", &ZeroMemoryEstimate::per_device_bytes)
        .def_readwrite("communication_volume_factor",
                       &ZeroMemoryEstimate::communication_volume_factor);
    module.def("estimate_zero_memory", &estimate_zero_memory,
               py::arg("num_parameters"), py::arg("stage"), py::arg("world_size"),
               py::arg("optimizer_multiplier") = 12.0);
    module.def("max_model_size", &max_model_size,
               py::arg("device_bytes"), py::arg("stage"), py::arg("world_size"),
               py::arg("optimizer_multiplier") = 12.0);

    py::class_<ZeroDataParallelEngine::Config>(module, "ZeroConfig")
        .def(py::init<>())
        .def_readwrite("stage", &ZeroDataParallelEngine::Config::stage)
        .def_readwrite("learning_rate", &ZeroDataParallelEngine::Config::learning_rate)
        .def_readwrite("beta1", &ZeroDataParallelEngine::Config::beta1)
        .def_readwrite("beta2", &ZeroDataParallelEngine::Config::beta2)
        .def_readwrite("epsilon", &ZeroDataParallelEngine::Config::epsilon)
        .def_readwrite("average_gradients",
                       &ZeroDataParallelEngine::Config::average_gradients);
    py::class_<ZeroDataParallelEngine>(module, "ZeroDataParallelEngine")
        .def(py::init<size_t, size_t, ZeroDataParallelEngine::Config>())
        .def("shard_for", &ZeroDataParallelEngine::shard_for,
             py::return_value_policy::reference_internal)
        .def("owner_of", &ZeroDataParallelEngine::owner_of)
        .def("step", &ZeroDataParallelEngine::step)
        .def("gather_parameters_for_forward",
             &ZeroDataParallelEngine::gather_parameters_for_forward)
        .def("memory_estimate", &ZeroDataParallelEngine::memory_estimate,
             py::arg("optimizer_multiplier") = 12.0)
        .def_property_readonly("local_optimizer_state_elements",
                               &ZeroDataParallelEngine::local_optimizer_state_elements)
        .def_property_readonly("step_count", &ZeroDataParallelEngine::step_count)
        .def_property_readonly(
            "collectives",
            py::overload_cast<>(&ZeroDataParallelEngine::collectives),
            py::return_value_policy::reference_internal);

    py::enum_<MemoryTier>(module, "MemoryTier")
        .value("GPU", MemoryTier::kGpu)
        .value("CPU", MemoryTier::kCpu)
        .value("NVME", MemoryTier::kNvme);
    py::class_<DeviceTier>(module, "DeviceTier")
        .def(py::init<>())
        .def_readwrite("tier", &DeviceTier::tier)
        .def_readwrite("capacity_bytes", &DeviceTier::capacity_bytes)
        .def_readwrite("bandwidth_bytes_per_second",
                       &DeviceTier::bandwidth_bytes_per_second);
    py::class_<StatePlacement>(module, "StatePlacement")
        .def_readonly("state", &StatePlacement::state)
        .def_readonly("tier", &StatePlacement::tier)
        .def_readonly("bytes", &StatePlacement::bytes);
    py::class_<OffloadPlan>(module, "OffloadPlan")
        .def_readonly("placements", &OffloadPlan::placements)
        .def_readonly("fits", &OffloadPlan::fits)
        .def_readonly("estimated_transfer_seconds",
                      &OffloadPlan::estimated_transfer_seconds)
        .def_readonly("resident_gpu_bytes", &OffloadPlan::resident_gpu_bytes);
    py::class_<InfinityOffloadEngine>(module, "InfinityOffloadEngine")
        .def(py::init<std::vector<DeviceTier>>())
        .def_property_readonly("tiers", &InfinityOffloadEngine::tiers)
        .def("plan", &InfinityOffloadEngine::plan)
        .def("bandwidth_centric_partition",
             &InfinityOffloadEngine::bandwidth_centric_partition)
        .def("aggregate_bandwidth", &InfinityOffloadEngine::aggregate_bandwidth)
        .def_static("memory_centric_tiles",
                    &InfinityOffloadEngine::memory_centric_tiles)
        .def_static("overlapped_seconds", &InfinityOffloadEngine::overlapped_seconds)
        .def_static("efficiency", &InfinityOffloadEngine::efficiency)
        .def_static("required_bandwidth", &InfinityOffloadEngine::required_bandwidth);

    py::class_<OneBitPayload>(module, "OneBitPayload")
        .def(py::init<>())
        .def_readwrite("signs", &OneBitPayload::signs)
        .def_readwrite("scales", &OneBitPayload::scales)
        .def_readwrite("chunk_size", &OneBitPayload::chunk_size)
        .def_readwrite("element_count", &OneBitPayload::element_count);
    module.def("compress_one_bit", [](const std::vector<double>& values,
                                      std::vector<double> error,
                                      size_t chunk_size) {
        OneBitPayload payload = compress_one_bit(values, error, chunk_size);
        return py::make_tuple(payload, error);
    }, py::arg("values"), py::arg("error") = std::vector<double>(),
       py::arg("chunk_size") = 512);
    module.def("decompress_one_bit", &decompress_one_bit);
    module.def("compression_ratio", &compression_ratio);

    py::class_<OneBitLamb::Config>(module, "OneBitLambConfig")
        .def(py::init<>())
        .def_readwrite("learning_rate", &OneBitLamb::Config::learning_rate)
        .def_readwrite("beta1", &OneBitLamb::Config::beta1)
        .def_readwrite("beta2", &OneBitLamb::Config::beta2)
        .def_readwrite("epsilon", &OneBitLamb::Config::epsilon)
        .def_readwrite("weight_decay", &OneBitLamb::Config::weight_decay)
        .def_readwrite("warmup_steps", &OneBitLamb::Config::warmup_steps)
        .def_readwrite("min_coeff", &OneBitLamb::Config::min_coeff)
        .def_readwrite("max_coeff", &OneBitLamb::Config::max_coeff)
        .def_readwrite("chunk_size", &OneBitLamb::Config::chunk_size)
        .def_readwrite("coeff_refresh_interval",
                       &OneBitLamb::Config::coeff_refresh_interval);
    py::class_<OneBitLamb, ml::deep_learning::Optimizer,
               std::shared_ptr<OneBitLamb>>(module, "OneBitLamb")
        .def(py::init<>())
        .def(py::init<OneBitLamb::Config>())
        .def_property_readonly("in_compression_phase",
                               &OneBitLamb::in_compression_phase)
        .def_property_readonly("step_count", &OneBitLamb::step_count)
        .def_property_readonly("last_trust_ratio", &OneBitLamb::last_trust_ratio)
        .def_property_readonly("last_compression_ratio",
                               &OneBitLamb::last_compression_ratio)
        .def_property_readonly("error_compensation",
                               &OneBitLamb::error_compensation)
        .def_property_readonly("frozen_variance", &OneBitLamb::frozen_variance);

    py::class_<QuantizedTensor>(module, "QuantizedTensor")
        .def(py::init<>())
        .def_readwrite("values", &QuantizedTensor::values)
        .def_readwrite("scales", &QuantizedTensor::scales)
        .def_readwrite("shape", &QuantizedTensor::shape)
        .def_readwrite("group_size", &QuantizedTensor::group_size)
        .def_readwrite("bits", &QuantizedTensor::bits)
        .def_property_readonly("num_groups", &QuantizedTensor::num_groups)
        .def_property_readonly("storage_bytes", &QuantizedTensor::storage_bytes);
    module.def("group_wise_quantize", &group_wise_quantize);
    module.def("token_wise_quantize", &token_wise_quantize);
    module.def("dequantize", &dequantize);
    module.def("fake_quantize", &fake_quantize);
    module.def("quantization_error", &quantization_error);
    module.def("memory_reduction", &memory_reduction);

    py::class_<QuantizationRecipe>(module, "QuantizationRecipe")
        .def(py::init<>())
        .def_readwrite("attention_weight_bits",
                       &QuantizationRecipe::attention_weight_bits)
        .def_readwrite("mlp_weight_bits", &QuantizationRecipe::mlp_weight_bits)
        .def_readwrite("activation_bits", &QuantizationRecipe::activation_bits)
        .def_readwrite("weight_group_size", &QuantizationRecipe::weight_group_size);
    py::class_<LayerwiseKnowledgeDistiller::Config>(module, "DistillerConfig")
        .def(py::init<>())
        .def_readwrite("iterations",
                       &LayerwiseKnowledgeDistiller::Config::iterations)
        .def_readwrite("learning_rate",
                       &LayerwiseKnowledgeDistiller::Config::learning_rate)
        .def_readwrite("group_size",
                       &LayerwiseKnowledgeDistiller::Config::group_size)
        .def_readwrite("bits", &LayerwiseKnowledgeDistiller::Config::bits);
    py::class_<LayerwiseKnowledgeDistiller>(module, "LayerwiseKnowledgeDistiller")
        .def(py::init<>())
        .def(py::init<LayerwiseKnowledgeDistiller::Config>())
        .def("distill", &LayerwiseKnowledgeDistiller::distill)
        .def_property_readonly("loss_history",
                               &LayerwiseKnowledgeDistiller::loss_history);

    py::class_<MoEConfig>(module, "MoEConfig")
        .def(py::init<>())
        .def_readwrite("d_model", &MoEConfig::d_model)
        .def_readwrite("d_ff", &MoEConfig::d_ff)
        .def_readwrite("num_experts", &MoEConfig::num_experts)
        .def_readwrite("top_k", &MoEConfig::top_k)
        .def_readwrite("capacity_factor", &MoEConfig::capacity_factor)
        .def_readwrite("residual_mlp", &MoEConfig::residual_mlp)
        .def_readwrite("seed", &MoEConfig::seed);
    py::class_<RoutingSlot>(module, "RoutingSlot")
        .def_readonly("token", &RoutingSlot::token)
        .def_readonly("expert", &RoutingSlot::expert)
        .def_readonly("gate_weight", &RoutingSlot::gate_weight);
    py::class_<MoELayer, ml::deep_learning::Layer,
               std::shared_ptr<MoELayer>>(module, "MoELayer")
        .def(py::init<MoEConfig>())
        .def_property_readonly("config", &MoELayer::config)
        .def("expert_capacity", &MoELayer::expert_capacity)
        .def_property_readonly("routing", &MoELayer::routing)
        .def_property_readonly("dropped_tokens", &MoELayer::dropped_tokens)
        .def_property_readonly("load_balancing_loss",
                               &MoELayer::load_balancing_loss)
        .def_property_readonly("activation_sparsity",
                               &MoELayer::activation_sparsity)
        .def_property_readonly("total_parameters", &MoELayer::total_parameters)
        .def_property_readonly("active_parameters_per_token",
                               &MoELayer::active_parameters_per_token);
    module.def("pyramid_expert_counts", &pyramid_expert_counts);
    module.def("expert_placement", &expert_placement);
    py::class_<DistillationStage>(module, "DistillationStage")
        .def_readonly("start_step", &DistillationStage::start_step)
        .def_readonly("end_step", &DistillationStage::end_step)
        .def_readonly("kd_weight", &DistillationStage::kd_weight);
    module.def("staged_distillation_schedule", &staged_distillation_schedule);
    module.def("distillation_loss", &distillation_loss);

    py::class_<DominoConfig>(module, "DominoConfig")
        .def(py::init<>())
        .def_readwrite("row_chunks", &DominoConfig::row_chunks)
        .def_readwrite("column_chunks", &DominoConfig::column_chunks)
        .def_readwrite("overlap", &DominoConfig::overlap);
    py::class_<OverlapStep>(module, "OverlapStep")
        .def_readonly("layer", &OverlapStep::layer)
        .def_readonly("chunk", &OverlapStep::chunk)
        .def_readonly("compute_tag", &OverlapStep::compute_tag)
        .def_readonly("overlapped_comm_chunk",
                      &OverlapStep::overlapped_comm_chunk)
        .def_readonly("has_overlapped_comm", &OverlapStep::has_overlapped_comm);
    py::class_<DominoScheduler>(module, "DominoScheduler")
        .def(py::init<DominoConfig>())
        .def_property_readonly("config", &DominoScheduler::config)
        .def("build_schedule", &DominoScheduler::build_schedule)
        .def("hidden_communication_fraction",
             &DominoScheduler::hidden_communication_fraction)
        .def("estimated_iteration_seconds",
             &DominoScheduler::estimated_iteration_seconds)
        .def("speedup", &DominoScheduler::speedup)
        .def_static("split_rows", &DominoScheduler::split_rows)
        .def_static("split_columns", &DominoScheduler::split_columns)
        .def_static("concat_rows", &DominoScheduler::concat_rows)
        .def_static("concat_columns", &DominoScheduler::concat_columns)
        .def("forward_sliced", &DominoScheduler::forward_sliced);

    py::class_<PackedBinaryMatrix>(module, "PackedBinaryMatrix")
        .def(py::init<>())
        .def_readwrite("rows", &PackedBinaryMatrix::rows)
        .def_readwrite("cols", &PackedBinaryMatrix::cols)
        .def_readwrite("words", &PackedBinaryMatrix::words)
        .def_property_readonly("words_per_row", &PackedBinaryMatrix::words_per_row)
        .def_property_readonly("logical_bits", &PackedBinaryMatrix::logical_bits)
        .def_property_readonly("packed_bits", &PackedBinaryMatrix::packed_bits)
        .def("value", &PackedBinaryMatrix::value);
    module.def("pack_binary_signs", &pack_binary_signs);
    module.def("unpack_binary_signs", &unpack_binary_signs);

    py::class_<LittleBitFactors>(module, "LittleBitFactors")
        .def_readonly("u_sign", &LittleBitFactors::u_sign)
        .def_readonly("v_sign", &LittleBitFactors::v_sign)
        .def_readonly("h", &LittleBitFactors::h)
        .def_readonly("latent", &LittleBitFactors::latent)
        .def_readonly("g", &LittleBitFactors::g)
        .def_property_readonly("logical_storage_bits",
                               &LittleBitFactors::logical_storage_bits)
        .def_property_readonly("packed_storage_bits",
                               &LittleBitFactors::packed_storage_bits)
        .def_property_readonly("bits_per_weight",
                               &LittleBitFactors::bits_per_weight)
        .def("reconstruct", &LittleBitFactors::reconstruct)
        .def("forward", &LittleBitFactors::forward);
    py::class_<LittleBitConfig>(module, "LittleBitConfig")
        .def(py::init<>())
        .def_readwrite("rank", &LittleBitConfig::rank)
        .def_readwrite("residual_compensation",
                       &LittleBitConfig::residual_compensation);
    py::class_<LittleBitResult>(module, "LittleBitResult")
        .def_readonly("primary", &LittleBitResult::primary)
        .def_readonly("has_residual", &LittleBitResult::has_residual)
        .def_readonly("residual", &LittleBitResult::residual)
        .def_property_readonly("logical_storage_bits",
                               &LittleBitResult::logical_storage_bits)
        .def_property_readonly("packed_storage_bits",
                               &LittleBitResult::packed_storage_bits)
        .def_property_readonly("bits_per_weight",
                               &LittleBitResult::bits_per_weight)
        .def("reconstruct", &LittleBitResult::reconstruct)
        .def("forward", &LittleBitResult::forward);
    module.def("little_bit_quantize", &little_bit_quantize,
               py::arg("weights"), py::arg("config") = LittleBitConfig());

    py::class_<LittleBit2Config>(module, "LittleBit2Config")
        .def(py::init<>())
        .def_readwrite("rank", &LittleBit2Config::rank)
        .def_readwrite("residual_compensation",
                       &LittleBit2Config::residual_compensation)
        .def_readwrite("alignment_iterations",
                       &LittleBit2Config::alignment_iterations)
        .def_readwrite("seed", &LittleBit2Config::seed);
    module.def("little_bit2_quantize", &little_bit2_quantize,
               py::arg("weights"), py::arg("config") = LittleBit2Config());
    module.def("joint_itq_rotation", &joint_itq_rotation,
               py::arg("latent_rows"), py::arg("iterations") = 50,
               py::arg("seed") = 0);
    module.def("latent_alignment_error", &latent_alignment_error);

    py::class_<NanoQuantConfig>(module, "NanoQuantConfig")
        .def(py::init<>())
        .def_readwrite("rank", &NanoQuantConfig::rank)
        .def_readwrite("outer_iterations", &NanoQuantConfig::outer_iterations)
        .def_readwrite("projection_iterations",
                       &NanoQuantConfig::projection_iterations)
        .def_readwrite("regularization", &NanoQuantConfig::regularization)
        .def_readwrite("epsilon", &NanoQuantConfig::epsilon)
        .def_readwrite("seed", &NanoQuantConfig::seed);
    py::class_<NanoQuantFactors>(module, "NanoQuantFactors")
        .def_readonly("u_sign", &NanoQuantFactors::u_sign)
        .def_readonly("v_sign", &NanoQuantFactors::v_sign)
        .def_readonly("pre_scale", &NanoQuantFactors::pre_scale)
        .def_readonly("post_scale", &NanoQuantFactors::post_scale)
        .def_property_readonly("logical_storage_bits",
                               &NanoQuantFactors::logical_storage_bits)
        .def_property_readonly("packed_storage_bits",
                               &NanoQuantFactors::packed_storage_bits)
        .def_property_readonly("bits_per_weight",
                               &NanoQuantFactors::bits_per_weight)
        .def("reconstruct", &NanoQuantFactors::reconstruct)
        .def("forward", &NanoQuantFactors::forward);
    module.def("nano_quantize", &nano_quantize,
               py::arg("weights"), py::arg("config") = NanoQuantConfig(),
               py::arg("input_importance") = Tensor(),
               py::arg("output_importance") = Tensor());

    py::class_<CheckpointShard>(module, "CheckpointShard")
        .def(py::init<>())
        .def_readwrite("name", &CheckpointShard::name)
        .def_readwrite("data", &CheckpointShard::data)
        .def_property_readonly("bytes", &CheckpointShard::bytes);
    py::class_<PersistResult>(module, "PersistResult")
        .def_readonly("files", &PersistResult::files)
        .def_readonly("bytes", &PersistResult::bytes)
        .def_readonly("seconds", &PersistResult::seconds)
        .def_property_readonly("throughput", &PersistResult::throughput);
    py::class_<FastPersistWriter::Config>(module, "FastPersistConfig")
        .def(py::init<>())
        .def_readwrite("directory", &FastPersistWriter::Config::directory)
        .def_readwrite("num_writers", &FastPersistWriter::Config::num_writers)
        .def_readwrite("staging_buffers",
                       &FastPersistWriter::Config::staging_buffers)
        .def_readwrite("overlap_with_compute",
                       &FastPersistWriter::Config::overlap_with_compute);
    py::class_<FastPersistWriter>(module, "FastPersistWriter")
        .def(py::init<FastPersistWriter::Config>())
        .def("write", &FastPersistWriter::write)
        .def("write_async", &FastPersistWriter::write_async)
        .def("wait", &FastPersistWriter::wait)
        .def_property_readonly("has_pending_write",
                               &FastPersistWriter::has_pending_write)
        .def("read", &FastPersistWriter::read)
        .def_static("assign_shards_to_writers",
                    &FastPersistWriter::assign_shards_to_writers)
        .def_static("estimated_seconds", &FastPersistWriter::estimated_seconds);

    py::enum_<ParamPattern>(module, "ParamPattern")
        .value("UNIQUE", ParamPattern::kUnique)
        .value("REPLICATED", ParamPattern::kReplicated)
        .value("ROW_PARALLEL", ParamPattern::kRowParallel)
        .value("COLUMN_PARALLEL", ParamPattern::kColumnParallel)
        .value("FLATTENED_SHARD", ParamPattern::kFlattenedShard);
    py::class_<ParallelConfig>(module, "ParallelConfig")
        .def(py::init<>())
        .def_readwrite("data_parallel", &ParallelConfig::data_parallel)
        .def_readwrite("tensor_parallel", &ParallelConfig::tensor_parallel)
        .def_readwrite("pipeline_parallel", &ParallelConfig::pipeline_parallel)
        .def_property_readonly("world_size", &ParallelConfig::world_size)
        .def("rank_of", &ParallelConfig::rank_of);
    py::class_<ParameterSpec>(module, "ParameterSpec")
        .def(py::init<>())
        .def_readwrite("name", &ParameterSpec::name)
        .def_readwrite("shape", &ParameterSpec::shape)
        .def_readwrite("pattern", &ParameterSpec::pattern)
        .def_readwrite("pipeline_stage", &ParameterSpec::pipeline_stage);
    py::class_<RankState>(module, "RankState")
        .def(py::init<>())
        .def_readwrite("rank", &RankState::rank)
        .def_readwrite("tensors", &RankState::tensors);
    py::class_<UniversalCheckpoint>(module, "UniversalCheckpoint")
        .def_static("consolidate", &UniversalCheckpoint::consolidate)
        .def("reconfigure", &UniversalCheckpoint::reconfigure)
        .def_property_readonly("specs", &UniversalCheckpoint::specs)
        .def("atom", &UniversalCheckpoint::atom,
             py::return_value_policy::reference_internal)
        .def("contains", &UniversalCheckpoint::contains)
        .def_property_readonly("atom_count", &UniversalCheckpoint::atom_count);

    py::class_<PaperCitation>(module, "PaperCitation")
        .def_readonly("key", &PaperCitation::key)
        .def_readonly("title", &PaperCitation::title)
        .def_readonly("authors", &PaperCitation::authors)
        .def_readonly("venue", &PaperCitation::venue)
        .def_readonly("arxiv_id", &PaperCitation::arxiv_id)
        .def_readonly("url", &PaperCitation::url)
        .def_readonly("component", &PaperCitation::component);
    module.def("citations", &scaling_citations);
    module.def("citations_for_component", &citations_for_component);
    module.def("to_bibtex", &to_bibtex);
}

} // namespace

void bind_deep_learning_extensions(py::module_& module) {
    bind_rag(module);
    bind_scaling(module);
}
