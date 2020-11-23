"""AWS configuration strings."""

import json

EC2_ROLE_FOR_ECS_POLICY_ARN = (
    "arn:aws:iam::aws:policy/service-role/AmazonEC2ContainerServiceforEC2Role"
)


ECS_INSTANCE_ROLE_POLICY_DOCUMENT = json.dumps(
    {
        "Version": "2008-10-17",
        "Statement": [
            {
                "Sid": "",
                "Effect": "Allow",
                "Principal": {"Service": "ec2.amazonaws.com"},
                "Action": "sts:AssumeRole",
            }
        ],
    }
)
