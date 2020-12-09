import requests


def get_loc_from_ip(ip):
    # necessary for testing, the endpoint will return "Not Found" otherwise for longitude etc
    if ip[0:4] == "172.":
        return "us-east-1"

    response = requests.get("https://geolocation-db.com/json/{}&position=true".format(ip)).json()
    long = abs(response["longitude"])
    if long > 90:
        return "us-west-1"
    else:
        return "us-east-1"


if __name__ == "__main__":
    print(get_loc_from_ip("73.167.118.123"))
    print(get_loc_from_ip("149.142.201.252"))
    print(get_loc_from_ip("172.18.0.1"))
