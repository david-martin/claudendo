import hmac
import os
import time
from collections import defaultdict

from fastapi import FastAPI, Request, Response, HTTPException

from relay import imaging, vision, tts

TOKEN = os.environ.get("CLAUDENDO_TOKEN", "")
RATE_LIMIT = 20            # requests per window
RATE_WINDOW = 60           # seconds
MAX_BODY = 1_000_000       # ~1 MB

app = FastAPI()
_hits: dict[str, list[float]] = defaultdict(list)

def _check_auth(request: Request) -> None:
    header = request.headers.get("authorization", "")
    prefix = "Bearer "
    presented = header[len(prefix):] if header.startswith(prefix) else ""
    if not TOKEN or not hmac.compare_digest(presented, TOKEN):
        raise HTTPException(status_code=401, detail="auth failed")

def _check_rate(key: str) -> None:
    now = time.time()
    hits = [t for t in _hits[key] if now - t < RATE_WINDOW]
    if len(hits) >= RATE_LIMIT:
        _hits[key] = hits
        raise HTTPException(status_code=429, detail="rate limited")
    hits.append(now)
    _hits[key] = hits

@app.post("/describe")
async def describe(request: Request):
    _check_auth(request)
    _check_rate("global")
    raw = await request.body()
    if len(raw) > MAX_BODY or not raw:
        raise HTTPException(status_code=400, detail="bad image")
    try:
        w = int(request.headers["x-image-width"])
        h = int(request.headers["x-image-height"])
        fmt = request.headers.get("x-image-format", "rgb565")
        jpeg = imaging.to_jpeg(raw, w, h, fmt)
    except (KeyError, ValueError):
        raise HTTPException(status_code=400, detail="bad image")
    try:
        text = vision.describe(jpeg)
    except Exception:
        raise HTTPException(status_code=502, detail="vision unavailable")
    try:
        pcm, rate = tts.synthesize(text or "I see nothing.")
    except Exception:
        return Response(content=b"", media_type="application/octet-stream",
                        headers={"X-Description": text, "X-Sample-Rate": "0"})
    return Response(content=pcm, media_type="application/octet-stream",
                    headers={"X-Description": text, "X-Sample-Rate": str(rate)})
