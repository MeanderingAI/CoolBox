// math_solvers_demo.js
// Demo: Compare performance of mytrix vs xarray backends using WASM solvers and render charts

import { Graph, GraphType, Colors } from '/extensions/graphics-chart.mjs';

// Placeholder: Load WASM module (assume pde_spde.js is available and exposes createPdeSpdeModule)
// In a real setup, you would import or load the Emscripten-generated JS/WASM here
// For demo, we mock the timing and results

async function runDemo() {
  // Simulate loading WASM module
  // const Module = await createPdeSpdeModule();

  // Simulate input data
  const N = 128;
  const steps = 20;
  const u0 = Array.from({length: N}, (_, i) => Math.sin(Math.PI * i / (N-1)));

  // Simulate timing for mytrix backend
  const t0_mytrix = performance.now();
  // const result_mytrix = Module.solve_pde_1d_mytrix(u0, 0.1, 0.01, steps);
  await new Promise(r => setTimeout(r, 80)); // mock
  const t1_mytrix = performance.now();
  const mytrix_time = t1_mytrix - t0_mytrix;

  // Simulate timing for xarray backend
  const t0_xarray = performance.now();
  // const result_xarray = Module.solve_pde_1d_xarray(u0, 0.1, 0.01, steps);
  await new Promise(r => setTimeout(r, 120)); // mock
  const t1_xarray = performance.now();
  const xarray_time = t1_xarray - t0_xarray;

  // Render chart
  const chart = new Graph(600, 350, GraphType.Bar);
  chart.set_title('Backend Performance (lower is better)');
  chart.add_series({
    label: 'Backend',
    x_values: ['mytrix', 'xarray'],
    y_values: [mytrix_time, xarray_time],
    color: Colors.Blue
  });
  chart.render_to_canvas(document.getElementById('perf-canvas'));

  // Show results
  document.getElementById('mytrix-time').textContent = mytrix_time.toFixed(1) + ' ms';
  document.getElementById('xarray-time').textContent = xarray_time.toFixed(1) + ' ms';
}

window.addEventListener('DOMContentLoaded', runDemo);
