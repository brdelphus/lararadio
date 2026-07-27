# Changelog — LaraRadio

All changes between the original `gutierre69/lararadio` release
and this fork (`brdelphus/lararadio`).

Original source: https://github.com/gutierre69/lararadio
Fork: https://github.com/brdelphus/lararadio

---

## [1.0.5] — 2026-07-27 — Changes on top of original 1.0.4

### Changed

#### `audioplayer.h` / `audioplayer.cpp`
- **Inheritance**: `AudioPlayer` changed from `QMediaPlayer` subclass to
  `QObject` subclass containing `QMediaPlayer *player` as a member
  (composition over inheritance). This decouples the audio backend from
  the public API and allows more flexible lifecycle management.
- **Volume**: default audio output volume changed from `0` to `1.0`
  (QMediaPlayer is now the actual audio source, see below)
- **MP3 transcoding**: added `transcodeIfNeeded()` — converts MP3 files
  with embedded album art to temporary WAV via `ffmpeg -vn` before
  playback. This prevents the `mp3float` decoder crash that occurred
  after multiple songs with album art.
- **Error handling**: connected `QMediaPlayer::errorOccurred` →
  `onPlayerError()` which emits `mediaError()` signal
- **End-of-track detection**: connected `QMediaPlayer::mediaStatusChanged`
  → emits `playbackFinished()` on `EndOfMedia`
- **Destructor**: explicit destructor to clean up `player` and `audioOutput`
- **Inline helpers**: `getPosition()`, `getDuration()`, `remainingTime()`,
  `isPlaying()`, `isPaused()`, `isStopped()`, `getVolume()`, `setVolume()`
  all simplified to single-line inline implementations
- **`isValidMediaFile()`**: static method using `ffprobe` to validate
  audio files before queueing
- **`hasError()`**: public error state flag

#### `main.cpp`
- **Crash handler**: added `crashHandler()` for `SIGSEGV`, `SIGABRT`,
  `SIGFPE` — prints diagnostic message to stderr and exits cleanly
  with code `128 + signal`. Prevents silent crashes from FFmpeg
  decoder bugs.
- **Dark theme**: activated the Fusion dark theme palette (was
  previously commented out). Dark background `#303030`, base `#242424`,
  text `#dcdcdc`, highlight `#55aaff`. Falls back to GTK3 theme when
  available.

#### `mainwindow.h` / `mainwindow.cpp`
- **`skipToNext()`**: public slot — resets both players, advances
  `current_play`, and calls `next()`. Used when a track errors out.
- **`checkAdvanceTrack()`**: public slot — advances the playlist when
  `playbackFinished` fires and both players are stopped. Single point
  of playlist advancement.
- **Error recovery**: `mediaError` signal from both `AudioPlayer` instances
  connected to `skipToNext()` — auto-skips on decoder errors
- **Playlist advance**: `playbackFinished` signal from both players
  connected to `checkAdvanceTrack()`
- **Time audio**: checks `QFile::exists()` before attempting to play
  the time announcement file. If missing, logs a warning and skips —
  prevents `SayingTimer` deadlock.
- **Timeplayer error handler**: connected `QMediaPlayer::errorOccurred`
  on the time player — clears `SayingTimer` and advances on error
- **Removed `flash()` double-advance**: the 500ms `flash()` timer no
  longer advances the playlist. Advancement is exclusively handled by
  `checkAdvanceTrack()` via `playbackFinished`. This prevents race
  conditions on short tracks (jingles) where both mechanisms could
  advance independently.
- **Silence / audio failure watchdog**: monitors the VU meter via
  `updateDisplay()` (10ms timer). If a player is in `PlayingState`
  but no audio reaches the VU meter for 10 seconds (and no fade is
  active), automatically calls `skipToNext()`. Covers device failure,
  silent decoder bugs, and stuck pipes.

#### `buttonhole.h`
- Added explicit `#include <QMediaPlayer>` and `#include <QAudioOutput>`
  (were previously pulled indirectly via `audioplayer.h` when it
  inherited from `QMediaPlayer`; now it inherits from `QObject`)

### Known issues (original, not introduced by us)
- TagLib `AudioProperties::length()` is deprecated in favor of
  `lengthInSeconds()` — 3 warnings during build
- VDPAU backend warning on Radeon GPUs (harmless, video acceleration
  only)
- GTK theme parsing warning with certain `gtk-contained-dark.css`
  versions (harmless)
