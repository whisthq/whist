#!/usr/bin/env python3

import urllib.parse

if __name__ == "__main__":
    urls_to_open = [
        "https://www.youtube.com/watch?v=owqq1Fn2gj0&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=1",
        "https://www.youtube.com/watch?v=5m7B7wiyRaI&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=2",
        "https://www.youtube.com/watch?v=xDQg-J0jLmI&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=3",
        "https://www.youtube.com/watch?v=r3te09rZb2E&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=4",
        "https://www.youtube.com/watch?v=qmiJwGn26Cs&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=5",
        "https://www.youtube.com/watch?v=p4814WWbC1M&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=6",
        "https://www.youtube.com/watch?v=k3wlhQUr9VE&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=7",
        "https://www.youtube.com/watch?v=iM3R2Gp0PWo&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=8",
        "https://www.youtube.com/watch?v=eYugwVbFCXU&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=9",
        "https://www.youtube.com/watch?v=ZfIJz-zSXJU&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=10",
        "https://www.youtube.com/watch?v=Sg9GmZPDXJQ&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=11",
        "https://www.youtube.com/watch?v=H6k5UOQK2t8&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=12",
        "https://www.youtube.com/watch?v=2UN3GXeu9fw&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=13",
        "https://www.youtube.com/watch?v=z7rC95kCGJ0&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=14",
        "https://www.youtube.com/watch?v=vZgwmm3cKIw&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=15",
        "https://www.youtube.com/watch?v=cLzHjYbxsD8&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=16",
        "https://www.youtube.com/watch?v=arBYduy0lic&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=17",
        "https://www.youtube.com/watch?v=Kqy2yAtXOiQ&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=18",
        "https://www.youtube.com/watch?v=EM0nb-ewJPw&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=19",
        "https://www.youtube.com/watch?v=3X-Ea2RTjmw&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=20",
        "https://www.youtube.com/watch?v=DnGxG_Qe2pY&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=21",
        "https://www.youtube.com/watch?v=3gZXZ47PtBg&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=22",
        "https://www.youtube.com/watch?v=sTT_XPfYudg&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=23",
        "https://www.youtube.com/watch?v=rRwaBt0eHB0&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=24",
        "https://www.youtube.com/watch?v=oDSovd0BT-0&list=PLbRoZ5Rrl5levOHRQ_cStcDDEMZdH31FY&index=25",
    ]
    safe_urls = " ".join([f"\"{urllib.parse.quote(url, safe=':/=?')}\"" for url in urls_to_open])

    print(
        f"Step 1: Create a EC2 instance (for the server) in a region of your choice, then provide the parameters below."
    )

    server_instance_id = input("Insert server instance ID: ")
    print(f"\tGot {server_instance_id}")

    default_region_name = "us-west-1"
    region_name = input("Insert the region name [us-east-1]: " or default_region_name)
    print(f"\tGot {region_name}")

    ssh_key_name = input("Insert the SSH key name: ")
    print(f"\tGot {ssh_key_name}")

    ssh_key_path = input("Insert the SSH key path: ")
    print(f"\tGot {ssh_key_path}")

    github_token = input("Insert the github token: ")
    print(f"\tGot {github_token}")

    print()
    print(
        f"Step 1: Prepare the server ({server_instance_id}) instance. Run the following command on a new terminal, and press a key when prompted"
    )
    server_command = f"python3 streaming_e2e_tester.py --ssh-key-name {ssh_key_name} --ssh-key-path {ssh_key_path} --github-token {github_token} --region-name {region_name} --existing-server-instance-id={server_instance_id} --skip-git-clone=false --skip-host-setup=false --use-two-instances=false --leave-instances-on=true"
    print(server_command)
    print()

    print(
        f"Step 2: While waiting for the server instance to be ready, prepare the client instances."
    )
    n_mandelboxes = int(input("How many mandelboxes do you want to run on the server [4]? ") or 4)
    print(f"\tGot {n_mandelboxes}")

    print(
        f"Run the following command below {n_mandelboxes} times in {n_mandelboxes} new terminal windows. Press a key when prompted, and wait until the script completes."
    )
    client_command = f"python3 streaming_e2e_tester.py --ssh-key-name {ssh_key_name} --ssh-key-path {ssh_key_path} --github-token {github_token} --region-name {region_name} --use-two-instances=false --leave-instances-on=true"
    print(client_command)
    print()

    print(
        f"Step 3: Obtain the client instance IDs (from the {n_mandelboxes} terminal windows that you just run) and provide them here:"
    )
    client_instance_ids = []
    for i in range(n_mandelboxes):
        client_instance_id = input(f"Insert the {i+1}° client instance ID: ")
        print(f"\tGot {client_instance_id}")
        client_instance_ids.append(client_instance_id)

    print()

    print(
        "Step 4: Manually connect to the server and launch the host service there. In this way, you'll be able to catch any errors."
    )
    print()
    input("Press any key when done...")
    print()

    print(
        f"Step 5. Run the actual experiment by executing the commands below, each in a new terminal window, one at at time. Wait until a processes is prompting you to press a key before starting the next one. Then, once all are ready, only then press a key at the same time on all terminal windows. In this way, the clients will all connect to the server at the same time."
    )

    for i, client_instance_id in enumerate(client_instance_ids):
        print()
        additional_opts = ""
        # if i != 0:
        #     additional_opts = f"{additional_opts} --server_running_independently=true"
        # if i != len(client_instance_ids) - 1:
        #    additional_opts = f"{additional_opts} --testing-time={15*60}"

        print(
            f"python3 streaming_e2e_tester.py --ssh-key-name {ssh_key_name} --ssh-key-path {ssh_key_path} --github-token {github_token} --region-name {region_name} --existing-server-instance-id={server_instance_id} --existing-client-instance-id={client_instance_id} --skip-git-clone=false --skip-host-setup=true --use-two-instances=true --leave-instances-on=false --server_running_independently=true {additional_opts} --testing-urls {safe_urls}"
        )

        print()

    print(f"Step 6: Terminate all client processes as soon as the last one has finished.")
    print()

    input(
        f"Step 7: Get CPU usage. Press a key when all the {n_mandelboxes} scripts have completed..."
    )

    print("Now run the following command: python3 extract_cpu_usage.py")
