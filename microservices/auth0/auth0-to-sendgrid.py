#!/usr/bin/env python3
import os
import csv
import time
import gzip
import requests as r

# SENDGRID_SECRET should be a SendGrid API key that has "Full Access".
# AUTH0_SECRET should be the "Global Client Secret" in the Auth0 dashboard.

if not os.environ.get("AUTH0_SECRET"):
    raise Exception("Must set environment variable AUTH0_SECRET.")
if not os.environ.get("SENDGRID_SECRET"):
    raise Exception("Must set environment variable SENDGRID_SECRET.")

# These values are only for the prod stage.
# Dev and staging require different values.
AUTH0_DOMAIN = "fractal-prod.us.auth0.com"
AUTH0_ID = "dnhVmqdHkF1O6aWwakv7jDVMd5Ii6VfX"
AUTH0_SECRET = os.environ["AUTH0_SECRET"]
SENDGRID_SECRET = os.environ["SENDGRID_SECRET"]
SENDGRID_LIST_NAME = "Auth0 Users"


def access_token_request(secret_key):
    return r.post(
        f"https://{AUTH0_DOMAIN}/oauth/token",
        json={
            "grant_type": "client_credentials",
            "client_id": AUTH0_ID,
            "client_secret": secret_key,
            "audience": "https://fractal-prod.us.auth0.com/api/v2/",
        },
    )


def access_token_parse(response):
    return {"access_token": response.json()["access_token"]}


def connections_request(access_token):
    return r.get(
        f"https://{AUTH0_DOMAIN}/api/v2/connections",
        headers={
            "authorization": f"Bearer {access_token}",
        },
    )


def connections_parse(response):
    return {"connection_ids": [c["id"] for c in response.json()]}


def user_export_job_request(access_token, connection_id):
    return r.post(
        f"https://{AUTH0_DOMAIN}/api/v2/jobs/users-exports",
        headers={
            "authorization": f"Bearer {access_token}",
            "content-type": "application/json",
        },
        json={
            "connection_id": connection_id,
            "format": "csv",
        },
    )


def user_export_job_parse(response):
    result = response.json()
    return {"job_id": result["id"], "status": result["status"]}


def job_status_request(access_token, job_id):
    return r.get(
        f"https://{AUTH0_DOMAIN}/api/v2/jobs/{job_id}",
        headers={
            "authorization": f"Bearer {access_token}",
        },
    )


def job_status_parse(response):
    result = response.json()
    return {"status": result["status"], "location": result.get("location")}


def csv_location_request(location):
    return r.get(location)


def csv_location_parse(response):
    return gzip.decompress(response.content).decode()


def csv_string_parse(csv_string):
    reader = csv.DictReader(csv_string.splitlines())
    for row in reader:
        yield {
            "email": row["email"],
            "first_name": row["given_name"],
            "last_name": row["family_name"],
            "verified": row["email_verified"],
        }


def all_auth0_users(access_token, connection_ids):
    for connection_id in connection_ids:
        job_response = user_export_job_request(access_token, connection_id)
        job = user_export_job_parse(job_response)
        job_id = job["job_id"]

        while job["status"] == "pending":
            time.sleep(0.2)
            job = job_status_parse(job_status_request(access_token, job_id))

        csv_str = csv_location_parse(csv_location_request(job["location"]))

        for user_data in csv_string_parse(csv_str):
            yield user_data


def delete_sendgrid_users_request(api_key):
    return r.delete(
        "https://api.sendgrid.com/v3/marketing/contacts",
        headers={
            "authorization": f"Bearer {api_key}",
        },
        params={"delete_all_contacts": "true"},
    )


def delete_sendgrid_users_parse(response):
    return {"job_id": response.json()["job_id"]}


def add_sendgrid_users_request(api_key, list_id, users):
    return r.put(
        "https://api.sendgrid.com/v3/marketing/contacts",
        headers={
            "authorization": f"Bearer {api_key}",
        },
        json={"list_ids": [list_id], "contacts": users},
    )


def sendgrid_lists_request(api_key):
    return r.get(
        "https://api.sendgrid.com/v3/marketing/lists",
        headers={"authorization": f"Bearer {api_key}"},
    )


def sendgrid_lists_parse(response):
    for result in response.json()["result"]:
        yield {
            "name": result["name"],
            "id": result["id"],
            "count": result["contact_count"],
        }


def all_sendgrid_lists(api_key):
    for ls in sendgrid_lists_parse(sendgrid_lists_request(api_key)):
        yield ls


if __name__ == "__main__":
    # Retrieving access token and connections list for Auth0.
    print(f"Retrieving access token and connections list for '{AUTH0_DOMAIN}'...")
    auth0_token = access_token_parse(access_token_request(AUTH0_SECRET))["access_token"]
    auth0_connections = connections_parse(connections_request(auth0_token))["connection_ids"]
    # All Auth0 users from all connections on this tenant.
    print(f"Retrieving Auth0 users from '{AUTH0_DOMAIN}'...")
    users = all_auth0_users(auth0_token, auth0_connections)

    # Delete all the SendGrid users. This is an asynchronous job, and usually
    # takes a few minutes before you can see the result in the dashboard.
    print("Deleting SendGrid users...")
    delete_sendgrid_users_request(SENDGRID_SECRET)

    # Get the SendGrid ID for the target contacts list. We will use the ID for
    # whatever list matches our expected SENDGRID_LIST_NAME.
    print(f"Retrieving SendGrid list ID for '{SENDGRID_LIST_NAME}'...")
    all_lists = all_sendgrid_lists(SENDGRID_SECRET)
    list_id = next(x for x in all_lists if x["name"] == SENDGRID_LIST_NAME)["id"]

    # Load all users into SendGrid contacts list. This is an asynchronous job
    # and usually takes a few minutes before you can see the result in
    # the dashboard.
    print(f"Loading Auth0 users into SendGrid list '{SENDGRID_LIST_NAME}'...")
    results = add_sendgrid_users_request(
        SENDGRID_SECRET,
        list_id,
        [
            {
                "email": u["email"],
                "first_name": u["first_name"],
                "last_name": u["last_name"],
            }
            for u in users
        ],
    )
