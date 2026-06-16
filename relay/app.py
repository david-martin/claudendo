import hmac
import os
import time
from collections import defaultdict

from fastapi import FastAPI, Request, Response

from relay import imaging, vision, tts

TOKEN = os.environ.get("CLAUDENDO_TOKEN", "")
RATE_LIMIT = 20            # requests per window
RATE_WINDOW = 60           # seconds
MAX_BODY = 1_000_000       # ~1 MB

app = FastAPI()
_hits: dict[str, list[float]] = defaultdict(list)

_PUNCT = {
    "‘": "'", "’": "'", "“": '"', "”": '"',
    "–": "-", "—": "-", "…": "...", " ": " ",
}

def _ascii_header(text: str) -> str:
    for uni, asc in _PUNCT.items():
        text = text.replace(uni, asc)
    # drop anything outside printable ASCII (control chars like \n/\t are valid
    # ASCII but illegal in an HTTP header value), collapse whitespace runs.
    text = text.encode("ascii", "ignore").decode("ascii")
    return " ".join(text.split())

def _err(status: int, msg: str) -> Response:
    return Response(content=msg.encode("ascii"), status_code=status,
                    media_type="text/plain",
                    headers={"X-Description": msg, "X-Sample-Rate": "0"})

def _rate_limited(key: str) -> bool:
    now = time.time()
    hits = [t for t in _hits[key] if now - t < RATE_WINDOW]
    if len(hits) >= RATE_LIMIT:
        _hits[key] = hits
        return True
    hits.append(now)
    _hits[key] = hits
    return False

@app.post("/describe")
async def describe(request: Request):
    header = request.headers.get("authorization", "")
    presented = header[len("Bearer "):] if header.startswith("Bearer ") else ""
    if not TOKEN or not hmac.compare_digest(presented, TOKEN):
        return _err(401, "auth failed")
    if _rate_limited(presented):
        return _err(429, "rate limited")
    raw = await request.body()
    if not raw or len(raw) > MAX_BODY:
        return _err(400, "bad image")
    try:
        w = int(request.headers["x-image-width"])
        h = int(request.headers["x-image-height"])
        fmt = request.headers.get("x-image-format", "rgb565")
        jpeg = imaging.to_jpeg(raw, w, h, fmt)
    except (KeyError, ValueError):
        return _err(400, "bad image")
    try:
        text = vision.describe(jpeg)
    except Exception:
        return _err(502, "vision unavailable")
    # TTS gets the full Unicode text; the HTTP header must be ASCII-safe
    # (HTTP headers are latin-1; smart quotes / em-dashes in Marvin's prose break send).
    desc_hdr = _ascii_header(text)
    try:
        pcm, rate = tts.synthesize(text or "I see nothing.")
    except Exception:
        return Response(content=b"", media_type="application/octet-stream",
                        headers={"X-Description": desc_hdr, "X-Sample-Rate": "0"})
    return Response(content=pcm, media_type="application/octet-stream",
                    headers={"X-Description": desc_hdr, "X-Sample-Rate": str(rate)})
