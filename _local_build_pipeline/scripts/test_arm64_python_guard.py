"""Check Python discovery guards without Windows compilers or dependencies."""

from pathlib import Path
import subprocess
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]


class Arm64PythonGuardTests(unittest.TestCase):
    def test_target_and_host_architectures(self):
        dependencies = (REPO_ROOT / "cmake/ExternalDependencies.cmake").read_text()
        dependency_guard = dependencies.split("if(_coolbox_skip_pybind11)", 1)[0]
        bindings = (
            REPO_ROOT
            / "_deliverables/libraries/bindings/python_bindings/CMakeLists.txt"
        ).read_text()
        binding_guard = bindings.split("# Add a CTest entry", 1)[0]

        cases = [
            ("vs-arm64-on-x64", "TRUE", "ARM64", "", "AMD64", "AMD64", "FALSE", True),
            ("generator-arm64", "TRUE", "", "ARM64", "AMD64", "AMD64", "FALSE", True),
            ("processor-arm64", "TRUE", "", "", "aarch64", "AMD64", "FALSE", True),
            ("explicit-cross", "TRUE", "ARM64", "", "ARM64", "ARM64", "TRUE", True),
            ("native-arm64", "TRUE", "ARM64", "", "ARM64", "ARM64", "FALSE", False),
            ("windows-x64", "TRUE", "x64", "", "AMD64", "AMD64", "FALSE", False),
            ("macos-arm64", "FALSE", "", "", "arm64", "arm64", "FALSE", False),
        ]
        for name, windows, vs, generator, target, host, cross, expected in cases:
            with self.subTest(name=name), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                (root / "bindings.cmake").write_text(binding_guard + "\nset(REACHED TRUE)\n")
                script = root / "check.cmake"
                script.write_text(
                    f'set(WIN32 {windows})\n'
                    f'set(CMAKE_VS_PLATFORM_NAME "{vs}")\n'
                    f'set(CMAKE_GENERATOR_PLATFORM "{generator}")\n'
                    f'set(CMAKE_SYSTEM_PROCESSOR "{target}")\n'
                    f'set(CMAKE_HOST_SYSTEM_PROCESSOR "{host}")\n'
                    f'set(CMAKE_CROSSCOMPILING {cross})\n'
                    + dependency_guard
                    + f'\ninclude("{(root / "bindings.cmake").as_posix()}")\n'
                    + 'message(STATUS "SKIP=${_coolbox_skip_pybind11};REACHED=${REACHED}")\n'
                )
                result = subprocess.run(
                    ["cmake", "-P", str(script)],
                    capture_output=True,
                    text=True,
                    check=True,
                )
                self.assertIn(
                    "SKIP=TRUE;REACHED=" if expected else "SKIP=FALSE;REACHED=TRUE",
                    result.stdout,
                )


if __name__ == "__main__":
    unittest.main()
