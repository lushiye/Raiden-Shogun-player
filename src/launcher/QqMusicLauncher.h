#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

class QqMusicLauncher : public QObject {
    Q_OBJECT

public:
    explicit QqMusicLauncher(QObject *parent = nullptr);

    QString musicPath() const { return m_musicPath; }
    Q_INVOKABLE bool launch();

signals:
    void launchStarted(const QString &exePath);
    void errorOccurred(const QString &message);
    void canNotFind();

private:
    QString findQQMusicPath() const;

    QProcess m_process;
    QString m_musicPath;
};