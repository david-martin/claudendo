import json
import os
import subprocess
import sys

# Piper neural TTS (local, CPU). Outputs raw s16le mono PCM at the model's native rate.
PIPER_MODEL = os.environ.get(
    "PIPER_MODEL",
    "/home/claudendo/claudendo/voices/en_US-lessac-medium.onnx",
)

# Marvin effect: report a lower playback rate so the 3DS plays Piper's audio
# deeper and slower (more morose). 1.0 = unchanged; lower = deeper/drearier.
PITCH_FACTOR = float(os.environ.get("PIPER_PITCH_FACTOR", "0.82"))

def _rate() -> int:
    try:
        with open(PIPER_MODEL + ".json") as f:
            return int(json.load(f)["audio"]["sample_rate"])
    except Exception:
        return 22050

def synthesize(text: str) -> tuple[bytes, int]:
    proc = subprocess.run(
        [sys.executable, "-m", "piper", "--model", PIPER_MODEL, "--output-raw"],
        input=text.encode("utf-8"), capture_output=True, check=True, timeout=30,
    )
    rate = max(8000, int(_rate() * PITCH_FACTOR))
    return proc.stdout, rate
