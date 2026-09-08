#include "src/launcher/QqMusicLauncher.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace {

bool isQQMusicExe(const QString &path) {
    const QFileInfo info(path);
    return info.isFile() && info.exists() && info.fileName().compare(QStringLiteral("QQMusic.exe"), Qt::CaseInsensitive) == 0;
}

QString exePathFromRegistryValue(const QString &value) {
    const QString clean = value.section(QLatin1Char(','), 0, 0).trimmed();
    if (clean.isEmpty()) return {};
    if (isQQMusicExe(clean)) return QDir::toNativeSeparators(clean);

    const QFileInfo info(clean);
    if (info.isDir()) {
        const QString exe = info.absoluteFilePath() + QStringLiteral("/QQMusic.exe");
        if (isQQMusicExe(exe)) return QDir::toNativeSeparators(exe);
    }
    return "";
}

QString findExeInRegistryKey(const QString &key) {
    const QSettings reg(key, QSettings::NativeFormat);
    const QStringList valueKeys = reg.allKeys();
    for (const QString &valueKey : valueKeys) {
        const QString exe = exePathFromRegistryValue(reg.value(valueKey).toString());
        if (!exe.isEmpty()) return exe;
    }
    return "";
}

}

QqMusicLauncher::QqMusicLauncher(QObject *parent) : QObject(parent), m_musicPath(findQQMusicPath()) { }

QString QqMusicLauncher::findQQMusicPath() const {
    const QStringList directKeys = {
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Tencent\\QQMusic"),
        QStringLiteral("HKEY_LOCAL_MACHINE\\Software\\Tencent\\QQMusic"),
        QStringLiteral("HKEY_LOCAL_MACHINE\\Software\\WOW6432Node\\Tencent\\QQMusic"),
    };
    for (const QString &key : directKeys) {
        const QString exe = findExeInRegistryKey(key);
        if (!exe.isEmpty()) return exe;
    }
    const QStringList uninstallBases = {
        QStringLiteral("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKEY_LOCAL_MACHINE\\Software\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
    };
    for (const QString &base : uninstallBases) {
        QSettings reg(base, QSettings::NativeFormat);
        const QStringList groups = reg.childGroups();
        for (const QString &group : groups) {
            reg.beginGroup(group);
            const QString name = reg.value(QStringLiteral("DisplayName")).toString();
            const bool isQQMusic = name.contains(QStringLiteral("QQ音乐"), Qt::CaseInsensitive) || name.contains(QStringLiteral("QQMusic"), Qt::CaseInsensitive);
            if (isQQMusic) {
                const QStringList valueKeys = reg.allKeys();
                for (const QString &valueKey : valueKeys) {
                    const QString exe = exePathFromRegistryValue(reg.value(valueKey).toString());
                    if (!exe.isEmpty()) {
                        reg.endGroup();
                        return exe;
                    }
                }
            }
            reg.endGroup();
        }
    }

    const QStringList tencentDirs = {
        QStringLiteral("C:/Program Files (x86)/Tencent"),
        QStringLiteral("C:/Program Files/Tencent"),
        QStringLiteral("D:/Program Files (x86)/Tencent"),
        QStringLiteral("D:/Program Files/Tencent"),
    };
    for (const QString &tencentDir : tencentDirs) {
        const QDir parent(tencentDir);
        if (!parent.exists()) continue;
        const QFileInfoList subDirs = parent.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &subDir : subDirs) {
            if (!subDir.fileName().contains(QStringLiteral("QQMusic"), Qt::CaseInsensitive)) continue;
            const QString exe = subDir.absoluteFilePath() + QStringLiteral("/QQMusic.exe");
            if (isQQMusicExe(exe)) return QDir::toNativeSeparators(exe);
        }
    }

    return {};
}

bool QqMusicLauncher::launch() {
    if (m_musicPath.isEmpty()) {
        emit errorOccurred(QStringLiteral("未找到 QQ 音乐"));
        return false;
    }

    m_process.setProgram(m_musicPath);
    if (!m_process.startDetached()) {
        emit errorOccurred(QStringLiteral("QQ 音乐启动失败：") + m_musicPath);
        return false;
    }

    emit launchStarted(m_musicPath);
    return true;
}