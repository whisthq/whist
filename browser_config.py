import sys
import browser_cookie3

import keyring
import pyaes
from pbkdf2 import PBKDF2


def get_browser_cookies(browser):
    # Consider
    # print("========== Browser_cookies3 ==========") # slight delay
    cookies = list(browser_cookie3.chrome())
    return cookies


# Encryption does not work. Seems like the incorrect encryption will make google chrome flush out cookies.
def set_browser_cookies(cookies):

    linux_cookies = [
        "~/.config/google-chrome/Default/Cookies",
        "~/.config/google-chrome-beta/Default/Cookies",
    ]

    cookie_file = browser_cookie3.expand_paths(linux_cookies, "linux")

    print(sys.platform.startswith("linux"))
    print(cookie_file)

    # Save cookies to sqlite
    import sqlite3

    con = sqlite3.connect(cookie_file)
    con.text_factory = browser_cookie3.text_factory
    cur = con.cursor()

    cur.executemany(
        "INSERT INTO cookies (creation_utc, top_frame_site_key, host_key, name, value, encrypted_value, path, expires_utc, is_secure, is_httponly, last_access_utc, has_expires, is_persistent, priority, samesite, source_scheme, source_port, is_same_party) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
        cookie_tests,
    )
    con.commit()

    con.close()

salt = b"saltysalt"
iv = b" " * 16
print(iv)
length = 16

my_pass = browser_cookie3.get_linux_pass("chrome")

iterations = 1

key = PBKDF2(my_pass, salt, iterations=iterations).read(length)


aes_cbc_encrypt = pyaes.Encrypter(pyaes.AESModeOfOperationCBC(key, iv=iv))
content = "2021-10-06-13".encode("utf-8")
print(content)
encrypted = aes_cbc_encrypt.feed(content)
encrypted += aes_cbc_encrypt.feed()


encrypted = "v11".encode("utf-8") + encrypted
#7631319f8d9c5c88fc0eb3ef18d9d009811dc3
#7631309f8d9c5c88fc0eb3ef18d9d009811dc3

print(encrypted)

# Test cookies
cookie_tests = [
    (
        "13277931465031667",
        "",
        ".google.com",
        "10_JAR",
        "",
        encrypted,
        "/",
        13993742665031667,
        1,
        0,
        13277931465031667,
        1,
        1,
        1,
        0,
        2,
        443,
        0,
    ),
    (
        "13277931465031667",
        "",
        ".google.com",
        "1w_JAR",
        "",
        encrypted,
        "/",
        13993742665031667,
        1,
        0,
        13277931465031667,
        1,
        1,
        1,
        0,
        2,
        443,
        0,
    ),
    (
        "13277931465031667",
        "",
        ".not.google.com",
        "12_JAR",
        "",
        encrypted,
        "/",
        13993742665031667,
        1,
        0,
        13277931465031667,
        1,
        1,
        1,
        0,
        2,
        443,
        0,
    ),
]


set_browser_cookies(cookie_tests)
"""
[Cookie(version=0, name='1P_JAR', value='2021-10-04-22', port=None, port_specified=False, domain='.google.com', domain_specified=True, domain_initial_dot=True, path='/', path_specified=True, secure=1, expires=1635992748, discard=False, comment=None, comment_url=None, rest={}, rfc2109=False), Cookie(version=0, name='NID', value='511=W84d5uX8uid2JG9Ab8G8idXkmZ8qSB782nVZznp1hmS4NIo5LcAvVQ_uBCbk-flEM9astTsjT8_Q3e72yA_TrIbdowjmayHG-ETfO6DMZqKjLoFHD7kZLsojFa9aMfIelnGGxcAr9TWzSsmq6y6iew05tUSgOK0J8dajwOjdvaU', port=None, port_specified=False, domain='.google.com', domain_specified=True, domain_initial_dot=True, path='/', path_specified=True, secure=1, expires=1649211948, discard=False, comment=None, comment_url=None, rest={}, rfc2109=False), Cookie(version=0, name='SNID', value='APx-0P1P1OPxWpT4PGTmwFeTQJzJuLKgqZeV-HXBrPPu_ilPBvh-ABke7zGZY_rGN5riHYE8A0SUZ6Ty1J5hmRmVd9G1', port=None, port_specified=False, domain='.google.com', domain_specified=True, domain_initial_dot=True, path='/verify', path_specified=True, secure=1, expires=1649211947, discard=False, comment=None, comment_url=None, rest={}, rfc2109=False)]
"""

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
