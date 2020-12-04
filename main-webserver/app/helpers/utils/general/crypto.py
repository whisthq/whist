import binascii
import hashlib

from flask import current_app

from app.helpers.utils.general.logs import fractal_log


def hash_value(value):
    decimal_key = hashlib.pbkdf2_hmac(
        "sha256",
        value.encode("utf-8"),
        current_app.config["SHA_SECRET_KEY"].encode("utf-8"),
        100000,
    )
    return binascii.hexlify(decimal_key).decode("utf-8")


def check_value(hashed_value, raw_value):
    decimal_key = hashlib.pbkdf2_hmac(
        "sha256",
        raw_value.encode("utf-8"),
        current_app.config["SHA_SECRET_KEY"].encode("utf-8"),
        100000,
    )
    hex_raw = binascii.hexlify(decimal_key).decode("utf-8")
    fractal_log(function="", label="", logs=str(hashed_value))
    fractal_log(function="", label="", logs=str(hex_raw))
    return hashed_value == hex_raw
