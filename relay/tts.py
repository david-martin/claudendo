import io
import subprocess
import wave

def synthesize(text: str) -> tuple[bytes, int]:
    proc = subprocess.run(
        ["espeak-ng", "-v", "en", "--stdout", text],
        capture_output=True, check=True, timeout=10,
    )
    with wave.open(io.BytesIO(proc.stdout), "rb") as w:
        if w.getnchannels() != 1 or w.getsampwidth() != 2:
            raise RuntimeError(
                f"unexpected espeak output: channels={w.getnchannels()} samplewidth={w.getsampwidth()}"
            )
        rate = w.getframerate()
        pcm = w.readframes(w.getnframes())
    return pcm, rate
