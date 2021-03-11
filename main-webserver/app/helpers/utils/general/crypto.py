import binascii
import hashlib

from flask import current_app


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
    return hashed_value == hex_raw
