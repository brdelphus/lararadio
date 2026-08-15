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
- **Silence / audio failure watchdog**: monitors playback **position
  advancement** via `updateDisplay()` (10ms timer). If the active
  player's `getPosition()` does not advance for 10 seconds (and no
  fade is active), automatically calls `skipToNext()`. Rewritten from
  the VU-meter version: the FFmpeg backend does not feed
  `QAudioBufferOutput`, so VU levels stayed at 0 while audio played,
  causing false-positive skips. Covers device failure, silent decoder
  bugs, and stuck pipes.
- **End of playlist stops instead of looping**: `repeat` now defaults
  to `false`. On the last track with repeat off, `checkAdvanceTrack()`
  stops playback (`isPlaying = false`) instead of wrapping to the
  start; the 5s pre-transition in `currentTimePosition()` also no
  longer wraps when repeat is off.
- **Segfault fix on playlist edit while playing**: `clearPlaylist()`
  emptied the playlist while `isPlaying` stayed true and
  `current_play` stayed stale, making `topLevelItem()` return
  nullptr → SIGSEGV. Now: `clearPlaylist()` resets both players and
  playback state; `updateAudioList()` null-checks `curItem`/`nextItem`
  before styling; `on_btn_remove_item_clicked()` clamps
  `current_play`/`next_play` after erase; `next()` clamps
  `current_play` before indexing.
- **Stop button works with empty playlist**: removing the last
  (currently-playing) track left the playlist empty while audio kept
  playing — `on_btn_stop_clicked()` early-returned on empty playlist
  and never stopped the players. Now it only early-returns when the
  playlist is empty **and** no player is playing. Also guarded
  `playlist[current_play]` access in `updateAudioList()` for the
  empty case (was out-of-bounds UB).

#### `buttonhole.h`
- Added explicit `#include <QMediaPlayer>` and `#include <QAudioOutput>`
  (were previously pulled indirectly via `audioplayer.h` when it
  inherited from `QMediaPlayer`; now it inherits from `QObject`)

### Added

#### `mainwindow.ui` / `mainwindow.cpp`
- **Repeat checkbox** (`chk_repeat`, left of the transport buttons,
  between the mic and play): toggles the `repeat` member that controls
  whether the playlist wraps at the end or stops on the last track.
  Default: off.

#### `buttonhole.h` / `buttonhole.cpp`
- **Loop button**: an 11th ButtonHole labeled "Loop" (left of the
  "Botoeira" label, y=574 line) with a new `setLoopMode()` flag. When
  enabled, the assigned audio restarts from the beginning on
  `EndOfMedia` (`mediaStatusChanged` → `player->play()`), replaying
  indefinitely. Assign audio via right-click → "Carregar Áudio", same
  as the other buttonholes (settings key `buttonhole/btn_Loop`). Click
  toggles play/stop like the others.

#### `mainwindow.h` / `mainwindow.cpp`
- **Drag & drop into the playlist**: `audio_list` accepts external drops
  (event filter installed on the tree). Audio files dropped from a file
  manager are added as `music` items; dropping a **folder** expands it
  into a sequence of `music` items (recursive scan, name-sorted — "one
  after another"). Non-audio files are ignored. Jingle/vinheta still
  comes only from its own tree.
- **`makePlaylistItem()` helper**: extracted from the TagLib item-building
  logic that was copy-pasted 3× (add button + 2 disabled double-click
  handlers). Now the single path for building a `Playlist` entry, reused
  by the add button and the drop. Bonus: `completeBaseName()` replaces
  the old `filename.remove(".mp3")` chain (removes only the final
  extension).
- **Multi-selection removal**: `audio_list` selection mode changed to
  `ExtendedSelection`; the remove button deletes **every selected row**
  (highest row first, keeping `current_play`/`next_play` clamped).
- **ON AIR button** (`onAirButton`, left of the Loop button, x=100/y=574):
  toggles a physical ON AIR light through an external script. Click flips
  the state — yellow `ON` / black `OFF` — and calls the configured script
  with the matching parameter (`onair/script` + `onair/param_on` /
  `onair/param_off`, defaults `on1`/`off1`, e.g. `usbrelay2 on1`). Label
  follows the UI language: "No Ar" (pt_BR) / "On Air" (en_US). Script path
  and parameters are set in ConfigDialog → Caminhos.
- **ConfigDialog scroll**: settings dialog now wraps its content in a
  `QScrollArea` so every field (including the ON AIR ones) stays reachable
  on small screens. The ON AIR script label shows a `~/bin/usbrelay2`
  example — resolved per-user, no hardcoded username.

### Known issues (original, not introduced by us)
- TagLib `AudioProperties::length()` is deprecated in favor of
  `lengthInSeconds()` — 3 warnings during build
- VDPAU backend warning on Radeon GPUs (harmless, video acceleration
  only)
- GTK theme parsing warning with certain `gtk-contained-dark.css`
  versions (harmless)
