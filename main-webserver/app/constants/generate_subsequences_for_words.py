import itertools

def generate_subsequence_for_word(word):
    '''generates all possible subsequences of names or Tokens
    and stores them in an array called result```

    result = []
    combinations = []
    
    for length in range(1, len(word)+1):
        combinations.append(list(itertools.combinations(word, length)))
    for combo in combinations:
        for seq in combo:
            result += [''.join(seq)]

    return result
