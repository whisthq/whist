#!/usr/bin/env python3

import yaml, argparse, os, sys, re

from e2e_helpers.common.constants import configs_filename

# Resolve environment variables in YAML
env_pattern = re.compile(r".*?\${(.*?)}.*?")


def env_constructor(loader, node):
    value = loader.construct_scalar(node)
    for group in env_pattern.findall(value):
        value = value.replace(f"${{{group}}}", os.environ.get(group) or "")
    return value


yaml.add_implicit_resolver("!pathex", env_pattern)
yaml.add_constructor("!pathex", env_constructor)


def load_yaml_configs(filepath):
    with open(filepath, "r+") as file:
        return yaml.load(file, Loader=yaml.FullLoader)


def save_configs_to_yaml(filepath, configs):
    with open(filepath, "w+") as file:
        yaml.dump(configs, file, sort_keys=False)


def update_yaml(filepath, config_updates):
    yaml_dict = load_yaml_configs(filepath)
    yaml_dict.update(config_updates)
    save_configs_to_yaml(filepath, yaml_dict)


def is_value_set(configs, key):
    return configs.get(key) is not None


# Add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))

configs = load_yaml_configs(os.path.join(".", "e2e_helpers", "configs", configs_filename))

DESCRIPTION = """
This script will spin one or two g4dn.xlarge EC2 instances (depending on the parameters you pass in), \
start two Docker containers (one for the Whist client, one for the Whist server), and run a protocol \
streaming performance test between the two of them. In case one EC2 instance is used, the two Docker \
containers are started on the same instance.
"""
parser = argparse.ArgumentParser(description=DESCRIPTION)
parser.add_argument(
    "--region-name",
    type=str,
    choices=[
        None,
        "",
        "us-east-1",
        "us-east-2",
        "us-west-1",
        "us-west-2",
        "af-south-1",
        "ap-east-1",
        "ap-south-1",
        "ap-northeast-3",
        "ap-northeast-2",
        "ap-southeast-1",
        "ap-southeast-2",
        "ap-southeast-3",
        "ap-northeast-1",
        "ca-central-1",
        "eu-central-1",
        "eu-west-1",
        "eu-west-2",
        "eu-south-1",
        "eu-west-3",
        "eu-north-1",
        "sa-east-1",
    ],
)
parser.add_argument("--existing-server-instance-id", type=str)
parser.add_argument("--existing-client-instance-id", type=str)
parser.add_argument("--use-two-instances", type=str, choices=[None, "false", "true"])
parser.add_argument("--skip-host-setup", type=str, choices=[None, "false", "true"])
parser.add_argument("--skip-git-clone", type=str, choices=[None, "false", "true"])
parser.add_argument("--cmake-build-type", type=str, choices=[None, "dev", "prod", "metrics"])
parser.add_argument("--network-conditions", type=str)
parser.add_argument("--testing-urls", nargs="+", type=str)
parser.add_argument("--testing-time", type=int)
parser.add_argument("--simulate-scrolling", type=int)
parser.add_argument("--leave-instances-on", type=str, choices=[None, "false", "true"])

args = parser.parse_args()
updated_configs = {k: v for k, v in vars(args).items() if v is not None}
configs.update(updated_configs)
