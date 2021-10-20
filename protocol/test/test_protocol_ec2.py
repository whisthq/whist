import argparse
import getpass
import os
import time
import threading
from typing import Dict, List, Tuple

import boto3
import paramiko


DESCRIPTION = """
This script will spin up 2 ec2 instances, run the host-service on both
of them. Then copy over your protocol directory into the ec2 instances,
create one base mandelbox for running the server and one base-client
mandelbox for running the client. You will see the output from the client.
"""

# This ami was created for protocol-testing.
AMI = 'ami-0ec20b64a8d8a7c80'


ec2 = boto3.resource('ec2')
client = boto3.client("ec2")

# This will be set with the correct client command when one host spins up the server
client_command = None

parser = argparse.ArgumentParser(description=DESCRIPTION)
parser.add_argument(
    "--key-name",
    help="The name of your AWS key as it appears in AWS. Required.",
    required=True,
)
parser.add_argument(
    "--key-path",
    help="The full path to your AWS private key corresponding to the AWS key name you provided. Required.",
    required=True,
)
args = parser.parse_args()


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
    print(f"Instance ips are: {retval}")
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
    # instance_ids = ['i-06f2e0d53aa31b723', 'i-07da17c0cc68127ca']
    print(f"Created ec2 instances with ids: {instance_ids}")
    return instance_ids


def wait_for_ssh(instance_ips: List[Dict[str, str]], key: paramiko.RSAKey) -> None:
    ssh_client = paramiko.SSHClient()
    ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    avaliable = False

    while not avaliable:
        avaliable = True
        for ip in instance_ips:
            try:
                ssh_client.connect(hostname=ip['public'], username='ubuntu', pkey=key)
            except:
                print(ip)
                avaliable = False
            finally:
                ssh_client.close()
        time.sleep(2)


def transfer_protocol(instance_ips: List[Dict[str, str]], key_path: str) -> None:
    print('Moving over your protocol directory for testing')
    threads = []

    # Change dir to outer protocol so we can copy
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    os.chdir("..")

    for ip in instance_ips:
        command = 'tar czf - . | ssh -i ' + key_path + ' ubuntu@' + ip['public'] + ' "cd /home/ubuntu/fractal/protocol && tar xvzf -" > /dev/null'
        t1 = threading.Thread(target=os.system, args=(command,))
        t1.start()
        threads.append(t1)
    
    for thread in threads:
        thread.join()


def setup_host_serive(instance_ips: List[Dict[str, str]], key: paramiko.RSAKey) -> Tuple[threading.Thread]:
    print("Setting up the host services on both instances")
    command = 'cd ~/fractal/host-service; make run; make run'
    
    t1 = threading.Thread(target=perform_work, args=(instance_ips[0], command, key, False, 180))
    t2 = threading.Thread(target=perform_work, args=(instance_ips[1], command, key, False, 180))
    
    t1.start()
    t2.start()

    return t1, t2


def perform_work(
    ip: Dict[str, str],
    cmd: str,
    key: paramiko.RSAKey,
    display_res: bool,
    timeout: int,
) -> None:
    global client_command
    ssh_client = paramiko.SSHClient()
    ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh_client.connect(hostname=ip['public'], username='ubuntu', pkey=key)
    try:
        _, stdout, _ = ssh_client.exec_command(cmd, timeout=timeout)
        if display_res:
            for line in iter(stdout.readline, ""):
                print(line, end="")
        else:
            for line in iter(stdout.readline, ""):
                if 'linux/macos' in line:
                    client_command = line[line.find('.')+2:].strip()
                    print(f"Client command: {client_command}")
    except:  # If timeout, we just close the connection
        pass
    finally:
        ssh_client.close()


if __name__ == "__main__":
    try:
        # Get the AWS key so that we can connect to the hosts.
        key_name, key_path = args.key_name, args.key_path
        key = paramiko.RSAKey.from_private_key_file(key_path)

        # Start two instances in ec2
        instance_ids = start_instances(key_name)

        # Give a little time for those instances to be recognized
        time.sleep(5)

        # Wait for those instances to be running
        wait_for_instances(instance_ids)

        # Get the ip's of the instances
        instance_ips = get_instance_ips(instance_ids)

        # # Wait for ssh connections to be accepted by the instances
        wait_for_ssh(instance_ips, key)

        # # Sleep so that the ssh connections reset
        time.sleep(1)

        # # Perform setup steps that are the same for both instances
        t1, t2 = setup_host_serive(instance_ips, key)

        # Perform the actual task that we want
        transfer_protocol(instance_ips, key_path)
        
        print('Running the server and the client')
        # Set up the host server
        cmd = '''
            cd ~/fractal/protocol;
            ./build_protocol_targets.sh FractalServer;
            cd ~/fractal/mandelboxes/helper_scripts;
            echo hello; 
            sudo python3 run_mandelbox_image.py fractal/base:current-build --update-protocol
        '''
        t3 = threading.Thread(target=perform_work, args=(instance_ips[0], cmd, key, False, 170))
        t3.start()

        # Set up the client server
        ssh_client = paramiko.SSHClient()
        ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        ssh_client.connect(hostname=instance_ips[1]['public'], username='ubuntu', pkey=key)
        cmd = '''
            cd ~/fractal/protocol;
            ./build_protocol_targets.sh FractalClient;
            cd ~/fractal/mandelboxes/helper_scripts;
            echo hello; 
            sudo python3 run_mandelbox_image.py fractal/base-client:current-build --update-protocol
        '''
        _, stdout, _ = ssh_client.exec_command(cmd, timeout=120)

        # Startup the docker container and get the ID
        _, stdout, _ = ssh_client.exec_command(cmd, timeout=120)
        output = []
        for line in iter(stdout.readline, ""):
            output.append(line)
        container_id = output[-1][:-1]

        # Wait for the client command
        while not client_command:
            time.sleep(1)

        # Exec the client and capture the output
        run_client_cmd = "docker exec " + container_id + " /usr/share/fractal/bin/" + client_command
        _, stdout, _ = ssh_client.exec_command(run_client_cmd, timeout=120)
        for line in iter(stdout.readline, ""):
            print(line, end="")

        t1.join()
        t2.join()
        t3.join()
    finally:
        # Terminating the instances and waiting for them to shutdown
        print(f"Testing complete, terminating instances with ids: {instance_ids}")
        client.terminate_instances(InstanceIds=instance_ids)

        wait_for_instances(instance_ids, stopping=True)
        print("Instances successfully terminated, goodbye")
