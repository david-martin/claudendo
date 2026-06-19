import json
import os
import subprocess
import sys

# Piper neural TTS (local, CPU). Outputs raw s16le mono PCM at the model's native rate.
VOICES_DIR = os.environ.get("PIPER_VOICES_DIR", "/home/claudendo/claudendo/voices")

# Per-persona voice: (model filename in VOICES_DIR, pitch factor).
# pitch < 1.0 reports a lower sample rate so the 3DS plays the clip deeper + slower.
# Distinct Piper models give each persona its own timbre/accent.
VOICES = {
    "marvin":       ("en_US-lessac-medium.onnx",     0.80),  # deep, slow, dreary
    "bobross":      ("en_US-ryan-medium.onnx",       0.98),  # warm, gentle
    "attenborough": ("en_GB-alan-medium.onnx",       0.93),  # measured British naturalist
    "mayaangelou":  ("en_US-hfc_female-medium.onnx", 0.88),  # rich, slow, resonant
}
DEFAULT_PERSONA = "marvin"

# Fallback if a persona's model isn't installed on the box.
FALLBACK_MODEL = os.environ.get(
    "PIPER_MODEL", os.path.join(VOICES_DIR, "en_US-lessac-medium.onnx")
)

def _voice(persona: str) -> tuple[str, float]:
    name, pitch = VOICES.get((persona or "").strip().lower(), VOICES[DEFAULT_PERSONA])
    path = os.path.join(VOICES_DIR, name)
    if not os.path.exists(path):
        return FALLBACK_MODEL, pitch
    return path, pitch

def _rate(model: str) -> int:
    try:
        with open(model + ".json") as f:
            return int(json.load(f)["audio"]["sample_rate"])
    except Exception:
        return 22050

def synthesize(text: str, persona: str = DEFAULT_PERSONA) -> tuple[bytes, int]:
    model, pitch = _voice(persona)
    proc = subprocess.run(
        [sys.executable, "-m", "piper", "--model", model, "--output-raw"],
        input=text.encode("utf-8"), capture_output=True, check=True, timeout=30,
    )
    rate = max(8000, int(_rate(model) * pitch))
    return proc.stdout, rate
