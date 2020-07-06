from app.imports import *

from cryptography.hazmat.primitives import serialization as crypto_serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.backends import default_backend as crypto_default_backend

import hashlib, binascii

def genPrivateKey():
    key = rsa.generate_private_key(
        backend=crypto_default_backend(), public_exponent=65537, key_size=2048
    )
    private_key = key.private_bytes(
        crypto_serialization.Encoding.PEM,
        crypto_serialization.PrivateFormat.PKCS8,
        crypto_serialization.NoEncryption(),
    ).decode("utf-8")

    return private_key

def hash_value(value):
    dk = hashlib.pbkdf2_hmac('sha256', value.encode('utf-8'), os.getenv('SECRET_KEY').encode('utf-8'), 100000)
    return binascii.hexlify(dk).decode('utf-8')

def check_value(hashed_value, raw_value):
    dk = hashlib.pbkdf2_hmac('sha256', raw_value.encode('utf-8'), os.getenv('SECRET_KEY').encode('utf-8'), 100000)
    hr = binascii.hexlify(dk).decode('utf-8')
    return hashed_value == hr
