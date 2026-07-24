"""Demo: run battery simulator script, export CSV, and render a PNG graph."""

from pathlib import Path
import sys

BASE_DIR = Path(__file__).resolve().parent
PROJECT_DIR = BASE_DIR.parent
if str(PROJECT_DIR) not in sys.path:
    sys.path.insert(0, str(PROJECT_DIR))

from ml_toolbox import battery_simulator as bs
from ml_toolbox import graphics as gfx


def main() -> None:
    script_path = BASE_DIR / "battery_simulator_script.txt"
    output_dir = BASE_DIR / "outputs"
    csv_path = output_dir / "battery_sim_history.csv"
    png_path = output_dir / "battery_sim_history.png"

    simulator = bs.PackSimulator.from_cell_config(
        label="demo_pack",
        series=4,
        parallel=2,
        chemistry=bs.Chemistry.LITHIUM_ION,
        initial_soc=0.65,
    )

    history = simulator.run_script_file(str(script_path))
    out_csv = simulator.export_history_csv(history, str(csv_path))
    out_png = gfx.save_snapshots_png(history, str(png_path))

    last = history[-1] if history else None

    print(f"Script: {script_path}")
    print(f"CSV: {out_csv}")
    print(f"PNG: {out_png}")
    if last is not None:
        print(
            "Last snapshot:",
            f"action={last.action}",
            f"elapsed_s={last.elapsed_s:.1f}",
            f"avg_soc={last.average_soc:.4f}",
            f"terminal_v={last.terminal_voltage_v:.4f}",
        )


if __name__ == "__main__":
    main()
