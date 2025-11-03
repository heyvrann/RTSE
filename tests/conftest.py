import sys
from pathlib import Path

def pytest_sessionstart(session):
    root = Path(__file__).resolve().parents[1]
    candidates = [
        root/"build",
        root/"build/Debug",
        root/"build/Release",
    ]
    so_parents = {p.parent for p in (root/"build").glob("**/rtse*.so")}
    for p in candidates + list(so_parents):
        if p.exists():
            sys.path.insert(0, str(p))