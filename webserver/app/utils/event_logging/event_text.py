# this is meant to help us format event text since
# we will need the text to be formatted to do more interesting logs
# (possible in the future it may be better to straight up do like a pickle
# or something - yea funniest thing I've ever seen)

# the format we have picked is a key-value format that is like this:
#
# key1:val1\nkey2:val2:\n...
#
# it is easy to parse -- split by \n,
# this is never going to be present in the timestamp/numerical values
# or names we have, and then split by (:, 1) i.e. the very first colon since the key will not
# have a colon, even if the value might)

RUNTIME = "lifetime"
SPINUP_TIME = "spinup_time"
SHUTDOWN_TIME = "shutdown_time"

MANDELBOX_NAME = "mandelbox_name"
CLUSTER_NAME = "cluster_name"
USER_NAME = "username"

START_DATE = "start_date"
END_DATE = "end_date"

FORMAT_VERSION = "format_version"
CURRENT_FORMAT_VERSION = "1.0"


def format_into_text(key_val_store):
    text = ""
    for key, value in key_val_store.items():
        text += str(key) + ":" + str(value) + "\n"
    text += FORMAT_VERSION + ":" + CURRENT_FORMAT_VERSION
    return text  # ignore the very last \n since we don't care about it


# a wrapper since we might want the original for fancy things later
def to_text(**kwargs):
    return format_into_text(kwargs)
