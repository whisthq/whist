import pytest
from import_user_browser_data import *

browserDefaultDir = [
    [
        "chrome",
        [
            "/whist/userConfigs/google-chrome/Default/",
            "/whist/userConfigs/google-chrome-beta/Default/",
        ],
    ],
    [
        "chromium",
        [
            "/whist/userConfigs/chromium/Default/",
        ],
    ],
    [
        "opera",
        [
            "/whist/userConfigs/opera/",
        ],
    ],
    [
        "edge",
        [
            "/whist/userConfigs/microsoft-edge/Default/",
            "/whist/userConfigs/microsoft-edge-dev/Default/",
        ],
    ],
    [
        "brave",
        [
            "/whist/userConfigs/BraveSoftware/Brave-Browser/Default/",
            "/whist/userConfigs/BraveSoftware/Brave-Browser-Beta/Default/",
        ],
    ],
]


@pytest.mark.parametrize("browser,default_dirs", browserDefaultDir)
def test_get_browser_default_dir(browser, default_dirs):
    dirs = get_browser_default_dir(browser)
    assert len(dirs) == len(default_dirs)
    assert set(dirs).difference(set(default_dirs)) == set()


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
    "WHIST",
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
    "name": "WHIST",
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
    "WHIST2",
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
    "name": "WHIST2",
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
def test_setting_browser_cookies(browser, cookies, browser_cookie_path, num_valid_cookies):
    # Upload cookies
    set_browser_cookies(browser, json.dumps(cookies), browser_cookie_path)

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
def test_setting_invalid_target_browsers_cookies(target_browser):
    with pytest.raises(Exception):
        set_browser_cookies(target_browser, "")


browser_bookmarks = [
    ["chrome", "~/.config/temp/google-chrome/Default/Bookmark"],
]


@pytest.mark.parametrize("browser,browser_bookmark_path", browser_bookmarks)
def test_create_bookmark_file(browser, browser_bookmark_path):

    bookmark_content = {
        "test": "value",
    }

    # Upload bookmarks
    create_bookmark_file(browser, bookmark_content, browser_bookmark_path)

    # Get bookmarks file to test
    bookmark_file = get_or_create_cookie_file(browser, browser_bookmark_path)

    with open(bookmark_file) as b_file:
        assert b_file.read() == '{"test": "value"}'

    os.remove(bookmark_file)


def test_extension_file():
    extension = "not_real_extension"
    create_extension_files(extension, "./tests/install-extension.sh")

    # Check if file is created from script
    assert os.path.isfile(f"./{extension}.json")

    os.remove(f"./{extension}.json")

local_storage_files = [
    [
        "chrome", 
        '{"binary.bin":"AAECAwQFBgcICQoLDA0ODw==","text.txt":"dGVzdDE="}', 
        "~/.config/temp/google-chrome/Default/Local Storage/leveldb",
        {
            "text.txt": b"test1",
            "binary.bin": b"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f",
        }
    ],
]

@pytest.mark.parametrize("browser,local_storage_json,local_storage_path,expected_files", local_storage_files)
def test_local_storage_file(browser, local_storage_json, local_storage_path, expected_files):
    # Set up a test file in the directory to start
    local_storage_path = os.path.expanduser(local_storage_path)
    os.makedirs(local_storage_path, exist_ok=True)
    with open(os.path.join(local_storage_path, "test"), "w") as test_file:
        test_file.write("test")

    create_local_storage_files(browser, local_storage_json, local_storage_path)

    # Check that the correct directory exists
    assert os.path.isdir(local_storage_path)

    # Check that the test file is no longer there
    assert not os.path.isfile(os.path.join(local_storage_path, "test"))

    # Check that the correct files are there with the correct contents
    for file_name, file_content in expected_files.items():
        assert os.path.isfile(os.path.join(local_storage_path, file_name))
        with open(os.path.join(local_storage_path, file_name), "rb") as file:
            assert file.read() == file_content
