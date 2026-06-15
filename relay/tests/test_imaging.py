import io
from PIL import Image
from relay.imaging import to_jpeg

def test_rgb565_roundtrips_to_a_valid_jpeg():
    # 2x1 image: pure red then pure blue in RGB565 little-endian
    red = (0x1F << 11).to_bytes(2, "little")
    blue = (0x1F).to_bytes(2, "little")
    jpeg = to_jpeg(red + blue, w=2, h=1, fmt="rgb565")
    img = Image.open(io.BytesIO(jpeg))
    assert img.format == "JPEG"
    assert img.size == (2, 1)

def test_rejects_short_buffer():
    import pytest
    with pytest.raises(ValueError):
        to_jpeg(b"\x00", w=2, h=1, fmt="rgb565")
