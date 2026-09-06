#include "src/input/TrackListModel.h"

#include "src/input/MusicLibrary.h"

#include <QDebug>
#include <QHash>
#include <QSqlError>

TrackListModel::TrackListModel(QObject *parent)
    : QSqlQueryModel(parent)
{
    // 查询结果变化时同步 count 属性
    connect(this, &QAbstractItemModel::modelReset,
            this, &TrackListModel::countChanged);
    connect(this, &QAbstractItemModel::rowsInserted,
            this, &TrackListModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved,
            this, &TrackListModel::countChanged);
}

QHash<int, QByteArray> TrackListModel::roleNames() const
{
    QHash<int, QByteArray> roles = QSqlQueryModel::roleNames();
    roles[TrackIdRole] = "trackId";
    roles[TitleRole] = "title";
    roles[ArtistRole] = "artist";
    roles[AlbumRole] = "album";
    roles[PathRole] = "path";
    roles[DurationRole] = "durationMs";
    return roles;
}

QVariant TrackListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    switch (role) {
    case TrackIdRole:
        return QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 0), Qt::DisplayRole);
    case TitleRole:
        return QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 1), Qt::DisplayRole);
    case ArtistRole:
        return QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 2), Qt::DisplayRole);
    case AlbumRole:
        return QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 3), Qt::DisplayRole);
    case PathRole:
        return QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 4), Qt::DisplayRole);
    case DurationRole:
        return QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 5), Qt::DisplayRole);
    default:
        return QSqlQueryModel::data(index, role);
    }
}

void TrackListModel::setLibrary(MusicLibrary *library)
{
    m_library = library;
}

void TrackListModel::refresh()
{
    if (!m_library || !m_library->isOpen()) {
        clear();
        return;
    }

    // 播放列表直接来自 SQLite，按 id 升序（即“SQLite 索引”顺序）
    setQuery(QStringLiteral(
                 "SELECT id, title, artist, album, path, duration_ms "
                 "FROM tracks ORDER BY id"),
             m_library->database());

    if (lastError().isValid())
        qWarning() << "TrackListModel query error:" << lastError().text();
}

int TrackListModel::trackIdAt(int row) const
{
    return data(index(row, 0), TrackIdRole).toInt();
}

QString TrackListModel::titleAt(int row) const
{
    return data(index(row, 0), TitleRole).toString();
}

QString TrackListModel::artistAt(int row) const
{
    return data(index(row, 0), ArtistRole).toString();
}

QString TrackListModel::pathAt(int row) const
{
    return data(index(row, 0), PathRole).toString();
}

int TrackListModel::durationAt(int row) const
{
    return data(index(row, 0), DurationRole).toInt();
}
