import subprocess
import json
import sys

# Uses AWS CLI to return all Auto Scaling Groups in AWS for a specified region
def get_all_aws_asgs(region):
    all_asgs, _ = subprocess.Popen(
        [
            "aws",
            "autoscaling",
            "describe-auto-scaling-groups",
            "--no-paginate",
            "--region",
            region,
        ],
        stdout=subprocess.PIPE,
    ).communicate()
    all_asgs = [
        asg["AutoScalingGroupARN"] for asg in json.loads(all_asgs)["AutoScalingGroups"]
    ]
    return all_asgs


# Uses AWS CLI to return all clusters in AWS for a specified region
def get_all_aws_clusters(region):
    all_cluster_arns, _ = subprocess.Popen(
        ["aws", "ecs", "list-clusters", "--no-paginate", "--region", region],
        stdout=subprocess.PIPE,
    ).communicate()
    all_cluster_arns = json.loads(all_cluster_arns)["clusterArns"]
    return all_cluster_arns


# Takes a Cluster ARN and maps it to its Auto Scaling Group(s) via its associated Capacity Provider(s)
def cluster_to_asgs(cluster_arn, region):
    capacity_providers, _ = subprocess.Popen(
        [
            "aws",
            "ecs",
            "describe-clusters",
            "--no-paginate",
            "--region",
            region,
            "--clusters",
            cluster_arn,
        ],
        stdout=subprocess.PIPE,
    ).communicate()
    capacity_providers = json.loads(capacity_providers)["clusters"][0][
        "capacityProviders"
    ]
    if len(capacity_providers) > 0:
        asgs, _ = subprocess.Popen(
            [
                "aws",
                "ecs",
                "describe-capacity-providers",
                "--no-paginate",
                "--region",
                region,
                "--capacity-providers",
                " ".join(capacity_providers),
            ],
            stdout=subprocess.PIPE,
        ).communicate()
        asgs = [
            asg["autoScalingGroupProvider"]["autoScalingGroupArn"]
            for asg in json.loads(asgs)["capacityProviders"]
            if "autoScalingGroupProvider" in asg
        ]
        return asgs
    else:
        return []


# Queries specified db for all clusters in a specified region
def get_db_clusters(url, secret, region):
    clusters, _ = subprocess.Popen(
        [
            "curl",
            "-L",
            "-X",
            "POST",
            url,
            "-H",
            "Content-Type: application/json",
            "-H",
            "x-hasura-admin-secret: %s" % (secret),
            "--data-raw",
            '{"query":"query get_clusters($_eq: String = \\"%s\\") { hardware_cluster_info(where: {location: {_eq: $_eq}}) { cluster }}"}'
            % (region),
        ],
        stdout=subprocess.PIPE,
    ).communicate()
    clusters = json.loads(clusters)["data"]["hardware_cluster_info"]
    return [list(cluster.values())[0] for cluster in clusters]


# Compares list of ASGs in AWS to list of ASGs associated with Clusters in AWS
def get_hanging_asgs(region):
    cluster_asgs = [
        asg
        for cluster_arn in get_all_aws_clusters(region)
        for asg in cluster_to_asgs(cluster_arn, region)
        if cluster_arn is not None
    ]
    all_asgs = get_all_aws_asgs(region)

    # return names (not ARNs) of ASGs not assocated a cluster (could be more than one ASG per cluster)

    return [asg.split("/")[-1] for asg in list(set(all_asgs) - set(cluster_asgs))]


# Compares list of clusters in AWS to list of clusters in all DBs
def get_hanging_clusters(urls, secrets, region):
    # parses names of clusters from ARNs since only the names are stored in the DB
    aws_clusters = set(
        [cluster.split("/")[-1] for cluster in get_all_aws_clusters(region)]
    )
    aws_clusters.remove("default")

    # loop through each of dev, staging, and prod; add clusters to db_clusters
    db_clusters = set()
    for url, secret in zip(urls, secrets):
        db_clusters |= set(get_db_clusters(url, secret, region))

    # return list of cluster names in aws but not in db, ignoring the 'default' cluster
    return list(aws_clusters - db_clusters)


def main():
    component = sys.argv[1]
    region = sys.argv[2]
    urls = sys.argv[3:6]
    secrets = sys.argv[6:9]

    # format as bulleted list for Slack notification
    if component == "ASGs":
        asgs = get_hanging_asgs(region)
        if len(asgs) > 0:
            print("- `" + "`\\n- `".join([str(x) for x in asgs]) + "`")
    elif component == "Clusters":
        clusters = get_hanging_clusters(urls, secrets, region)
        if len(clusters) > 0:
            print("- `" + "`\\n- `".join([str(x) for x in clusters]) + "`")


if __name__ == "__main__":
    main()
