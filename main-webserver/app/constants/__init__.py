# A preshared client commit hash that is accepted along with the
# client commit hash corresponding to the active AMI in the database
# in dev environment. Used for enabling client app developers to be able to
# connect to dev environment without updating entries in the development
# database to match their local commit hash values.
# TODO: Move CLIENT_COMMIT_HASH_DEV_OVERRIDE to the mono-repo config 
# as this needs to be a shared secret between client_app and main-webserver.
CLIENT_COMMIT_HASH_DEV_OVERRIDE = "local_dev"
