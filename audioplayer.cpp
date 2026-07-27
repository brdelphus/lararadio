#include <QtWidgets>
#include <QMediaPlayer>
#include "audioplayer.h"
#include <QDebug>
#include <QTimer>
#include <QAudioOutput>
#include <QAudioBufferOutput>
#include <QProcess>
#include <QFileInfo>
#include <QDir>

AudioPlayer::AudioPlayer()
{
    current_length = 0;
    player = new QMediaPlayer;
    audioOutput = new QAudioOutput;

    player->setAudioOutput(audioOutput);
    audioOutput->setVolume(0);  // muted — ffplay provides actual audio

    connect(player, &QMediaPlayer::positionChanged, this, &AudioPlayer::positionChanged);
    connect(player, &QMediaPlayer::errorOccurred, this, &AudioPlayer::onPlayerError);

    ffplayProc = new QProcess(this);
    connect(ffplayProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AudioPlayer::onFfplayFinished);

    ffplayWatchdog = new QTimer(this);
    connect(ffplayWatchdog, &QTimer::timeout, this, &AudioPlayer::checkFfplay);
    ffplayWatchdog->start(500);

    QTimer *fadeTimer = new QTimer(this);
    connect(fadeTimer, &QTimer::timeout, this, &AudioPlayer::fade);
    fadeTimer->start(500);
}

AudioPlayer::~AudioPlayer()
{
    killFfplay();
    delete player;
    delete audioOutput;
}

void AudioPlayer::Reset()
{
    killFfplay();
    player->stop();
    player->setSource(QUrl());
    _fadeOut = false;
    _fadeIn = false;
    isFading = false;
    fadeFactor = 0.1f;
    maxVolume = 1.0f;
    current_length = 0;
    m_hasError = false;
    m_ffplayPlaying = false;
}

void AudioPlayer::Play()
{
    if (!cleanFilePath.isEmpty() && !m_ffplayPlaying) {
        audioOutput->setVolume(0);   // keep muted
        player->play();              // start UI tracker
        startFfplay(cleanFilePath);  // start actual audio
    }
}

void AudioPlayer::Stop()
{
    killFfplay();
    player->stop();
    m_ffplayPlaying = false;
}

void AudioPlayer::addMedia(QString file)
{
    cleanFilePath = transcodeIfNeeded(file);
    player->setSource(QUrl::fromLocalFile(cleanFilePath));
}

QString AudioPlayer::transcodeIfNeeded(const QString &file)
{
    if (!file.endsWith(".mp3", Qt::CaseInsensitive))
        return file;

    QString tmpWav = QDir::tempPath() + "/laradio_"
        + QString::number(qAbs(QFileInfo(file).lastModified().toMSecsSinceEpoch()
            ^ QFileInfo(file).size()))
        + ".wav";
    if (!QFileInfo::exists(tmpWav)) {
        QProcess ffmpeg;
        ffmpeg.start("ffmpeg", {
            "-y", "-v", "error",
            "-i", file,
            "-vn",
            "-acodec", "pcm_s16le",
            "-f", "wav",
            tmpWav
        });
        ffmpeg.waitForFinished(30000);
        if (ffmpeg.exitCode() == 0)
            return tmpWav;
    } else {
        return tmpWav;
    }
    return file;
}

void AudioPlayer::startFfplay(const QString &path)
{
    killFfplay();
    ffplayProc->start("ffplay", {
        "-nodisp", "-autoexit", "-v", "0", "-loglevel", "panic",
        "-i", path
    });
    if (ffplayProc->waitForStarted(3000))
        m_ffplayPlaying = true;
}

void AudioPlayer::killFfplay()
{
    if (ffplayProc && ffplayProc->state() == QProcess::Running) {
        ffplayProc->kill();
        ffplayProc->waitForFinished(1000);
    }
    m_ffplayPlaying = false;
}

void AudioPlayer::onFfplayFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(status);
    m_ffplayPlaying = false;
    emit playbackFinished();
}

void AudioPlayer::checkFfplay()
{
    if (m_ffplayPlaying && ffplayProc && ffplayProc->state() == QProcess::NotRunning) {
        m_ffplayPlaying = false;
        emit playbackFinished();
    }
}

void AudioPlayer::positionChanged(qint64 position)
{
    emit update_position(position);
}

void AudioPlayer::onPlayerError(QMediaPlayer::Error error, const QString &errorString)
{
    m_hasError = true;
    qWarning() << "AudioPlayer error:" << error << errorString;
    emit mediaError(player->source().toLocalFile(), errorString);
}

void AudioPlayer::Seek(int mseconds)
{
    player->setPosition(mseconds);
    // Restart ffplay at new position
    if (m_ffplayPlaying && !cleanFilePath.isEmpty()) {
        killFfplay();
        startFfplay(cleanFilePath);
    }
}

qint64 AudioPlayer::getPosition() { return player->position(); }
qint64 AudioPlayer::getDuration() { return player->duration(); }
qint64 AudioPlayer::remainingTime() { return player->duration() - player->position(); }
bool AudioPlayer::isPlaying() { return m_ffplayPlaying; }
bool AudioPlayer::isPaused() { return player->playbackState() == QMediaPlayer::PausedState && !m_ffplayPlaying; }
bool AudioPlayer::isStopped() { return !m_ffplayPlaying && player->playbackState() == QMediaPlayer::StoppedState; }

qreal AudioPlayer::getVolume() { return audioOutput->volume(); }
void AudioPlayer::setVolume(float volume) { audioOutput->setVolume(volume); }

void AudioPlayer::fadeOut() { if(isPlaying()) _fadeOut = true; }
void AudioPlayer::fadeIn() { if(isStopped()) _fadeIn = true; }

void AudioPlayer::fade()
{
    if(_fadeOut || (isPlaying() && getVolume()>maxVolume && _fadeIn==false )) { _fadeIn=false; isFading = true; setVolume( getVolume() - fadeFactor ); }
    if(_fadeIn || (isPlaying() && getVolume()<maxVolume && _fadeOut==false )) { _fadeOut=false; isFading = true; setVolume( getVolume() + fadeFactor ); }

    if(_fadeOut && getVolume()<=0 && isPlaying()) { Stop(); _fadeOut = false; isFading = false; }
    if(getVolume()>=maxVolume && isPlaying()) { _fadeIn = false; isFading = false; }

    if(getVolume()>0 && isStopped()) { _fadeIn = false; _fadeIn = true; isFading = false; maxVolume = 0; setVolume(0); }
}

void AudioPlayer::setBuffer(QAudioBufferOutput *output)
{
    audioBufferOutput = output;
    player->setAudioBufferOutput(output);
}

bool AudioPlayer::isValidMediaFile(const QString &path)
{
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isReadable())
        return false;

    QProcess ffprobe;
    ffprobe.start("ffprobe", {
        "-v", "error",
        "-show_entries", "format=duration",
        "-of", "default=noprint_wrappers=1:nokey=1",
        path
    });
    ffprobe.waitForFinished(5000);

    if (ffprobe.exitCode() != 0)
        return false;

    QString out = ffprobe.readAllStandardOutput().trimmed();
    if (out.isEmpty())
        return false;

    bool ok = false;
    out.toDouble(&ok);
    return ok;
}
