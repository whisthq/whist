from app.constants import env_names

PRODUCTION = "production"
STAGING = "staging"
DEVELOPMENT = "dev"  # FIXME switch to development


def from_env(env: str):
    return {
        env_names.PRODUCTION: PRODUCTION,
        env_names.STAGING: STAGING,
        env_names.DEVELOPMENT: DEVELOPMENT,
        env_names.TESTING: DEVELOPMENT,
        env_names.LOCAL: DEVELOPMENT,
    }[env]
