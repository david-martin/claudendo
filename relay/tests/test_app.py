import os
import pytest
from fastapi.testclient import TestClient

os.environ["CLAUDENDO_TOKEN"] = "test-token"
from relay import app as appmod

@pytest.fixture
def client(monkeypatch):
    monkeypatch.setattr(appmod.vision, "describe", lambda jpeg: "a cat")
    monkeypatch.setattr(appmod.tts, "synthesize", lambda text: (b"\x01\x02" * 600, 22050))
    monkeypatch.setattr(appmod.imaging, "to_jpeg", lambda raw, w, h, fmt: b"jpeg")
    appmod._hits.clear()
    return TestClient(appmod.app)

def _hdrs(tok="test-token"):
    return {"Authorization": f"Bearer {tok}", "X-Image-Width": "2",
            "X-Image-Height": "1", "X-Image-Format": "rgb565"}

def test_happy_path(client):
    r = client.post("/describe", content=b"\x00\x00\x00\x00", headers=_hdrs())
    assert r.status_code == 200
    assert r.headers["X-Description"] == "a cat"
    assert r.headers["X-Sample-Rate"] == "22050"
    assert len(r.content) == 1200

def test_rejects_bad_token(client):
    r = client.post("/describe", content=b"\x00\x00\x00\x00", headers=_hdrs("wrong"))
    assert r.status_code == 401

def test_rate_limited(client):
    for _ in range(appmod.RATE_LIMIT):
        client.post("/describe", content=b"\x00\x00\x00\x00", headers=_hdrs())
    r = client.post("/describe", content=b"\x00\x00\x00\x00", headers=_hdrs())
    assert r.status_code == 429

def test_error_responses_carry_x_description(client):
    r = client.post("/describe", content=b"\x00\x00\x00\x00", headers=_hdrs("wrong"))
    assert r.status_code == 401
    assert r.headers["X-Description"] == "auth failed"
