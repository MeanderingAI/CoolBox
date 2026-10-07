Adding a quantum computer simulator

## Summary

Added a general-purpose, exact (noiseless) state-vector quantum computer
simulator library plus a GUI/CLI app demonstrating it with classic
algorithms, including a real implementation of Shor's factoring algorithm.

## New library (`_deliverables/libraries/groups/sim_group/PHYSICS/quantum_simulator/`)

- `QuantumState` — dense complex-amplitude state vector (little-endian
  qubit indexing). Single-qubit gates (X, Y, Z, H, S, T, Rx/Ry/Rz, phase),
  controlled and multi-controlled gates (CNOT, CZ, Toffoli via a single
  generalized `apply_multi_controlled_gate` primitive), SWAP, measurement
  (single-qubit collapse and full measure-all), and outcome-probability
  inspection.
- Two general oracle primitives — `apply_permutation()` and
  `apply_diagonal_phase()` — used to simulate classically-described
  reversible oracles (XOR oracles, modular multiplication, marked-item
  phase flips) by applying their exact unitary action directly to the
  state vector, instead of hand-compiling each one into elementary
  reversible gates (adders/carry chains). This is standard practice (the
  same approach Qiskit's own textbook Shor's implementation uses) and
  doesn't shortcut the actual quantum behavior (superposition, entanglement,
  interference) being simulated.
- `QuantumCircuit` — a reusable, named gate-sequence builder with a
  `describe()` method for a plain-text circuit listing.
- A true gate-level Quantum Fourier Transform (`apply_qft`, H + controlled-
  phase cascade + bit-reversal swaps). Caught and fixed a real bug during
  development: the initial implementation processed qubits LSB-first
  instead of MSB-first, which silently produced *a* valid-looking unitary
  but the wrong one — caught by a test comparing against the closed-form
  DFT matrix, not by inspection.
- `quantum_algorithms.h/.cpp` — Bell state, GHZ state, Deutsch-Jozsa,
  Grover's search (with the standard optimal-iteration-count formula), and
  **Shor's algorithm**: quantum order-finding via simulated phase
  estimation (controlled modular-multiplication oracle + inverse QFT),
  continued-fraction period extraction with direct verification of each
  convergent, and the full classical wrapper (perfect-power pre-check,
  random base selection, gcd shortcuts, even/prime rejection, retry loop).
  Verified factoring 15, 21, 33, and 35, including a genuine quantum
  order-finding success for 33 (period r=10 recovered from real simulated
  amplitude interference, not a classical gcd shortcut).

54 new unit tests across `QuantumSimulatorTests` (gates, circuits,
measurement, QFT) and `QuantumAlgorithmsTests` (Bell/GHZ/DJ/Grover/Shor's),
all passing.

## The app (`_deliverables/apps/quantum_simulator/`)

A GUI (canvas-drawn, built on `full_application_window`) with a sidebar of
demo presets — Bell, GHZ, Deutsch-Jozsa (constant/balanced), Grover's
search, and Shor's factoring of 15/21/33/35 — plus a `--cli` headless mode
for scripting. Clicking a demo runs the real algorithm and shows a bar
chart of the resulting outcome probabilities (for demos with a single final
state) and/or a step-by-step narration log (every demo, and the only output
for Shor's, which runs multiple attempts rather than holding one final
state).

Known limitation, intentionally not worked around: `full_application_window`
has no keyboard/text-input support at all (mouse-position polling only), so
parameters like "which N to factor" are chosen from a fixed preset list via
buttons rather than typed in freely. Real arbitrary-N entry would require
adding keyboard event plumbing to `full_application_window` first — flagged
to the user as follow-up work rather than silently working around it.

## Shared library fix ported from the movie_editor branch

This branch predated the X11 fixes made while building `movie_editor`
(`present_canvas()`, `client_size()`, `query_pointer_state()` were
`#if defined(_WIN32)`-only stubs on X11, and `request_redraw()` had a
clear-then-async-redraw "strobe" bug). Re-applied the same fixes here to
`_deliverables/libraries/groups/app_builder/OS_GENERICS/full_application_window/`
so the quantum_simulator GUI actually renders on Linux instead of showing a
blank window. Verified via `full_application_window_tests` (no regressions)
and X11 screenshot capture of the running app.

## Verification

Full project rebuild + full `ctest` run clean: 108/108 passing, 0
regressions. GUI visually verified via X11 screenshots for both a
state-chart demo (Grover's) and a log-only demo (Shor's factor 33).
