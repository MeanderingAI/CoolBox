from __future__ import annotations

import ctypes
import os
from dataclasses import dataclass
from pathlib import Path
from typing import List


DEFAULT_ENDPOINT = "local://coolbox"


def _candidate_library_paths() -> list[Path]:
    root = Path(__file__).resolve().parent
    build_dir = (root / "../c_bindings/build").resolve()
    names = ["coolbox_c_bindings.dll", "libcoolbox_c_bindings.so", "libcoolbox_c_bindings.dylib"]
    candidates: list[Path] = []

    env_dir = os.getenv("COOLBOX_C_BINDINGS_DIR")
    if env_dir:
        env_path = Path(env_dir).resolve()
        candidates.extend(env_path / name for name in names)

    for directory in (build_dir, build_dir / "Release"):
        candidates.extend(directory / name for name in names)

    return candidates


def _load_native() -> ctypes.CDLL:
    errors: list[str] = []
    for candidate in _candidate_library_paths():
        if candidate.exists():
            try:
                library = ctypes.CDLL(str(candidate))
                library.coolbox_c_version.restype = ctypes.c_char_p
                library.coolbox_c_describe.restype = ctypes.c_char_p
                library.coolbox_c_capability_count.restype = ctypes.c_size_t
                library.coolbox_c_capability_at.argtypes = [ctypes.c_size_t]
                library.coolbox_c_capability_at.restype = ctypes.c_char_p
                library.coolbox_c_is_ready.restype = ctypes.c_int
                return library
            except OSError as exc:
                errors.append(f"{candidate}: {exc}")

    joined = "; ".join(errors) if errors else "no candidate library found"
    raise ImportError(
        "Unable to load coolbox_c_bindings. Set COOLBOX_C_BINDINGS_DIR or build _libraries/c_bindings first. "
        f"Details: {joined}"
    )


_NATIVE = _load_native()


@dataclass(frozen=True)
class Client:
    endpoint: str = DEFAULT_ENDPOINT

    def version(self) -> str:
        return (_NATIVE.coolbox_c_version() or b"").decode("utf-8")

    def describe(self) -> str:
        return (_NATIVE.coolbox_c_describe() or b"").decode("utf-8")

    def is_ready(self) -> bool:
        return bool(_NATIVE.coolbox_c_is_ready())

    def capability_at(self, index: int) -> str:
        if index < 0 or index >= self.capability_count():
            return ""
        return (_NATIVE.coolbox_c_capability_at(index) or b"").decode("utf-8")

    def capability_count(self) -> int:
        return int(_NATIVE.coolbox_c_capability_count())

    def capabilities(self) -> List[str]:
        return [self.capability_at(index) for index in range(self.capability_count())]


def create_default() -> Client:
    return Client()


def for_endpoint(endpoint: str) -> Client:
    return Client(endpoint=endpoint or DEFAULT_ENDPOINT)