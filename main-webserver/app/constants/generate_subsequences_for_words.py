import itertools

def generate_subsequence_for_word(word):
    result = []
    combinations = []
    
    for length in range(1, len(word)+1):
        combinations.append(list(itertools.combinations(word, length)))
    for combo in combinations:
        for seq in combo:
            result += [''.join(seq)]

    return result