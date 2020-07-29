from haikunator import Haikunator
import numpy as np


def genHaiku(n):
    """Generates an array of haiku names (no more than 15 characters) using haikunator

    Args:
        n (int): Length of the array to generate

    Returns:
        arr: An array of haikus
    """
    haikunator = Haikunator()
    haikus = [
        haikunator.haikunate(delimiter="") + str(np.random.randint(0, 10000))
        for _ in range(0, n)
    ]
    haikus = [haiku[0 : np.min([15, len(haiku)])] for haiku in haikus]
    return haikus
