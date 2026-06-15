import io
import numpy as np
from PIL import Image

MAX_DIM = 4096

def to_jpeg(raw: bytes, w: int, h: int, fmt: str) -> bytes:
    fmt = fmt.lower()
    if not (0 < w <= MAX_DIM and 0 < h <= MAX_DIM):
        raise ValueError(f"dimensions out of range: {w}x{h}")
    if fmt == "rgb565":
        if len(raw) != w * h * 2:
            raise ValueError(f"rgb565 expects {w*h*2} bytes, got {len(raw)}")
        px = np.frombuffer(raw, dtype="<u2").reshape(h, w)
        r = ((px >> 11) & 0x1F) << 3
        g = ((px >> 5) & 0x3F) << 2
        b = (px & 0x1F) << 3
        rgb = np.dstack([r, g, b]).astype(np.uint8)
    elif fmt == "rgb888":
        if len(raw) != w * h * 3:
            raise ValueError(f"rgb888 expects {w*h*3} bytes, got {len(raw)}")
        rgb = np.frombuffer(raw, dtype=np.uint8).reshape(h, w, 3)
    else:
        raise ValueError(f"unsupported format {fmt!r}")
    out = io.BytesIO()
    Image.fromarray(rgb, "RGB").save(out, format="JPEG", quality=85)
    return out.getvalue()
