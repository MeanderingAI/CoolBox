"""Convenience wrappers for the ml_core graphics submodule."""

from __future__ import annotations

from pathlib import Path
from typing import Iterable

from . import ml_core as _ml_core
from .battery_simulator import SimulationSnapshot

graphics = _ml_core.graphics

Color = graphics.Color
Colors = graphics
Graph = graphics.Graph
GraphType = graphics.GraphType
DataSeries = graphics.DataSeries
Canvas = graphics.Canvas
Table = graphics.Table


def snapshots_to_graph(
    snapshots: Iterable[SimulationSnapshot],
    width: int = 1200,
    height: int = 700,
) -> Canvas:
    history = list(snapshots)
    graph = Graph(width, height, GraphType.LINE)
    graph.set_title("Battery Simulation Metrics")
    graph.set_x_label("Elapsed Time (s)")
    graph.set_y_label("Value")

    if history:
        x = [float(s.elapsed_s) for s in history]
        soc_pct = [float(s.average_soc) * 100.0 for s in history]
        terminal_v = [float(s.terminal_voltage_v) for s in history]

        graph.add_series(DataSeries("Average SoC (%)", x, soc_pct, graphics.BLUE))
        graph.add_series(DataSeries("Terminal Voltage (V)", x, terminal_v, graphics.ORANGE))

    return graph.render()


def snapshots_to_metric_graph(
    snapshots: Iterable[SimulationSnapshot],
    metric: str,
    y_label: str,
    title: str,
    color: Color,
    width: int = 1200,
    height: int = 700,
) -> Canvas:
    history = list(snapshots)
    graph = Graph(width, height, GraphType.LINE)
    graph.set_title(title)
    graph.set_x_label("Elapsed Time (s)")
    graph.set_y_label(y_label)

    if history:
        x = [float(s.elapsed_s) for s in history]
        y = [float(getattr(s, metric)) for s in history]
        graph.add_series(DataSeries(y_label, x, y, color))

    return graph.render()


def save_snapshots_png(
    snapshots: Iterable[SimulationSnapshot],
    output_path: str,
    width: int = 1200,
    height: int = 700,
) -> str:
    canvas = snapshots_to_graph(snapshots, width=width, height=height)
    target = Path(output_path)
    target.parent.mkdir(parents=True, exist_ok=True)
    canvas.save_png(str(target))
    return str(target)


def save_soc_png(
    snapshots: Iterable[SimulationSnapshot],
    output_path: str,
    width: int = 1200,
    height: int = 700,
) -> str:
    canvas = snapshots_to_metric_graph(
        snapshots=snapshots,
        metric="average_soc",
        y_label="Average SoC",
        title="Battery Simulation - Average SoC",
        color=graphics.BLUE,
        width=width,
        height=height,
    )
    target = Path(output_path)
    target.parent.mkdir(parents=True, exist_ok=True)
    canvas.save_png(str(target))
    return str(target)


def save_terminal_voltage_png(
    snapshots: Iterable[SimulationSnapshot],
    output_path: str,
    width: int = 1200,
    height: int = 700,
) -> str:
    canvas = snapshots_to_metric_graph(
        snapshots=snapshots,
        metric="terminal_voltage_v",
        y_label="Terminal Voltage (V)",
        title="Battery Simulation - Terminal Voltage",
        color=graphics.ORANGE,
        width=width,
        height=height,
    )
    target = Path(output_path)
    target.parent.mkdir(parents=True, exist_ok=True)
    canvas.save_png(str(target))
    return str(target)


__all__ = [
    "Canvas",
    "Color",
    "Colors",
    "DataSeries",
    "Graph",
    "GraphType",
    "Table",
    "graphics",
    "save_soc_png",
    "save_snapshots_png",
    "save_terminal_voltage_png",
    "snapshots_to_metric_graph",
    "snapshots_to_graph",
]
