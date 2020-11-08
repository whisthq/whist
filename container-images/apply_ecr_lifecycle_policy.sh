#!/bin/bash

# define vars
AWS_REGION=$2
REPO=$1
LIFECYCLE_POLICY='{"rules":[{"rulePriority":1,"description":"Removes outdated images.","selection":{"tagStatus":"any","countType":"imageCountMoreThan","countNumber":10},"action":{"type": "expire"}}]}'

# push Lifecycle policy
aws ecr put-lifecycle-policy --region "${AWS_REGION}" --repository-name "fractal/${REPO}" --lifecycle-policy-text "${LIFECYCLE_POLICY}" || echo "Failed to put lifecycle policy on ${REPO} repo"
