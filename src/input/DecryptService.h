#pragma once

#include <QByteArray>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

class QTimer;

// 解密服务：以 QProcess 子进程方式调用 qqmusic_des.exe（Frida 注入正在运行的
// QQ 音乐进程，调用其解密函数还原 .mflac -> .flac、.mgg -> .ogg）。
// 通过暂存目录把待解密文件集中，解析其 stdout 输出实时进度，转发给输入模块。
class DecryptService : public QObject
{
    Q_OBJECT

public:
    explicit DecryptService(QObject *parent = nullptr);

    QString exePath() const { return m_exePath; }
    void setExePath(const QString &path);

    QString outputDir() const { return m_outputDir; }
    void setOutputDir(const QString &dir); // 解密产物输出目录（工具在其下创建 output 子目录）

    bool isRunning() const { return m_running; }

public slots:
    void decrypt(const QStringList &files); // files = .mflac/.mgg 路径
    void cancel();

signals:
    void progressChanged(int current, int total, const QString &file);
    void finished(const QStringList &addedPaths, const QStringList &errors);
    void failed(const QString &message); // 致命错误（找不到 exe / QQ 音乐未运行等）

private:
    void handleStdout();
    void handleFinished(int exitCode, QProcess::ExitStatus status);
    void handleTimeout();
    void parseLine(const QString &line);
    static bool qqMusicRunning();

    QProcess m_process;
    QTimer *m_timeout = nullptr;
    QString m_exePath;
    QString m_outputDir;
    QString m_stagingDir;
    QStringList m_addedPaths;
    QStringList m_errors;
    int m_total = 0;
    int m_done = 0;
    bool m_running = false;
    QByteArray m_stdoutBuf;
};
