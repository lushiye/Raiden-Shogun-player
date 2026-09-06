#!/usr/bin/env python3
"""Qt 播放器输入模块的解密桥接。

协议：
  - stdin 读入一行 JSON 请求：
        {"output_dir": "输出目录", "files": ["路径1", "路径2", ...]}
  - stdout 逐行输出 JSON 事件（每行一个对象，实时 flush）：
        {"event":"start",    "total": N}
        {"event":"progress", "index": i, "total": N, "file": "..."}
        {"event":"result",   "type": "plain"|"decrypted"|"error", "input":"...", "path":"...", "note":"..."}
        {"event":"done",     "total": N}
        {"event":"fatal",    "note":"..."}

每个文件自动识别：普通音频（已解密）→ 直接保留原路径；加密音乐 → 解密后输出；
其余 → 报错。进度粒度为“按文件”。
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

# 强制 stdin/stdout 使用 UTF-8，避免 Windows 默认 GBK 导致中文路径/错误乱码
for _stream in (sys.stdin, sys.stdout):
    try:
        _stream.reconfigure(encoding="utf-8")
    except Exception:
        pass

# 保证以脚本所在目录为基准，可直接 import 同级 music_unlock 包
sys.path.insert(0, str(Path(__file__).resolve().parent))

IMPORT_ERROR: str | None = None
SUPPORTED_EXTS: list[str] = []
try:
    from music_unlock.batch import BatchOptions, process_one
    from music_unlock.errors import UnsupportedError
    from music_unlock.formats import pick_decoder, supported_extensions
    SUPPORTED_EXTS = list(supported_extensions())
except Exception as exc:  # 依赖缺失等
    IMPORT_ERROR = f"解密引擎加载失败（需要 Python 3.10+ 与 pycryptodome、mutagen）: {exc}"

# 已解密的普通音频扩展名（与播放器支持的导入格式一致）
PLAIN_EXTS = {".mp3", ".wav", ".flac", ".ogg", ".m4a"}


def emit(obj: dict) -> None:
    sys.stdout.write(json.dumps(obj, ensure_ascii=False) + "\n")
    sys.stdout.flush()


def is_known_encrypted(path: Path) -> bool:
    """按完整文件名结尾匹配已知加密扩展名（含复合如 .kgm.flac / .vpr.flac）。"""
    lower = path.name.lower()
    return any(lower.endswith(ext) for ext in SUPPORTED_EXTS)


def is_plain_audio(path: Path) -> bool:
    return path.suffix.lower() in PLAIN_EXTS


def emit_outcome(path: Path, outcome) -> None:
    if outcome.status == "ok" and outcome.output:
        emit({
            "event": "result", "type": "decrypted",
            "input": str(path), "path": str(outcome.output),
            "note": outcome.note,
        })
    elif outcome.status == "skipped":
        emit({
            "event": "result", "type": "plain",
            "input": str(path), "path": str(outcome.output or path),
            "note": outcome.note,
        })
    else:
        emit({
            "event": "result", "type": "error",
            "input": str(path), "note": outcome.note,
        })


def main() -> int:
    if IMPORT_ERROR:
        emit({"event": "fatal", "note": IMPORT_ERROR})
        return 1

    raw = sys.stdin.read()
    if not raw.strip():
        emit({"event": "fatal", "note": "空输入"})
        return 1

    try:
        req = json.loads(raw)
    except Exception as exc:
        emit({"event": "fatal", "note": f"请求 JSON 解析失败: {exc}"})
        return 1

    output_dir = Path(req.get("output_dir") or "unlocked")
    try:
        output_dir.mkdir(parents=True, exist_ok=True)
    except Exception as exc:
        emit({"event": "fatal", "note": f"无法创建输出目录 {output_dir}: {exc}"})
        return 1

    files = [Path(p) for p in req.get("files", [])]
    total = len(files)
    emit({"event": "start", "total": total})

    opts = BatchOptions(output_dir=output_dir)

    for idx, path in enumerate(files, start=1):
        emit({"event": "progress", "index": idx, "total": total, "file": str(path)})

        # 1) 普通音频（且不是复合加密扩展名）→ 直接保留原路径
        if is_plain_audio(path) and not is_known_encrypted(path):
            if path.is_file():
                emit({"event": "result", "type": "plain",
                      "input": str(path), "path": str(path),
                      "note": "已是普通音频"})
            else:
                emit({"event": "result", "type": "error",
                      "input": str(path), "note": "文件不存在"})
            continue

        # 2) 已知加密扩展名 → 解密
        if is_known_encrypted(path):
            emit_outcome(path, process_one(path, opts))
            continue

        # 3) 未知扩展名 → 魔数嗅探兜底（例如被改名的加密文件）
        try:
            pick_decoder(path)
            encrypted = True
        except Exception:
            encrypted = False

        if encrypted:
            emit_outcome(path, process_one(path, opts))
        else:
            emit({"event": "result", "type": "error",
                  "input": str(path),
                  "note": "无法识别的格式（既非普通音频也非支持的加密格式）"})

    emit({"event": "done", "total": total})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
