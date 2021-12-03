import requests
import os
import json

# To set your enviornment variables in your terminal run the following line:
# export 'TWITTER_BEARER_TOKEN'='<your_bearer_token>'
bearer_token = os.environ.get("TWITTER_BEARER_TOKEN")

# Only stream Tweets that match the following rules
my_rules = [
    {
        "value": "(chrome slow) OR (chrome laggy) OR (chrome performance) -is:retweet -is:reply lang:en",
        "tag": "1",
    },
    {"value": "(figma slow) OR (figma laggy) -is:retweet -is:reply lang:en", "tag": "2"},
    {
        "value": "(chrome drain battery) OR (chrome too much battery) -is:retweet -is:reply lang:en",
        "tag": "3",
    },
    {"value": "(chrome tabs slow) OR (too many tabs) -is:retweet -is:reply lang:en", "tag": "4"},
    {
        "value": "(chrome ram) OR (chrome too much memory) OR (chrome memory hog) -is:retweet -is:reply lang:en",
        "tag": "5",
    },
    {"value": "(computer fan spin running google chrome) -is:retweet -is:reply lang:en", "tag": 6},
]


def bearer_oauth(r):
    """
    Method required by bearer token authentication.
    """

    r.headers["Authorization"] = f"Bearer {bearer_token}"
    r.headers["User-Agent"] = "v2FilteredStreamPython"
    return r


def get_rules():
    response = requests.get(
        "https://api.twitter.com/2/tweets/search/stream/rules", auth=bearer_oauth
    )
    if response.status_code != 200:
        raise Exception(
            "Cannot get rules (HTTP {}): {}".format(response.status_code, response.text)
        )
    return response.json()


def delete_all_rules(rules):
    if rules is None or "data" not in rules:
        return None

    ids = list(map(lambda rule: rule["id"], rules["data"]))
    payload = {"delete": {"ids": ids}}
    response = requests.post(
        "https://api.twitter.com/2/tweets/search/stream/rules", auth=bearer_oauth, json=payload
    )
    if response.status_code != 200:
        raise Exception(
            "Cannot delete rules (HTTP {}): {}".format(response.status_code, response.text)
        )
    print("Successfully deleted all old rules")


def set_rules(delete):
    payload = {"add": my_rules}
    response = requests.post(
        "https://api.twitter.com/2/tweets/search/stream/rules",
        auth=bearer_oauth,
        json=payload,
    )
    if response.status_code != 201:
        raise Exception(
            "Cannot add rules (HTTP {}): {}".format(response.status_code, response.text)
        )
    print("Successfully created new rules")


def get_stream(set):
    print("Listening for incoming tweets...")
    response = requests.get(
        "https://api.twitter.com/2/tweets/search/stream",
        auth=bearer_oauth,
        stream=True,
    )
    if response.status_code != 200:
        raise Exception(
            "Cannot get stream (HTTP {}): {}".format(response.status_code, response.text)
        )
    for response_line in response.iter_lines():
        if response_line:
            json_response = json.loads(response_line)
            print(json.dumps(json_response, indent=4, sort_keys=True))


def main():
    rules = get_rules()
    delete = delete_all_rules(rules)
    set = set_rules(delete)
    get_stream(set)


if __name__ == "__main__":
    main()
