# Changelog — Modificações no LaraRadio

Todas as alterações entre o release original `gutierre69/lararadio`
e este fork (`brdelphus/lararadio`).

Original: https://github.com/gutierre69/lararadio
Fork: https://github.com/brdelphus/lararadio

---

## [1.0.5] — 2026-07-27 — Modificações sobre o original 1.0.4

### Alterado

#### `audioplayer.h` / `audioplayer.cpp`
- **Herança**: `AudioPlayer` mudou de subclasse de `QMediaPlayer` para
  subclasse de `QObject` contendo `QMediaPlayer *player` como membro
  (composição sobre herança). Desacopla o backend de áudio da API
  pública e permite gerenciamento de ciclo de vida mais flexível.
- **Volume**: volume padrão do `QAudioOutput` mudou de `0` para `1.0`
  (QMediaPlayer agora é a fonte de áudio real)
- **Transcodificação MP3**: adicionado `transcodeIfNeeded()` — converte
  MP3s com album art embedado para WAV temporário via `ffmpeg -vn`
  antes da reprodução. Previne o crash do decoder `mp3float` que
  ocorria após várias músicas com album art.
- **Tratamento de erro**: conectado `QMediaPlayer::errorOccurred` →
  `onPlayerError()` que emite o sinal `mediaError()`
- **Detecção de fim de faixa**: conectado
  `QMediaPlayer::mediaStatusChanged` → emite `playbackFinished()`
  em `EndOfMedia`
- **Destrutor**: destrutor explícito para limpar `player` e `audioOutput`
- **Helpers inline**: `getPosition()`, `getDuration()`, `remainingTime()`,
  `isPlaying()`, `isPaused()`, `isStopped()`, `getVolume()`, `setVolume()`
  simplificados para implementações inline de uma linha
- **`isValidMediaFile()`**: método estático usando `ffprobe` para
  validar arquivos de áudio antes de enfileirar
- **`hasError()`**: flag pública de estado de erro

#### `main.cpp`
- **Crash handler**: adicionado `crashHandler()` para `SIGSEGV`,
  `SIGABRT`, `SIGFPE` — imprime mensagem descritiva no stderr e
  sai com código `128 + signal`. Previne crashes silenciosos do
  decoder FFmpeg.
- **Tema escuro**: ativada a palette do tema escuro Fusion (estava
  comentada). Fundo `#303030`, base `#242424`, texto `#dcdcdc`,
  destaque `#55aaff`. Usa GTK3 quando disponível.

#### `mainwindow.h` / `mainwindow.cpp`
- **`skipToNext()`**: slot público — reseta ambos players, avança
  `current_play` e chama `next()`. Usado quando uma faixa dá erro.
- **`checkAdvanceTrack()`**: slot público — avança a playlist quando
  `playbackFinished` é emitido e ambos os players estão parados.
  Ponto único de avanço da playlist.
- **Recuperação de erro**: sinal `mediaError` de ambos `AudioPlayer`
  conectado a `skipToNext()` — auto-avança em erros do decoder
- **Avanço da playlist**: sinal `playbackFinished` de ambos os players
  conectado a `checkAdvanceTrack()`
- **Hora certa**: verifica `QFile::exists()` antes de tocar o arquivo
  de hora. Se não existir, loga warning e pula sem ativar
  `SayingTimer` — previne deadlock da playlist.
- **Timeplayer error handler**: conectado `QMediaPlayer::errorOccurred`
  no timeplayer — limpa `SayingTimer` e avança em caso de erro
- **Removido avanço duplicado do `flash()`**: o timer de 500ms `flash()`
  não avança mais a playlist. O avanço é feito exclusivamente por
  `checkAdvanceTrack()` via `playbackFinished`. Impede condições de
  corrida em faixas curtas (vinhetas) onde ambos os mecanismos
  avançavam independentemente.
- **Watchdog de silêncio / falha de áudio**: monitora o **avanço de
  posição** via `updateDisplay()` (timer de 10ms). Se o `getPosition()`
  do player ativo não avança por 10 segundos (sem fade ativo), chama
  `skipToNext()` automaticamente. Reescrito da versão por VU meter: o
  backend FFmpeg não alimenta o `QAudioBufferOutput`, então o nível do
  VU ficava em 0 com o áudio tocando, causando saltos falsos. Cobre
  falha de dispositivo, bugs silenciosos do decoder e pipes travados.
- **Fim de playlist para em vez de repetir**: `repeat` agora é `false`
  por padrão. Na última faixa com repeat desligado,
  `checkAdvanceTrack()` para a reprodução (`isPlaying = false`) em vez
  de voltar ao início; a pré-transição de 5s em `currentTimePosition()`
  também não faz mais wrap quando o repeat está desligado.
- **Fix de segfault ao editar playlist durante reprodução**:
  `clearPlaylist()` esvaziava a playlist com `isPlaying` ainda true e
  `current_play` desatualizado, fazendo `topLevelItem()` retornar
  nullptr → SIGSEGV. Agora: `clearPlaylist()` reseta os dois players e
  o estado de reprodução; `updateAudioList()` verifica null de
  `curItem`/`nextItem` antes de estilizar;
  `on_btn_remove_item_clicked()` ajusta `current_play`/`next_play`
  após apagar; `next()` limita `current_play` antes de indexar.
- **Botão Stop funciona com playlist vazia**: remover a última faixa
  (em reprodução) deixava a playlist vazia com áudio ainda tocando —
  `on_btn_stop_clicked()` retornava cedo e nunca parava os players.
  Agora só retorna cedo se a playlist estiver vazia **e** nenhum
  player tocando. Também protegido o acesso `playlist[current_play]`
  no `updateAudioList()` para o caso vazio (era UB fora dos limites).

#### `buttonhole.h`
- Adicionado `#include <QMediaPlayer>` e `#include <QAudioOutput>`
  explicitamente (eram puxados indiretamente via `audioplayer.h`
  quando ele herdava de `QMediaPlayer`)

### Adicionado

#### `mainwindow.ui` / `mainwindow.cpp`
- **Checkbox Repeat** (`chk_repeat`, à esquerda dos botões de
  transporte, entre o microfone e o play): liga/desliga o membro
  `repeat` que controla se a playlist faz wrap no fim ou para na
  última faixa. Padrão: desligado.

#### `buttonhole.h` / `buttonhole.cpp`
- **Botão Loop**: um 11º ButtonHole com texto "Loop" (à esquerda do
  label "Botoeira", linha y=574) com a nova flag `setLoopMode()`. Quando
  ativo, o áudio atribuído reinicia do início no `EndOfMedia`
  (`mediaStatusChanged` → `player->play()`), tocando indefinidamente.
  Atribua o áudio via botão direito → "Carregar Áudio", igual aos
  outros buttonholes (settings key `buttonhole/btn_Loop`). Clique
  alterna tocar/parar como os demais.

#### `mainwindow.h` / `mainwindow.cpp`
- **Drag & drop na playlist**: `audio_list` aceita drops externos (event
  filter instalado na tree). Arquivos de áudio soltos do gerenciador de
  arquivos entram como itens `music`; arrastar uma **pasta** expande em
  uma sequência de itens `music` (varredura recursiva, ordenada por nome
  — "uma atrás da outra"). Arquivos não-áudio são ignorados. Vinheta
  continua vindo só da caixa dela.
- **Helper `makePlaylistItem()`**: extraído da lógica de montagem via
  TagLib que estava copiada 3× (botão add + 2 handlers de double-click
  desligados). Agora é o caminho único pra montar um item `Playlist`,
  usado pelo botão add e pelo drop. Bônus: `completeBaseName()`
  substitui a cadeia de `filename.remove(".mp3")` (remove só a extensão
  final).
- **Remoção múltipla**: o `audio_list` mudou pra `ExtendedSelection`; o
  botão de remover apaga **todos os selecionados** (da maior row pra
  menor, mantendo `current_play`/`next_play` limitados).
- **Botão ON AIR** (`onAirButton`, à esquerda do botão Loop, x=100/y=574):
  alterna uma luz física de ON AIR via script externo. O clique inverte o
  estado — amarelo `ON` / preto `OFF` — e chama o script configurado com
  o parâmetro correspondente (`onair/script` + `onair/param_on` /
  `onair/param_off`, defaults `on1`/`off1`, ex: `usbrelay2 on1`). O texto
  acompanha o idioma da interface: "No Ar" (pt_BR) / "On Air" (en_US).
  Script e parâmetros configurados em ConfigDialog → Caminhos.
- **Scroll no ConfigDialog**: o diálogo de configuração agora envolve o
  conteúdo numa `QScrollArea` pra todos os campos (inclusive os do ON
  AIR) ficarem acessíveis em telas pequenas. O exemplo do script mostra
  `~/bin/usbrelay2` — resolvido por usuário, sem nome hardcoded.

### Problemas conhecidos (originais, não introduzidos por nós)
- `TagLib::AudioProperties::length()` deprecated — usar
  `lengthInSeconds()` (3 warnings no build)
- Warning do VDPAU em GPUs Radeon (inofensivo, aceleração de vídeo)
- Warning de parsing do tema GTK com certas versões de
  `gtk-contained-dark.css` (inofensivo)
