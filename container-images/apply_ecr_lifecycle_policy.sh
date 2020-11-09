#!/bin/bash

# define vars
REPO=$1
AWS_REGION=$2
LIFECYCLE_POLICY='{"rules":[{"rulePriority":1,"description":"Removes outdated images.","selection":{"tagStatus":"any","countType":"imageCountMoreThan","countNumber":10},"action":{"type": "expire"}}]}'

# push Lifecycle policy
output=$(aws ecr put-lifecycle-policy --region ${AWS_REGION} --repository-name fractal/${REPO} --lifecycle-policy-text "${LIFECYCLE_POLICY}")
if [ -z "$output" ];
then
    echo "Successfully put lifecycle policy in fractal/${REPO} ECR repository in AWS region ${AWS_REGION}."
else
    echo "Failed to put lifecycle policy on fractal/${REPO} ECR repository in AWS region ${AWS_REGION}. Error output: $output"
fi
