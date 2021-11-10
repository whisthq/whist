import os
import json
import sys
import browser_cookie3
import sqlite3
import keyring
import pyaes
from pbkdf2 import PBKDF2


def get_browser(browser_name):
    if browser_name == "chrome":
        return browser_cookie3.Chrome()
    elif browser_name == "chromium":
        return browser_cookie3.Chromium()
    elif browser_name == "opera":
        return browser_cookie3.Opera()
    elif browser_name == "edge":
        return browser_cookie3.Edge()
    elif browser_name == "brave":
        return browser_cookie3.Brave()
    # elif browser_name == "firefox":
    #     browser = browser_cookie3.Firefox()
    else:
        raise ("unknown browser name")


def get_or_create_cookie_file(browser_name):
    """
    Gets or create file containing all cookies
    Args:
        browser_name (str): the name of the browser we got cookies from
    Return:
        str: the path to cookies
    """

    if sys.platform.startswith("linux"):
        linux_cookies = []
        if browser_name == "chrome":
            linux_cookies = [
                "~/.config/google-chrome/Default/Cookies",
                "~/.config/google-chrome-beta/Default/Cookies",
            ]
        elif browser_name == "chromium":
            linux_cookies = ["~/.config/chromium/Default/Cookies"]
        elif browser_name == "opera":
            linux_cookies = ["~/.config/opera/Cookies"]
        elif browser_name == "edge":
            linux_cookies = [
                "~/.config/microsoft-edge/Default/Cookies",
                "~/.config/microsoft-edge-dev/Default/Cookies",
            ]
        elif browser_name == "brave":
            linux_cookies = [
                "~/.config/BraveSoftware/Brave-Browser/Default/Cookies",
                "~/.config/BraveSoftware/Brave-Browser-Beta/Default/Cookies",
            ]

        print(linux_cookies)
        paths = linux_cookies
        paths = list(map(os.path.expanduser, paths))

        path = browser_cookie3.expand_paths(linux_cookies, "linux")

        # If not defined then create file
        if not path:

            # Create directories if it does not exist
            directory = os.path.dirname(paths[0])
            if not os.path.exists(directory):
                os.makedirs(directory)

            connection = sqlite3.connect(paths[0])
            cursor = connection.cursor()

            cursor.execute(
                """CREATE TABLE IF NOT EXISTS cookies
                    (
                        creation_utc INTEGER NOT NULL,top_frame_site_key TEXT NOT NULL,host_key TEXT NOT NULL,name TEXT NOT NULL,value TEXT NOT NULL,encrypted_value BLOB DEFAULT '',path TEXT NOT NULL,expires_utc INTEGER NOT NULL,is_secure INTEGER NOT NULL,is_httponly INTEGER NOT NULL,last_access_utc INTEGER NOT NULL,has_expires INTEGER NOT NULL DEFAULT 1,is_persistent INTEGER NOT NULL DEFAULT 1,priority INTEGER NOT NULL DEFAULT 1,samesite INTEGER NOT NULL DEFAULT -1,source_scheme INTEGER NOT NULL DEFAULT 0,source_port INTEGER NOT NULL DEFAULT -1,is_same_party INTEGER NOT NULL DEFAULT 0,UNIQUE (top_frame_site_key, host_key, name, path)
                    )"""
            )
            connection.commit()
            connection.close()
            path = paths[0]

        return path

    else:
        raise browser_cookie3.BrowserCookieError("OS not recognized. Works on OSX and Linux.")


get_or_create_cookie_file("chrome")


def encrypt(browser_name, value, encrypt_prefix):
    """
    Encrypt value based on the prefix
    Args:
        browser_name (str): the name of the browser we got cookies from
        value (str): the value that will be encrypted
        encrypt_prefix (str): prefix that is v10 or v11
    Returns:
        str: encrypted string
    """
    salt = b"saltysalt"
    iv = b" " * 16
    length = 16
    iterations = 1

    if sys.platform == "darwin":
        if browser_name == "chrome":
            osx_key_service = ("Chrome Safe Storage",)
            osx_key_user = "Chrome"
        elif browser_name == "chromium":
            osx_key_service = ("Chromium Safe Storage",)
            osx_key_user = "Chromium"
        elif browser_name == "opera":
            osx_key_service = ("Opera Safe Storage",)
            osx_key_user = "Opera"
        elif browser_name == "edge":
            osx_key_service = ("Microsoft Edge Safe Storage",)
            osx_key_user = "Microsoft Edge"
        my_pass = keyring.get_password(osx_key_service, osx_key_user)
    else:  # will assume it's linux for now
        # We will hardcode the my_pass for now as GNOME-Keyring is not properly configured
        # and will result in the default value `peanuts`

        # os_crypt_name = "chromium"
        # if browser_name == "chrome":
        #     os_crypt_name = browser_name
        # my_pass = browser_cookie3.get_linux_pass(os_crypt_name)

        my_pass = b"peanuts"

    key = PBKDF2(my_pass, salt, iterations=iterations).read(length)

    aes_cbc_encrypt = pyaes.Encrypter(pyaes.AESModeOfOperationCBC(key, iv=iv))

    encoded_value = value.encode("utf-8")
    encrypted_value = aes_cbc_encrypt.feed(encoded_value)
    encrypted_value += aes_cbc_encrypt.feed()

    # Defaulting encryption to v10
    encrypted_value = b"v10" + encrypted_value
    # encrypted_value = encrypt_prefix.encode("utf-8") + encrypted_value

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
        cookie.get("secure", cookie.get("is_secure", False)),
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


def set_browser_cookies(to_browser_name, cookie_full_path):
    """
    Sets cookies from one browser to another
    Args:
        to_browser_name (str): the name of the browser we will import cookies to
        cookie_full_path (str): path to cookie file
    """
    cookie_file = get_or_create_cookie_file(to_browser_name)

    con = sqlite3.connect(cookie_file)
    con.text_factory = browser_cookie3.text_factory

    with open(cookie_full_path, "r") as f:
        for cookie_line in f:
            cookie_json = cookie_line.rstrip("\n")
            cookie = json.loads(cookie_json)

            if cookie["value"] == "":
                decrypted_value = cookie.get("decrypted_value", None)

                # Encrypted_value should not be included with cookiess
                cookie.pop("encrypted_value", None)

                # Skip encryption if decrypted value does not exist
                if decrypted_value:
                    encrypted_prefix = cookie["encrypted_prefix"]
                    cookie["encrypted_value"] = encrypt(
                        to_browser_name, decrypted_value, encrypted_prefix
                    )

            cookie.pop("decrypted_value", None)
            cookie.pop("encryption_prefix", None)

            # We only want the values in a list form
            formatted_cookie = format_chromium_based_cookie(cookie)

            cur = con.cursor()

            cur.execute(
                "INSERT INTO cookies (creation_utc, top_frame_site_key, host_key, name, value, encrypted_value, path, expires_utc, is_secure, is_httponly, last_access_utc, has_expires, is_persistent, priority, samesite, source_scheme, source_port, is_same_party) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                formatted_cookie,
            )

            con.commit()

    con.close()


if __name__ == "__main__":
    browser = os.getenv("WHIST_COOKIE_UPLOAD_TARGET")
    cookie_full_path = os.getenv("WHIST_INITIAL_USER_COOKIES_FILE", None)

    if os.path.exists(cookie_full_path):
        set_browser_cookies(browser, cookie_full_path)
