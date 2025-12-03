from pathlib import Path
import sys

root = Path(__file__).resolve().parent
build = root / "build"

if build.exists():
    sys.path.insert(0, str(build))

for so in build.rglob("rtse*.so"):
    sys.path.insert(0, str(so.parent))
    break
