from unittest.mock import MagicMock, patch
from relay import tts

def test_synthesize_runs_piper_and_returns_pcm_and_rate(tmp_path, monkeypatch):
    model = tmp_path / "voice.onnx"
    model.write_bytes(b"x")
    (tmp_path / "voice.onnx.json").write_text('{"audio": {"sample_rate": 22050}}')
    monkeypatch.setattr(tts, "PIPER_MODEL", str(model))

    fake = MagicMock()
    fake.stdout = b"\x01\x02" * 1000   # raw PCM bytes piper would emit
    with patch.object(tts.subprocess, "run", return_value=fake) as run:
        pcm, rate = tts.synthesize("hello world")

    assert pcm == b"\x01\x02" * 1000
    assert rate == 22050
    assert len(pcm) % 2 == 0
    # piper invoked with the configured model and raw output
    argv = run.call_args.args[0]
    assert "piper" in argv and str(model) in argv and "--output-raw" in argv

def test_synthesize_falls_back_to_default_rate_when_config_missing(tmp_path, monkeypatch):
    model = tmp_path / "noconfig.onnx"      # no sidecar .json
    monkeypatch.setattr(tts, "PIPER_MODEL", str(model))
    fake = MagicMock(); fake.stdout = b"\x00\x00" * 10
    with patch.object(tts.subprocess, "run", return_value=fake):
        _, rate = tts.synthesize("x")
    assert rate == 22050
