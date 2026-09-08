#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "src/input/MusicLibrary.h"
#include "src/input/TrackListModel.h"
#include "src/launcher/QqMusicLauncher.h"
#include "src/player/PlayerController.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    app.setApplicationName(QStringLiteral("RaidenShogunPlayer"));
    app.setOrganizationName(QStringLiteral("RaidenShogun"));
    app.setOrganizationDomain(QStringLiteral("raiden.local"));

    // ---- 输入模块：SQLite 曲库 + 列表模型 ----
    MusicLibrary library;
    if (!library.openDefault())
        qWarning() << "Failed to open music library database";

    TrackListModel trackModel;
    trackModel.setLibrary(&library);
    trackModel.refresh();
    QObject::connect(&library, &MusicLibrary::tracksChanged,
                     &trackModel, &TrackListModel::refresh);

    // ---- 播放引擎模块 ----
    PlayerController player;
    player.setLibrary(&library);
    player.setModel(&trackModel);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("library"), &library);
    engine.rootContext()->setContextProperty(QStringLiteral("trackModel"), &trackModel);
    engine.rootContext()->setContextProperty(QStringLiteral("player"), &player);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("RaidenShogunPlayer"), QStringLiteral("Main"));

    QqMusicLauncher qqMusic;
    QObject::connect(&qqMusic, &QqMusicLauncher::launchStarted, &app,
                     [](const QString &path) { qInfo() << "QQ Music launched:" << path; });
    QObject::connect(&qqMusic, &QqMusicLauncher::errorOccurred, &app,
                     [](const QString &message) { qWarning() << "QQ Music launcher:" << message; });
    qqMusic.launch();

    return app.exec();
}
