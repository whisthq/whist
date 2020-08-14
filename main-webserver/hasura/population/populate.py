from sqlalchemy.orm import sessionmaker
import sqlalchemy as db

import os
import random
import string
import time
import sys
import logging
import requests
import os.path
import traceback
import datetime
import pytz
import socket
import sqlalchemy as db
import json
import hashlib, binascii

from jose import jwt


def main():
    OLD_DATABASE_URL = "postgres://ufsj4f8so50sl5:p3a075e5151b02b739c7b3bbbf3f2a1f25b0fbe3fb2e1164ac949b44839ec0e26@ec2-35-168-221-115.compute-1.amazonaws.com:5432/d4lf18ud6qj6nr"
    NEW_DATABASE_URL = "postgres://twszrurldwyufl:9648cb0d7dd6306084a2148273103f00997dce04cb4c898e8dbbb6f4cf06c8ea@ec2-52-72-34-184.compute-1.amazonaws.com:5432/d3a9stgidpj3ig"
    SHA_SECRET_KEY = "ASDF89!#3q4;kaSJF!!ljsa4598@Az)00o!?"

    old_engine = db.create_engine(
        OLD_DATABASE_URL, echo=False, pool_pre_ping=True, pool_recycle=3600
    )
    old_session = sessionmaker(bind=old_engine, autocommit=False)

    new_engine = db.create_engine(
        NEW_DATABASE_URL, echo=False, pool_pre_ping=True, pool_recycle=3600
    )
    new_session = sessionmaker(bind=new_engine, autocommit=False)

    migrate_users()
    migrate_user_vms()
    migrate_os_disks()
    migrate_secondary_disks()
    migrate_install_commands()
    migrate_release_groups()
    migrate_alembic_versions()
    migrate_protocol_logs()
    migrate_monitor_logs()


def hash_value(value):
    dk = hashlib.pbkdf2_hmac(
        "sha256", value.encode("utf-8"), SHA_SECRET_KEY.encode("utf-8"), 100000
    )
    return binascii.hexlify(dk).decode("utf-8")


def check_value(hashed_value, raw_value):
    dk = hashlib.pbkdf2_hmac(
        "sha256", raw_value.encode("utf-8"), SHA_SECRET_KEY.encode("utf-8"), 100000,
    )
    hr = binascii.hexlify(dk).decode("utf-8")
    return hashed_value == hr


def migrate_users():
    print("STARTING TO MIGRATE USER TABLE \n ------------------------ \n")

    session = old_session()

    command = """
        SELECT users.username, users.password, users.code, users.credits_outstanding, users.verified, users.id, users.name, users.reason_for_signup, users.created, users.google_login, customers.id
        FROM users LEFT OUTER JOIN customers on users.username = customers.username
    """

    params = {}

    rows = session.execute(command, params).fetchall()

    session.commit()
    session.close()

    session = new_session()

    for row in rows:
        row = dict(row)

        command = """
            INSERT INTO "users"("email", "name", "password", "using_google_login", "release_stage", "stripe_customer_id", "created_timestamp", "reason_for_signup", "referral_code", "credits_outstanding")
            VALUES(:email, :name, :password, :using_google_login, :release_stage, :stripe_customer_id, :created_timestamp, :reason_for_signup, :referral_code, :credits_outstanding)
        """

        if row["password"] and row["username"]:

            password = jwt.decode(row["password"], SECRET_KEY)

            hashed_password = hash_value(password["pwd"])

            params = {
                "email": row["username"],
                "name": row["name"],
                "password": hashed_password,
                "using_google_login": row["google_login"]
                if row["google_login"]
                else False,
                "release_stage": 50,
                "created_timestamp": 0,
                "reason_for_signup": row["reason_for_signup"],
                "referral_code": row["code"],
                "credits_outstanding": row["credits_outstanding"],
                "stripe_customer_id": row["id"],
            }

            session.execute(command, params)

    session.commit()
    session.close()

    print("DONE MIGRATING USER TABLE \n ------------------------ \n")


def migrate_user_vms():
    print("STARTING TO MIGRATE USER VM TABLE \n ------------------------ \n")

    session = old_session()

    command = """
        SELECT v_ms.vm_name, v_ms.ip, v_ms.location, v_ms.state, v_ms.lock, v_ms.temporary_lock, v_ms.os, v_ms.username 
        FROM v_ms
    """

    params = {}

    rows = session.execute(command, params).fetchall()

    session.commit()
    session.close()

    session = new_session()

    for row in rows:
        row = dict(row)
        user_id = None

        if row["username"]:
            command = """
                SELECT user_id FROM users WHERE "email"=:username
            """
            params = {"username": row["username"]}

            new_row = session.execute(command, params).fetchone()

            if new_row:
                user_id = new_row["user_id"]

        command = """
            INSERT INTO hardware.user_vms("vm_id", "ip", "location", "os", "state", "lock", "temporary_lock", "user_id")
            VALUES(:vm_id, :ip, :location, :os, :state, :lock, :temporary_lock, :user_id)
        """

        params = {
            "vm_id": row["vm_name"],
            "ip": row["ip"],
            "location": row["location"],
            "os": row["os"],
            "state": row["state"],
            "lock": row["lock"],
            "temporary_lock": row["temporary_lock"],
            "user_id": user_id,
        }

        session.execute(command, params)
        print("Migrated vm {vm_name}".format(vm_name=row["vm_name"]))

    session.commit()
    session.close()

    print("DONE MIGRATING USER VM TABLE \n ------------------------ \n")


def migrate_os_disks():
    print("STARTING TO MIGRATE OS DISKS TABLE \n ------------------------ \n")

    session = old_session()

    command = """
        SELECT disks.disk_name, disks.username, disks.location, disks.os, disks.disk_size, v_ms.dev, v_ms.ready_to_connect, disks.version, v_ms.rsa_private_key  
        FROM disks LEFT OUTER JOIN v_ms on disks.username = v_ms.username
        WHERE disks.main=:main
    """

    params = {"main": True}

    rows = session.execute(command, params).fetchall()

    session.commit()
    session.close()

    session = new_session()

    for row in rows:
        row = dict(row)
        user_id = None

        if row["username"]:
            command = """
                SELECT user_id FROM users WHERE "email"=:username
            """
            params = {"username": row["username"]}

            new_row = session.execute(command, params).fetchone()

            if new_row:
                user_id = new_row["user_id"]

        command = """
            INSERT INTO hardware.os_disks("disk_id", "user_id", "location", "os", "disk_size", "allow_autoupdate", "has_dedicated_vm", "last_pinged", "version", "rsa_private_key", "using_stun", "ssh_password")
            VALUES(:disk_id, :user_id, :location, :os, :disk_size, :allow_autoupdate, :has_dedicated_vm, :last_pinged, :version, :rsa_private_key, :using_stun, :ssh_password)
        """

        params = {
            "disk_id": row["disk_name"],
            "user_id": user_id,
            "location": row["location"],
            "os": row["os"],
            "disk_size": row["disk_size"],
            "allow_autoupdate": True,
            "has_dedicated_vm": row["dev"] if row["dev"] else False,
            "last_pinged": row["ready_to_connect"],
            "version": row["version"],
            "rsa_private_key": row["rsa_private_key"],
            "using_stun": False,
            "ssh_password": None,
        }

        session.execute(command, params)
        print("Migrated disk {disk_name}".format(disk_name=row["disk_name"]))

    session.commit()
    session.close()

    print("DONE MIGRATING OS DISKS TABLE \n ------------------------ \n")


def migrate_secondary_disks():
    print("STARTING TO MIGRATE SECONDARY DISKS TABLE \n ------------------------ \n")

    session = old_session()

    command = """
        SELECT disks.disk_name, disks.username, disks.os, disks.disk_size, disks.location
        FROM disks LEFT OUTER JOIN v_ms on disks.username = v_ms.username
        WHERE disks.main=:main
    """

    params = {"main": False}

    rows = session.execute(command, params).fetchall()

    session.commit()
    session.close()

    session = new_session()

    for row in rows:
        row = dict(row)
        user_id = None

        if row["username"]:
            command = """
                SELECT user_id FROM users WHERE "email"=:username
            """
            params = {"username": row["username"]}

            new_row = session.execute(command, params).fetchone()

            if new_row:
                user_id = new_row["user_id"]

        command = """
            INSERT INTO hardware.secondary_disks("disk_id", "user_id", "location", "os", "disk_size")
            VALUES(:disk_id, :user_id, :location, :os, :disk_size)
        """

        params = {
            "disk_id": row["disk_name"],
            "user_id": user_id,
            "location": row["location"],
            "os": row["os"],
            "disk_size": row["disk_size"],
        }

        session.execute(command, params)
        print("Migrated disk {disk_name}".format(disk_name=row["disk_name"]))

    session.commit()
    session.close()

    print("DONE MIGRATING SECONDARY DISKS TABLE \n ------------------------ \n")


def migrate_install_commands():
    OLD_DATABASE_URL = "postgres://vdtioxxnnibmfb:882cc2a76ce1fc35a3aaa41bf3145f42c0357104cf25b896ea7735f03f72c679@ec2-54-156-121-142.compute-1.amazonaws.com:5432/d5jt3jdkhft5hs"
    NEW_DATABASE_URL = "postgres://twszrurldwyufl:9648cb0d7dd6306084a2148273103f00997dce04cb4c898e8dbbb6f4cf06c8ea@ec2-52-72-34-184.compute-1.amazonaws.com:5432/d3a9stgidpj3ig"

    old_engine = db.create_engine(
        OLD_DATABASE_URL, echo=False, pool_pre_ping=True, pool_recycle=3600
    )
    old_session = sessionmaker(bind=old_engine, autocommit=False)

    print("STARTING TO MIGRATE INSTALL COMMANDS TABLE \n ------------------------ \n")

    session = old_session()

    command = """
        SELECT * FROM install_commands
    """

    params = {}

    rows = session.execute(command, params).fetchall()

    session.commit()
    session.close()

    session = new_session()

    for row in rows:
        row = dict(row)

        command = """
            INSERT INTO hardware.install_commands("windows_install_command", "linux_install_command", "app_name")
            VALUES(:windows_install_command, :linux_install_command, :app_name)
        """

        params = {
            "windows_install_command": row["Windows"],
            "linux_install_command": row["Linux"],
            "app_name": row["app_name"],
        }

        session.execute(command, params)

    session.commit()
    session.close()

    print("DONE MIGRATING INSTALL COMMANDS TABLE \n ------------------------ \n")


def migrate_release_groups():
    print("STARTING TO MIGRATE VERSIONS TABLE \n ------------------------ \n")

    session = old_session()

    command = """
        SELECT branch, version
        FROM versions
    """

    params = {}

    rows = session.execute(command, params).fetchall()

    session.commit()
    session.close()

    session = new_session()

    for row in rows:
        row = dict(row)

        command = """
            INSERT INTO "release_groups"("release_stage", "branch")
            VALUES(:release_stage, :branch)
        """

        print("Migrating {stage}".format(stage=row["branch"]))

        params = {
            "release_stage": row["version"],
            "branch": row["branch"],
        }

        session.execute(command, params)

    session.commit()
    session.close()

    print("DONE MIGRATING VERSIONS TABLE \n ------------------------ \n")


def migrate_alembic_versions():
    print("STARTING TO ALEMBIC VERSIONS TABLE \n ------------------------ \n")

    session = old_session()

    command = """
        SELECT version_num
        FROM alembic_version
    """

    params = {}

    rows = session.execute(command, params).fetchall()

    session.commit()
    session.close()

    session = new_session()

    for row in rows:
        row = dict(row)

        command = """
            INSERT INTO devops.alembic_version("version_num")
            VALUES(:num)
        """

        print("Migrating {stage}".format(stage=row["version_num"]))

        params = {"num": row["version_num"]}

        session.execute(command, params)

    session.commit()
    session.close()

    print("DONE MIGRATING ALEMBIC VERSIONS TABLE \n ------------------------ \n")


def migrate_protocol_logs():
    print("STARTING TO MIGRATE PROTOCOL_LOGS TABLE \n ------------------------ \n")

    session = old_session()

    command = """
        SELECT username, last_updated, server_logs, client_logs, connection_id, version
        FROM logs
    """

    params = {}

    rows = session.execute(command, params).fetchall()

    command = """
        SELECT connection_id
        FROM bookmarked_logs
    """

    bookmarks = []
    resp = session.execute(command, params).fetchall()
    for bookmark in resp:
        bookmarks.append((bookmark)[0])

    session.commit()
    session.close()

    session = new_session()

    for row in rows:
        row = dict(row)

        command = """
            INSERT INTO logs.protocol_logs("user_id", "server_logs", "client_logs", "version", "connection_id", "timestamp", "bookmarked")
            VALUES(:user_id, :server_logs, :client_logs, :version, :connection_id, :timestamp, :bookmarked)
        """

        print("Migrating {id}".format(id=row["connection_id"]))

        timestamp = int(
            dt.timestamp(dt.strptime(row["last_updated"], "%m/%d/%Y, %H:%M"))
        )
        version = None
        if row["version"] != "NONE":
            version = row["version"]

        params = {
            "user_id": row["username"],
            "server_logs": row["server_logs"],
            "client_logs": row["client_logs"],
            "version": version,
            "connection_id": row["connection_id"],
            "timestamp": timestamp,
            "bookmarked": row["connection_id"] in bookmarks,
        }

        session.execute(command, params)

    session.commit()
    session.close()

    print("DONE MIGRATING PROTOCOL_LOGS TABLE \n ------------------------ \n")


def migrate_monitor_logs():
    print("STARTING TO MIGRATE MONITOR LOGS TABLE \n ------------------------ \n")

    session = old_session()

    command = """
        SELECT timestamp, logons, logoffs, users_online, eastus_available, southcentralus_available, northcentralus_available, eastus_unavailable, southcentralus_unavailable, northcentralus_unavailable, eastus_deallocated, southcentralus_deallocated, northcentralus_deallocated, total_vms_deallocated
        FROM status_report
    """

    params = {}

    rows = session.execute(command, params).fetchall()

    session.commit()
    session.close()

    session = new_session()

    for row in rows:
        row = dict(row)

        command = """
            INSERT INTO logs.monitor_logs(timestamp, logons, logoffs, users_online, eastus_available, southcentralus_available, northcentralus_available, eastus_unavailable, southcentralus_unavailable, northcentralus_unavailable, eastus_deallocated, southcentralus_deallocated, northcentralus_deallocated, total_vms_deallocated)
            VALUES(:timestamp, :logons, :logoffs, :users_online, :eastus_available, :southcentralus_available, :northcentralus_available, :eastus_unavailable, :southcentralus_unavailable, :northcentralus_unavailable, :eastus_deallocated, :southcentralus_deallocated, :northcentralus_deallocated, :total_vms_deallocated)
        """

        print("Migrating {time}".format(time=row["timestamp"]))

        params = {
            "timestamp": row["timestamp"],
            "logons": row["logons"],
            "logoffs": row["logoffs"],
            "users_online": row["users_online"],
            "eastus_available": row["eastus_available"],
            "southcentralus_available": row["southcentralus_available"],
            "northcentralus_available": row["northcentralus_available"],
            "eastus_unavailable": row["eastus_unavailable"],
            "southcentralus_unavailable": row["southcentralus_unavailable"],
            "northcentralus_unavailable": row["northcentralus_unavailable"],
            "eastus_deallocated": row["eastus_deallocated"],
            "southcentralus_deallocated": row["southcentralus_deallocated"],
            "northcentralus_deallocated": row["northcentralus_deallocated"],
            "total_vms_deallocated": row["total_vms_deallocated"],
        }

        session.execute(command, params)

    session.commit()
    session.close()

    print("DONE MIGRATING MONITOR LOGS TABLE \n ------------------------ \n")


if __name__ == "__main__":
    main()
