#!/usr/bin/env python3
import argparse
import os
import platform
import re
import json
import shutil
import paramiko
from scpclient import closing, Write
from scp import SCPClient
from pathlib import Path
from typing import List, Dict
import requests
import github
import subprocess

GITHUB_TOKEN = '3dd175ed9ef231590537b9401a60edd9b9dca475'

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


def get_server_asset_from_release(
    release: github.GitRelease.GitRelease
) -> github.GitReleaseAsset.GitReleaseAsset:
    all_assets = release.get_assets()
    os, flavor = "Windows-64bit", "server"
    for asset in all_assets:
        # Asset IDs are expected to be in the form `protocol_dev-20200715.2_Windows-64bit_client.tar.gz`
        #
        # Compare against lowercase for caller ease of use since the OS and flavor
        # identifiers are not dependent on case
        if (
            os.lower() in asset.name.lower()
            and flavor.lower() in asset.name.lower()
        ):
            return asset
    raise Exception(
        f"Unable to find sever asset for '{release}'!"
    )

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "--vm-ip",
        required=True,
        help="IP address of VM to test on",
    )
    parser.add_argument(
        "--vm-user",
        help="user for VM to test on",
        default="Fractal",
    )
    parser.add_argument(
        "--ssh-key",
        help="path to SSH key pub file, should end in .pub",
        default=f'desktop/build64/{platform.system()}/sshkey.pub',
    )
    parser.add_argument(
        "--release",
        help="which release to download the packages from; either a fully qualified release name or 'latest:branch' which will pick the latest for the branch",
        default="latest:dev",
    )
    parser.add_argument(
        "--protocol-repo",
        help="name of the protocol repository",
        default="fractalcomputers/protocol",
    )
    parser.add_argument(
        "--out-dir",
        help="where to store the downloaded protocol in the VM",
        default=default_protocol_dir,
    )
    parser.add_argument(
        "--wipe-old",
        help="delete older protocol packages in the specified out_dir",
        action="store_true",
        default=False
    )
    args = parser.parse_args()

    # github_client = github.Github(GITHUB_TOKEN)
    # release = get_release(github_client.get_repo(args.protocol_repo), args.release)
    # print(f"Selected release '{release.title}'")
    # asset = get_server_asset_from_release(release)
    # if args.wipe_old:
    #     print(f"Clearing out all files currently in '{args.out_dir}'")
    #     shutil.rmtree(args.out_dir, ignore_errors=True)
    # Path(args.out_dir).mkdir(parents=True, exist_ok=True)
    # out_path = os.path.join(args.out_dir, asset.name)
    # print(f"Downloading {asset.url}")
    # with requests.get(
    #     asset.url,
    #     auth=requests.auth.HTTPBasicAuth(GITHUB_TOKEN, ""),
    #     headers={"Accept": "application/octet-stream"},
    #     stream=True,
    # ) as r:
    #     r.raise_for_status()
    #     print(f"Asset size = {r.headers.get('Content-Length', 'unknown')} bytes")
    #     with open(out_path, "wb") as out:
    #         shutil.copyfileobj(r.raw, out)
    # print(f"Saved '{out_path}'")
    
    out_path = '/Users/tinalu/Desktop/protocol/protocol-packages/protocol_tlu2.deploy-server-release-20200821.2_Windows-64bit_client.zip'
    ssh_client = paramiko.SSHClient()
    ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh_client.connect(hostname=args.vm_ip, username=args.vm_user, port=22, key_filename=args.ssh_key, timeout=10)
    ssh_client.exec_command('powershell net stop fractal ;\
        taskkill /IM "FractalService.exe" /F ;\
        taskkill /IM "FractalServer.exe" /F ;\
        Remove-Item C:\ProgramData\FractalCache\log.txt ;\
        mkdir C:\server_build ;')
    #sftp_client = ssh_client.open_sftp()
    #subprocess.run(['scp', out_path, 'Fractal@52.168.66.248:C:\server_build\Windows-64bit_server'], check=True)
    # with closing(Write(ssh_client.get_transport(), 'C:\server_build')) as scp:
    #     scp.send_file(out_path, remote_filename='Windows-64bit_server')
    scp_client = SCPClient(ssh_client.get_transport(), socket_timeout=10)
    scp_client.put(args.out_dir, recursive=True, remote_path='C:/server_build')
    scp_client.put(out_path, remote_path='C:\Program Files\Fractal')
    # ssh_client.exec_command('powershell net start fractal ; shutdown /r ;')