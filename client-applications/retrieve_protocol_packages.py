#!/usr/bin/env python3

import argparse
import os
import re
import json
import shutil
import requests

from pathlib import Path

from typing import List, Dict

# this is pygithub, use pip install PyGithub (i.e. not github.py, not github3)
from github import Github
from github.Repository import Repository
from github.GitRelease import GitRelease
from github.GitReleaseAsset import GitReleaseAsset

default_protocol_dir = os.path.join(
    os.path.dirname(os.path.realpath(__file__)), "protocol-packages"
)

""" repo: Repository, desired_release: str -> GitRelease """


def get_release(repo, desired_release):
    all_releases = repo.get_releases()

    print(
        f"Looking for '{desired_release}' in {all_releases.totalCount} releases",
        flush=True,
    )

    version_id_re = re.compile(r"(\S+)-(\d{8})\.(\d+)")  # BRANCH-YYYYMMDD.#

    if "latest:" in desired_release:
        desired_branch = desired_release.split(":")[1]

        valid_release = (
            lambda version_id, release_title: version_id
            and version_id.group(1) == desired_branch
        )

        try:
            return max(
                (
                    release
                    for release in all_releases
                    if valid_release(version_id_re.match(release.title), release.title)
                ),
                key=lambda release: release.published_at,
            )
        except ValueError:
            pass  # we will print that we failed to find anything below, value error should raise on empty generator
    else:
        for release in all_releases:
            if release.title == desired_release:
                return release

            version_id = version_id_re.match(release.title)
            if version_id and version_id.group() == desired_release:
                return release

    all_release_names = "\n".join(
        list(map(lambda release: release.title, all_releases))
    )
    raise Exception(
        f"Unable to find a release for '{desired_release}' in {len(all_release_names)} releases: {all_release_names}"
    )


""" release: GitRelease, platforms: List[str] -> Dict[str, GitReleaseAsset] """


def get_assets_for_platforms(release, platforms):
    matched_assets = {}

    all_assets = release.get_assets()

    for platform in platforms:
        # Trim to support ", " space delimation in addition to "," delimation
        os, flavor = platform.strip().split(":")

        print(
            f"Looking for {os} {flavor} in {all_assets.totalCount} assets", flush=True
        )

        for asset in all_assets:
            # Asset IDs are expected to be in the form `protocol_dev-20200715.2_Windows-64bit_client.tar.gz`
            #
            # Compare against lowercase for caller ease of use since the OS and flavor
            # identifiers are not dependent on case
            asset_name = asset.name.lower()

            if os.lower() in asset_name and flavor.lower() in asset_name:
                print(f"Selected {asset.name} for {os} {flavor}")

                matched_assets[platform] = asset
                break

    return matched_assets


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "--protocol-repo",
        help="name of the protocol repository",
        default="fractalcomputers/protocol",
    )
    parser.add_argument(
        "--platforms",
        help="platforms and client/server flavor to download the protocol for (comma delimited)",
        default="Windows-64bit:client,macOS-64bit:client,Linux-64bit:client",
    )
    parser.add_argument(
        "--release",
        help="which release to download the packages from; either a fully qualified release name or 'latest:branch' which will pick the latest for the branch",
        default="latest:dev",
    )
    parser.add_argument(
        "--out-dir",
        help="where to store the downloaded protocols",
        default=default_protocol_dir,
    )
    parser.add_argument(
        "--wipe-old",
        help="delete older protocol packages in the specified out_dir",
        action="store_true",
        default=False,
    )
    args = parser.parse_args()

    # This should be passed as an env var so that won't be saved
    # in the bash history file (which would happen if it was a CLI arg)
    github_token = os.getenv("GITHUB_TOKEN")
    if not github_token:
        raise Exception(
            "GITHUB_TOKEN was not found in the env vars %s"
            % json.dumps(dict(os.environ), indent=2)
        )
    github_client = Github(github_token)

    release = get_release(github_client.get_repo(args.protocol_repo), args.release)

    print(f"Selected release '{release.title}'", flush=True)

    platforms = args.platforms.split(",")

    assets = get_assets_for_platforms(release, platforms)

    if len(assets) != len(platforms):
        print(
            f"WARNING: Only matched {len(assets)} of {len(platforms)} requested platforms",
            flush=True,
        )

    if args.wipe_old:
        print(f"Clearing out all files currently in '{args.out_dir}'", flush=True)
        shutil.rmtree(args.out_dir, ignore_errors=True)

    Path(args.out_dir).mkdir(parents=True, exist_ok=True)

    for platform, asset in assets.items():
        out_path = os.path.join(args.out_dir, asset.name)

        print(f"Downloading {asset.url}", flush=True)

        with requests.get(
            asset.url,
            auth=requests.auth.HTTPBasicAuth(github_token, ""),
            headers={"Accept": "application/octet-stream"},
            stream=True,
        ) as r:
            r.raise_for_status()

            print(
                f"Asset size = {r.headers.get('Content-Length', 'unknown')} bytes",
                flush=True,
            )

            with open(out_path, "wb") as out:
                shutil.copyfileobj(r.raw, out)

        print(f"Saved '{out_path}'", flush=True)
