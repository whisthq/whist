import argparse
import subprocess
import re

# Argument parser
parser = argparse.ArgumentParser(description='Filter jobs against git diff.')
parser.add_argument('--commit', required=True,
                    help='The git commit to check comparisons against')
parser.add_argument('--specific-job', nargs='?',
                    help='If only a specific job should be run, pass it into here')
args = parser.parse_args()

# Job folder dependency mapping
job_dependencies = {
    "container-images-publish-images-ghcr": [
        "container-images",
    ],
    "ami-publish-ec2": [
        "ecs-host-setup",
        "ecs-host-service",
    ],
    "main-webserver-deploy-heroku": [
        "main-webserver",
    ],
    "client-applications-publish-build-s3": [
        "client-applications",
    ],
}

# Input Variables
commit = args.commit
specific_job = args.specific_job

# Get list of changed files from git diff
diff_process = subprocess.Popen("git diff --name-only " + commit, shell=True, stdout=subprocess.PIPE)
changed_files = diff_process.communicate()[0].decode("utf-8").strip()

# If a specific job is provided, then remove the rest from the JSON list
if specific_job and specific_job != "all":
    new_job_dependencies = {}
    if specific_job in job_dependencies:
        new_job_dependencies[specific_job] = job_dependencies[specific_job]
    job_dependencies = new_job_dependencies

# Keep a list of the jobs whose dependency files have been changed since "commit"
found_jobs = []
for job in job_dependencies:
    # Make a regex that matches (^|\n)(any job_dependency)
    regex = re.compile("(^|\n)(" + "|".join(job_dependencies[job]) + ")")
    # If the list of changed files matches that regex, then at least one changed file
    # has a dependency path as a prefix of that file
    if regex.search(changed_files):
        found_jobs.append(job)

print(" ".join(found_jobs), end='')
