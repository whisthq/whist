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


def getBranchVersion(branch, ID=-1):
    """Gets the client version corresponding to the given branch

    Args:
        branch (str): Name of the branch
        ID (int, optional): Papertrail logging id. Defaults to -1.
    """
    command = text(
        """
        SELECT * FROM versions WHERE "branch" = :branch
        """
    )
    params = {"branch": branch}
    with engine.connect() as conn:
        version = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        return version["version"]
