import io
import subprocess
import wave

def synthesize(text: str) -> tuple[bytes, int]:
    proc = subprocess.run(
        ["espeak-ng", "-v", "en", "--stdout", text],
        capture_output=True, check=True,
    )
    with wave.open(io.BytesIO(proc.stdout), "rb") as w:
        assert w.getnchannels() == 1 and w.getsampwidth() == 2
        rate = w.getframerate()
        pcm = w.readframes(w.getnframes())
    return pcm, rate
