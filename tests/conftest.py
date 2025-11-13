import os
import sys
from pathlib import Path
from hypothesis import settings, HealthCheck

settings.register_profile(
    "ci",
    settings(max_examples=50, deadline=None,
             suppress_health_check=[HealthCheck.too_slow]),
)
settings.register_profile(
    "dev", 
    settings(max_examples=200, deadline=None,
             suppress_health_check=[HealthCheck.too_slow]),
)
settings.load_profile(os.getenv("HYPOTHESIS_PROFILE", "dev"))

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