"""Standard model-only exports for CoolBox decoder language models."""

from __future__ import annotations

import json
import math
import os
import struct
from pathlib import Path
from typing import Any, Iterable

import numpy as np


_DTYPES = {
    "F64": (np.dtype("<f8"), "F64"),
    "F32": (np.dtype("<f4"), "F32"),
    "F16": (np.dtype("<f2"), "F16"),
}
_GGML_TYPES = {"F32": 0, "F16": 1, "Q8_0": 8, "F64": 28}
_GGUF_VALUE_TYPES = {
    "UINT32": 4,
    "INT32": 5,
    "STRING": 8,
    "ARRAY": 9,
}


def _normalize_dtype(dtype: str) -> str:
    normalized = dtype.upper().replace("FP", "F")
    if normalized not in _DTYPES:
        raise ValueError("dtype must be FP64, FP32, or FP16")
    return normalized


def _state_arrays(model: Any) -> dict[str, np.ndarray]:
    arrays: dict[str, np.ndarray] = {}
    for name, tensor in model.state_dict().items():
        shape = tuple(tensor.shape())
        arrays[name] = np.asarray(tensor.data(), dtype=np.float64).reshape(shape)
    if not arrays:
        raise ValueError("model state_dict is empty")
    return arrays


def _atomic_write(path: Path, payload: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    try:
        temporary.write_bytes(payload)
        os.replace(temporary, path)
    except BaseException:
        temporary.unlink(missing_ok=True)
        raise


def save_safetensors(model: Any, path: str | Path, dtype: str = "FP32") -> Path:
    """Write a model-only checkpoint using the SafeTensors wire format."""
    encoding = _normalize_dtype(dtype)
    numpy_dtype, safetensors_dtype = _DTYPES[encoding]
    arrays = _state_arrays(model)
    header: dict[str, Any] = {
        "__metadata__": {
            "format": "pt",
            "architecture": "coolbox_decoder",
            "source_dtype": "F64",
        }
    }
    chunks: list[bytes] = []
    offset = 0
    for name in sorted(arrays):
        array = np.ascontiguousarray(arrays[name], dtype=numpy_dtype)
        data = array.tobytes(order="C")
        header[name] = {
            "dtype": safetensors_dtype,
            "shape": list(array.shape),
            "data_offsets": [offset, offset + len(data)],
        }
        chunks.append(data)
        offset += len(data)
    encoded_header = json.dumps(
        header, separators=(",", ":"), ensure_ascii=False
    ).encode("utf-8")
    encoded_header += b" " * ((8 - len(encoded_header) % 8) % 8)
    payload = struct.pack("<Q", len(encoded_header)) + encoded_header + b"".join(chunks)
    destination = Path(path)
    _atomic_write(destination, payload)
    return destination


def load_safetensors(model: Any, path: str | Path, strict: bool = True) -> None:
    """Load F64/F32/F16 SafeTensors values into a compatible CoolBox model."""
    from . import ml_core

    payload = Path(path).read_bytes()
    if len(payload) < 8:
        raise ValueError("SafeTensors file is truncated")
    header_size = struct.unpack_from("<Q", payload)[0]
    data_start = 8 + header_size
    if data_start > len(payload):
        raise ValueError("SafeTensors header extends beyond the file")
    try:
        header = json.loads(payload[8:data_start])
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ValueError("SafeTensors header is invalid") from error
    state: dict[str, Any] = {}
    for name, metadata in header.items():
        if name == "__metadata__":
            continue
        dtype_name = metadata.get("dtype")
        dtype = {
            "F64": np.dtype("<f8"),
            "F32": np.dtype("<f4"),
            "F16": np.dtype("<f2"),
        }.get(dtype_name)
        if dtype is None:
            raise ValueError(f"unsupported SafeTensors dtype for {name}: {dtype_name}")
        start, end = metadata["data_offsets"]
        if start < 0 or end < start or data_start + end > len(payload):
            raise ValueError(f"invalid SafeTensors offsets for {name}")
        shape = [int(value) for value in metadata["shape"]]
        values = np.frombuffer(
            payload[data_start + start : data_start + end], dtype=dtype
        ).astype(np.float64)
        if values.size != math.prod(shape):
            raise ValueError(f"SafeTensors shape does not match data for {name}")
        state[name] = ml_core.deep_learning.Tensor(shape, values.tolist())
    model.load_state_dict(state, strict=strict)


def _byte_encoder() -> dict[int, str]:
    byte_values = list(range(ord("!"), ord("~") + 1))
    byte_values += list(range(ord("¡"), ord("¬") + 1))
    byte_values += list(range(ord("®"), ord("ÿ") + 1))
    code_points = byte_values.copy()
    missing_index = 0
    for byte in range(256):
        if byte not in byte_values:
            byte_values.append(byte)
            code_points.append(256 + missing_index)
            missing_index += 1
    return dict(zip(byte_values, map(chr, code_points)))


def _tokenizer_description(tokenizer: Any) -> tuple[list[str], list[str]]:
    encoder = _byte_encoder()
    tokens = ["<pad>", "<bos>", "<eos>", "<unk>"]
    token_bytes: list[bytes] = [b"", b"", b"", b""]
    for byte in range(256):
        token_bytes.append(bytes([byte]))
        tokens.append(encoder[byte])
    merges: list[str] = []
    for left, right in getattr(tokenizer, "merges", []):
        left = int(left)
        right = int(right)
        if left >= len(token_bytes) or right >= len(token_bytes):
            raise ValueError("tokenizer contains an invalid BPE merge")
        merged = token_bytes[left] + token_bytes[right]
        token_bytes.append(merged)
        left_token = "".join(encoder[byte] for byte in token_bytes[left])
        right_token = "".join(encoder[byte] for byte in token_bytes[right])
        merges.append(f"{left_token} {right_token}")
        tokens.append("".join(encoder[byte] for byte in merged))
    vocabulary_size = int(tokenizer.vocabulary.size)
    if len(tokens) != vocabulary_size:
        raise ValueError(
            "tokenizer vocabulary is inconsistent with its BPE merge table"
        )
    return tokens, merges


def _tokenizer_json(tokenizer: Any) -> dict[str, Any]:
    tokens, merges = _tokenizer_description(tokenizer)
    added_tokens = [
        {
            "id": index,
            "content": token,
            "single_word": False,
            "lstrip": False,
            "rstrip": False,
            "normalized": False,
            "special": True,
        }
        for index, token in enumerate(tokens[:4])
    ]
    return {
        "version": "1.0",
        "truncation": None,
        "padding": None,
        "added_tokens": added_tokens,
        "normalizer": None,
        "pre_tokenizer": {
            "type": "ByteLevel",
            "add_prefix_space": False,
            "trim_offsets": True,
            "use_regex": False,
        },
        "post_processor": None,
        "decoder": {
            "type": "ByteLevel",
            "add_prefix_space": False,
            "trim_offsets": True,
            "use_regex": False,
        },
        "model": {
            "type": "BPE",
            "dropout": None,
            "unk_token": "<unk>",
            "continuing_subword_prefix": "",
            "end_of_word_suffix": "",
            "fuse_unk": False,
            "byte_fallback": False,
            "vocab": {token: index for index, token in enumerate(tokens)},
            "merges": merges,
        },
    }


def export_huggingface(
    model: Any,
    tokenizer: Any,
    directory: str | Path,
    dtype: str = "FP32",
) -> Path:
    """Export a Hugging Face-style custom CoolBox model directory.

    The files use standard Hugging Face configuration, tokenizer, and
    SafeTensors formats. Because the network is a CoolBox-specific architecture,
    Transformers requires a matching ``CoolBoxDecoderForCausalLM`` implementation
    to execute it.
    """
    destination = Path(directory)
    destination.mkdir(parents=True, exist_ok=True)
    config = model.config
    vocabulary = tokenizer.vocabulary
    if int(config.vocab_size) != int(vocabulary.size):
        raise ValueError("model and tokenizer vocabulary sizes differ")
    model_config = {
        "model_type": "coolbox_decoder",
        "architectures": ["CoolBoxDecoderForCausalLM"],
        "vocab_size": int(config.vocab_size),
        "hidden_size": int(config.d_model),
        "intermediate_size": int(config.d_ff),
        "num_hidden_layers": int(config.num_layers),
        "num_attention_heads": int(config.num_heads),
        "max_position_embeddings": int(config.max_sequence_length),
        "hidden_act": "relu",
        "layer_norm_eps": 1e-5,
        "position_embedding_type": "sinusoidal",
        "tie_word_embeddings": False,
        "use_cache": False,
        "bos_token_id": int(vocabulary.bos_token_id),
        "eos_token_id": int(vocabulary.eos_token_id),
        "pad_token_id": int(vocabulary.pad_token_id),
        "unk_token_id": int(vocabulary.unk_token_id),
        "torch_dtype": {
            "F64": "float64",
            "F32": "float32",
            "F16": "float16",
        }[_normalize_dtype(dtype)],
    }
    generation_config = {
        "_from_model_config": True,
        "bos_token_id": int(vocabulary.bos_token_id),
        "eos_token_id": int(vocabulary.eos_token_id),
        "pad_token_id": int(vocabulary.pad_token_id),
        "transformers_version": None,
    }
    tokenizer_config = {
        "tokenizer_class": "PreTrainedTokenizerFast",
        "model_max_length": int(config.max_sequence_length),
        "bos_token": "<bos>",
        "eos_token": "<eos>",
        "unk_token": "<unk>",
        "pad_token": "<pad>",
        "clean_up_tokenization_spaces": False,
    }
    special_tokens = {
        "bos_token": "<bos>",
        "eos_token": "<eos>",
        "unk_token": "<unk>",
        "pad_token": "<pad>",
    }
    for filename, value in {
        "config.json": model_config,
        "generation_config.json": generation_config,
        "tokenizer.json": _tokenizer_json(tokenizer),
        "tokenizer_config.json": tokenizer_config,
        "special_tokens_map.json": special_tokens,
    }.items():
        _atomic_write(
            destination / filename,
            (json.dumps(value, indent=2, ensure_ascii=False) + "\n").encode("utf-8"),
        )
    save_safetensors(model, destination / "model.safetensors", dtype)
    return destination


def _sinusoidal_positions(length: int, width: int) -> np.ndarray:
    result = np.empty((length, width), dtype=np.float64)
    for position in range(length):
        for index in range(width):
            denominator = 10000.0 ** (2 * (index // 2) / width)
            value = position / denominator
            result[position, index] = math.sin(value) if index % 2 == 0 else math.cos(value)
    return result


def export_onnx(
    model: Any,
    path: str | Path,
    dtype: str = "FP32",
) -> Path:
    """Export a dynamic-sequence inference graph. Requires the optional `onnx` package."""
    try:
        import onnx
        from onnx import TensorProto, helper, numpy_helper
    except ImportError as error:
        raise RuntimeError(
            "ONNX export requires the optional 'onnx' package"
        ) from error

    encoding = _normalize_dtype(dtype)
    numpy_dtype = _DTYPES[encoding][0]
    tensor_type = {
        "F64": TensorProto.DOUBLE,
        "F32": TensorProto.FLOAT,
        "F16": TensorProto.FLOAT16,
    }[encoding]
    arrays = {
        name: np.ascontiguousarray(array, dtype=numpy_dtype)
        for name, array in _state_arrays(model).items()
    }
    config = model.config
    heads = int(config.num_heads)
    head_width = int(config.d_model) // heads
    initializers = [
        numpy_helper.from_array(array, name=name)
        for name, array in arrays.items()
    ]
    initializers.extend(
        [
            numpy_helper.from_array(
                _sinusoidal_positions(
                    int(config.max_sequence_length), int(config.d_model)
                ).astype(numpy_dtype),
                name="position_encoding",
            ),
            numpy_helper.from_array(np.array(0, dtype=np.int64), name="zero_i64"),
            numpy_helper.from_array(np.array(1, dtype=np.int64), name="one_i64"),
            numpy_helper.from_array(np.array([0], dtype=np.int64), name="axis_zero"),
            numpy_helper.from_array(np.array([1], dtype=np.int64), name="axis_one"),
            numpy_helper.from_array(
                np.array([-1, heads, head_width], dtype=np.int64),
                name="attention_shape",
            ),
            numpy_helper.from_array(
                np.array([-1, int(config.d_model)], dtype=np.int64),
                name="hidden_shape",
            ),
            numpy_helper.from_array(
                np.array(1.0 / math.sqrt(head_width), dtype=numpy_dtype),
                name="attention_scale",
            ),
            numpy_helper.from_array(np.array(0.0, dtype=numpy_dtype), name="mask_zero"),
            numpy_helper.from_array(np.array(-1e4, dtype=numpy_dtype), name="mask_value"),
        ]
    )
    nodes = [
        helper.make_node("Gather", ["model.embed_tokens.weight", "input_ids"], ["token_embeddings"]),
        helper.make_node("Shape", ["input_ids"], ["input_shape"]),
        helper.make_node("Gather", ["input_shape", "zero_i64"], ["sequence_length"], axis=0),
        helper.make_node(
            "Range", ["zero_i64", "sequence_length", "one_i64"], ["positions"]
        ),
        helper.make_node("Gather", ["position_encoding", "positions"], ["positions_encoded"]),
        helper.make_node("Add", ["token_embeddings", "positions_encoded"], ["hidden_0"]),
        helper.make_node("Unsqueeze", ["positions", "axis_one"], ["mask_rows"]),
        helper.make_node("Unsqueeze", ["positions", "axis_zero"], ["mask_columns"]),
        helper.make_node("LessOrEqual", ["mask_columns", "mask_rows"], ["causal_condition"]),
        helper.make_node(
            "Where", ["causal_condition", "mask_zero", "mask_value"], ["causal_mask"]
        ),
    ]
    hidden = "hidden_0"
    for layer in range(int(config.num_layers)):
        prefix = f"model.layers.{layer}"
        projections: dict[str, str] = {}
        for projection in ("q", "k", "v"):
            multiplied = f"layer_{layer}_{projection}_matmul"
            projected = f"layer_{layer}_{projection}"
            reshaped = f"{projected}_reshape"
            heads_name = f"{projected}_heads"
            nodes.extend(
                [
                    helper.make_node(
                        "MatMul",
                        [hidden, f"{prefix}.self_attn.{projection}_proj.weight"],
                        [multiplied],
                    ),
                    helper.make_node(
                        "Add",
                        [multiplied, f"{prefix}.self_attn.{projection}_proj.bias"],
                        [projected],
                    ),
                    helper.make_node(
                        "Reshape", [projected, "attention_shape"], [reshaped]
                    ),
                    helper.make_node(
                        "Transpose", [reshaped], [heads_name], perm=[1, 0, 2]
                    ),
                ]
            )
            projections[projection] = heads_name
        nodes.extend(
            [
                helper.make_node(
                    "Transpose",
                    [projections["k"]],
                    [f"layer_{layer}_key_transposed"],
                    perm=[0, 2, 1],
                ),
                helper.make_node(
                    "MatMul",
                    [projections["q"], f"layer_{layer}_key_transposed"],
                    [f"layer_{layer}_scores_raw"],
                ),
                helper.make_node(
                    "Mul",
                    [f"layer_{layer}_scores_raw", "attention_scale"],
                    [f"layer_{layer}_scores_scaled"],
                ),
                helper.make_node(
                    "Add",
                    [f"layer_{layer}_scores_scaled", "causal_mask"],
                    [f"layer_{layer}_scores_masked"],
                ),
                helper.make_node(
                    "Softmax",
                    [f"layer_{layer}_scores_masked"],
                    [f"layer_{layer}_probabilities"],
                    axis=-1,
                ),
                helper.make_node(
                    "MatMul",
                    [f"layer_{layer}_probabilities", projections["v"]],
                    [f"layer_{layer}_context_heads"],
                ),
                helper.make_node(
                    "Transpose",
                    [f"layer_{layer}_context_heads"],
                    [f"layer_{layer}_context_transposed"],
                    perm=[1, 0, 2],
                ),
                helper.make_node(
                    "Reshape",
                    [f"layer_{layer}_context_transposed", "hidden_shape"],
                    [f"layer_{layer}_context"],
                ),
                helper.make_node(
                    "MatMul",
                    [f"layer_{layer}_context", f"{prefix}.self_attn.o_proj.weight"],
                    [f"layer_{layer}_attention_matmul"],
                ),
                helper.make_node(
                    "Add",
                    [f"layer_{layer}_attention_matmul", f"{prefix}.self_attn.o_proj.bias"],
                    [f"layer_{layer}_attention"],
                ),
                helper.make_node(
                    "Add", [hidden, f"layer_{layer}_attention"], [f"layer_{layer}_residual_1"]
                ),
                helper.make_node(
                    "LayerNormalization",
                    [
                        f"layer_{layer}_residual_1",
                        f"{prefix}.input_layernorm.weight",
                        f"{prefix}.input_layernorm.bias",
                    ],
                    [f"layer_{layer}_normalized_1"],
                    axis=-1,
                    epsilon=1e-5,
                ),
                helper.make_node(
                    "MatMul",
                    [f"layer_{layer}_normalized_1", f"{prefix}.mlp.fc1.weight"],
                    [f"layer_{layer}_ff1_matmul"],
                ),
                helper.make_node(
                    "Add",
                    [f"layer_{layer}_ff1_matmul", f"{prefix}.mlp.fc1.bias"],
                    [f"layer_{layer}_ff1"],
                ),
                helper.make_node(
                    "Relu", [f"layer_{layer}_ff1"], [f"layer_{layer}_activated"]
                ),
                helper.make_node(
                    "MatMul",
                    [f"layer_{layer}_activated", f"{prefix}.mlp.fc2.weight"],
                    [f"layer_{layer}_ff2_matmul"],
                ),
                helper.make_node(
                    "Add",
                    [f"layer_{layer}_ff2_matmul", f"{prefix}.mlp.fc2.bias"],
                    [f"layer_{layer}_ff2"],
                ),
                helper.make_node(
                    "Add",
                    [f"layer_{layer}_normalized_1", f"layer_{layer}_ff2"],
                    [f"layer_{layer}_residual_2"],
                ),
                helper.make_node(
                    "LayerNormalization",
                    [
                        f"layer_{layer}_residual_2",
                        f"{prefix}.post_attention_layernorm.weight",
                        f"{prefix}.post_attention_layernorm.bias",
                    ],
                    [f"hidden_{layer + 1}"],
                    axis=-1,
                    epsilon=1e-5,
                ),
            ]
        )
        hidden = f"hidden_{layer + 1}"
    nodes.extend(
        [
            helper.make_node("MatMul", [hidden, "lm_head.weight"], ["logits_matmul"]),
            helper.make_node("Add", ["logits_matmul", "lm_head.bias"], ["logits"]),
        ]
    )
    graph = helper.make_graph(
        nodes,
        "CoolBoxDecoderForCausalLM",
        [
            helper.make_tensor_value_info(
                "input_ids", TensorProto.INT64, ["sequence_length"]
            )
        ],
        [
            helper.make_tensor_value_info(
                "logits",
                tensor_type,
                ["sequence_length", int(config.vocab_size)],
            )
        ],
        initializer=initializers,
    )
    exported = helper.make_model(
        graph,
        producer_name="CoolBox",
        opset_imports=[helper.make_opsetid("", 17)],
    )
    exported.ir_version = min(exported.ir_version, 9)
    onnx.checker.check_model(exported)
    destination = Path(path)
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_name(destination.name + ".tmp")
    try:
        onnx.save_model(exported, temporary)
        os.replace(temporary, destination)
    except BaseException:
        temporary.unlink(missing_ok=True)
        raise
    return destination


def _gguf_string(value: str) -> bytes:
    encoded = value.encode("utf-8")
    return struct.pack("<Q", len(encoded)) + encoded


def _gguf_metadata(key: str, kind: str, value: Any) -> bytes:
    result = bytearray(_gguf_string(key))
    result.extend(struct.pack("<I", _GGUF_VALUE_TYPES[kind]))
    if kind == "STRING":
        result.extend(_gguf_string(value))
    elif kind == "UINT32":
        result.extend(struct.pack("<I", value))
    elif kind == "INT32":
        result.extend(struct.pack("<i", value))
    else:
        element_kind, values = value
        result.extend(struct.pack("<I", _GGUF_VALUE_TYPES[element_kind]))
        result.extend(struct.pack("<Q", len(values)))
        for item in values:
            if element_kind == "STRING":
                result.extend(_gguf_string(item))
            elif element_kind == "INT32":
                result.extend(struct.pack("<i", item))
            else:
                raise ValueError(f"unsupported GGUF array type: {element_kind}")
    return bytes(result)


def _quantize_q8_0(array: np.ndarray) -> bytes:
    rows = np.asarray(array, dtype=np.float32).reshape(-1, array.shape[-1])
    result = bytearray()
    for row in rows:
        for start in range(0, row.size, 32):
            block = row[start : start + 32]
            maximum = float(np.max(np.abs(block)))
            scale = maximum / 127.0 if maximum else 0.0
            quantized = (
                np.rint(block / scale).clip(-127, 127).astype(np.int8)
                if scale
                else np.zeros(32, dtype=np.int8)
            )
            result.extend(np.float16(scale).tobytes())
            result.extend(quantized.tobytes())
    return bytes(result)


def export_gguf(
    model: Any,
    tokenizer: Any,
    path: str | Path,
    dtype: str = "FP16",
    name: str = "CoolBox Decoder",
) -> Path:
    """Write a GGUF v3 model using F64, F32, F16, or mixed Q8_0 tensors."""
    encoding = dtype.upper().replace("FP", "F")
    if encoding not in _GGML_TYPES:
        raise ValueError("dtype must be FP64, FP32, FP16, or Q8_0")
    arrays = _state_arrays(model)
    config = model.config
    vocabulary = tokenizer.vocabulary
    if int(config.vocab_size) != int(vocabulary.size):
        raise ValueError("model and tokenizer vocabulary sizes differ")
    tokens, merges = _tokenizer_description(tokenizer)
    metadata = [
        ("general.architecture", "STRING", "coolbox"),
        ("general.name", "STRING", name),
        ("general.alignment", "UINT32", 32),
        ("coolbox.context_length", "UINT32", int(config.max_sequence_length)),
        ("coolbox.embedding_length", "UINT32", int(config.d_model)),
        ("coolbox.feed_forward_length", "UINT32", int(config.d_ff)),
        ("coolbox.block_count", "UINT32", int(config.num_layers)),
        ("coolbox.attention.head_count", "UINT32", int(config.num_heads)),
        ("coolbox.tensor_encoding", "STRING", encoding),
        ("tokenizer.ggml.model", "STRING", "gpt2"),
        ("tokenizer.ggml.tokens", "ARRAY", ("STRING", tokens)),
        ("tokenizer.ggml.token_type", "ARRAY", ("INT32", [3] * 4 + [1] * (len(tokens) - 4))),
        ("tokenizer.ggml.merges", "ARRAY", ("STRING", merges)),
        ("tokenizer.ggml.bos_token_id", "UINT32", int(vocabulary.bos_token_id)),
        ("tokenizer.ggml.eos_token_id", "UINT32", int(vocabulary.eos_token_id)),
        ("tokenizer.ggml.unknown_token_id", "UINT32", int(vocabulary.unk_token_id)),
        ("tokenizer.ggml.padding_token_id", "UINT32", int(vocabulary.pad_token_id)),
    ]
    tensor_records: list[tuple[str, tuple[int, ...], int, int, bytes]] = []
    data_offset = 0
    for tensor_name in sorted(arrays):
        array = arrays[tensor_name]
        tensor_encoding = encoding
        if encoding == "Q8_0" and (
            array.ndim < 2 or array.shape[-1] % 32 != 0
        ):
            tensor_encoding = "F16"
        if tensor_encoding == "Q8_0":
            data = _quantize_q8_0(array)
        else:
            data = np.ascontiguousarray(
                array, dtype=_DTYPES[tensor_encoding][0]
            ).tobytes()
        data_offset = (data_offset + 31) // 32 * 32
        tensor_records.append(
            (
                tensor_name,
                tuple(int(value) for value in array.shape),
                _GGML_TYPES[tensor_encoding],
                data_offset,
                data,
            )
        )
        data_offset += len(data)
    result = bytearray(b"GGUF")
    result.extend(struct.pack("<IQQ", 3, len(tensor_records), len(metadata)))
    for key, kind, value in metadata:
        result.extend(_gguf_metadata(key, kind, value))
    for tensor_name, shape, tensor_type, offset, _ in tensor_records:
        result.extend(_gguf_string(tensor_name))
        result.extend(struct.pack("<I", len(shape)))
        for dimension in reversed(shape):
            result.extend(struct.pack("<Q", dimension))
        result.extend(struct.pack("<IQ", tensor_type, offset))
    result.extend(b"\0" * ((32 - len(result) % 32) % 32))
    data_start = len(result)
    for _, _, _, offset, data in tensor_records:
        absolute_offset = data_start + offset
        if len(result) < absolute_offset:
            result.extend(b"\0" * (absolute_offset - len(result)))
        result.extend(data)
    destination = Path(path)
    _atomic_write(destination, bytes(result))
    return destination


__all__ = [
    "export_gguf",
    "export_huggingface",
    "export_onnx",
    "load_safetensors",
    "save_safetensors",
]
