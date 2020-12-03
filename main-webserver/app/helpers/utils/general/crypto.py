import binascii
import hashlib

from flask import current_app

from app.helpers.utils.general.logs import fractalLog


def hash_value(value):
    dk = hashlib.pbkdf2_hmac(
        "sha256",
        value.encode("utf-8"),
        current_app.config["SHA_SECRET_KEY"].encode("utf-8"),
        100000,
    )
    return binascii.hexlify(dk).decode("utf-8")


def check_value(hashed_value, raw_value):
    dk = hashlib.pbkdf2_hmac(
        "sha256",
        raw_value.encode("utf-8"),
        current_app.config["SHA_SECRET_KEY"].encode("utf-8"),
        100000,
    )
    hr = binascii.hexlify(dk).decode("utf-8")
    fractalLog(function="", label="", logs=str(hashed_value))
    fractalLog(function="", label="", logs=str(hr))
    return hashed_value == hr
