from bad_words import BAD_WORDS
import hashlib

def hash_bad_words():
    bad_words_hash = {}

    for bad_word in BAD_WORDS:
        bad_words_hash[hashlib.md5(bad_word.encode("utf-8")).hexdigest()] = 0

    return bad_words_hash