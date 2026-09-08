#pragma once

#include <QSqlQueryModel>

class MusicLibrary;

// 输入模块：把 SQLite 中的曲目以只读列表模型暴露给 QML。
// 播放顺序即 SQLite 的索引顺序（ORDER BY id）。
class TrackListModel : public QSqlQueryModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        TrackIdRole = Qt::UserRole + 1,
        TitleRole,
        ArtistRole,
        AlbumRole,
        PathRole,
        DurationRole,
    };
    Q_ENUM(Roles)

    explicit TrackListModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setLibrary(MusicLibrary *library);
    void refresh();

    Q_INVOKABLE void setFilter(const QString &filter);
    QString filter() const { return m_filter; }

    int count() const { return rowCount(); }

    // 播放引擎按 SQLite 索引(row)取曲目信息
    int trackIdAt(int row) const;
    QString titleAt(int row) const;
    QString artistAt(int row) const;
    QString pathAt(int row) const;
    int durationAt(int row) const;

signals:
    void countChanged();

private:
    MusicLibrary *m_library = nullptr;
    QString m_filter;
};
