# claudendo

Nintendo 3DS homebrew that photographs a scene and speaks a description of it.
Point the camera, press **A**: the frame is sent over HTTPS to a relay that runs
Claude Haiku vision, and the 3DS reads the reply aloud in a deep, dreary
Marvin-from-Hitchhiker's-Guide voice — two short, weary, cynical sentences.

Live camera on the top screen; on **A** the shot freezes while Marvin speaks
(with a shutter + processing sound), then returns to the live feed.

## Relay

The Python relay lives in `relay/`. FastAPI endpoint that takes the JPEG, calls
Claude Haiku (vision), synthesizes speech with **Piper** neural TTS, and returns
PCM audio + the description text. See `requirements.txt` for dependencies.

## 3DS

The 3DS homebrew app lives in `3ds/`. Requires devkitPro (`3ds-dev`). TLS to the
relay is via bundled mbedTLS with the ISRG Root X1 CA pinned (in `3ds/romfs/`).

- `make` → `claudendo.3dsx` (run via the Homebrew Launcher / `3dslink` netload).
- It can also be packaged as a **HOME-menu CIA** (makerom + bannertool); the build
  recipe and gotchas live in the private `claudendo-hosted` repo under
  `deploy/cia/`.

Reads its relay host + token from `sdmc:/claudendo/config.cfg` (line 1 = host,
line 2 = token).
