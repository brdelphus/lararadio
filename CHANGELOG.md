# Changelog — LaraRadio Modifications

All source code changes made to fix crashes,
add dark theme, and improve stability.

---

## [1.0.5] — 2026-07-27

### Fixed

#### `audioplayer.h` / `audioplayer.cpp`
- **Removed ffplay hybrid system** that caused double audio playback:
  - QMediaPlayer (muted) + ffplay (child process) played the same
    audio simultaneously
  - ffplay removed: `QProcess`, `watchdog`, `onFfplayFinished`,
    `checkFfplay`, `killFfplay`, `startFfplay`
- **QMediaPlayer is now the sole audio source**, with volume controlled
  via `QAudioOutput` — fade/crossfade works correctly
- `transcodeIfNeeded()` kept: converts MP3 with album art to WAV,
  preventing the `mp3float` decoder crash
- `playbackFinished` now emitted via `QMediaPlayer::EndOfMedia`
- `isPlaying()` / `isStopped()` use `QMediaPlayer::playbackState()`

#### `mainwindow.cpp`
- **Time announcement**: checks if audio file exists before playing.
  If missing, logs a warning and skips the item without locking the playlist
- `timeplayer`: connected `errorOccurred` as safety net —
  clears `SayingTimer` and advances on error
- **Removed redundant playlist advance from `flash()`**: `flash()` (500ms
  timer) and `checkAdvanceTrack()` (from `playbackFinished`) both independently
  advanced the playlist with the same `isStopped()` condition. On short tracks
  (jingles), both could fire in sequence, causing double-advance and repeated
  playback. The advancement block was removed from `flash()` — it now only
  handles UI (talk blink, fade indicator, play/stop indicator). Single
  advancement path: `playbackFinished` → `checkAdvanceTrack()` → `next()`.

---

## [1.0.4] — 2026-07-25

### Added

#### `audioplayer.h`
- `hasError()` — public error state flag
- `mediaError(file, errorMsg)` — signal emitted when QMediaPlayer reports an error
- `onPlayerError(error, errorString)` — private slot to capture player errors
- `playbackFinished()` — signal emitted when ffplay finishes
- `isValidMediaFile(path)` — static method validating file with `ffprobe`
- `audioBufferOutput` — stored pointer for reconnection after Reset()
- `ffplayProc` — QProcess for audio playback in child process
- `ffplayWatchdog` — timer detecting unexpected ffplay death
- `cleanFilePath` — path to the transcoded file
- `m_ffplayPlaying` — ffplay state flag
- `transcodeIfNeeded()` — private method to transcode MP3 with album art
- `startFfplay()` / `killFfplay()` — child process management
- `onFfplayFinished()` — handles ffplay termination
- `checkFfplay()` — watchdog detecting silent ffplay death

#### `audioplayer.cpp`
- Added `<QProcess>`, `<QFileInfo>`, `<QDir>`
- Connected `QMediaPlayer::errorOccurred` → `onPlayerError()`
- Connected `QMediaPlayer::mediaStatusChanged` → detect `InvalidMedia`
- `addMedia()` — transcodes MP3s to temporary WAV via `ffmpeg -vn`,
  stripping embedded album art that crashes the `mp3float` decoder
- Hybrid system implementation:
  - QMediaPlayer runs **muted** (volume 0), serving only VU meter, seek, position
  - `startFfplay()` launches `ffplay -nodisp -autoexit` as a **child process**
  - Each track = new ffplay process, dies cleanly at end with no accumulated state
- `playbackFinished` emitted when ffplay finishes (via `finished` signal + watchdog)
- `Reset()` — now also kills ffplay and clears QMediaPlayer source
- Destructor — cleans up ffplay and QMediaPlayer
- `isValidMediaFile()` — validates with ffprobe before playing
- `onPlayerError()` — logs and emits `mediaError` signal

#### `buttonhole.h`
- Added `<QMediaPlayer>` and `<QAudioOutput>` (were being pulled
  indirectly via `audioplayer.h`, which changed from `QMediaPlayer` to `QObject`)

#### `main.cpp`
- Added `<QMessageBox>`, `<csignal>`, `<cstdlib>`
- `crashHandler(sig)` — signal handler for SIGSEGV/SIGABRT/SIGFPE:
  - Prevents loops with static flag
  - Prints descriptive message to stderr
  - Exits with code `128 + sig`
- Handler installation at the start of `main()`
- **Fusion dark theme** (code existed commented out, now enabled):
  - Dark palette `#303030` background, `#242424` base, `#dcdcdc` text
  - Blue highlight `#55aaff`
  - Dark blue tooltip with white border
  - GTK3 detected automatically if available

#### `mainwindow.h`
- `skipToNext()` — skips to next track when current one fails
- `checkAdvanceTrack()` — advances playlist when ffplay finishes

#### `mainwindow.cpp`
- `audioBufferOutput` restored (VU meter active)
- Connected `mediaError` → calls `skipToNext()` automatically
- Connected `playbackFinished` → calls `checkAdvanceTrack()`
- `skipToNext()` — resets both players, advances index, calls `next()`
- `checkAdvanceTrack()` — advances if both players stopped and no timer active

### Removed

- Redundant `isValidMediaFile()` validation block in `next()` (validation
  is now done inside `addMedia()`)
- Commented-out dark theme code (now active)

### Architecture

**Before (version 1.0.3):**
```
QMediaPlayer (FFmpeg) → plays audio + UI (VU/seek)
         ↓
   mp3float decoder accumulates state → CRASH after N tracks
```

**After (version 1.0.4, ffplay hybrid system):**
```
QMediaPlayer (FFmpeg, muted) → UI only (VU/seek/position)
ffplay (child process)       → actual audio, dies per track

MP3 with album art → ffmpeg -vn → temporary WAV → QMediaPlayer + ffplay
```

**Now (version 1.0.5, simplified system):**
```
MP3 with album art → ffmpeg -vn → temporary WAV → QMediaPlayer (audio + UI)

transcodeIfNeeded() prevents the mp3float crash.
Fade/crossfade between two players with independent QAudioOutput.
No ffplay — no double audio.
```
