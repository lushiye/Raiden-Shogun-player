#include "src/input/MusicLibrary.h"

#include "src/input/DecryptService.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMetaType>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUrl>
#include <QVariant>

namespace {
constexpr auto kConnectionName = "raiden_music_library";

// 已解密（可直接播放）的普通音频扩展名
const QStringList kPlainExts = {
    QStringLiteral(".mp3"),
    QStringLiteral(".wav"),
    QStringLiteral(".flac"),
    QStringLiteral(".ogg"),
    QStringLiteral(".m4a"),
};

// 加密音乐扩展名（qqmusic_decrypt 仅支持 QQ 音乐的 mflac / mgg）
const QStringList kEncryptedExts = {
    QStringLiteral(".mflac"),
    QStringLiteral(".mgg"),
};
} // namespace

MusicLibrary::MusicLibrary(QObject *parent)
    : QObject(parent)
{
    m_decrypt = new DecryptService(this);

    connect(m_decrypt, &DecryptService::progressChanged, this, [this](int current, int total, const QString &file) {
        m_decryptCurrent = current;
        m_decryptTotal = total;
        m_decryptFileName = file;
        emit decryptProgress(current, total, file);
    });
    connect(m_decrypt, &DecryptService::finished,
            this, &MusicLibrary::onDecryptFinished);
    connect(m_decrypt, &DecryptService::failed,
            this, &MusicLibrary::onDecryptFailed);
}

MusicLibrary::~MusicLibrary()
{
    if (m_db.isOpen())
        m_db.close();
}

bool MusicLibrary::open(const QString &dbPath)
{
    const QString conn = QString::fromLatin1(kConnectionName);
    if (QSqlDatabase::contains(conn)) {
        m_db = QSqlDatabase::database(conn);
    } else {
        m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
    }
    m_db.setDatabaseName(dbPath);

    // 确保库文件所在目录存在
    const QFileInfo fi(dbPath);
    const QDir dir = fi.dir();
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));

    if (!m_db.open()) {
        emit errorOccurred(m_db.lastError().text());
        return false;
    }

    m_dbPath = dbPath;
    if (!createSchema())
        return false;
    return true;
}

bool MusicLibrary::openDefault()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return open(dir + QStringLiteral("/library.db"));
}

bool MusicLibrary::createSchema()
{
    QSqlQuery q(m_db);
    const QString sql = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS tracks ("
        "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  path        TEXT NOT NULL UNIQUE,"
        "  title       TEXT NOT NULL DEFAULT '',"
        "  artist      TEXT NOT NULL DEFAULT '',"
        "  album       TEXT NOT NULL DEFAULT '',"
        "  duration_ms INTEGER NOT NULL DEFAULT 0,"
        "  added_at    TEXT NOT NULL DEFAULT (datetime('now','localtime'))"
        ")");
    if (!q.exec(sql)) {
        emit errorOccurred(q.lastError().text());
        return false;
    }
    return true;
}

QString MusicLibrary::localPathFromVariant(const QVariant &v)
{
    if (v.typeId() == QMetaType::QUrl) {
        return v.toUrl().toLocalFile();
    }
    const QString s = v.toString();
    if (s.startsWith(QLatin1String("file://"))) {
        return QUrl(s).toLocalFile();
    }
    return s;
}

QStringList MusicLibrary::toPathList(const QVariantList &urlsOrPaths)
{
    QStringList paths;
    for (const QVariant &v : urlsOrPaths) {
        QString p = QDir::cleanPath(localPathFromVariant(v));
        if (!p.isEmpty())
            paths << p;
    }
    return paths;
}

bool MusicLibrary::isPlainAudio(const QString &path)
{
    const QString ext = QFileInfo(path).suffix().toLower();
    // QFileInfo::suffix() 不含点，kPlainExts 含点，需补上再比较
    return kPlainExts.contains(QStringLiteral(".") + ext);
}

bool MusicLibrary::isEncrypted(const QString &path)
{
    const QString lower = path.toLower();
    for (const QString &ext : kEncryptedExts) {
        if (lower.endsWith(ext))
            return true;
    }
    return false;
}

QString MusicLibrary::titleFromPath(const QString &path)
{
    return QFileInfo(path).completeBaseName();
}

int MusicLibrary::insertPaths(const QStringList &paths)
{
    if (!isOpen() || paths.isEmpty())
        return 0;

    int inserted = 0;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO tracks (path, title) VALUES (:path, :title)"));

    QSet<QString> seen;
    for (const QString &raw : paths) {
        const QString p = QDir::cleanPath(raw); // 统一反/正斜杠，避免同一文件重复入库
        if (!isPlainAudio(p))
            continue;
        if (seen.contains(p))
            continue;
        seen.insert(p);
        q.bindValue(QStringLiteral(":path"), p);
        q.bindValue(QStringLiteral(":title"), titleFromPath(p));
        if (q.exec()) {
            if (q.numRowsAffected() > 0)
                ++inserted;
        } else {
            emit errorOccurred(q.lastError().text());
        }
    }

    if (inserted > 0)
        emit tracksChanged();
    return inserted;
}

int MusicLibrary::addFiles(const QVariantList &urlsOrPaths)
{
    return insertPaths(toPathList(urlsOrPaths));
}

void MusicLibrary::importFiles(const QVariantList &urlsOrPaths)
{
    importPaths(toPathList(urlsOrPaths));
}

void MusicLibrary::importFolder(const QVariant &dirUrlOrPath)
{
    const QString dirPath = QDir::cleanPath(localPathFromVariant(dirUrlOrPath));
    if (dirPath.isEmpty()) {
        emit importFinished(0, { tr("无效的目录路径") });
        return;
    }

    const QDir dir(dirPath);
    if (!dir.exists()) {
        emit importFinished(0, { tr("目录不存在：%1").arg(dirPath) });
        return;
    }

    QStringList found;
    QDirIterator it(dir.absolutePath(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString p = it.next();
        if (isPlainAudio(p) || isEncrypted(p))
            found << p;
    }

    if (found.isEmpty()) {
        emit importFinished(0, { tr("未在该目录找到支持的音频或加密音乐") });
        return;
    }
    importPaths(found);
}

void MusicLibrary::importPaths(const QStringList &paths)
{
    if (m_decrypting) {
        emit importFinished(0, { tr("已有解密任务进行中，请等待完成后再导入") });
        return;
    }

    QStringList plain;
    QStringList toDecrypt;
    QStringList errors;
    for (const QString &p : paths) {
        if (isPlainAudio(p))
            plain << p;
        else if (isEncrypted(p))
            toDecrypt << p;
        else
            errors << tr("不支持的格式：%1").arg(p);
    }
    qDebug() << "[Import] 总数 =" << paths.size() << "| 直接入列表 =" << plain
             << "| 需解密 =" << toDecrypt << "| 不支持 =" << errors;

    // 普通音频直接入列表
    const int plainAdded = insertPaths(plain);

    if (toDecrypt.isEmpty()) {
        emit importFinished(plainAdded, errors);
        return;
    }

    m_pendingPlainAdded = plainAdded;
    m_pendingErrors = errors;
    startDecrypt(toDecrypt);
}

void MusicLibrary::startDecrypt(const QStringList &files)
{
    if (m_decrypting)
        return;

    m_decrypt->setOutputDir(unlockedDir());

    m_decrypting = true;
    m_decryptCurrent = 0;
    m_decryptTotal = files.size();
    m_decryptFileName.clear();
    emit decryptingChanged(true);

    m_decrypt->decrypt(files);
}

void MusicLibrary::onDecryptFinished(const QStringList &addedPaths, const QStringList &errors)
{
    qDebug() << "[Import] 解密完成，产物 =" << addedPaths << "| 错误 =" << errors;
    const int decryptedAdded = insertPaths(addedPaths);
    const int totalAdded = m_pendingPlainAdded + decryptedAdded;

    QStringList allErrors = m_pendingErrors;
    allErrors << errors;
    m_pendingPlainAdded = 0;
    m_pendingErrors.clear();

    emit importFinished(totalAdded, allErrors);
    m_decrypting = false;
    emit decryptingChanged(false);
}

void MusicLibrary::onDecryptFailed(const QString &message)
{
    const int totalAdded = m_pendingPlainAdded; // 普通音频已写入
    QStringList allErrors = m_pendingErrors;
    allErrors << message;
    m_pendingPlainAdded = 0;
    m_pendingErrors.clear();

    emit importFinished(totalAdded, allErrors);
    m_decrypting = false;
    emit decryptingChanged(false);
}

void MusicLibrary::cancelImport()
{
    if (m_decrypting)
        m_decrypt->cancel();
}

QString MusicLibrary::unlockedDir() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/unlocked");
    QDir().mkpath(dir);
    return dir;
}

bool MusicLibrary::removeTrack(int id)
{
    if (!isOpen())
        return false;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM tracks WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        emit errorOccurred(q.lastError().text());
        return false;
    }
    if (q.numRowsAffected() > 0)
        emit tracksChanged();
    return true;
}

bool MusicLibrary::clear()
{
    if (!isOpen())
        return false;

    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("DELETE FROM tracks"))) {
        emit errorOccurred(q.lastError().text());
        return false;
    }
    emit tracksChanged();
    return true;
}

int MusicLibrary::trackCount() const
{
    if (!m_db.isOpen())
        return 0;
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT COUNT(*) FROM tracks")) && q.next())
        return q.value(0).toInt();
    return 0;
}

bool MusicLibrary::updateDuration(int id, qint64 durationMs)
{
    if (!m_db.isOpen())
        return false;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE tracks SET duration_ms = :d WHERE id = :id"));
    q.bindValue(QStringLiteral(":d"), durationMs);
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}
