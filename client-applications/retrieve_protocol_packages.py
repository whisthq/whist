#!/usr/bin/env python3
import argparse
import os
import re
import json
import shutil
from pathlib import Path
from typing import List, Dict
import requests
import github

default_protocol_dir = os.path.join(
    os.path.dirname(os.path.realpath(__file__)), "protocol-packages"
)


def get_release(
    repo: github.Repository.Repository, desired_release: str
) -> github.GitRelease.GitRelease:
    all_releases = repo.get_releases()
    print(f"Looking for '{desired_release}' in {all_releases.totalCount} releases")
    version_id_re = re.compile(r"(\S+)-(\d{8})\.(\d+)")  # BRANCH-YYYYMMDD.#
    if "latest:" in desired_release:
        desired_branch = desired_release.split(":")[1]
        valid_releases = []
        for release in all_releases:
            version_id = version_id_re.match(release.title)
            if version_id and version_id.group(1) == desired_branch:
                valid_releases.append(release)
        valid_releases.sort(key=lambda r: release.published_at, reverse=True)
        if len(valid_releases) > 0:
            return valid_releases[0]
    else:
        for release in all_releases:
            if release.title == desired_release:
                return release
            version_id = version_id_re.match(release.title)
            if version_id and version_id.group() == desired_release:
                return release
    all_release_names = "\n".join(list(map(lambda r: r.title, all_releases)))
    raise Exception(
        f"Unable to find a release for '{desired_release}' in {len(all_release_names)} releases: {all_release_names}"
    )


def get_assets_for_platforms(
    release: github.GitRelease.GitRelease, platforms: List[str]
) -> Dict[str, github.GitReleaseAsset.GitReleaseAsset]:
    matched_assets = {}
    all_assets = release.get_assets()
    for platform in platforms:
        # Trim to support ", " space delimation in addition to "," delimation
        os, flavor = platform.strip().split(":")
        print(f"Looking for {os} {flavor} in {all_assets.totalCount} assets")
        for asset in all_assets:
            # Asset IDs are expected to be in the form `protocol_dev-20200715.2_Windows-64bit_client.tar.gz`
            #
            # Compare against lowercase for caller ease of use since the OS and flavor
            # identifiers are not dependent on case
            if (
                os.lower() in asset.name.lower()
                and flavor.lower() in asset.name.lower()
            ):
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
        default=False
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
    github_client = github.Github(github_token)

    release = get_release(github_client.get_repo(args.protocol_repo), args.release)
    print(f"Selected release '{release.title}'")
    platforms = args.platforms.split(",")
    assets = get_assets_for_platforms(release, platforms)
    if len(assets) != len(platforms):
        print(
            f"WARNING: Only matched {len(assets)} of {len(platforms)} requested platforms"
        )
    if args.wipe_old:
        print(f"Clearing out all files currently in '{args.out_dir}'")
        shutil.rmtree(args.out_dir, ignore_errors=True)
    Path(args.out_dir).mkdir(parents=True, exist_ok=True)
    for platform, asset in assets.items():
        out_path = os.path.join(args.out_dir, asset.name)
        print(f"Downloading {asset.url}")
        with requests.get(
            asset.url,
            auth=requests.auth.HTTPBasicAuth(github_token, ""),
            headers={"Accept": "application/octet-stream"},
            stream=True,
        ) as r:
            r.raise_for_status()
            print(f"Asset size = {r.headers.get('Content-Length', 'unknown')} bytes")
            with open(out_path, "wb") as out:
                shutil.copyfileobj(r.raw, out)
        print(f"Saved '{out_path}'")
