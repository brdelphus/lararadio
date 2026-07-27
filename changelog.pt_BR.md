# Changelog — Modificações no LaraRadio

Todas as alterações feitas no código-fonte para corrigir crashes,
adicionar tema escuro, e melhorar a estabilidade.

---

## [1.0.5] — 2026-07-27

### Corrigido

#### `audioplayer.h` / `audioplayer.cpp`
- **Removido sistema híbrido ffplay** que causava áudio duplicado:
  - QMediaPlayer (mudo) + ffplay (processo filho) tocavam o mesmo
    áudio simultaneamente
  - ffplay removido: `QProcess`, `watchdog`, `onFfplayFinished`,
    `checkFfplay`, `killFfplay`, `startFfplay`
- **QMediaPlayer volta a ser a única fonte de áudio**, com volume
  controlado via `QAudioOutput` — fade/crossfade funciona corretamente
- `transcodeIfNeeded()` mantido: converte MP3 com album art para WAV,
  prevenindo o crash do decoder `mp3float`
- `playbackFinished` agora é emitido via `QMediaPlayer::EndOfMedia`
- `isPlaying()` / `isStopped()` usam `QMediaPlayer::playbackState()`

#### `mainwindow.cpp`
- **Hora certa**: verifica se o arquivo de áudio existe antes de tocar.
  Se não existir, loga warning e pula o item sem travar a playlist
- `timeplayer`: conectado `errorOccurred` como segurança —
  limpa `SayingTimer` e avança em caso de erro
- **Removido avanço redundante do `flash()`**: `flash()` (timer de 500ms)
  e `checkAdvanceTrack()` (do `playbackFinished`) avançavam a playlist
  independentemente com a mesma condição `isStopped()`. Em faixas curtas
  (vinhetas), ambos podiam disparar em sequência, causando avanço duplicado
  e repetição. O bloco de avanço foi removido do `flash()` — agora ele só
  cuida da interface (piscar talk, indicador de fade, play/stop). Caminho
  único: `playbackFinished` → `checkAdvanceTrack()` → `next()`.

---

## [1.0.4] — 2026-07-25

### Adicionado

#### `audioplayer.h`
- `hasError()` — flag pública de estado de erro
- `mediaError(file, errorMsg)` — sinal emitido quando o QMediaPlayer reporta erro
- `onPlayerError(error, errorString)` — slot privado para capturar erros do player
- `playbackFinished()` — sinal emitido quando o ffplay termina de tocar
- `isValidMediaFile(path)` — método estático que valida arquivo com `ffprobe`
- `audioBufferOutput` — ponteiro armazenado para reconectar após Reset()
- `ffplayProc` — QProcess para tocar áudio em processo filho
- `ffplayWatchdog` — timer que detecta se o ffplay morreu inesperadamente
- `cleanFilePath` — path do arquivo transcodificado
- `m_ffplayPlaying` — flag de estado do ffplay
- `transcodeIfNeeded()` — método privado para transcodificar MP3 com album art
- `startFfplay()` / `killFfplay()` — gerenciamento do processo filho
- `onFfplayFinished()` — trata término do ffplay
- `checkFfplay()` — watchdog que detecta morte silenciosa do ffplay

#### `audioplayer.cpp`
- Inclusão de `<QProcess>`, `<QFileInfo>`, `<QDir>`
- Conexão de `QMediaPlayer::errorOccurred` → `onPlayerError()`
- Conexão de `QMediaPlayer::mediaStatusChanged` → detecta `InvalidMedia`
- `addMedia()` — transcodifica MP3s para WAV temporário via `ffmpeg -vn`,
  removendo album art embedado que causa crash no decoder `mp3float`
- Implementação do sistema híbrido:
  - QMediaPlayer roda **mudo** (volume 0), servindo apenas VU meter, seek e posição
  - `startFfplay()` lança `ffplay -nodisp -autoexit` em **processo filho**
  - Cada música = processo ffplay novo, que morre ao final sem acumular estado
- `playbackFinished` é emitido quando ffplay termina (via `finished` signal + watchdog)
- `Reset()` — agora também mata o ffplay e limpa a source do QMediaPlayer
- Destrutor — limpa ffplay e QMediaPlayer
- `isValidMediaFile()` — valida com ffprobe antes de tocar
- `onPlayerError()` — loga e emite sinal `mediaError`

#### `buttonhole.h`
- Inclusão de `<QMediaPlayer>` e `<QAudioOutput>` (estavam sendo puxados
  indiretamente via `audioplayer.h`, que mudou de `QMediaPlayer` para `QObject`)

#### `main.cpp`
- Inclusão de `<QMessageBox>`, `<csignal>`, `<cstdlib>`
- `crashHandler(sig)` — signal handler para SIGSEGV/SIGABRT/SIGFPE:
  - Previne loops com flag estática
  - Imprime mensagem descritiva no stderr
  - Sai com código `128 + sig`
- Instalação dos handlers no início da `main()`
- **Tema escuro Fusion** (código existia comentado, foi ativado):
  - Palette escura `#303030` fundo, `#242424` base, `#dcdcdc` texto
  - Highlight azul `#55aaff`
  - Tooltip azul escuro com borda branca
  - GTK3 detectado automaticamente se disponível

#### `mainwindow.h`
- `skipToNext()` — pula para próxima faixa quando a atual falha
- `checkAdvanceTrack()` — avança na playlist quando o ffplay termina

#### `mainwindow.cpp`
- `audioBufferOutput` restaurado (VU meter ativo)
- Conexão de `mediaError` → chama `skipToNext()` automaticamente
- Conexão de `playbackFinished` → chama `checkAdvanceTrack()`
- `skipToNext()` — reseta ambos players, avança índice, chama `next()`
- `checkAdvanceTrack()` — avança se ambos players parados e sem timer ativo

### Removido

- Bloco de validação `isValidMediaFile()` no `next()` (duplicado — a
  validação é feita dentro de `addMedia()`)
- Código comentado do tema escuro (agora ativo)

### Arquitetura

**Antes (versão 1.0.3):**
```
QMediaPlayer (FFmpeg) → toca áudio + UI (VU/seek)
         ↓
   decoder mp3float acumula estado → CRASH após N músicas
```

**Depois (versão 1.0.4, sistema híbrido com ffplay):**
```
QMediaPlayer (FFmpeg, mudo) → só UI (VU/seek/posição)
ffplay (processo filho)     → áudio real, morre a cada música

MP3 com album art → ffmpeg -vn → WAV temporário → QMediaPlayer + ffplay
```

**Agora (versão 1.0.5, sistema simplificado):**
```
MP3 com album art → ffmpeg -vn → WAV temporário → QMediaPlayer (áudio + UI)

transcodeIfNeeded() previne o crash do mp3float.
Fade/crossfade entre dois players com QAudioOutput independente.
Sem ffplay — sem áudio duplicado.
```
