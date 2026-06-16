from unittest.mock import MagicMock, patch
from relay import tts

def test_synthesize_runs_piper_and_returns_pitched_rate(tmp_path, monkeypatch):
    model = tmp_path / "voice.onnx"
    model.write_bytes(b"x")
    (tmp_path / "voice.onnx.json").write_text('{"audio": {"sample_rate": 22050}}')
    monkeypatch.setattr(tts, "PIPER_MODEL", str(model))
    monkeypatch.setattr(tts, "PITCH_FACTOR", 1.0)   # neutral for this assertion

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

def test_pitch_factor_lowers_reported_rate(tmp_path, monkeypatch):
    model = tmp_path / "voice.onnx"
    model.write_bytes(b"x")
    (tmp_path / "voice.onnx.json").write_text('{"audio": {"sample_rate": 22050}}')
    monkeypatch.setattr(tts, "PIPER_MODEL", str(model))
    monkeypatch.setattr(tts, "PITCH_FACTOR", 0.82)
    fake = MagicMock(); fake.stdout = b"\x00\x00" * 10
    with patch.object(tts.subprocess, "run", return_value=fake):
        _, rate = tts.synthesize("x")
    assert rate == int(22050 * 0.82)   # 18081 — deeper/slower Marvin drone

def test_synthesize_falls_back_to_default_rate_when_config_missing(tmp_path, monkeypatch):
    model = tmp_path / "noconfig.onnx"      # no sidecar .json
    monkeypatch.setattr(tts, "PIPER_MODEL", str(model))
    monkeypatch.setattr(tts, "PITCH_FACTOR", 1.0)
    fake = MagicMock(); fake.stdout = b"\x00\x00" * 10
    with patch.object(tts.subprocess, "run", return_value=fake):
        _, rate = tts.synthesize("x")
    assert rate == 22050
