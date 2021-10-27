import os
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
        os_crypt_name = "chromium"
        if browser_name == "chrome":
            os_crypt_name = browser_name

        my_pass = browser_cookie3.get_linux_pass(os_crypt_name)

    key = PBKDF2(my_pass, salt, iterations=iterations).read(length)

    aes_cbc_encrypt = pyaes.Encrypter(pyaes.AESModeOfOperationCBC(key, iv=iv))

    encoded_value = value.encode("utf-8")
    encrypted_value = aes_cbc_encrypt.feed(encoded_value)
    encrypted_value += aes_cbc_encrypt.feed()

    encrypted_value = encrypt_prefix + encrypted_value

    return encrypted_value


def set_browser_cookies(to_browser_name, cookies):
    """
    Sets cookies from one browser to another
    Args:
        to_browser_name (str): the name of the browser we will import cookies to
        cookies (dict): the cookies being imported
    """
    to_browser = get_browser(to_browser_name)

    cookie_file = to_browser.cookie_file

    encrypted_cookies = []

    for cookie in cookies:
        if cookie["value"] == "":
            value = cookie["decrypted_value"]
            encrypted_prefix = cookie["encrypted_prefix"]
            cookie["encrypted_value"] = encrypt(to_browser_name, value, encrypted_prefix)

        cookie.pop("decrypted_value", None)
        cookie.pop("encryption_prefix", None)

        # We only want the values in a list form
        encrypted_cookies.append(list(cookie.values()))

    con = sqlite3.connect(cookie_file)
    con.text_factory = browser_cookie3.text_factory
    cur = con.cursor()

    try:
        # chrome <=55
        cur.executemany(
            "INSERT INTO cookies (creation_utc, top_frame_site_key, host_key, name, value, encrypted_value, path, expires_utc, secure, is_httponly, last_access_utc, has_expires, is_persistent, priority, samesite, source_scheme, source_port, is_same_party) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            encrypted_cookies,
        )
    except sqlite3.OperationalError:
        # chrome >=56
        cur.executemany(
            "INSERT INTO cookies (creation_utc, top_frame_site_key, host_key, name, value, encrypted_value, path, expires_utc, is_secure, is_httponly, last_access_utc, has_expires, is_persistent, priority, samesite, source_scheme, source_port, is_same_party) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            encrypted_cookies,
        )

    con.commit()
    con.close()


if __name__ == "__main__":
    browser = os.getenv("WHIST_COOKIE_UPLOAD_TARGET")
    cookies = os.getenv("WHIST_INITIAL_USER_COOKIES", None)

    if not (cookies is None) and len(cookies) > 0:
        set_browser_cookies(browser, cookies)
