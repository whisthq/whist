from os import error
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
    # elif browser_name == "firefox":
    #     browser = browser_cookie3.Firefox()
    else:
        raise ("unknown browser name")


def get_browser_cookies(browser_name):
    """
    Gets all cookie information for a given browser

    Args:
        browser_name (str): the name of the browser

    Returns:
        arr: all field values for the cookies
    """
    browser = get_browser(browser_name)  # deal with error?

    con = sqlite3.connect(browser.tmp_cookie_file)
    con.text_factory = browser_cookie3.text_factory
    con.row_factory = lambda cursor, row: list(row)
    cur = con.cursor()

    # This query works for chrome specifically and needs testings for the rest
    # We need a different query for firefox:
    # https://github.com/borisbabic/browser_cookie3/blob/master/__init__.py#L572
    try:
        # chrome <=55
        cur.execute(
            "SELECT creation_utc, top_frame_site_key, host_key, name, value, "
            "encrypted_value, path, expires_utc, secure, is_httponly, last_access_utc, "
            "has_expires, is_persistent, priority, samesite, source_scheme, source_port, "
            "is_same_party FROM cookies;"
        )
    except sqlite3.OperationalError:
        # chrome >=56
        cur.execute(
            "SELECT creation_utc, top_frame_site_key, host_key, name, value, "
            "encrypted_value, path, expires_utc, is_secure, is_httponly, last_access_utc, "
            "has_expires, is_persistent, priority, samesite, source_scheme, source_port, "
            "is_same_party FROM cookies;"
        )

    cookies_data = cur.fetchall()
    con.close()

    return cookies_data


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

    my_pass = browser_cookie3.get_linux_pass(browser_name)
    key = PBKDF2(my_pass, salt, iterations=iterations).read(length)
    aes_cbc_encrypt = pyaes.Encrypter(pyaes.AESModeOfOperationCBC(key, iv=iv))

    encoded_value = value.encode("utf-8")
    encrypted_value = aes_cbc_encrypt.feed(encoded_value)
    encrypted_value += aes_cbc_encrypt.feed()

    encrypted_value = encrypt_prefix + encrypted_value

    return encrypted_value


def get_cookie_file(browser_name):
    """
    Gets file containing all cookies

    Args:
        browser_name (str): the name of the browser we got cookies from

    Return:
        str: the path to cookies
    """

    if sys.platform == "darwin":
        osx_cookies = []
        if browser_name == "chrome":
            osx_cookies = ["~/Library/Application Support/Google/Chrome/Default/Cookies"]
        elif browser_name == "chromium":
            osx_cookies = ["~/Library/Application Support/Chromium/Default/Cookies"]
        elif browser_name == "opera":
            osx_cookies = ["~/Library/Application Support/com.operasoftware.Opera/Cookies"]
        elif browser_name == "edge":
            osx_cookies = ["~/Library/Application Support/Microsoft Edge/Default/Cookies"]
        return browser_cookie3.expand_paths(osx_cookies, "osx")

    elif sys.platform.startswith("linux"):
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
        return browser_cookie3.expand_paths(linux_cookies, "linux")

    else:
        raise browser_cookie3.BrowserCookieError("OS not recognized. Works on OSX and Linux.")


def set_browser_cookies(browser_name, cookies):
    """
    Sets cookies to a given browser

    Args:
        browser_name (str): the name of the browser we got cookies from
        cookies (arr): all information about the browser cookies
    """
    browser = get_browser(browser_name)  # deal with error?

    # Decrypt old cookie 'encrypted_values' and re-encrypt it
    for cookie in cookies:
        # Only decrypt if value is not empty
        if not cookie[4]:
            decrypted_value = browser._decrypt(cookie[4], cookie[5])
            cookie[5] = encrypt(browser_name, decrypted_value, cookie[5][:3])

    cookie_file = browser.cookie_file
    if cookie_file is None:
        cookie_file = get_cookie_file(browser_name)

    con = sqlite3.connect(cookie_file)
    con.text_factory = browser_cookie3.text_factory
    cur = con.cursor()

    try:
        # chrome <=55
        cur.executemany(
            "INSERT INTO cookies (creation_utc, top_frame_site_key, host_key, name, value, encrypted_value, path, expires_utc, secure, is_httponly, last_access_utc, has_expires, is_persistent, priority, samesite, source_scheme, source_port, is_same_party) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            cookies,
        )
    except sqlite3.OperationalError:
        # chrome >=56
        cur.executemany(
            "INSERT INTO cookies (creation_utc, top_frame_site_key, host_key, name, value, encrypted_value, path, expires_utc, is_secure, is_httponly, last_access_utc, has_expires, is_persistent, priority, samesite, source_scheme, source_port, is_same_party) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            cookies,
        )

    con.commit()
    con.close()

# browser_history.browsers import Chrome


# print("========== Browser_History ==========")
# f = Chrome()
# outputs = f.fetch_history()

# his = outputs.histories
# from
# print(his)


# def load_cookies(cookies):


# def load(self):
#     con = sqlite3.connect(self.tmp_cookie_file)
#     cur = con.cursor()
#     # perform insert command instead
#     cur.execute('select host, path, isSecure, expiry, name, value from moz_cookies '
#                 'where host like "%{}%"'.format(self.domain_name))

#     cj = http.cookiejar.CookieJar()
#     for item in cur.fetchall():
#         c = create_cookie(*item)
#         cj.set_cookie(c)
#     con.close()

#     self.__add_session_cookies(cj)
#     self.__add_session_cookies_lz4(cj)

#     return cj
