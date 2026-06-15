from relay.tts import synthesize

def test_synthesize_returns_pcm_and_rate():
    pcm, rate = synthesize("hello world")
    assert isinstance(pcm, bytes) and len(pcm) > 1000
    assert rate in (22050, 16000, 44100)   # espeak default is 22050
    assert len(pcm) % 2 == 0               # PCM16 → even byte count
