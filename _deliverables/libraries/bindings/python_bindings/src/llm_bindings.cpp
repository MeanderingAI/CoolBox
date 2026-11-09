#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "DL/layers/include/layer.h"
#include "DL/llm/include/decoder_language_model.h"
#include "DL/llm/include/tokenizer.h"

namespace py = pybind11;

void bind_llm(py::module_& deep_learning_module) {
    using namespace ml::deep_learning;
    using namespace ml::deep_learning::llm;

    py::class_<
        EmbeddingLayer, Layer, std::shared_ptr<EmbeddingLayer>>(
        deep_learning_module, "EmbeddingLayer")
        .def(py::init<std::size_t, std::size_t>())
        .def(
            "update_parameters",
            &EmbeddingLayer::update_parameters,
            py::arg("learning_rate"));

    py::class_<
        MultiHeadAttentionLayer, Layer,
        std::shared_ptr<MultiHeadAttentionLayer>>(
        deep_learning_module, "MultiHeadAttentionLayer")
        .def(py::init<std::size_t, std::size_t>())
        .def(
            "forward_attention",
            py::overload_cast<
                const Tensor&, const Tensor&, const Tensor&,
                const Tensor*>(
                &MultiHeadAttentionLayer::forward),
            py::arg("query"),
            py::arg("key"),
            py::arg("value"),
            py::arg("mask") = nullptr)
        .def(
            "update_parameters",
            &MultiHeadAttentionLayer::update_parameters,
            py::arg("learning_rate"));

    py::class_<
        FeedForwardLayer, Layer,
        std::shared_ptr<FeedForwardLayer>>(
        deep_learning_module, "FeedForwardLayer")
        .def(py::init<std::size_t, std::size_t>())
        .def(
            "update_parameters",
            &FeedForwardLayer::update_parameters,
            py::arg("learning_rate"));

    py::class_<
        PositionalEncodingLayer, Layer,
        std::shared_ptr<PositionalEncodingLayer>>(
        deep_learning_module, "PositionalEncodingLayer")
        .def(
            py::init<std::size_t, std::size_t, double>(),
            py::arg("d_model"),
            py::arg("max_sequence_length") = 5000,
            py::arg("dropout_rate") = 0.1)
        .def(
            "set_training",
            &PositionalEncodingLayer::set_training);

    py::class_<
        TransformerEncoderLayer, Layer,
        std::shared_ptr<TransformerEncoderLayer>>(
        deep_learning_module, "TransformerEncoderLayer")
        .def(
            py::init<std::size_t, std::size_t, std::size_t, double>(),
            py::arg("d_model"),
            py::arg("num_heads"),
            py::arg("d_ff"),
            py::arg("dropout_rate") = 0.1)
        .def(
            "update_parameters",
            &TransformerEncoderLayer::update_parameters,
            py::arg("learning_rate"));

    py::class_<
        TransformerDecoderLayer, Layer,
        std::shared_ptr<TransformerDecoderLayer>>(
        deep_learning_module, "TransformerDecoderLayer")
        .def(
            py::init<std::size_t, std::size_t, std::size_t, double>(),
            py::arg("d_model"),
            py::arg("num_heads"),
            py::arg("d_ff"),
            py::arg("dropout_rate") = 0.1)
        .def(
            "forward_with_memory",
            py::overload_cast<
                const Tensor&, const Tensor&, const Tensor*,
                const Tensor*>(
                &TransformerDecoderLayer::forward),
            py::arg("target"),
            py::arg("encoder_output"),
            py::arg("target_mask") = nullptr,
            py::arg("memory_mask") = nullptr)
        .def(
            "update_parameters",
            &TransformerDecoderLayer::update_parameters,
            py::arg("learning_rate"));

    py::class_<
        DecoderOnlyTransformerLayer, Layer,
        std::shared_ptr<DecoderOnlyTransformerLayer>>(
        deep_learning_module, "DecoderOnlyTransformerLayer")
        .def(
            py::init<std::size_t, std::size_t, std::size_t, double>(),
            py::arg("d_model"),
            py::arg("num_heads"),
            py::arg("d_ff"),
            py::arg("dropout_rate") = 0.1)
        .def(
            "set_training",
            &DecoderOnlyTransformerLayer::set_training)
        .def(
            "update_parameters",
            &DecoderOnlyTransformerLayer::update_parameters,
            py::arg("learning_rate"));

    py::module_ module = deep_learning_module.def_submodule(
        "llm", "Decoder-only language models and tokenizers");

    py::class_<VocabularyInfo>(module, "VocabularyInfo")
        .def_readonly("size", &VocabularyInfo::size)
        .def_readonly("pad_token_id", &VocabularyInfo::pad_token_id)
        .def_readonly("bos_token_id", &VocabularyInfo::bos_token_id)
        .def_readonly("eos_token_id", &VocabularyInfo::eos_token_id)
        .def_readonly("unk_token_id", &VocabularyInfo::unk_token_id);

    py::class_<ByteTokenizer>(module, "ByteTokenizer")
        .def(py::init<>())
        .def(
            "encode", &ByteTokenizer::encode,
            py::arg("text"),
            py::arg("add_bos") = false,
            py::arg("add_eos") = false)
        .def(
            "decode", &ByteTokenizer::decode,
            py::arg("token_ids"),
            py::arg("skip_special_tokens") = true)
        .def_property_readonly(
            "vocabulary", &ByteTokenizer::vocabulary);

    py::class_<BpeTokenizer>(module, "BpeTokenizer")
        .def(py::init<>())
        .def(
            "train", &BpeTokenizer::train,
            py::arg("corpus"),
            py::arg("vocab_size"),
            py::arg("min_frequency") = 2)
        .def(
            "encode", &BpeTokenizer::encode,
            py::arg("text"),
            py::arg("add_bos") = false,
            py::arg("add_eos") = false)
        .def(
            "decode", &BpeTokenizer::decode,
            py::arg("token_ids"),
            py::arg("skip_special_tokens") = true)
        .def_property_readonly(
            "vocabulary", &BpeTokenizer::vocabulary)
        .def_property_readonly("merges", &BpeTokenizer::merges)
        .def("save", &BpeTokenizer::save, py::arg("path"))
        .def_static("load", &BpeTokenizer::load, py::arg("path"));

    py::enum_<LanguageModelOptimizer>(module, "Optimizer")
        .value("SGD", LanguageModelOptimizer::SGD)
        .value("ADAMW", LanguageModelOptimizer::ADAMW);

    py::class_<DecoderLanguageModelConfig>(module, "ModelConfig")
        .def(py::init<>())
        .def_readwrite("vocab_size", &DecoderLanguageModelConfig::vocab_size)
        .def_readwrite("d_model", &DecoderLanguageModelConfig::d_model)
        .def_readwrite("num_heads", &DecoderLanguageModelConfig::num_heads)
        .def_readwrite("d_ff", &DecoderLanguageModelConfig::d_ff)
        .def_readwrite("num_layers", &DecoderLanguageModelConfig::num_layers)
        .def_readwrite(
            "max_sequence_length",
            &DecoderLanguageModelConfig::max_sequence_length)
        .def_readwrite(
            "dropout_rate", &DecoderLanguageModelConfig::dropout_rate)
        .def_readwrite(
            "optimizer", &DecoderLanguageModelConfig::optimizer)
        .def_readwrite(
            "learning_rate", &DecoderLanguageModelConfig::learning_rate)
        .def_readwrite(
            "weight_decay", &DecoderLanguageModelConfig::weight_decay)
        .def_readwrite("beta1", &DecoderLanguageModelConfig::beta1)
        .def_readwrite("beta2", &DecoderLanguageModelConfig::beta2)
        .def_readwrite("epsilon", &DecoderLanguageModelConfig::epsilon)
        .def_readwrite(
            "gradient_clip_norm",
            &DecoderLanguageModelConfig::gradient_clip_norm)
        .def_readwrite("seed", &DecoderLanguageModelConfig::seed);

    py::class_<GenerationConfig>(module, "GenerationConfig")
        .def(py::init<>())
        .def_readwrite(
            "max_new_tokens", &GenerationConfig::max_new_tokens)
        .def_readwrite("temperature", &GenerationConfig::temperature)
        .def_readwrite("top_k", &GenerationConfig::top_k)
        .def_readwrite("top_p", &GenerationConfig::top_p)
        .def_readwrite(
            "eos_token_id", &GenerationConfig::eos_token_id)
        .def_readwrite("seed", &GenerationConfig::seed);

    py::class_<DecoderLanguageModel>(
        module, "DecoderLanguageModel")
        .def(py::init<const DecoderLanguageModelConfig&>())
        .def(
            "logits", &DecoderLanguageModel::logits,
            py::arg("input_ids"),
            py::call_guard<py::gil_scoped_release>())
        .def(
            "train_step", &DecoderLanguageModel::train_step,
            py::arg("token_ids"),
            py::call_guard<py::gil_scoped_release>())
        .def(
            "fit", &DecoderLanguageModel::fit,
            py::arg("token_sequences"),
            py::arg("epochs") = 1,
            py::arg("verbose") = false,
            py::call_guard<py::gil_scoped_release>())
        .def(
            "evaluate", &DecoderLanguageModel::evaluate,
            py::arg("token_sequences"),
            py::call_guard<py::gil_scoped_release>())
        .def(
            "generate", &DecoderLanguageModel::generate,
            py::arg("prompt"),
            py::arg("config") = GenerationConfig{},
            py::call_guard<py::gil_scoped_release>())
        .def("set_training", &DecoderLanguageModel::set_training)
        .def_property_readonly(
            "training", &DecoderLanguageModel::training)
        .def_property_readonly(
            "config", &DecoderLanguageModel::config,
            py::return_value_policy::reference_internal)
        .def_property_readonly(
            "parameter_count",
            &DecoderLanguageModel::parameter_count)
        .def_property_readonly(
            "optimizer_step",
            &DecoderLanguageModel::optimizer_step)
        .def(
            "state_dict",
            [](DecoderLanguageModel& model) {
                py::dict result;
                for (NamedTensor& parameter : model.state_dict()) {
                    result[py::str(parameter.name)] =
                        std::move(parameter.tensor);
                }
                return result;
            })
        .def(
            "load_state_dict",
            [](DecoderLanguageModel& model,
               const py::dict& state,
               bool strict) {
                std::vector<NamedTensor> tensors;
                tensors.reserve(state.size());
                for (const auto& item : state) {
                    tensors.push_back(NamedTensor{
                        py::cast<std::string>(item.first),
                        py::cast<Tensor>(item.second)});
                }
                model.load_state_dict(tensors, strict);
            },
            py::arg("state"),
            py::arg("strict") = true)
        .def("save", &DecoderLanguageModel::save, py::arg("path"))
        .def_static(
            "load", &DecoderLanguageModel::load, py::arg("path"))
        .def(
            "fit_text_byte",
            [](DecoderLanguageModel& model,
               const std::vector<std::string>& corpus,
               const ByteTokenizer& tokenizer,
               std::size_t epochs,
               bool verbose) {
                std::vector<std::vector<std::size_t>> sequences;
                sequences.reserve(corpus.size());
                for (const std::string& text : corpus) {
                    sequences.push_back(
                        tokenizer.encode(text, true, true));
                }
                py::gil_scoped_release release;
                return model.fit(sequences, epochs, verbose);
            },
            py::arg("corpus"),
            py::arg("tokenizer"),
            py::arg("epochs") = 1,
            py::arg("verbose") = false)
        .def(
            "fit_text_bpe",
            [](DecoderLanguageModel& model,
               const std::vector<std::string>& corpus,
               const BpeTokenizer& tokenizer,
               std::size_t epochs,
               bool verbose) {
                std::vector<std::vector<std::size_t>> sequences;
                sequences.reserve(corpus.size());
                for (const std::string& text : corpus) {
                    sequences.push_back(
                        tokenizer.encode(text, true, true));
                }
                py::gil_scoped_release release;
                return model.fit(sequences, epochs, verbose);
            },
            py::arg("corpus"),
            py::arg("tokenizer"),
            py::arg("epochs") = 1,
            py::arg("verbose") = false)
        .def(
            "generate_text_byte",
            [](DecoderLanguageModel& model,
               const std::string& prompt,
               const ByteTokenizer& tokenizer,
               const GenerationConfig& config) {
                std::vector<std::size_t> prompt_ids =
                    tokenizer.encode(prompt, true, false);
                std::vector<std::size_t> generated;
                {
                    py::gil_scoped_release release;
                    generated = model.generate(prompt_ids, config);
                }
                return tokenizer.decode(generated);
            },
            py::arg("prompt"),
            py::arg("tokenizer"),
            py::arg("config") = GenerationConfig{})
        .def(
            "generate_text_bpe",
            [](DecoderLanguageModel& model,
               const std::string& prompt,
               const BpeTokenizer& tokenizer,
               const GenerationConfig& config) {
                std::vector<std::size_t> prompt_ids =
                    tokenizer.encode(prompt, true, false);
                std::vector<std::size_t> generated;
                {
                    py::gil_scoped_release release;
                    generated = model.generate(prompt_ids, config);
                }
                return tokenizer.decode(generated);
            },
            py::arg("prompt"),
            py::arg("tokenizer"),
            py::arg("config") = GenerationConfig{});
}
