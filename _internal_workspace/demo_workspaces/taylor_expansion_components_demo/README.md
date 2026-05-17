# Taylor Expansion Components Demo

An interactive demo that shows how Taylor series converge as more components are added.

## What it demonstrates

1. Taylor series partial sums build up one term at a time.
2. The component slider controls how many terms are visible in the approximation.
3. You can compare sin(x), cos(x), and e^x across the same convergence view.
4. The canvas fills the available space and can be exported as PNG or as an animated GIF sweep.

## Controls

- Function buttons switch between sin(x), cos(x), and e^x.
- Components slider changes the number of Taylor terms included in the partial sum.
- X range slider changes the visible domain.
- Display toggles show or hide the exact curve, selected approximation, and earlier partial sums.
- Export PNG saves the current frame.
- Export GIF Sweep captures a short animation while the component count increases.

## Files

- `demo.json` - Demo metadata.
- `index.html` - Full interactive demo implementation.

## Notes

This demo is designed as a focused convergence visualizer, separate from the signal-processing Taylor demo.