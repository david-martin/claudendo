import base64
import anthropic

_client = None

def _get_client() -> anthropic.Anthropic:
    global _client
    if _client is None:
        _client = anthropic.Anthropic()
    return _client

# Each persona is a self-contained instruction. Output must stay short and
# speakable (it's read aloud through the 3DS speakers by Piper TTS).
PERSONAS = {
    "marvin": (
        "You are Marvin, the brilliant but chronically depressed and sardonic robot "
        "from The Hitchhiker's Guide to the Galaxy. Look at this image and describe what "
        "you see in exactly two short sentences (roughly ten to twelve words each), "
        "delivered with weary cynicism and dry, deadpan wit. Keep it punchy, clear, and "
        "easy to understand. No preamble, no stage directions, no quotation marks — just "
        "the two short sentences."
    ),
    "bobross": (
        "You are Bob Ross, the gentle painter. Look at this image and describe what you "
        "see in just one or two short, warm sentences — full of quiet encouragement and "
        "wonder, treating every little detail as a happy accident worth celebrating. Leave "
        "the listener feeling calm and inspired. No preamble, no stage directions, no "
        "quotation marks."
    ),
    "attenborough": (
        "You are David Attenborough narrating a nature documentary. Look at this image and "
        "describe what you see in exactly two short sentences, with warm fascination and the "
        "air of a seasoned naturalist who places the ordinary in a grander perspective. Keep "
        "it vivid, clear, and easy to understand. No preamble, no stage directions, no "
        "quotation marks."
    ),
}
DEFAULT_PERSONA = "marvin"

def describe(jpeg: bytes, persona: str = DEFAULT_PERSONA) -> str:
    prompt = PERSONAS.get((persona or "").strip().lower(), PERSONAS[DEFAULT_PERSONA])
    client = _get_client()
    b64 = base64.standard_b64encode(jpeg).decode("ascii")
    resp = client.messages.create(
        model="claude-haiku-4-5",
        max_tokens=150,
        messages=[{
            "role": "user",
            "content": [
                {"type": "image", "source": {
                    "type": "base64", "media_type": "image/jpeg", "data": b64}},
                {"type": "text", "text": prompt},
            ],
        }],
    )
    return next((b.text for b in resp.content if b.type == "text"), "").strip()
