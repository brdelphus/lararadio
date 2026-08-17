# Features Under Review and Development

- [x] Export of logs and playback reports
- [x] Drag and drop support for playlist items
- [ ] Playlist scheduling system (automatic programming by time/date)
- [x] Automatic insertion of jingles at configurable intervals (X a cada Y músicas)
- [ ] Migrar QMediaPlayer → libmpv (wrapper mpv_handle* em AudioPlayer/ButtonHole/timeplayer/preview)
      — habilita filtros de áudio em tempo real (af-command), saída por sink
      (audio-device), pode remover transcodeIfNeeded; retestar fade/crossfade
      e avanço de playlist
- [ ] Built-in graphical equalizer (10 bandas 31Hz–16kHz, sliders em tempo
      real via af-command no mpv — depende da migração acima)
- [x] Automatic saving of settings and playlists
- [x] Streaming integration (Icecast / Shoutcast) — botão liga/desliga, URL/user/pass + name/description, curr playing via /admin/metadata
- [x] Automatic silence or audio failure detection
- [x] Audio preview (pre-listen) — menu de contexto, 15s, sink próprio (audio/preview_device)
- [x] Select output device for broadcast and monitoring separately — sink broadcast (audio/output_device) + loopback monitoring no sink padrão
- [x] Add items to the playlist below a pre-selected entry
