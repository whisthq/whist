# -*- coding: utf-8 -*-
import os
import json
import sys
import browser_cookie3
import sqlite3
import pyaes
import subprocess
import sentry_sdk
from pbkdf2 import PBKDF2

# Set in the base Dockerfile
sentry_sdk.init(dsn=os.getenv("SENTRY_DSN"))

USER_CONFIG_PATH = "/whist/userConfigs/"
GNOME_KEYRING_SECRET = b"peanuts"
# Default password is peanuts. Could be due to
# (1) there is no password in the keyring for chrome or
# (2) the keyring is not discoverable by chrome.
# The b"" indicates that it is a bytestring.


def get_gnome_keyring_secret():
    os.seteuid(1000)  # the d-bus is running on `whist`; we need its euid to connect

    my_pass = browser_cookie3.get_linux_pass("chrome")
    if my_pass.decode("utf-8") == "peanuts":  # Misconfiguration of GNOME keyring + Chrome
        print(
            "WARN: could not find the GNOME keyring password for Chrome. Resorting to Chrome default..."
        )
    else:
        print("GNOME keyring password successfully retrieved")

    os.seteuid(0)  # return to `root` euid
    return my_pass


def get_browser_default_dir(browser_name):
    """
    Gets browser default profile dir
    Args:
        browser_name (str): the name of the browser we want the directory of
    Return:
        str: the directories to the browser's default profile
    """
    global USER_CONFIG_PATH

    if browser_name == "chrome":
        browser_default_dir = [
            USER_CONFIG_PATH + "google-chrome/Default/",
            USER_CONFIG_PATH + "google-chrome-beta/Default/",
        ]
    elif browser_name == "chromium":
        browser_default_dir = [USER_CONFIG_PATH + "chromium/Default/"]
    elif browser_name == "opera":
        browser_default_dir = [USER_CONFIG_PATH + "opera/"]
    elif browser_name == "edge":
        browser_default_dir = [
            USER_CONFIG_PATH + "microsoft-edge/Default/",
            USER_CONFIG_PATH + "microsoft-edge-dev/Default/",
        ]
    elif browser_name == "brave":
        browser_default_dir = [
            USER_CONFIG_PATH + "BraveSoftware/Brave-Browser/Default/",
            USER_CONFIG_PATH + "BraveSoftware/Brave-Browser-Beta/Default/",
        ]
    else:
        raise browser_cookie3.BrowserCookieError(
            "Browser not recognized. Works on chrome, chromium, opera, edge, and brave."
        )

    return browser_default_dir


def get_or_create_cookie_file(browser_name, custom_cookie_file_path=None):
    """
    Gets or create file containing all cookies
    Args:
        browser_name (str): the name of the browser we got cookies from
        custom_cookie_file_path (str): [optional] the path to the cookie file
    Return:
        str: the path to cookies
    """

    if sys.platform.startswith("linux"):
        linux_cookies = []
        if custom_cookie_file_path:
            linux_cookies.append(custom_cookie_file_path)
        else:
            linux_cookies = [
                directory + "Cookies" for directory in get_browser_default_dir(browser_name)
            ]

        path = browser_cookie3.expand_paths(linux_cookies, "linux")

        # If not defined then create file
        if not path:
            path = os.path.expanduser(linux_cookies[0])

            # Create directories if it does not exist
            directory = os.path.dirname(path)
            if not os.path.exists(directory):
                os.makedirs(directory)
                os.chmod(directory, 0o777)

            connection = sqlite3.connect(path)
            cursor = connection.cursor()

            cursor.execute(
                """CREATE TABLE IF NOT EXISTS cookies
                    (
                        creation_utc INTEGER NOT NULL,top_frame_site_key TEXT NOT NULL,host_key TEXT NOT NULL,name TEXT NOT NULL,value TEXT NOT NULL,encrypted_value BLOB DEFAULT '',path TEXT NOT NULL,expires_utc INTEGER NOT NULL,is_secure INTEGER NOT NULL,is_httponly INTEGER NOT NULL,last_access_utc INTEGER NOT NULL,has_expires INTEGER NOT NULL DEFAULT 1,is_persistent INTEGER NOT NULL DEFAULT 1,priority INTEGER NOT NULL DEFAULT 1,samesite INTEGER NOT NULL DEFAULT -1,source_scheme INTEGER NOT NULL DEFAULT 0,source_port INTEGER NOT NULL DEFAULT -1,is_same_party INTEGER NOT NULL DEFAULT 0,UNIQUE (top_frame_site_key, host_key, name, path)
                    )"""
            )
            connection.commit()
            connection.close()
            os.chmod(path, 0o777)

        return path

    else:
        raise browser_cookie3.BrowserCookieError("OS not recognized. Works on Linux.")


def encrypt(value):
    """
    Encrypts plain cookie value
    Args:
        value (str): the value that will be encrypted
    Returns:
        str: encrypted string
    """
    global GNOME_KEYRING_SECRET

    salt = b"saltysalt"
    iv = b" " * 16
    length = 16
    iterations = 1

    key = PBKDF2(GNOME_KEYRING_SECRET, salt, iterations=iterations).read(length)
    aes_cbc_encrypt = pyaes.Encrypter(pyaes.AESModeOfOperationCBC(key, iv=iv))

    encoded_value = value.encode("utf-8")
    encrypted_value = aes_cbc_encrypt.feed(encoded_value)
    encrypted_value += aes_cbc_encrypt.feed()

    # Defaulting encryption to v10
    encrypted_value = b"v10" + encrypted_value

    return encrypted_value


def format_chromium_based_cookie(cookie):
    formatted_cookie = [
        cookie["creation_utc"],
        cookie["top_frame_site_key"],
        cookie["host_key"],
        cookie["name"],
        cookie["value"],
        cookie.get("encrypted_value", ""),
        cookie["path"],
        cookie["expires_utc"],
        cookie.get("secure", cookie.get("is_secure", 0)),
        cookie["is_httponly"],
        cookie["last_access_utc"],
        cookie["has_expires"],
        cookie["is_persistent"],
        cookie["priority"],
        cookie["samesite"],
        cookie["source_scheme"],
        cookie["source_port"],
        cookie["is_same_party"],
    ]

    return formatted_cookie


def set_browser_cookies(target_browser_name, cookies_json, target_cookie_file=None):
    """
    Set cookies from file to target browser
    Args:
        target_browser_name (str): the name of the browser we will import cookies to
        cookies_json (str): cookies in json format
        target_cookie_file (str): [optional] path to target browser cookie file
    """
    # This function only supports the targets Brave, Opera, Chrome, and any browser with
    # the same db columns. Otherwise it will not work and error out.
    if (
        target_browser_name != "chrome"
        and target_browser_name != "brave"
        and target_browser_name != "opera"
    ):
        raise ("Unrecognized browser type. Only works for brave, chrome, and opera.")

    cookie_file = get_or_create_cookie_file(target_browser_name, target_cookie_file)

    # Set up database
    con = sqlite3.connect(cookie_file)
    con.text_factory = browser_cookie3.text_factory
    cur = con.cursor()

    cookies = json.loads(cookies_json)

    formatted_cookies = []
    for cookie in cookies:

        # Encrypted_value should not be included with cookiess
        cookie.pop("encrypted_value", None)

        decrypted_value = cookie.get("decrypted_value", None)
        if decrypted_value:
            encrypted_prefix = cookie.get("encrypted_prefix", None)
            cookie["encrypted_value"] = encrypt(decrypted_value)

        cookie.pop("decrypted_value", None)
        cookie.pop("encryption_prefix", None)

        # We only want the values in a list form
        try:
            formatted_cookies.append(format_chromium_based_cookie(cookie))
        except KeyError as err:
            subprocess.run(["echo", f"Cookie failed to format with key err: {err}"])

    try:
        # This is very specific to Chrome/Brave/Opera
        # TODO (aaron): when we add support to more browsers on mandelbox we will need to support diff cookie db columns
        cur.executemany(
            "INSERT OR REPLACE INTO cookies (creation_utc, top_frame_site_key, host_key, name, value, encrypted_value, path, expires_utc, is_secure, is_httponly, last_access_utc, has_expires, is_persistent, priority, samesite, source_scheme, source_port, is_same_party) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            formatted_cookies,
        )
        con.commit()
    except sqlite3.InterfaceError as err:
        subprocess.run(["echo", f"Cookie failed to import with the interface error: {err}"])
    except sqlite3.IntegrityError as err:
        subprocess.run(["echo", f"Cookie failed to import with the integrity error: {err}"])

    con.close()

    # Remove singleton cookie file if exist
    global USER_CONFIG_PATH
    if target_browser_name == "chrome":
        singleton_cookie_file = USER_CONFIG_PATH + "google-chrome/SingletonCookie"
        if os.path.exists(singleton_cookie_file):
            os.remove(singleton_cookie_file)


def create_bookmark_file(target_browser_name, bookmarks_json, custom_bookmark_file_path=None):
    """
    Create bookmarks file for target browser
    Args:
        target_browser_name (str): the name of the browser we will import cookies to
        bookmarks_json (str): bookmarks in json format json
        custom_bookmark_file_path (str): [optional] path to target browser bookmark file
    """
    bookmarks_paths = []
    if custom_bookmark_file_path:
        bookmarks_paths.append(custom_bookmark_file_path)
    else:
        bookmarks_paths = [
            directory + "Bookmarks" for directory in get_browser_default_dir(target_browser_name)
        ]

    path = os.path.expanduser(bookmarks_paths[0])

    # Create directories if it does not exist
    directory = os.path.dirname(path)
    if not os.path.exists(directory):
        os.makedirs(directory)
        os.chmod(directory, 0o777)

    with open(path, "w") as browser_bookmark_file:
        browser_bookmark_file.write(bookmarks_json)


def create_extension_files(extensions, custom_script=None):
    """
    Creates extension files with the help of a given script
    Args:
        extensions (str): comma separated list of extensions in a form of a string
        custom_script (str): [optional] script that helps create extension file
    """
    # Use custom script path if not None
    extension_install_script = custom_script if custom_script else "/usr/bin/install-extension.sh"

    # Install extensions one by one
    for extension in extensions.split(","):
        subprocess.run([extension_install_script, extension])


if __name__ == "__main__":
    """
    The expected use of this function is:

    python3 import_user_browser_data.py <browser target> <user data file>

    """
    GNOME_KEYRING_SECRET = get_gnome_keyring_secret()

    if len(sys.argv) == 3:
        browser = sys.argv[1]
        browser_data_file = sys.argv[2]
        # Get browser data in file
        with open(browser_data_file, "r") as file:
            browser_data = json.load(file)
            if "cookiesJSON" in browser_data and len(browser_data["cookiesJSON"]) > 0:
                set_browser_cookies(browser, browser_data["cookiesJSON"])

            if "bookmarks" in browser_data and len(browser_data["bookmarks"]) > 0:
                create_bookmark_file(browser, browser_data["bookmarks"])

            if "extensions" in browser_data and len(browser_data["extensions"]) > 0:
                create_extension_files(browser_data["extensions"])
