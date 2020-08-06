import os

DATABASE_URL = (
    os.getenv("DATABASE_URL")
    if os.getenv("USE_PRODUCTION_KEYS").upper() == "true"
    else os.getenv("HEROKU_POSTGRESQL_ORANGE_URL")
)

VM_GROUP = (
    "Fractal"
    if os.getenv("USE_PRODUCTION_KEYS").upper() == "true"
    else "FractalStaging"
)

SILENCED_ENDPOINTS = ["/status", "/ping"]
