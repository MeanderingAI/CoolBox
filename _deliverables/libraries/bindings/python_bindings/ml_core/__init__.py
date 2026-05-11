"""Compatibility package for legacy `import ml_core` consumers.

The canonical install target is `ml_toolbox`, but a large part of the
existing examples and tests still import `ml_core` directly. This package
aliases the compiled extension shipped as `ml_toolbox.ml_core` so both import
styles work after `pip install`.
"""

from importlib import import_module
import sys


_core = import_module("ml_toolbox.ml_core")
sys.modules[__name__] = _core