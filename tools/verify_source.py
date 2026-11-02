"""Verify preserved delivery files and declared build inputs without compiling."""
import hashlib
import json
from pathlib import Path
import sys


def main():
    root = Path(__file__).resolve().parent.parent
    baseline = json.loads((root / "source-manifest.json").read_text(encoding="utf-8"))
    sources = json.loads((root / "sources.json").read_text(encoding="utf-8"))
    checks = {name: record["sha256"] for name, record in baseline["files"].items()}
    inputs = {}
    for key in ("canonical_encoder_inputs_sha256", "canonical_gui_inputs_sha256"):
        inputs.update(sources[key])
    for name, expected in inputs.items():
        if name in checks and checks[name] != expected:
            raise ValueError("Conflicting inventory: " + name)
        checks[name] = expected
    failures = []
    for name, expected in sorted(checks.items()):
        path = (root / name).resolve()
        path.relative_to(root)
        if not path.is_file():
            failures.append({"file": name, "error": "missing"})
        elif hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            failures.append({"file": name, "error": "sha256 mismatch"})
    print(json.dumps({
        "version": baseline["version"],
        "baseline_entries": len(baseline["files"]),
        "build_inputs": len(inputs),
        "all_passed": not failures,
        "failures": failures,
        "note": "Publication documentation is outside the preserved baseline inventory.",
    }, indent=2))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
