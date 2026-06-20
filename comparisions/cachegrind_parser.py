#!/usr/bin/env python3
"""Extract function-scoped data-cache counters from Cachegrind output."""

from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


@dataclass(frozen=True)
class CachegrindCounters:
    """Cachegrind data-reference counters selected by function name."""

    dr: int = 0
    d1mr: int = 0
    dlmr: int = 0
    dw: int = 0
    d1mw: int = 0
    dlmw: int = 0

    @property
    def accesses(self) -> int:
        """Return all data references in the selected functions."""
        return self.dr + self.dw

    @property
    def l1_misses(self) -> int:
        """Return combined D1 read and write misses."""
        return self.d1mr + self.d1mw

    @property
    def ll_misses(self) -> int:
        """Return combined last-level read and write misses."""
        return self.dlmr + self.dlmw


def parse_functions(path, function_names: Iterable[str]) -> CachegrindCounters:
    """Sum data-cache counters belonging to the requested functions.

    # Raises

    `ValueError` if required events or requested functions are absent.
    """
    requested = set(function_names)
    events = []
    totals = {name: 0 for name in ("Dr", "D1mr", "DLmr", "Dw", "D1mw", "DLmw")}
    current = None
    found = set()

    for raw_line in Path(path).read_text(errors="replace").splitlines():
        line = raw_line.strip()
        if line.startswith("events:"):
            events = line.split()[1:]
            missing = set(totals) - set(events)
            if missing:
                raise ValueError(f"missing Cachegrind events: {sorted(missing)}")
        elif line.startswith("fn="):
            current = line[3:]
            if current in requested:
                found.add(current)
        elif current in requested and line and line[0].isdigit():
            fields = line.split()
            if len(fields) != len(events) + 1:
                continue
            values = dict(zip(events, map(int, fields[1:])))
            for event in totals:
                totals[event] += values[event]

    missing_functions = requested - found
    if missing_functions:
        raise ValueError(f"functions not found: {sorted(missing_functions)}")
    return CachegrindCounters(*(totals[name] for name in totals))
