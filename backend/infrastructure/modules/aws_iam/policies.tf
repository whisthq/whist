# In this file we define the policies for IAM roles and user groups

# Data for IAM role assume policies

data "aws_iam_policy_document" "EC2AssumeRolePolicy" {
  statement {
    actions = ["sts:AssumeRole"]

    principals {
      type        = "Service"
      identifiers = ["ec2.amazonaws.com"]
    }
  }
}

# Custom policies for IAM roles

# This policy is meant to have S3 read only access, EC2 read access
# and permissions to start and terminate instances (on-demand and spot)
data "aws_iam_policy_document" "DeploymentRoleInlinePolicy" {
  statement {
    # This statement represents the EC2ReadOnlyAccess permissions
    actions = [
      "ec2:Describe*",
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

# This policy gives Packer the minimum amount of permissions to work. 
# It is a slightly modified version to include spot instance access.
# See: https://www.packer.io/plugins/builders/amazon#iam-task-or-instance-role
data "aws_iam_policy_document" "PackerAMIBuilderInlinePolicy" {
  # This statement adds permissions to interact w
  statement {
    sid = "PackerImageAccess"
    actions = [
      "ec2:CopyImage",
      "ec2:CreateImage",
      "ec2:DeregisterImage",
      "ec2:DescribeImageAttribute",
      "ec2:DescribeImages",
      "ec2:ModifyImageAttribute",
      "ec2:RegisterImage",
    ]
    effect = "Allow"
    resources = [
      "*"
    ]
  }

  statement {
    sid = "PackerSnapshotAccess"
    actions = [
      "ec2:CreateSnapshot",
      "ec2:DeleteSnapshot",
      "ec2:DescribeSnapshots",
      "ec2:ModifySnapshotAttribute",
    ]
    effect = "Allow"
    resources = [
      "*"
    ]
  }

  statement {
    sid = "PackerVolumeAccess"
    actions = [
      "ec2:AttachVolume",
      "ec2:CreateVolume",
      "ec2:DeleteVolume",
      "ec2:DescribeVolumes",
      "ec2:DetachVolume",
    ]
    effect = "Allow"
    resources = [
      "*"
    ]
  }

  statement {
    sid = "PackerSecurityGroupAccess"
    actions = [
      "ec2:AuthorizeSecurityGroupIngress",
      "ec2:CreateSecurityGroup",
      "ec2:DeleteSecurityGroup",
      "ec2:DescribeSecurityGroups",
    ]
    effect = "Allow"
    resources = [
      "*"
    ]
  }

  statement {
    sid = "PackerMiscAccess"
    actions = [
      "ec2:CreateKeypair",
      "ec2:CreateTags",
      "ec2:DeleteKeyPair",
      "ec2:DescribeRegions",
      "ec2:DescribeSubnets",
      "ec2:DescribeTags",
      "ec2:GetPasswordData",
    ]
    effect = "Allow"
    resources = [
      "*"
    ]
  }

  statement {
    sid = "PackerInstanceAccess"
    actions = [
      "ec2:DescribeInstances",
      "ec2:DescribeInstanceStatus",
      "ec2:RegisterImage",
      "ec2:RunInstances",
      "ec2:StopInstances",
      "ec2:TerminateInstances",
      "ec2:RequestSpotInstances",
      "ec2:CancelSpotInstanceRequests",
      "ec2:SendSpotInstanceInterruptions",
      "ec2:ModifyInstanceAttribute",
    ]
    effect = "Allow"
    resources = [
      "*"
    ]
  }
}

# Custom group policies

# This policy forces user to enable 2FA on their account, and
# restricts access to resources otherwise.
data "aws_iam_policy_document" "MFAPolicy" {
  name = "ForceMFA"

  statement {
    sid = "AllowViewAccountInfo"
    actions = [
      "iam:GetAccountPasswordPolicy",
      "iam:GetAccountSummary",
      "iam:ListVirtualMFADevices"
    ]
    effect    = "Allow"
    resources = ["*"]
  }

  statement {
    sid = "AllowManageOwnPasswords"
    actions = [
      "iam:ChangePassword",
      "iam:GetUser"
    ]
    effect    = "Allow"
    resources = ["*"]
  }

  statement {
    sid = "AllowManageOwnPasswords"
    actions = [
      "iam:ChangePassword",
      "iam:GetUser"
    ]
    effect    = "Allow"
    resources = ["arn:aws:iam::*:user/$${aws:username}"]
  }
  statement {
    sid = "AllowManageOwnSigningCertificates"
    actions = [
      "iam:DeleteSigningCertificate",
      "iam:ListSigningCertificates",
      "iam:UpdateSigningCertificate",
      "iam:UploadSigningCertificate"
    ]
    effect    = "Allow"
    resources = ["arn:aws:iam::*:user/$${aws:username}"]
  }
  statement {
    sid = "AllowManageOwnSSHPublicKeys"
    actions = [
      "iam:DeleteSSHPublicKey",
      "iam:GetSSHPublicKey",
      "iam:ListSSHPublicKeys",
      "iam:UpdateSSHPublicKey",
      "iam:UploadSSHPublicKey"
    ]
    effect    = "Allow"
    resources = ["arn:aws:iam::*:user/$${aws:username}"]
  }
  statement {
    sid = "AllowManageOwnGitCredentials"
    actions = [
      "iam:CreateServiceSpecificCredential",
      "iam:DeleteServiceSpecificCredential",
      "iam:ListServiceSpecificCredentials",
      "iam:ResetServiceSpecificCredential",
      "iam:UpdateServiceSpecificCredential"
    ]
    effect    = "Allow"
    resources = ["arn:aws:iam::*:user/$${aws:username}"]
  }
  statement {
    sid = "AllowManageOwnVirtualMFADevice"
    actions = [
      "iam:CreateVirtualMFADevice",
      "iam:DeleteVirtualMFADevice"
    ]
    effect    = "Allow"
    resources = ["arn:aws:iam::*:user/$${aws:username}"]
  }
  statement {
    sid = "AllowManageOwnUserMFA"
    actions = [
      "iam:DeactivateMFADevice",
      "iam:EnableMFADevice",
      "iam:ListMFADevices",
      "iam:ResyncMFADevice"
    ]
    effect    = "Allow"
    resources = ["arn:aws:iam::*:user/$${aws:username}"]
  }
  statement {
    sid = "DenyAllExceptListedIfNoMFA"
    not_actions = [
      "iam:CreateVirtualMFADevice",
      "iam:EnableMFADevice",
      "iam:GetUser",
      "iam:ChangePassword",
      "iam:CreateLoginProfile",
      "iam:ListMFADevices",
      "iam:ListVirtualMFADevices",
      "iam:ResyncMFADevice",
      "sts:GetSessionToken"
    ]
    effect    = "Deny"
    resources = ["*"]

    condition {
      test     = "Bool"
      values   = "false"
      variable = "aws:MultiFactorAuthPresent"
    }
  }
}
