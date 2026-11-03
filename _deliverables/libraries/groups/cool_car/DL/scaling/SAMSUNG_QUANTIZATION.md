# Samsung sub-1-bit CPU references

This module implements layer-local, deterministic CPU references for:

- **LittleBit**, Lee et al., NeurIPS 2025, arXiv:2506.13771.
- **NanoQuant**, Chong et al., 2026, arXiv:2602.06694.
- **LittleBit-2**, Lee and Kim, ICML 2026, arXiv:2603.00042.

The papers and the official SamsungLabs `LittleBit` and `NanoQuant`
repositories were used as the algorithm authorities.

## Scope and assumptions

- `Tensor` is a double-backed reference type. Scale accounting nevertheless
  uses the papers' deployed FP16 cost (16 bits per scale); binary factors are
  physically packed in row-major, LSB-first 32-bit words.
- LittleBit uses balanced truncated SVD, binary factors, and separate rank-one
  SVD magnitude envelopes. Its optional residual path decomposes the primary
  reconstruction residual.
- LittleBit-2 changes initialization only. Joint-ITQ alternates binary
  assignment and orthogonal Procrustes updates on `[U; V^T]`; its deployed
  representation and bit cost are exactly LittleBit's.
- NanoQuant implements the layer-local importance-weighted ADMM and
  rank-one-magnitude projection. Empty importance vectors mean unit weights.
  Its random initialization is driven by the explicit deterministic config
  seed; the default iteration counts match the official repository.
  The full paper pipeline's calibration-data collection, block-level
  fine-tuning, and LLM evaluation are not part of this CPU primitive.
- Jacobi SVD and pivoted Gaussian elimination are intentionally local
  implementations. This keeps standalone installation dependency-free and is
  suitable for correctness tests and small matrices, not production LLM
  conversion throughput.

No paper-equivalent perplexity or task accuracy is claimed. Those require the
published model/data pipeline and long-running GPU experiments.
