import argparse
import boto3

# from botocore.config import Config
# from os import getenv
from datetime import datetime

# possibly something to keep track of the names of the repos if necessary

SECRET_BUCKET = "fractal-dev-secrets"
AWS_ACCESS_KEY_ID = "AWS_ACCESS_KEY_ID"
AWS_SECRET_ACCESS_KEY = "AWS_ACCESS_KEY_ID"
AWS_REGION = "us-east-1"

# local filenames by the types
TYPES_LOCAL = {
    "env" : "../.env"
}

def get_upload_filename(repo, filetype, timestamp=None):
    return "-".join([
        repo,
        filetype,
        timestamp if timestamp else datetime.now().strftime("%Y%m%d")
    ])

def get_latest_filename(repo, filetype, s3):

    file_objects = s3.Bucket(SECRET_BUCKET).objects.all()

    # the latest name in the format repo-filetype-name
    repo, filetype, timestamp = max(
            filter(
                lambda name: name[0] == repo and name[1] == filetype, # such that repo and filetype match
                map(
                    lambda fo: fo.key.split("-"), # only by name
                    file_objects,
                    ),
            ),
            key=lambda name: name[-1], # sorting by the timestamp
        ) # and take filetype and get it's mapped version from TYPES_LOCAL to store as secret correctly

    # remote, local
    return get_upload_filename(repo, filetype, timestamp=timestamp), TYPES_LOCAL[filetype]



if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "--repo",
        help="name of the repository for which to fetch secrets (default : website)",
        default="website",
    )
    parser.add_argument(
        "--type",
        help="name of the type of secrets to fetch (default : env)",
        default="env",
    )
    parser.add_argument(
        "--file",
        help="name of the file to fetch (no default)",
        default="",
    )
    # possibly add date
    # posibly add sort by
    # possibly add option to upload
    args = parser.parse_args()

    # this will check the environment variables automatically
    s3 = boto3.resource("s3")

    # config = Config(
    #     region_name = AWS_REGION,
    #     signature_version = 'v4', # not sure what this is
    #     retries = {
    #         'max_attempts': 10,
    #         'mode': 'standard'
    #     }
    # )

    # s3 = boto3.client(
    #     's3',
    #     aws_access_key_id=getenv(AWS_ACCESS_KEY_ID),
    #     aws_secret_access_key= getenv(AWS_SECRET_ACCESS_KEY)
    # )

    if not args.file:
        remote, local = get_latest_filename(args.repo, args.type, s3)
    else:
        remote, local = args.file, TYPES_LOCAL[args.file.split("-")[1]]
    
    s3.Bucket(SECRET_BUCKET).download_file(remote, local)

    print(f"Downloaded {remote} to {local}.")


    