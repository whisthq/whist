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
        return yaml.load(file)


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
    "--ssh-key-name",
    help="The name of the AWS RSA SSH key you wish to use to access the E2 instance(s). If you are running the \
    script locally, the key name is likely of the form <yourname-key>. Make sure to pass the key-name for the \
    region of interest. Required.",
    required=not is_value_set(configs, "ssh_key_name"),
)
parser.add_argument(
    "--ssh-key-path",
    help="The path to the .pem certificate on this machine to use in connection to the RSA SSH key passed to \
    the --ssh-key-name arg. Required.",
    required=not is_value_set(configs, "ssh_key_path"),
)
parser.add_argument(
    "--github-token",
    help="The GitHub Personal Access Token with permission to fetch the whisthq/whist repository. Required.",
    required=not is_value_set(configs, "github_token"),
)

parser.add_argument(
    "--region-name",
    help="The AWS region to use for testing. Passing an empty string will let the script run the test on any \
    region with space available for the new instance(s). If you are looking to re-use an instance for the client \
    and/or server, the instance(s) must live on the region passed to this parameter. If you pass an empty string, \
    the key-pair that you pass must be valid on all AWS regions.",
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

parser.add_argument(
    "--existing-server-instance-id",
    help="The instance ID of an existing instance to use for the Whist server during the E2E test. You can only \
    pass a value to this parameter if you passed `true` to --use-two-instances. Otherwise, the server will be \
    installed and run on the same instance as the client. The instance will be stopped upon completion. \
    If left empty (and --use-two-instances=true), a clean new instance will be generated instead, and it will \
    be terminated (instead of being stopped) upon completion of the test.",
    type=str,
)

parser.add_argument(
    "--existing-client-instance-id",
    help="The instance ID of an existing instance to use for the Whist dev client during the E2E test. If the flag \
    --use-two-instances=false is used (or if the flag --use-two-instances is not used), the Whist server will also \
    run on this instance. The instance will be stopped upon completion. If left empty, a clean new instance will \
    be generated instead, and it will be terminated (instead of being stopped) upon completion of the test.",
    type=str,
)

parser.add_argument(
    "--use-two-instances",
    help="Whether to run the client on a separate AWS instance, instead of the same as the server.",
    type=str,
    choices=[None, "false", "true"],
)

parser.add_argument(
    "--leave-instances-on",
    help="This option allows you to override the default behavior and leave the instances running upon completion \
    of the test, instead of stopping (if reusing existing ones) or terminating (if creating new ones) them.",
    type=str,
    choices=[None, "false", "true"],
)

parser.add_argument(
    "--skip-host-setup",
    help="This option allows you to skip the host-setup on the instances to be used for the test. \
    This will save you a good amount of time.",
    type=str,
    choices=[None, "false", "true"],
)

parser.add_argument(
    "--skip-git-clone",
    help="This option allows you to skip cloning the Whist repository on the instance(s) to be used for the test. \
    The test will also not checkout the current branch or pull from Github, using the code from the /whist folder \
    on the existing instance(s) as is. This option is useful if you need to run several tests in succession \
    using code from the same commit.",
    type=str,
    choices=[None, "false", "true"],
)

parser.add_argument(
    "--simulate-scrolling",
    help="Number of rounds of scrolling that should be simulated on the client side. One round of scrolling = \
    Slow scroll down + Slow scroll up + Fast scroll down + Fast scroll up",
    type=int,
)

parser.add_argument(
    "--testing-urls",
    help="The URL to visit during the test. The default is a 4K video stored on our AWS S3.",
    nargs="+",
    type=str,
)

parser.add_argument(
    "--testing-time",
    help="The time (in seconds) to wait at the URL from the `--testing-url` flag before shutting down the \
    client/server and grabbing the logs and metrics. The default value is the duration of the default 4K video \
    from AWS S3.",
    type=int,
)

parser.add_argument(
    "--cmake-build-type",
    help="The Cmake build type to use when building the protocol.",
    type=str,
    choices=[None, "dev", "prod", "metrics"],
)

parser.add_argument(
    "--network-conditions",
    help="The network conditions for the experiment. The input is in the form of up to five comma-separated values \
    indicating the max bandwidth, delay (in ms), percentage of packet drops (in the range [0.0,1.0]), queue capacity, \
    and the interval of change of the network conditions. Each condition can be expressed using a single float (for \
    conditions that do not change over time) or as a range expressed using a min and max value separated by a hyphen. \
    `normal` will allow the network to run with no degradation. Passing `None` to one of the five parameters will result \
    in no limitations being imposed to the corresponding network condition. For more details about the usage of the five \
    network condition parameters, check out the apply_network_conditions.sh script in protocol/test/helpers/setup.",
    type=str,
)

args = parser.parse_args()
updated_configs = {k: v for k, v in vars(args).items() if v is not None}
configs.update(updated_configs)
