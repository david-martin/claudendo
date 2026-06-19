from unittest.mock import MagicMock, patch
from relay import tts

def _install_voice(tmp_path, monkeypatch, name="en_US-lessac-medium.onnx", rate=22050):
    model = tmp_path / name
    model.write_bytes(b"x")
    (tmp_path / (name + ".json")).write_text('{"audio": {"sample_rate": %d}}' % rate)
    monkeypatch.setattr(tts, "VOICES_DIR", str(tmp_path))
    monkeypatch.setattr(tts, "FALLBACK_MODEL", str(model))
    return model

def test_synthesize_runs_piper_with_persona_model_and_pitched_rate(tmp_path, monkeypatch):
    model = _install_voice(tmp_path, monkeypatch, "en_US-lessac-medium.onnx", 22050)
    # marvin -> lessac model, pitch 0.80
    fake = MagicMock(); fake.stdout = b"\x01\x02" * 1000
    with patch.object(tts.subprocess, "run", return_value=fake) as run:
        pcm, rate = tts.synthesize("hello world", "marvin")
    assert pcm == b"\x01\x02" * 1000
    assert rate == int(22050 * 0.80)          # 17640 — deep dreary Marvin
    argv = run.call_args.args[0]
    assert "piper" in argv and str(model) in argv and "--output-raw" in argv

def test_each_persona_picks_its_own_model(tmp_path, monkeypatch):
    # install all four voice models
    for name in ("en_US-lessac-medium.onnx", "en_US-ryan-medium.onnx",
                 "en_GB-alan-medium.onnx", "en_US-hfc_female-medium.onnx"):
        (tmp_path / name).write_bytes(b"x")
        (tmp_path / (name + ".json")).write_text('{"audio": {"sample_rate": 22050}}')
    monkeypatch.setattr(tts, "VOICES_DIR", str(tmp_path))
    fake = MagicMock(); fake.stdout = b"\x00\x00" * 4
    for persona, (fname, pitch) in tts.VOICES.items():
        with patch.object(tts.subprocess, "run", return_value=fake) as run:
            _, rate = tts.synthesize("x", persona)
        argv = run.call_args.args[0]
        assert str(tmp_path / fname) in argv, persona
        assert rate == int(22050 * pitch), persona

def test_unknown_persona_falls_back_to_default(tmp_path, monkeypatch):
    model = _install_voice(tmp_path, monkeypatch)  # only lessac present
    fake = MagicMock(); fake.stdout = b"\x00\x00" * 4
    with patch.object(tts.subprocess, "run", return_value=fake) as run:
        _, rate = tts.synthesize("x", "nonsense")
    argv = run.call_args.args[0]
    assert str(model) in argv                  # default = marvin = lessac
    assert rate == int(22050 * 0.80)

def test_missing_model_uses_fallback(tmp_path, monkeypatch):
    # ryan not installed; bobross should fall back to FALLBACK_MODEL but keep its pitch
    model = _install_voice(tmp_path, monkeypatch, "en_US-lessac-medium.onnx", 22050)
    fake = MagicMock(); fake.stdout = b"\x00\x00" * 4
    with patch.object(tts.subprocess, "run", return_value=fake) as run:
        _, rate = tts.synthesize("x", "bobross")
    argv = run.call_args.args[0]
    assert str(model) in argv                  # fell back to installed lessac
    assert rate == int(22050 * 0.98)           # bobross pitch retained
