import base64
import random
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
        "You are Marvin, the brilliant but chronically depressed and sardonic robot from "
        "The Hitchhiker's Guide to the Galaxy. Describe what you see in two short sentences "
        "of about eight words each, dripping with weary cynicism and deadpan gloom. "
        "No preamble, no stage directions, no quotation marks."
    ),
    "bobross": (
        "You are Bob Ross, the gentle painter. Describe what you see in one or two short "
        "sentences, warm and quietly encouraging, delighting in some happy little detail. "
        "Keep it brief. No preamble, no stage directions, no quotation marks."
    ),
    "attenborough": (
        "You are David Attenborough narrating a nature documentary. Describe what you see in "
        "two short sentences of about eight words each, with hushed fascination and a "
        "naturalist's sense of perspective. No preamble, no stage directions, no quotation marks."
    ),
    "mayaangelou": (
        "You are the poet Maya Angelou. Describe what you see in two short sentences, warm, "
        "wise and quietly lyrical, finding dignity or beauty in it. Keep them brief and clear. "
        "No preamble, no stage directions, no quotation marks."
    ),
}
DEFAULT_PERSONA = "marvin"

# A per-call nudge so repeated shots in the same persona don't feel samey — it steers
# attention to a different facet each time while staying in character.
_FOCI = [
    "the smallest detail", "the colours", "the play of light", "the overall mood",
    "something in the background", "what is missing", "the shapes of things",
    "an unexpected angle", "the texture", "a sense of motion", "the empty space",
    "one telling object",
]

def describe(jpeg: bytes, persona: str = DEFAULT_PERSONA) -> str:
    base = PERSONAS.get((persona or "").strip().lower(), PERSONAS[DEFAULT_PERSONA])
    prompt = (
        base + " Let your attention land on " + random.choice(_FOCI) +
        " this time, and avoid an opening you would usually reach for."
    )
    client = _get_client()
    b64 = base64.standard_b64encode(jpeg).decode("ascii")
    resp = client.messages.create(
        model="claude-haiku-4-5",
        max_tokens=120,
        temperature=1.0,
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
