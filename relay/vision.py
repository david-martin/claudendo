import base64
import anthropic

_client = anthropic.Anthropic()  # reads ANTHROPIC_API_KEY from env

PROMPT = ("Describe what you see in one short spoken sentence, "
          "about 15 words or fewer. Plain language, no preamble.")

def describe(jpeg: bytes) -> str:
    b64 = base64.standard_b64encode(jpeg).decode("ascii")
    resp = _client.messages.create(
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
