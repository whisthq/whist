import os
import sqlite3
import json
import pytest
import browser_cookie3
from ..utils.import_custom_cookies import *


browserFiles = [
    ["chrome", "~/.config/temp/google-chrome/Default/Cookies"],
    ["chrome", "~/.config/temp/google-chrome-beta/Default/Cookies"],
    ["chromium", "~/.config/temp/chromium/Default/Cookies"],
    ["opera", "~/.config/temp/opera/Cookies"],
    ["edge", "~/.config/temp/microsoft-edge/Default/Cookies"],
    ["edge", "~/.config/temp/microsoft-edge-dev/Default/Cookies"],
    ["brave", "~/.config/temp/BraveSoftware/Brave-Browser/Default/Cookies"],
    ["brave", "~/.config/temp/BraveSoftware/Brave-Browser-Beta/Default/Cookies"],
]


@pytest.mark.parametrize("browser,file_path", browserFiles)
def test_get_existing_cookie_file(browser, file_path):
    file_path = os.path.expanduser(file_path)
    directory = os.path.dirname(file_path)
    if not os.path.exists(directory):
        os.makedirs(directory)

    with open(file_path, "x") as f:
        f.write("Create a Cookie file!")

    file_location = get_or_create_cookie_file(browser, file_path)

    assert file_location == file_path

    os.remove(file_location)


browserFilesCreate = [
    ["chrome", "~/.config/temp/google-chrome/Default/Cookies"],
    ["chromium", "~/.config/temp/chromium/Default/Cookies"],
    ["opera", "~/.config/temp/opera/Cookies"],
    ["edge", "~/.config/temp/microsoft-edge/Default/Cookies"],
    ["brave", "~/.config/temp/BraveSoftware/Brave-Browser/Default/Cookies"],
]


@pytest.mark.parametrize("browser,created_file_path", browserFilesCreate)
def test_create_cookie_file(browser, created_file_path):
    file_location = get_or_create_cookie_file(browser, created_file_path)
    assert file_location == os.path.expanduser(created_file_path)
    os.remove(file_location)


browsers = ["firefox"]


@pytest.mark.parametrize("browser", browsers)
def test_invalid_browser(browser):
    with pytest.raises(browser_cookie3.BrowserCookieError):
        get_or_create_cookie_file(browser)


values_to_encrypt = [
    ["", b"v10+\xbcq{oF\x0c:+\xd0\x02\xdc\xb1\xe4\x8a\x97"],
    ["abc", b"v10\x00\xebn\x0bB\xc5\xe8\xdc\xb7\xbfW\xc6\x11\n\xf1\x8d"],
]


@pytest.mark.parametrize("value_to_encrypt,expected_encrypted_value", values_to_encrypt)
def test_encrypt_value(value_to_encrypt, expected_encrypted_value):
    resulting_encrypted_value = encrypt(value_to_encrypt)
    assert resulting_encrypted_value == expected_encrypted_value


expected_cookie_format_with_secure = [
    12415323,
    "",
    ".whist.com",
    "FRACTAL",
    "",
    "",
    "/",
    1241532300,
    1,
    0,
    12415323,
    1,
    1,
    1,
    -1,
    2,
    443,
    0,
]
cookie_with_secure = {
    "creation_utc": 12415323,
    "top_frame_site_key": "",
    "host_key": ".whist.com",
    "name": "FRACTAL",
    "value": "",
    "path": "/",
    "expires_utc": 1241532300,
    "secure": 1,
    "is_httponly": 0,
    "last_access_utc": 12415323,
    "has_expires": 1,
    "is_persistent": 1,
    "priority": 1,
    "samesite": -1,
    "source_scheme": 2,
    "source_port": 443,
    "is_same_party": 0,
}


expected_cookie_format_with_is_secure = [
    12415323,
    "",
    ".whist2.com",
    "FRACTAL2",
    "",
    "",
    "/",
    1241532300,
    1,
    0,
    12415323,
    1,
    1,
    1,
    -1,
    2,
    443,
    0,
]

cookie_with_is_secure = {
    "creation_utc": 12415323,
    "top_frame_site_key": "",
    "host_key": ".whist2.com",
    "name": "FRACTAL2",
    "value": "",
    "path": "/",
    "expires_utc": 1241532300,
    "is_secure": 1,
    "is_httponly": 0,
    "last_access_utc": 12415323,
    "has_expires": 1,
    "is_persistent": 1,
    "priority": 1,
    "samesite": -1,
    "source_scheme": 2,
    "source_port": 443,
    "is_same_party": 0,
}


cookies = [
    [cookie_with_secure, expected_cookie_format_with_secure],
    [cookie_with_is_secure, expected_cookie_format_with_is_secure],
]


@pytest.mark.parametrize("cookie,expected_format", cookies)
def test_formatting_chromium_based_cookie(cookie, expected_format):
    formatted_cookie = format_chromium_based_cookie(cookie)
    for index, field in enumerate(formatted_cookie):
        assert field == expected_format[index]


cookie_missing_all_fields = {}

browser_cookies = [
    ["chrome", [cookie_with_is_secure], "~/.config/temp/google-chrome/Default/Cookies", 1],
    [
        "chrome",
        [cookie_with_is_secure, cookie_missing_all_fields, cookie_with_secure],
        "~/.config/temp/google-chrome/Default/Cookies",
        2,
    ],
]


@pytest.mark.parametrize("browser,cookies,browser_cookie_path,num_valid_cookies", browser_cookies)
def test_setting_browser_cookies(
    browser, cookies, browser_cookie_path, num_valid_cookies, tmp_path
):
    # Create file to store cookies
    temp_dir = tmp_path / "sub"
    temp_dir.mkdir()
    cookie_json_path = temp_dir / "cookies.txt"
    cookie_json_path.write_text(json.dumps(cookies))

    # Upload cookies
    set_browser_cookies(browser, cookie_json_path, browser_cookie_path)

    # Get cookie file to test
    cookie_file = get_or_create_cookie_file(browser, browser_cookie_path)

    con = sqlite3.connect(cookie_file)
    cur = con.cursor()
    cur.execute("SELECT COUNT(*) FROM cookies")
    numOfRows = cur.fetchone()[0]

    assert numOfRows == num_valid_cookies
    os.remove(cookie_file)


invalid_target_mandelbox_browsers = ["firefox", "safari"]


@pytest.mark.parametrize("target_browser", invalid_target_mandelbox_browsers)
def test_setting_invalid_target_browsers(target_browser):
    with pytest.raises(Exception):
        set_browser_cookies(target_browser, "")
