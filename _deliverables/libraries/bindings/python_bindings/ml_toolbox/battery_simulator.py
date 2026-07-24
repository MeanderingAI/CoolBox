"""High-level battery simulation helpers built on top of ``battery_lib``."""

from __future__ import annotations

import csv
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional

from .battery_lib import BatteryPack, Cell, Chemistry, ChemistryDefaults, chemistry_name


def _clamp(value: float, lo: float, hi: float) -> float:
    return max(lo, min(hi, value))


@dataclass
class SimulationSnapshot:
    action: str
    elapsed_s: float
    current_a: float
    pack_voltage_v: float
    terminal_voltage_v: float
    average_soc: float
    min_soc: float
    delivered_wh: float
    absorbed_wh: float
    throughput_ah: float


def _coerce_scalar(value: str):
    text = value.strip()
    lowered = text.lower()
    if lowered in {"true", "false"}:
        return lowered == "true"
    try:
        number = float(text)
    except ValueError:
        return text
    if number.is_integer():
        return int(number)
    return number


def parse_script_text(text: str) -> List[Dict]:
    """Parse a simple script format into run_script-compatible commands.

    Format examples:
    - discharge duration_s=120 current_a=8 step_s=5
    - optimization duration_s=240 step_s=5 current_limit_a=12 target_soc=0.9
    - rest duration_s=30 step_s=5
    """

    commands: List[Dict] = []
    for line_no, raw_line in enumerate(text.splitlines(), start=1):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue

        tokens = line.split()
        if not tokens:
            continue

        command = tokens[0]
        item: Dict = {"command": command}
        for token in tokens[1:]:
            if "=" not in token:
                raise ValueError(
                    f"Line {line_no}: token '{token}' must be in key=value format"
                )
            key, value = token.split("=", 1)
            key = key.strip()
            if not key:
                raise ValueError(f"Line {line_no}: empty argument name")
            item[key] = _coerce_scalar(value)

        commands.append(item)

    return commands


def parse_script_file(script_path: str) -> List[Dict]:
    return parse_script_text(Path(script_path).read_text(encoding="utf-8"))


def export_snapshots_csv(snapshots: Iterable[SimulationSnapshot], csv_path: str) -> str:
    rows = [
        {
            "action": s.action,
            "elapsed_s": s.elapsed_s,
            "current_a": s.current_a,
            "pack_voltage_v": s.pack_voltage_v,
            "terminal_voltage_v": s.terminal_voltage_v,
            "average_soc": s.average_soc,
            "min_soc": s.min_soc,
            "delivered_wh": s.delivered_wh,
            "absorbed_wh": s.absorbed_wh,
            "throughput_ah": s.throughput_ah,
        }
        for s in snapshots
    ]

    target = Path(csv_path)
    target.parent.mkdir(parents=True, exist_ok=True)
    with target.open("w", newline="", encoding="utf-8") as handle:
        fieldnames = [
            "action",
            "elapsed_s",
            "current_a",
            "pack_voltage_v",
            "terminal_voltage_v",
            "average_soc",
            "min_soc",
            "delivered_wh",
            "absorbed_wh",
            "throughput_ah",
        ]
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    return str(target)


class PackSimulator:
    """Script-friendly simulator facade for ``BatteryPack``.

    It mirrors the CLI concepts: ``discharge``, ``charge``, ``rest``, and an
    optimization-flavored charge loop with tapering behavior.
    """

    def __init__(self, pack: BatteryPack, ambient_c: float = 25.0):
        self.pack = pack
        self.ambient_c = ambient_c
        self.elapsed_s = 0.0
        self.delivered_wh = 0.0
        self.absorbed_wh = 0.0
        self.throughput_ah = 0.0

    @classmethod
    def from_cell_config(
        cls,
        label: str,
        series: int,
        parallel: int,
        chemistry: Chemistry = Chemistry.LITHIUM_ION,
        capacity_ah: Optional[float] = None,
        internal_resistance_ohm: Optional[float] = None,
        initial_soc: float = 1.0,
        ambient_c: float = 25.0,
    ) -> "PackSimulator":
        defaults = ChemistryDefaults.for_chemistry(chemistry)
        capacity = defaults.typical_capacity if capacity_ah is None else capacity_ah
        resistance = (
            defaults.typical_internal_resistance
            if internal_resistance_ohm is None
            else internal_resistance_ohm
        )
        prototype = Cell(label + "_prototype", chemistry, capacity, resistance, initial_soc)
        pack = BatteryPack(label, series, parallel, prototype)
        return cls(pack=pack, ambient_c=ambient_c)

    def snapshot(self, action: str, current_a: float) -> SimulationSnapshot:
        return SimulationSnapshot(
            action=action,
            elapsed_s=self.elapsed_s,
            current_a=current_a,
            pack_voltage_v=self.pack.pack_voltage(),
            terminal_voltage_v=self.pack.pack_terminal_voltage(current_a),
            average_soc=self.pack.average_soc(),
            min_soc=self.pack.min_soc(),
            delivered_wh=self.delivered_wh,
            absorbed_wh=self.absorbed_wh,
            throughput_ah=self.throughput_ah,
        )

    def _advance(self, duration_s: float, step_s: float, fn) -> List[SimulationSnapshot]:
        if duration_s < 0.0:
            raise ValueError("duration_s must be non-negative")
        if step_s <= 0.0:
            raise ValueError("step_s must be positive")

        history: List[SimulationSnapshot] = []
        remaining = duration_s
        while remaining > 1e-9:
            dt = min(step_s, remaining)
            history.append(fn(dt))
            remaining -= dt
        return history

    def discharge(self, duration_s: float, current_a: float, step_s: float = 1.0) -> List[SimulationSnapshot]:
        if current_a < 0.0:
            raise ValueError("current_a must be non-negative for discharge")

        def run_step(dt: float) -> SimulationSnapshot:
            delivered = self.pack.discharge(current_a, dt)
            self.delivered_wh += delivered
            self.throughput_ah += current_a * dt / 3600.0
            self.elapsed_s += dt
            return self.snapshot("discharge", current_a)

        return self._advance(duration_s, step_s, run_step)

    def charge(self, duration_s: float, current_a: float, step_s: float = 1.0) -> List[SimulationSnapshot]:
        if current_a < 0.0:
            raise ValueError("current_a must be non-negative for charge")

        def run_step(dt: float) -> SimulationSnapshot:
            absorbed = self.pack.charge(current_a, dt)
            self.absorbed_wh += absorbed
            self.throughput_ah += current_a * dt / 3600.0
            self.elapsed_s += dt
            return self.snapshot("charge", -current_a)

        return self._advance(duration_s, step_s, run_step)

    def rest(self, duration_s: float, step_s: float = 1.0) -> List[SimulationSnapshot]:
        def run_step(dt: float) -> SimulationSnapshot:
            self.elapsed_s += dt
            return self.snapshot("rest", 0.0)

        return self._advance(duration_s, step_s, run_step)

    def _default_voltage_ceiling(self) -> float:
        try:
            first_cell = self.pack.cell(0, 0)
            return first_cell.max_voltage() * self.pack.series_count()
        except Exception:
            return self.pack.pack_voltage() * 1.05

    def _tapered_charge_current(
        self,
        current_limit_a: float,
        voltage_ceiling_v: float,
        min_current_a: float,
    ) -> float:
        current = max(0.0, current_limit_a)
        for _ in range(12):
            if current <= min_current_a:
                break
            terminal_v = self.pack.pack_terminal_voltage(-current)
            if terminal_v <= voltage_ceiling_v:
                break
            current *= 0.5
        return _clamp(current, min_current_a, current_limit_a)

    def optimization_charge(
        self,
        duration_s: float,
        step_s: float = 1.0,
        current_limit_a: Optional[float] = None,
        voltage_ceiling_v: Optional[float] = None,
        target_soc: float = 0.95,
        min_current_a: float = 0.05,
    ) -> List[SimulationSnapshot]:
        current_limit = self.pack.pack_capacity_ah() if current_limit_a is None else current_limit_a
        voltage_ceiling = self._default_voltage_ceiling() if voltage_ceiling_v is None else voltage_ceiling_v
        target_soc = _clamp(target_soc, 0.0, 1.0)

        if current_limit < 0.0:
            raise ValueError("current_limit_a must be non-negative")

        history: List[SimulationSnapshot] = []
        remaining = duration_s
        while remaining > 1e-9:
            if self.pack.average_soc() >= target_soc:
                break
            dt = min(step_s, remaining)
            optimized_current = self._tapered_charge_current(
                current_limit_a=current_limit,
                voltage_ceiling_v=voltage_ceiling,
                min_current_a=min_current_a,
            )
            absorbed = self.pack.charge(optimized_current, dt)
            self.absorbed_wh += absorbed
            self.throughput_ah += optimized_current * dt / 3600.0
            self.elapsed_s += dt
            history.append(self.snapshot("optimization", -optimized_current))
            remaining -= dt
        return history

    def run_command(self, command: str, **kwargs) -> List[SimulationSnapshot]:
        normalized = command.strip().lower().replace("-", "_")
        if normalized in {"load", "discharge"}:
            return self.discharge(**kwargs)
        if normalized == "charge":
            return self.charge(**kwargs)
        if normalized == "rest":
            return self.rest(**kwargs)
        if normalized in {"optimization", "optimization_charge"}:
            return self.optimization_charge(**kwargs)
        raise ValueError(f"Unknown command: {command}")

    def run_script(self, commands: Iterable[Dict]) -> List[SimulationSnapshot]:
        history: List[SimulationSnapshot] = []
        for item in commands:
            item = dict(item)
            command = str(item.pop("command"))
            history.extend(self.run_command(command, **item))
        return history

    def run_script_file(self, script_path: str) -> List[SimulationSnapshot]:
        return self.run_script(parse_script_file(script_path))

    def export_history_csv(self, history: Iterable[SimulationSnapshot], csv_path: str) -> str:
        return export_snapshots_csv(history, csv_path)

    def run_script_file_to_csv(self, script_path: str, csv_path: str) -> str:
        history = self.run_script_file(script_path)
        return self.export_history_csv(history, csv_path)


__all__ = [
    "BatteryPack",
    "Cell",
    "Chemistry",
    "ChemistryDefaults",
    "PackSimulator",
    "SimulationSnapshot",
    "export_snapshots_csv",
    "parse_script_file",
    "parse_script_text",
    "chemistry_name",
]
