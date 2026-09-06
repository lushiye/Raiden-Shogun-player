# RaidenShogunPlayer

一个使用 **Qt 6.8+ / CMake / QML** 编写的桌面音频播放器，按两个模块组织：

| 模块 | 职责 | 核心文件 |
| --- | --- | --- |
| 输入模块 | 以 **SQLite** 为核心，播放列表直接通过 SQLite 获取；导入音乐时自动区分普通音频（直接入列表）与加密音乐（解密后入列表） | `src/input/MusicLibrary.{h,cpp}`、`src/input/TrackListModel.{h,cpp}`、`src/input/DecryptService.{h,cpp}` |
| 播放引擎 | 暂用软解（Qt Multimedia 后端），依据 SQLite 索引逐个播放；QML 侧实现播放 / 暂停 / 倍速等控制 | `src/player/PlayerController.{h,cpp}` |

## 依赖

- CMake ≥ 3.21（使用 `CMakePresets.json` 需要 ≥ 3.24）
- Qt 6.8 或更高，包含组件：`Quick`、`Multimedia`、`Sql`
- 支持 C++17 的编译器（Windows 上通常为 MSVC 或 MinGW）
- （解密功能，可选）`qqmusic_des.exe`（随项目提供）与**正在运行的 QQ 音乐客户端**——仅导入 `.mflac/.mgg` 时需要；普通音频导入不依赖它

## 构建

### 方式一：CMake Presets（推荐）

先在环境中设置 `QTDIR` 指向 Qt 安装目录（例如 `C:\Qt\6.8.2\msvc2022_64`），然后：

```powershell
cmake --preset default
cmake --build --preset default
```

### 方式二：手动命令行

```powershell
cmake -S . -B build -G "Ninja" -DCMAKE_PREFIX_PATH=C:/Qt/6.8.2/msvc2022_64
cmake --build build
```

> 若使用 Visual Studio 生成器，把 `-G "Ninja"` 换成 `-G "Visual Studio 17 2022"`，构建命令改为 `cmake --build build --config Debug`。

### 运行

```powershell
.\build\RaidenShogunPlayer.exe
```

或使用 Qt Creator 直接打开 `CMakeLists.txt` 运行。

## 数据存储

曲库数据库默认位于系统应用数据目录（Windows 下为
`%APPDATA%\RaidenShogunPlayer\library.db`）。表结构：

```sql
CREATE TABLE tracks (
  id          INTEGER PRIMARY KEY AUTOINCREMENT,
  path        TEXT NOT NULL UNIQUE,      -- 音乐文件路径（输入模块写入）
  title       TEXT NOT NULL DEFAULT '',  -- 默认取文件名（不含扩展名）
  artist      TEXT NOT NULL DEFAULT '',
  album       TEXT NOT NULL DEFAULT '',
  duration_ms INTEGER NOT NULL DEFAULT 0,
  added_at    TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);
```

播放列表即 `SELECT ... FROM tracks ORDER BY id`，播放引擎按该顺序（SQLite 索引）逐个播放，并在单曲播完后自动切下一首。

## 支持格式

- **直接播放（普通音频）**：mp3 / wav / flac / ogg / m4a
- **自动解密（加密音乐）**：QQ 音乐 `.mflac` → `.flac`、`.mgg` → `.ogg`

解密后写入 SQLite 播放列表。

## 加密音乐解密集成

导入时输入模块会自动分流：

1. 普通音频（mp3/wav/flac/ogg/m4a）→ 直接写入 SQLite、进入播放列表；
2. 加密音乐（`.mflac`/`.mgg`）→ 弹出**解密弹窗**，通过 QProcess 调用 `qqmusic_des.exe`
   （Frida 注入正在运行的 QQ 音乐进程，调用其解密函数），按文件显示进度，完成后把解密产物写入列表；
3. 无法识别的文件 → 在弹窗中列出失败原因。

解密产物输出到 `%APPDATA%\RaidenShogunPlayer\unlocked\output\`（路径持久化，重启后仍有效）。

**运行前提**：解密时必须**先启动 QQ 音乐客户端**（`qqmusic_des.exe` 需附加到其进程）。导入前会检测
QQ 音乐进程，未运行时在弹窗中提示。

> 说明：该引擎仅覆盖 QQ 音乐的 `.mflac`/`.mgg`。旧的 `music-geshizhuanhuan`（Python，支持
> ncm/kgm/kwm 等）已不再接入；如需保留可作为后续兜底。

## 关于“软解”的说明

本项目的播放引擎目前使用 Qt 内置的 `QMediaPlayer`（`Qt Multimedia` 模块）作为“软解”占位实现，代码量与维护成本最低，倍速通过 `playbackRate` 直接支持。

> 若后续需要**真正的 FFmpeg 软件解码 + PCM 输出**（`QAudioSink`），只需替换 `PlayerController` 的实现（或新增一个实现同一套 Q_INVOKABLE 接口的引擎类），输入模块与 QML 界面无需改动——这正是把播放引擎独立成模块的目的。

## 目录结构

```
├── CMakeLists.txt
├── CMakePresets.json
├── src/
│   ├── main.cpp                    # 装配：实例化两个模块并注册到 QML 上下文
│   ├── input/
│   │   ├── MusicLibrary.h/.cpp     # 输入模块：SQLite 核心 + 导入分流（普通/加密）
│   │   ├── TrackListModel.h/.cpp   # 输入模块：SQLite 查询结果 → QML 列表模型
│   │   └── DecryptService.h/.cpp   # 输入模块：QProcess 调用 qqmusic_des.exe 解密
│   └── player/
│       └── PlayerController.h/.cpp # 播放引擎：QMediaPlayer 封装（播放/暂停/倍速/切歌）
├── qqmusic_des.exe                 # 解密程序（Frida 注入 QQ 音乐进程，源码见 qqmusic_decrypt/）
├── qqmusic_decrypt/                # 解密引擎源码（Rust + Frida，外部项目）
└── qml/
    ├── Main.qml                    # 主窗口
    ├── PlaylistView.qml            # 播放列表 + 导入对话框
    ├── PlayerControls.qml          # 底部控制栏（播放/暂停/倍速/进度/音量）
    └── DecryptDialog.qml           # 解密弹窗（实时进度 + 失败汇总）
```
