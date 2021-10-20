import argparse
import getpass
import os
import time
import threading
from typing import Dict, List

import boto3
import paramiko


# This ami was created for protocol-testing.
AMI = 'ami-006774cd92ab0b9fd'


ec2 = boto3.resource('ec2')
client = boto3.client("ec2")
ssh_client = paramiko.SSHClient()
ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())


def get_or_create_key_pair() -> None:
    ssh_path = os.path.join(os.path.expanduser("~"), '.ssh')
    key_name = 'protocol_testing_' + getpass.getuser()
    key_path = os.path.join(ssh_path, key_name + '.pem')
    if not os.path.exists(key_path):
        new_keypair = ec2.create_key_pair(KeyName=key_name)
        print(f"Creating new keypair: {key_path}")
        with open(key_path, 'w') as file:
            file.write(new_keypair.key_material)
        os.chmod(key_path, 400)
    else:
        print(f"Detected keypair: {key_path}")
    return key_name, key_path


def get_instance_ips(instance_ids: List[str]) -> List[Dict[str, str]]:
    retval = []
    
    resp = client.describe_instances(InstanceIds=instance_ids)
    for instance in resp['Reservations']:
        if instance:
            for i in instance['Instances']:
                net = i['NetworkInterfaces'][0]
                retval.append(
                    {
                        'public': net['Association']['PublicDnsName'],
                        'private': net['PrivateIpAddress']
                    }
                )
    return retval


def wait_for_instances(instance_ids: List[str], stopping: bool = False) -> None:
    should_wait = True

    while should_wait:
        resp = client.describe_instances(InstanceIds=instance_ids)
        instance_info = resp["Reservations"][0]["Instances"]
        states = [instance["State"]["Name"] for instance in instance_info]
        should_wait = False
        for i, state in enumerate(states):
            if (state != "running" and not stopping) or (state == "running" and stopping):
                should_wait = True
    
    print(f"Instances are {'not' if stopping else ''} running: {instance_ids}")


def start_instances(key_name: str) -> List[str]:
    kwargs = {
        "ImageId": AMI,
        "InstanceType": "g4dn.xlarge",
        "MaxCount": 2,
        "MinCount": 2,
        "TagSpecifications": [
            {
                "ResourceType": "instance",
                "Tags": [
                    {"Key": "Name", "Value": "protocol-testing"},
                ],
            },
        ],
        "SecurityGroups": [
            'container-tester',
        ],
        "InstanceInitiatedShutdownBehavior": "terminate",
        "IamInstanceProfile": {"Name": "auto_scaling_instance_profile"},
        "KeyName": key_name,
    }
    resp = client.run_instances(**kwargs)
    instance_ids = [instance["InstanceId"] for instance in resp["Instances"]]
    # instance_ids = ['i-0d986b8a2ad01e2ad', 'i-01978ad566413bce7']
    print(f"Created ec2 instances with ids: {instance_ids}")
    return instance_ids


def wait_for_ssh(instance_ips: List[Dict[str, str]], key: paramiko.RSAKey) -> None:
    avaliable = False

    while not avaliable:
        avaliable = True
        for ip in instance_ips:
            try:
                ssh_client.connect(hostname=ip['public'], username='ec2-user', pkey=key)
            except:
                avaliable = False
            finally:
                ssh_client.close()
        time.sleep(2)


def setup_instances(instance_ips: List[Dict[str, str]], key: paramiko.RSAKey) -> None:
    print("Setting up the instances")
    for ip in instance_ips:
        ssh_client.connect(hostname=ip['public'], username='ec2-user', pkey=key)
        _, stdout, _ = ssh_client.exec_command('sudo yum -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm && sudo yum -y install iperf')
        for line in iter(stdout.readline, ""):
            print(line, end="")
        ssh_client.close()


def perform_work(
    ip: Dict[str, str],
    cmd: str,
    key: paramiko.RSAKey,
    display_res: bool,
) -> None:
    ssh_client.connect(hostname=ip['public'], username='ec2-user', pkey=key)
    try:
        _, stdout, _ = ssh_client.exec_command(cmd, timeout=30)
        if display_res:
            for line in iter(stdout.readline, ""):
                print(line, end="")
        else:
            stdout.read()
    except:  # We expect the one command to timeout so we do nothing on exception
        pass
    finally:
        ssh_client.close()


if __name__ == "__main__":
    # Create new key or get existing key
    key_name, key_path = get_or_create_key_pair()
    key = paramiko.RSAKey.from_private_key_file(key_path)

    # Start two instances in ec2
    instance_ids = start_instances(key_name)

    # Give a little time for those instances to be recognized
    time.sleep(5)

    # Wait for those instances to be running
    wait_for_instances(instance_ids)

    # Get the ip's of the instances
    instance_ips = get_instance_ips(instance_ids)
    print(instance_ips)

    # # Wait for ssh connections to be accepted by the instances
    # wait_for_ssh(instance_ips, key)

    # # Sleep so that the ssh connections reset
    # time.sleep(1)

    # # Perform setup steps that are the same for both instances
    # setup_instances(instance_ips, key)
    
    # # Sleep so that the ssh connections reset
    # time.sleep(1)

    # # Perform the actual task that we want
    # print(f'Starting iperf')
    
    # cmd1 = 'sudo iperf -s -u'
    # t1 = threading.Thread(target=perform_work, args=(instance_ips[0], cmd1, key, True))
    # t1.start()

    # # Wait a couple seconds for that command to be executed
    # time.sleep(2)

    # cmd2 = 'iperf -c ' + instance_ips[0]['private'] + ' -u -b 5g'
    # t2 = threading.Thread(target=perform_work, args=(instance_ips[1], cmd2, key, False))
    # t2.start()

    # t1.join()
    # t2.join()

    # Terminating the instances and waiting for them to shutdown
    # print(f"Testing complete, terminating instances with ids: {instance_ids}")
    # client.terminate_instances(InstanceIds=instance_ids)

    # wait_for_instances(instance_ids, stopping=True)
    # print("Instances successfully terminated, goodbye")
