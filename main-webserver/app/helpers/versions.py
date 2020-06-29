from app.logger import *
from app import *


def setBranchVersion(branch, version, ID=-1):
    """Maps disk branch to client version. Master is latest stable version, staging is second latest version

    Args:
        branch (str): The name of the branch
        version (str): The version number of the client application
    """
    sendInfo(ID, "Settting version for {} to {}".format(branch, version))
    command = text(
        """
        UPDATE versions
        SET version = :version
        WHERE
        "branch" = :branch
        """
    )
    params = {"branch": branch, "version": version}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def getAllVersions(ID=-1):
    """Gets the client versions

    Args:
        ID (int, optional): Papertrail logging id. Defaults to -1.
    """
    command = text(
        """
        SELECT * FROM versions
        """
    )
    params = {}
    with engine.connect() as conn:
        versions = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        result = {}
        for item in versions:
            result[item["branch"]] = item["version"]
        return result
