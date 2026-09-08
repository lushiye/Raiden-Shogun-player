#include "src/player/PlayerController.h"

#include "src/input/MusicLibrary.h"
#include "src/input/TrackListModel.h"

#include <QtGlobal>

PlayerController::PlayerController(QObject *parent)
    : QObject(parent)
{
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);

    connect(m_player, &QMediaPlayer::positionChanged,
            this, &PlayerController::positionChanged);

    connect(m_player, &QMediaPlayer::durationChanged, this, [this](qint64 d) {
        emit durationChanged(d);
        persistDuration(d);
    });

    connect(m_player, &QMediaPlayer::playbackRateChanged,
            this, &PlayerController::playbackRateChanged);

    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState) {
        emit playingChanged(isPlaying());
    });

    // 单曲播完自动跳到下一首（按 SQLite 索引逐个播放）
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia)
            next();
    });

    connect(m_player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &msg) {
        emit playbackError(msg);
    });
}

void PlayerController::setLibrary(MusicLibrary *library)
{
    m_library = library;
}

void PlayerController::setModel(TrackListModel *model)
{
    m_model = model;
    if (m_model) {
        connect(m_model, &QAbstractItemModel::modelReset, this, [this]() {
            if (!m_model || m_model->rowCount() == 0) {
                m_player->stop();
                m_currentIndex = -1;
                emit currentTrackChanged();
                return;
            }
            if (m_currentTrackId >= 0) {
                int found = -1;
                for (int i = 0; i < m_model->rowCount(); ++i) {
                    if (m_model->trackIdAt(i) == m_currentTrackId) {
                        found = i;
                        break;
                    }
                }
                if (found != m_currentIndex) {
                    m_currentIndex = found;
                    emit currentTrackChanged();
                }
            } else if (m_currentIndex >= m_model->rowCount()) {
                m_currentIndex = -1;
                emit currentTrackChanged();
            }
        });
    }
}

QString PlayerController::currentTitle() const
{
    if (m_model && m_currentIndex >= 0 && m_currentIndex < m_model->rowCount())
        return m_model->titleAt(m_currentIndex);
    return {};
}

QString PlayerController::currentArtist() const
{
    if (m_model && m_currentIndex >= 0 && m_currentIndex < m_model->rowCount())
        return m_model->artistAt(m_currentIndex);
    return {};
}

QString PlayerController::currentPath() const
{
    if (m_model && m_currentIndex >= 0 && m_currentIndex < m_model->rowCount())
        return m_model->pathAt(m_currentIndex);
    return {};
}

QString PlayerController::formatDuration(qint64 ms) const
{
    if (ms <= 0)
        return QStringLiteral("--:--");
    const qint64 totalSec = ms / 1000;
    const qint64 minutes = totalSec / 60;
    const qint64 seconds = totalSec % 60;
    return QStringLiteral("%1:%2").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
}

void PlayerController::setPlaybackRate(qreal rate)
{
    // QMediaPlayer 的 playbackRate 即倍速（0.5x / 1.0x / 2.0x ...）
    m_player->setPlaybackRate(rate);
}

void PlayerController::setVolume(qreal volume)
{
    m_audioOutput->setVolume(float(qBound<qreal>(0.0, volume, 1.0)));
}

void PlayerController::playTrack(int index)
{
    if (!m_model || index < 0 || index >= m_model->rowCount())
        return;
    const QString path = m_model->pathAt(index);
    if (path.isEmpty())
        return;

    m_currentIndex = index;
    m_currentTrackId = m_model->trackIdAt(index);
    m_persistedDuration = -1;
    m_player->setSource(QUrl::fromLocalFile(path));
    m_player->play();
    emit currentTrackChanged();
}

void PlayerController::toggle()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState)
        pause();
    else
        play();
}

void PlayerController::play()
{
    if (!m_model || m_model->rowCount() == 0)
        return;
    if (m_currentIndex < 0)
        playTrack(0);
    else
        m_player->play();
}

void PlayerController::pause()
{
    m_player->pause();
}

void PlayerController::stop()
{
    m_player->stop();
}

void PlayerController::next()
{
    if (!m_model || m_model->rowCount() == 0)
        return;
    const int n = m_model->rowCount();
    const int nextIndex = (m_currentIndex + 1 + n) % n; // 到尾后回到第一首（循环）
    playTrack(nextIndex);
}

void PlayerController::previous()
{
    if (!m_model || m_model->rowCount() == 0)
        return;
    const int n = m_model->rowCount();
    const int prevIndex = (m_currentIndex - 1 + n) % n; // 到首后回到最后一首
    playTrack(prevIndex);
}

void PlayerController::seek(qint64 positionMs)
{
    m_player->setPosition(positionMs);
}

void PlayerController::persistDuration(qint64 duration)
{
    if (!m_library || !m_model || m_currentIndex < 0)
        return;
    if (duration <= 0 || duration == m_persistedDuration)
        return;
    m_persistedDuration = duration;
    m_library->updateDuration(m_model->trackIdAt(m_currentIndex), duration);
}
