import os
from hypothesis import settings, HealthCheck

settings.register_profile(
    "dev",
    settings(
        max_examples=50, deadline=None, suppress_health_check=[HealthCheck.too_slow]
    ),
)
settings.register_profile(
    "ci",
    settings(
        max_examples=200, deadline=None, suppress_health_check=[HealthCheck.too_slow]
    ),
)
settings.load_profile(os.getenv("HYPOTHESIS_PROFILE", "dev"))
