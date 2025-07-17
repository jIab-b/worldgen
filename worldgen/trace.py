import json
from pathlib import Path
from typing import List, Dict, Any

class CallTrace:
    """
    Records a sequence of function calls and their arguments, and saves to JSON.
    """
    def __init__(self):
        self.calls: List[Dict[str, Any]] = []

    def record(self, fname: str, kwargs: Dict[str, Any]):
        """
        Records a single function call.
        """
        self.calls.append({"fn": fname, "args": kwargs})

    def save(self, path: str):
        """
        Saves the recorded call trace to a JSON file.
        """
        Path(path).write_text(json.dumps(self.calls, indent=2))

    def load(self, path: str):
        """
        Loads a call trace from a JSON file.
        """
        self.calls = json.loads(Path(path).read_text())

    def __repr__(self):
        return f"CallTrace({len(self.calls)} calls recorded)" 