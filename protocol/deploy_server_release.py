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
default_protocol_dir = 'C:\server_build'

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
        asset_flavor = asset.name.split("_")[-1]
        if (
            os.lower() in asset.name.lower()
            and flavor.lower() in asset_flavor.lower()
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
    args = parser.parse_args()

    github_client = github.Github(GITHUB_TOKEN)
    release = get_release(github_client.get_repo(args.protocol_repo), args.release)
    print(f"Selected release '{release.title}'")
    asset = get_server_asset_from_release(release)
    print(f"Selected asset '{asset.name}'")
    out_path = args.out_dir + "\\" + asset.name
    out_path_unzipped = out_path + '_unzipped'
    new_server_build = out_path_unzipped + '\Windows-64bit_server\*'

    ssh_client = paramiko.SSHClient()
    ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh_client.connect(hostname=args.vm_ip, username=args.vm_user, port=22, key_filename=args.ssh_key, timeout=10)
    cmd = f'powershell net stop fractal ;\
        taskkill /IM "FractalService.exe" /F ;\
        taskkill /IM "FractalServer.exe" /F ;\
        Remove-Item C:\ProgramData\FractalCache\log.txt ;\
        New-Item -ItemType Directory -Force -Path C:\server_build ;\
        $Uri = \\"{asset.url}\\";\
        $Token = \\"{GITHUB_TOKEN}\\";\
        $Outfile = \\"{out_path}\\";\
        $Base64Token = [System.Convert]::ToBase64String([System.Text.Encoding]::UTF8.GetBytes($Token));\
        $Headers = @{{\\"Authorization\\" = \\"Basic {{0}}\\" -f $Base64Token;\
                      \\"Accept\\" = \\"application/octet-stream\\"}};\
        $ProgressPreference = \\"SilentlyContinue\\";\
        Invoke-WebRequest -Uri $Uri -Headers $Headers -Outfile $Outfile -UseBasicParsing;\
        Expand-Archive -Force -LiteralPath \\"{out_path}\\" -DestinationPath \\"{out_path_unzipped}\\";\
        Copy-item -Force -Recurse \\"{new_server_build}\\" -Destination \\"C:\server_build\Windows-64bit_server\\";\
        Copy-item -Force -Recurse \\"C:\server_build\Windows-64bit_server\*\\"  -Destination \\"C:\Program Files\Fractal\\\";\
        net start fractal ; shutdown /r ;'
    stdin, stdout, stderr = ssh_client.exec_command(cmd)
    error = stderr.readlines()
    if error:
        err_message = ""
        for err in error:
            err_message += err
        raise Exception(f'SSH resulted in error: {err_message}!')
    print("Deployed server release to VM")
