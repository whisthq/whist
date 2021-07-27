# JQ filter to merge stdin with a patchfile in $patch
# Usage: jq -f merge.jq --slurpfile patch patch.json

# Iterate over keys in the input object
reduce keys[] as $key (
    # Initialize with the input object
    .;
    # If this key is for an array, concatenate the arrays
    if (.[$key] | type == "array") then
        .[$key] += $patch[0][$key]
    # If this key is for an object, naively merge the objects
    elif (.[$key] | type == "object") then
        .[$key] *= $patch[0][$key]
    # If this key is for anything else, overwrite on non-null
    else
        .[$key] |= ($patch[0][$key] // .)
    end
)
