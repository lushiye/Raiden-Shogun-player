#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QVariantList>

class DecryptService;

// 输入模块核心：以 SQLite 为存储的曲库。
// 播放列表直接通过 SQLite 获取；导入音乐时自动区分普通音频（直接入列表）
// 与加密音乐（走 DecryptService 解密后再入列表）。
class MusicLibrary : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool decrypting READ isDecrypting NOTIFY decryptingChanged)
    Q_PROPERTY(int decryptCurrent READ decryptCurrent NOTIFY decryptProgress)
    Q_PROPERTY(int decryptTotal READ decryptTotal NOTIFY decryptProgress)
    Q_PROPERTY(QString decryptFileName READ decryptFileName NOTIFY decryptProgress)

public:
    explicit MusicLibrary(QObject *parent = nullptr);
    ~MusicLibrary() override;

    // 打开（必要时创建）数据库并建表
    bool open(const QString &dbPath);
    bool openDefault(); // 使用系统应用数据目录下的默认库文件

    bool isOpen() const { return m_db.isOpen(); }
    QString databasePath() const { return m_dbPath; }
    QSqlDatabase database() const { return m_db; }

    // 导入入口：自动区分普通音频（直接入列表）与加密音频（弹窗解密后入列表）
    Q_INVOKABLE void importFiles(const QVariantList &urlsOrPaths);
    Q_INVOKABLE void importFolder(const QVariant &dirUrlOrPath);
    Q_INVOKABLE void cancelImport();

    // 直接把普通音频路径写入 SQLite（供内部/高级用途）
    Q_INVOKABLE int addFiles(const QVariantList &urlsOrPaths);

    Q_INVOKABLE bool removeTrack(int id);
    Q_INVOKABLE bool clear();
    Q_INVOKABLE int trackCount() const;
    Q_INVOKABLE bool updateDuration(int id, qint64 durationMs);

    // 解密状态（供 QML 弹窗绑定）
    bool isDecrypting() const { return m_decrypting; }
    int decryptCurrent() const { return m_decryptCurrent; }
    int decryptTotal() const { return m_decryptTotal; }
    QString decryptFileName() const { return m_decryptFileName; }

signals:
    void tracksChanged();
    void errorOccurred(const QString &message);
    void decryptingChanged(bool decrypting);
    void decryptProgress(int current, int total, const QString &file);
    void importFinished(int addedCount, const QStringList &errors);

private:
    bool createSchema();
    int insertPaths(const QStringList &paths);

    static bool isPlainAudio(const QString &path);
    static bool isEncrypted(const QString &path);
    static QString titleFromPath(const QString &path);
    static QString localPathFromVariant(const QVariant &v);
    static QStringList toPathList(const QVariantList &urlsOrPaths);

    void importPaths(const QStringList &paths);
    void startDecrypt(const QStringList &files);
    void onDecryptFinished(const QStringList &addedPaths, const QStringList &errors);
    void onDecryptFailed(const QString &message);
    QString unlockedDir() const;

    QSqlDatabase m_db;
    QString m_dbPath;
    DecryptService *m_decrypt = nullptr;
    bool m_decrypting = false;
    int m_decryptCurrent = 0;
    int m_decryptTotal = 0;
    QString m_decryptFileName;
    int m_pendingPlainAdded = 0; // 开始解密前已直接写入的普通音频数量
    QStringList m_pendingErrors; // 开始解密前已记录的错误（结束后合并上报）
};
