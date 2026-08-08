// Minimal JS usage example for PDE/SPDE bindings
// Assumes Emscripten build output is pde_spde.js and pde_spde.wasm

// Usage (in browser or Node.js):
// const createPdeSpdeModule = require('./pde_spde.js');
// createPdeSpdeModule().then(Module => {
//   const u0 = [1, 2, 3, 4, 5];
//   const alpha = 0.5, dx = 0.1, dt = 0.01, steps = 10;
//   const result = Module.solve_pde_1d(u0, alpha, dx, dt, steps);
//   console.log(result);
// });

// For 2D: pass a nested array (array of arrays)
// For SPDE: pass alpha, sigma, and an optional RNG seed
