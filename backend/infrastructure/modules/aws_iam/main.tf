# Data for IAM role assume policies

data "aws_iam_policy_document" "spot-assume-role-policy" {
  statement {
    actions = ["sts:AssumeRole"]

    principals {
      type        = "Service"
      identifiers = ["spot.amazonaws.com"]
    }
  }
}

data "aws_iam_policy_document" "ec2-assume-role-policy" {
  statement {
    actions = ["sts:AssumeRole"]

    principals {
      type        = "Service"
      identifiers = ["ec2.amazonaws.com"]
    }
  }
}

# IAM service-liked roles

resource "aws_iam_service_linked_role" "ServiceRoleForSSM" {
  aws_service_name = "ssm.amazonaws.com"
}

resource "aws_iam_service_linked_role" "ServiceRoleForComputeOptimizer" {
  aws_service_name = "compute-optimizer.amazonaws.com"
}

resource "aws_iam_service_linked_role" "ServiceRoleForEC2Spot" {
  aws_service_name = "spot.amazonaws.com"
}

resource "aws_iam_service_linked_role" "ServiceRoleForServiceQuotas" {
  aws_service_name = "servicequotas.amazonaws.com"  
}

# Custom policies for IAM roles

# This policy is meant to have S3 read only access, EC2 read access
# and permissions to start and terminate instances (on-demand and spot)
data "aws_iam_policy_document" "DeploymentRoleInlinePolicy" {
  statement {
    # This statement represents the EC2ReadOnlyAcceess permissions
    actions = [
      "ec2:Describe*",
      "elasticloadbalancing:Describe*",
      "cloudwatch:ListMetrics",
      "cloudwatch:GetMetricStatistics",
      "cloudwatch:Describe*",
      "autoscaling:Describe*",
    ]
    effect = "Allow"
    resources = [
      "*"
    ]
  }

  # This statement adds permissions to interact with instances
  statement {
    actions = [  
      "ec2:CreateTags",
      "ec2:RunInstances",
      "ec2:StopInstances",
      "ec2:RequestSpotInstances",
      "ec2:CancelSpotInstanceRequests",
      "ec2:SendSpotInstanceInterruptions",
      "ec2:TerminateInstances",
    ]
    effect = "Allow"
    resources = [ 
      "*"
    ]
  }

  # This statement represents S3ReadOnlyAccess permissions
  statement {
    actions = [
      "s3:Get*",
      "s3:List*",
      "s3-object-lambda:Get*",
      "s3-object-lambda:List*",
    ]
    effect = "Allow"
    resources = [
      "*",
    ]
  }

  # This statement will only evaluate if the environment is not prod.
  # It enables SSM to access instances.
  dynamic "statement" {
    for_each = var.env != "prod" ? [1] : []
    content {
      actions = [
        "ssm:DescribeSession",
        "ssm:StartSession",
        "ssm:TerminateSession",
        "ssm:ResumeSession",
      ]
      effect = "Allow"
      resources = [
        "session*",
        "document",
        "instance",
        "managed-instance",
        "task",
      ]
    }
  }
}

# IAM roles

resource "aws_iam_role" "PackerAMIBuilder" {
  name = "PackerAMIBuilder"
  assume_role_policy  = data.aws_iam_policy_document.ec2-assume-role-policy.json

  managed_policy_arns = [
    "arn:aws:iam::aws:policy/AmazonS3ReadOnlyAccess"
  ]
}

resource "aws_iam_role" "DeploymentRole" {
  name = "DeploymentRole"
  assume_role_policy  = data.aws_iam_policy_document.ec2-assume-role-policy.json

  inline_policy {
    name = "deployment-role-policy"
    policy = data.aws_iam_policy_document.DeploymentRoleInlinePolicy.json
  }
}

#IAM User groups

resource "aws_iam_group" "whist-2FA" {
  name = "Whist2FA"
}

resource "aws_iam_group" "whist-admins" {
  name = "WhistAdmins"
}

resource "aws_iam_group" "whist-ci" {
  name = "WhistCI"
}

resource "aws_iam_group" "whist-engineers" {
  name = "WhistEngineers"
}

resource "aws_iam_group" "whist-env" {
  name = "Whist${var.env}"
}

# Custom group policies

resource "aws_iam_group_policy" "MFA-policy" {
  name   = "Force_MFA"
  group  = aws_iam_group.whist-2FA.id
  policy = jsonencode(aws_iam_group_policy.mfa-policy)
}

# AWS managed group policies

resource "aws_iam_group_policy_attachment" "admin-policy" {
  group      = aws_iam_group.whist-admins.name
  policy_arn = "arn:aws:iam::aws:policy/AdministratorAccess"
}

resource "aws_iam_group_policy_attachment" "ci-policy" {
  group      = aws_iam_group.whist-engineers.name
  for_each = toset([
    "arn:aws:iam::aws:policy/AmazonEC2FullAccess",
    "arn:aws:iam::aws:policy/IAMFullAccess",
    "arn:aws:iam::aws:policy/AmazonS3FullAccess",
  ])
  policy_arn = each.value
}

resource "aws_iam_group_policy_attachment" "engineering-policy" {
  group      = aws_iam_group.whist-engineers.name
  for_each = toset([
    "arn:aws:iam::aws:policy/AmazonEC2FullAccess",
    "arn:aws:iam::aws:policy/IAMFullAccess",
    "arn:aws:iam::aws:policy/AmazonS3FullAccess",
    "arn:aws:iam::aws:policy/AmazonSSMFullAccess",
    "arn:aws:iam::aws:policy/AWSSupportAccess",
  ])
  policy_arn = each.value
}

resource "aws_iam_group_policy_attachment" "whist-env-policy" {
  group      = aws_iam_group.whist-env.name
  for_each   = var.whist-env-managed-policies
  policy_arn = each.value
}

# Custom group policies

resource "aws_iam_group_policy" "mfa-policy" {
    name = "ForceMFA"
    group = aws_iam_group.whist-2FA.id
    policy = jsonencode({
    "Version" : "2012-10-17",
    "Statement" : [
      {
        "Sid" : "AllowViewAccountInfo",
        "Effect" : "Allow",
        "Action" : [
          "iam:GetAccountPasswordPolicy",
          "iam:GetAccountSummary",
          "iam:ListVirtualMFADevices"
        ],
        "Resource" : "*"
      },
      {
        "Sid" : "AllowManageOwnPasswords",
        "Effect" : "Allow",
        "Action" : [
          "iam:ChangePassword",
          "iam:GetUser"
        ],
        "Resource" : "arn:aws:iam::*:user/$${aws:username}"
      },
      {
        "Sid" : "AllowManageOwnAccessKeys",
        "Effect" : "Allow",
        "Action" : [
          "iam:CreateAccessKey",
          "iam:DeleteAccessKey",
          "iam:ListAccessKeys",
          "iam:UpdateAccessKey"
        ],
        "Resource" : "arn:aws:iam::*:user/$${aws:username}"
      },
      {
        "Sid" : "AllowManageOwnSigningCertificates",
        "Effect" : "Allow",
        "Action" : [
          "iam:DeleteSigningCertificate",
          "iam:ListSigningCertificates",
          "iam:UpdateSigningCertificate",
          "iam:UploadSigningCertificate"
        ],
        "Resource" : "arn:aws:iam::*:user/$${aws:username}"
      },
      {
        "Sid" : "AllowManageOwnSSHPublicKeys",
        "Effect" : "Allow",
        "Action" : [
          "iam:DeleteSSHPublicKey",
          "iam:GetSSHPublicKey",
          "iam:ListSSHPublicKeys",
          "iam:UpdateSSHPublicKey",
          "iam:UploadSSHPublicKey"
        ],
        "Resource" : "arn:aws:iam::*:user/$${aws:username}"
      },
      {
        "Sid" : "AllowManageOwnGitCredentials",
        "Effect" : "Allow",
        "Action" : [
          "iam:CreateServiceSpecificCredential",
          "iam:DeleteServiceSpecificCredential",
          "iam:ListServiceSpecificCredentials",
          "iam:ResetServiceSpecificCredential",
          "iam:UpdateServiceSpecificCredential"
        ],
        "Resource" : "arn:aws:iam::*:user/$${aws:username}"
      },
      {
        "Sid" : "AllowManageOwnVirtualMFADevice",
        "Effect" : "Allow",
        "Action" : [
          "iam:CreateVirtualMFADevice",
          "iam:DeleteVirtualMFADevice"
        ],
        "Resource" : "arn:aws:iam::*:mfa/$${aws:username}"
      },
      {
        "Sid" : "AllowManageOwnUserMFA",
        "Effect" : "Allow",
        "Action" : [
          "iam:DeactivateMFADevice",
          "iam:EnableMFADevice",
          "iam:ListMFADevices",
          "iam:ResyncMFADevice"
        ],
        "Resource" : "arn:aws:iam::*:user/$${aws:username}"
      },
      {
        "Sid" : "DenyAllExceptListedIfNoMFA",
        "Effect" : "Deny",
        "NotAction" : [
          "iam:CreateVirtualMFADevice",
          "iam:EnableMFADevice",
          "iam:GetUser",
          "iam:ChangePassword",
          "iam:CreateLoginProfile",
          "iam:ListMFADevices",
          "iam:ListVirtualMFADevices",
          "iam:ResyncMFADevice",
          "sts:GetSessionToken"
        ],
        "Resource" : "*",
        "Condition" : {
          "Bool" : {
            "aws:MultiFactorAuthPresent" : "false"
          }
        }
      }
    ]
  })
}