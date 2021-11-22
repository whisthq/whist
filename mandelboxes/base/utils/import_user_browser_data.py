import os
import json
import sys
import browser_cookie3
import sqlite3
import pyaes
from pbkdf2 import PBKDF2
import subprocess

USER_CONFIG_PATH = "/fractal/userConfigs/"


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
    salt = b"saltysalt"
    iv = b" " * 16
    length = 16
    iterations = 1

    # will assume it's linux for now
    # We will hardcode the my_pass for now as GNOME-Keyring is not properly configured
    # and will result in the default value `peanuts`

    my_pass = b"peanuts"

    key = PBKDF2(my_pass, salt, iterations=iterations).read(length)

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


def set_browser_cookies(target_browser_name, cookie_full_path, target_cookie_file=None):
    """
    Set cookies from file to target browser
    Args:
        target_browser_name (str): the name of the browser we will import cookies to
        cookie_full_path (str): path to file containing cookie json
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

    with open(cookie_full_path, "r") as f:
        cookies_json = f.read()
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


def create_bookmark_file(target_browser_name, temp_bookmark_path, custom_bookmark_file_path=None):
    """
    Create bookmarks file for target browser
    Args:
        target_browser_name (str): the name of the browser we will import cookies to
        temp_bookmark_path (str): path to file containing bookmark json
        custom_bookmark_file_path (str): [optional] path to target browser bookmark file
    """
    # This function only supports the targets Brave, Opera, Chrome, and any browser with
    # the same db columns. Otherwise it will not work and error out.
    if (
        target_browser_name != "chrome"
        and target_browser_name != "brave"
        and target_browser_name != "opera"
    ):
        raise ("Unrecognized browser type. Only works for brave, chrome, and opera.")

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

    with open(temp_bookmark_path, "r") as temp_bookmark_file:
        bookmarks = temp_bookmark_file.read()
        with open(path, "w") as browser_bookmark_file:
            browser_bookmark_file.write(json.loads(bookmarks))


if __name__ == "__main__":
    browser = os.getenv("WHIST_COOKIE_UPLOAD_TARGET", None)
    cookie_full_path = os.getenv("WHIST_INITIAL_USER_COOKIES_FILE", None)
    bookmark_full_path = os.getenv("WHIST_INITIAL_USER_BOOKMARKS_FILE", None)

    if browser:
        if cookie_full_path and len(cookie_full_path) > 0 and os.path.exists(cookie_full_path):
            set_browser_cookies(browser, cookie_full_path)

        if (
            bookmark_full_path
            and len(bookmark_full_path) > 0
            and os.path.exists(bookmark_full_path)
        ):
            create_bookmark_file(browser, bookmark_full_path)
