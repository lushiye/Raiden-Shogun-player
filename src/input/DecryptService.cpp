#include "src/input/DecryptService.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QTimer>

namespace {
// qqmusic_des.exe 的 stdout 标记（Rust println! 输出，UTF-8）
const QString kDonePrefix = QStringLiteral("[*] 处理文件: ");
const QString kDoneSuffix = QStringLiteral(" 完成");
const QString kSkipPrefix = QStringLiteral("[*] 文件已存在: ");
const QString kSkipSuffix = QStringLiteral(" 跳过处理");
} // namespace

DecryptService::DecryptService(QObject *parent)
    : QObject(parent)
{
#ifdef QQMUSIC_DES_EXE
    m_exePath = QStringLiteral(QQMUSIC_DES_EXE);
#else
    m_exePath = QCoreApplication::applicationDirPath()
        + QStringLiteral("/qqmusic_des.exe");
#endif

    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &DecryptService::handleStdout);
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DecryptService::handleFinished);

    m_timeout = new QTimer(this);
    m_timeout->setSingleShot(true);
    connect(m_timeout, &QTimer::timeout, this, &DecryptService::handleTimeout);
}

void DecryptService::setExePath(const QString &path)
{
    m_exePath = path;
}

void DecryptService::setOutputDir(const QString &dir)
{
    m_outputDir = dir;
}

bool DecryptService::qqMusicRunning()
{
#ifdef Q_OS_WIN
    QProcess p;
    p.start(QStringLiteral("tasklist"),
            { QStringLiteral("/FI"), QStringLiteral("IMAGENAME eq QQMusic.exe"),
              QStringLiteral("/NH") });
    if (!p.waitForFinished(3000))
        return false;
    const QString out = QString::fromLocal8Bit(p.readAllStandardOutput());
    return out.contains(QStringLiteral("QQMusic"), Qt::CaseInsensitive);
#else
    return true; // 该解密程序仅面向 Windows
#endif
}

void DecryptService::decrypt(const QStringList &files)
{
    if (m_running)
        return;
    if (files.isEmpty()) {
        emit finished({}, {});
        return;
    }

    if (!QFileInfo::exists(m_exePath)) {
        qWarning() << "[Decrypt] 找不到解密程序:" << m_exePath;
        emit failed(tr("找不到解密程序：%1").arg(m_exePath));
        return;
    }
    const bool running = qqMusicRunning();
    qDebug() << "[Decrypt] exe =" << m_exePath << "| 文件数 =" << files.size()
             << "| QQ音乐运行中 =" << running;
    if (!running) {
        emit failed(tr("未检测到 QQ 音乐正在运行，请先启动 QQ 音乐后再导入加密文件"));
        return;
    }

    m_errors.clear();
    m_addedPaths.clear();
    m_stdoutBuf.clear();
    m_done = 0;

    // 1) 暂存目录：把待解密的 mflac/mgg 复制到一起（扁平化、处理重名）
    m_stagingDir = QDir::tempPath()
        + QStringLiteral("/raiden_decrypt_%1").arg(QDateTime::currentMSecsSinceEpoch());
    QDir().mkpath(m_stagingDir);

    QStringList staged;
    QSet<QString> usedNames;
    for (const QString &f : files) {
        const QFileInfo fi(f);
        if (!fi.exists() || !fi.isFile()) {
            m_errors << tr("文件不存在：%1").arg(f);
            continue;
        }
        QString base = fi.fileName();
        QString dst = m_stagingDir + QLatin1Char('/') + base;
        int n = 1;
        while (usedNames.contains(base.toLower())) {
            base = QStringLiteral("%1 (%2).%3")
                       .arg(fi.completeBaseName()).arg(n++).arg(fi.suffix());
            dst = m_stagingDir + QLatin1Char('/') + base;
        }
        usedNames.insert(base.toLower());
        if (QFile::copy(f, dst))
            staged << dst;
        else
            m_errors << tr("复制文件失败：%1").arg(f);
    }

    if (staged.isEmpty()) {
        QDir(m_stagingDir).removeRecursively();
        emit finished({}, m_errors);
        return;
    }

    m_total = staged.size();
    qDebug() << "[Decrypt] 暂存目录 =" << m_stagingDir << "| 输出目录 =" << m_outputDir
             << "| 待解密 =" << staged;
    QDir().mkpath(m_outputDir);

    m_running = true;
    m_process.setWorkingDirectory(m_outputDir); // 工具在 cwd 下创建 output 子目录
    m_process.start(m_exePath, QStringList());
    if (!m_process.waitForStarted(5000)) {
        m_running = false;
        QDir(m_stagingDir).removeRecursively();
        emit failed(tr("无法启动解密程序：%1").arg(m_exePath));
        return;
    }

    // 2) 应答交互：n = 使用自定义目录，随后粘贴暂存目录路径
    m_process.write(QByteArrayLiteral("n\n"));
    m_process.write((m_stagingDir + QLatin1Char('\n')).toUtf8());
    m_process.closeWriteChannel();

    // 3) 超时保护：Frida 附加失败/挂起时给用户明确提示
    m_timeout->start(300000); // 5 分钟
}

void DecryptService::cancel()
{
    if (m_running)
        m_process.kill();
}

void DecryptService::handleStdout()
{
    m_stdoutBuf += m_process.readAllStandardOutput();
    int idx = 0;
    while ((idx = m_stdoutBuf.indexOf('\n')) >= 0) {
        const QByteArray line = m_stdoutBuf.left(idx);
        m_stdoutBuf.remove(0, idx + 1);
        if (!line.trimmed().isEmpty())
            parseLine(QString::fromUtf8(line));
    }
}

void DecryptService::handleFinished(int exitCode, QProcess::ExitStatus)
{
    if (m_timeout)
        m_timeout->stop();

    handleStdout();
    if (!m_stdoutBuf.trimmed().isEmpty()) {
        parseLine(QString::fromUtf8(m_stdoutBuf));
        m_stdoutBuf.clear();
    }

    const QString stderrText = QString::fromUtf8(m_process.readAllStandardError()).trimmed();
    qDebug() << "[Decrypt] 退出码 =" << exitCode << "| 产物 =" << m_addedPaths
             << "| stderr =" << stderrText;
    if (exitCode != 0 && m_errors.isEmpty()) {
        if (!stderrText.isEmpty())
            m_errors << tr("解密程序异常退出（代码 %1）：%2").arg(exitCode).arg(stderrText);
        else
            m_errors << tr("解密程序异常退出（代码 %1）").arg(exitCode);
    }

    // 兜底：扫描输出目录，确保所有解密产物都已收录
    const QDir outDir(m_outputDir + QStringLiteral("/output"));
    if (outDir.exists()) {
        const QStringList names = outDir.entryList(
            { QStringLiteral("*.flac"), QStringLiteral("*.ogg") }, QDir::Files);
        for (const QString &n : names) {
            const QString p = QDir::cleanPath(outDir.absoluteFilePath(n));
            if (!m_addedPaths.contains(p))
                m_addedPaths << p;
        }
    }

    // 静默失败兜底：处理了文件却既无产物也无错误
    if (m_addedPaths.isEmpty() && m_errors.isEmpty() && m_total > 0) {
        m_errors << tr("解密未产生任何结果：请确认 QQ 音乐正在运行，且文件为有效的 .mflac/.mgg");
    }

    m_running = false;
    QDir(m_stagingDir).removeRecursively();
    emit finished(m_addedPaths, m_errors);
}

void DecryptService::handleTimeout()
{
    if (!m_running)
        return;
    m_errors << tr("解密超时（可能 QQ 音乐被占用或附加失败），已中止");
    m_process.kill(); // 随后触发 handleFinished 统一收尾
}

void DecryptService::parseLine(const QString &line)
{
    const QString s = line.trimmed();
    QString path;

    if (s.startsWith(kDonePrefix) && s.endsWith(kDoneSuffix)) {
        path = s.mid(kDonePrefix.length(),
                     s.length() - kDonePrefix.length() - kDoneSuffix.length());
    } else if (s.startsWith(kSkipPrefix) && s.endsWith(kSkipSuffix)) {
        path = s.mid(kSkipPrefix.length(),
                     s.length() - kSkipPrefix.length() - kSkipSuffix.length());
    } else {
        return; // 其它输出（Frida 版本、设备名、目录提示等）忽略
    }

    path = QDir::cleanPath(path.trimmed());
    if (path.isEmpty())
        return;
    if (!m_addedPaths.contains(path))
        m_addedPaths << path;
    if (m_done < m_total) {
        ++m_done;
        emit progressChanged(m_done, m_total, QFileInfo(path).fileName());
    }
}
