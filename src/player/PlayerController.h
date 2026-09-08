#pragma once

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>
#include <QUrl>

class MusicLibrary;
class TrackListModel;

// 播放引擎：以 QMediaPlayer 为后端（暂用软解，走 Qt Multimedia）。
// 依据 SQLite 索引（列表模型行号）逐个播放；QML 侧实现播放/暂停/倍速等控制。
class PlayerController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentTrackChanged)
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY currentTrackChanged)
    Q_PROPERTY(QString currentArtist READ currentArtist NOTIFY currentTrackChanged)
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentTrackChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qreal playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)

public:
    explicit PlayerController(QObject *parent = nullptr);

    void setLibrary(MusicLibrary *library);
    void setModel(TrackListModel *model);

    int currentIndex() const { return m_currentIndex; }
    QString currentTitle() const;
    QString currentArtist() const;
    QString currentPath() const;
    qint64 position() const { return m_player->position(); }
    qint64 duration() const { return m_player->duration(); }
    qreal playbackRate() const { return m_player->playbackRate(); }
    qreal volume() const { return m_audioOutput->volume(); }
    bool isPlaying() const { return m_player->playbackState() == QMediaPlayer::PlayingState; }

    // 格式化毫秒为 m:ss，供 QML 显示
    Q_INVOKABLE QString formatDuration(qint64 ms) const;

public slots:
    void setPlaybackRate(qreal rate);
    void setVolume(qreal volume);

public:
    Q_INVOKABLE void playTrack(int index);
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void seek(qint64 positionMs);

signals:
    void currentTrackChanged();
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void playbackRateChanged(qreal rate);
    void volumeChanged(qreal volume);
    void playingChanged(bool playing);
    void playbackError(const QString &message);

private:
    void persistDuration(qint64 duration);

    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    MusicLibrary *m_library = nullptr;
    TrackListModel *m_model = nullptr;
    int m_currentIndex = -1;
    int m_currentTrackId = -1;
    qint64 m_persistedDuration = -1;
};
