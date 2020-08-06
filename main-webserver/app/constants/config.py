import os

DATABASE_URL = (
    os.getenv("DATABASE_URL")
    if os.getenv("USE_PRODUCTION_KEYS").upper() == "true"
    else os.getenv("STAGING_DATABASE_URL")
)

VM_GROUP = (
    "Fractal"
    if os.getenv("USE_PRODUCTION_KEYS").upper() == "true"
    else "FractalStaging"
)

STRIPE_SECRET = (
    os.getenv("STRIPE_LIVE_SECRET")
    if os.getenv("USE_PRODUCTION_KEYS").upper() == "true"
    else os.getenv("STRIPE_STAGING_SECRET")
)

SILENCED_ENDPOINTS = ["/status", "/ping"]
