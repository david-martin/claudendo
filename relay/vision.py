import base64
import anthropic

_client = None

def _get_client() -> anthropic.Anthropic:
    global _client
    if _client is None:
        _client = anthropic.Anthropic()
    return _client

PROMPT = ("Describe what you see in one short spoken sentence, "
          "about 15 words or fewer. Plain language, no preamble.")

def describe(jpeg: bytes) -> str:
    client = _get_client()
    b64 = base64.standard_b64encode(jpeg).decode("ascii")
    resp = client.messages.create(
        model="claude-haiku-4-5",
        max_tokens=100,
        messages=[{
            "role": "user",
            "content": [
                {"type": "image", "source": {
                    "type": "base64", "media_type": "image/jpeg", "data": b64}},
                {"type": "text", "text": PROMPT},
            ],
        }],
    )
    return next((b.text for b in resp.content if b.type == "text"), "").strip()
