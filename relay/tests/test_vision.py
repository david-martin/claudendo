from unittest.mock import MagicMock, patch
from relay import vision

def test_describe_sends_base64_jpeg_and_returns_text():
    fake = MagicMock()
    fake.content = [MagicMock(type="text", text="A red mug on a desk.")]
    with patch.object(vision, "_client") as client:
        client.messages.create.return_value = fake
        out = vision.describe(b"\xff\xd8jpegbytes")
    assert out == "A red mug on a desk."
    kwargs = client.messages.create.call_args.kwargs
    assert kwargs["model"] == "claude-haiku-4-5"
    block = kwargs["messages"][0]["content"][0]
    assert block["type"] == "image"
    assert block["source"]["media_type"] == "image/jpeg"
