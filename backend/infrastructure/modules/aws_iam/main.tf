# IAM service-linked roles

resource "aws_iam_service_linked_role" "ServiceRoleForSSM" {
  aws_service_name = "ssm.amazonaws.com"
  tags = {
    Name      = "ServiceRoleForSSM"
    Description = "This role is linked to Systems Manager service, and includes all of the permissions required by that service."
    Env       = var.env
    Terraform = true
  }
}

resource "aws_iam_service_linked_role" "ServiceRoleForComputeOptimizer" {
  aws_service_name = "compute-optimizer.amazonaws.com"
  tags = {
    Name      = "ServiceRoleForComputeOptimizer"
    Description = "This role is linked to Compute Optimizer service, and includes all of the permissions required by that service."
    Env       = var.env
    Terraform = true
  }
}

resource "aws_iam_service_linked_role" "ServiceRoleForEC2Spot" {
  aws_service_name = "spot.amazonaws.com"
  tags = {
    Name      = "ServiceRoleForEC2Spot"
    Description = "This role is linked to EC2 Spot service, and includes all of the permissions required by that service."
    Env       = var.env
    Terraform = true
  }
}

resource "aws_iam_service_linked_role" "ServiceRoleForServiceQuotas" {
  aws_service_name = "servicequotas.amazonaws.com"
  tags = {
    Name      = "ServiceRoleForServiceQuotas"
    Description = "This role is linked to Service Quotas service, and includes all of the permissions required by that service."
    Env       = var.env
    Terraform = true
  }
}

# IAM roles

resource "aws_iam_role" "PackerAMIBuilder" {
  name               = "PackerAMIBuilder"
  assume_role_policy = data.aws_iam_policy_document.EC2AssumeRolePolicy.json

  inline_policy {
    name   = "packer-policy"
    policy = data.aws_iam_policy_document.PackerAMIBuilderInlinePolicy.json
  }

  tags = {
    Name      = "PackerAMIBuilder"
    Env       = var.env
    Description = "This role is used by Packer to build AMIs, its policy has the minimum amount of permissions to operate."
    Terraform = true
  }
}

resource "aws_iam_role" "EC2DeploymentRole" {
  name               = "EC2DeploymentRole"
  assume_role_policy = data.aws_iam_policy_document.EC2AssumeRolePolicy.json

  tags = {
    Name      = "EC2DeploymentRole"
    Description = "This role is used by the scaling service and webserver to manage instances. It gets its permissions from the WhistEC2PassRoleUser."
    Env       = var.env
    Terraform = true
  }
}

#IAM User groups

resource "aws_iam_group" "Whist2FA" {
  name = "Whist2FA"
}

resource "aws_iam_group" "WhistAdmins" {
  name = "WhistAdmins"
}

resource "aws_iam_group" "WhistCI" {
  name = "WhistCI"
}

resource "aws_iam_group" "WhistEngineers" {
  name = "WhistEngineers"
}

resource "aws_iam_group" "WhistEC2InstanceManager" {
  name = "WhistEC2InstanceManager${var.env}"
}

# Users

resource "aws_iam_user" "WhistEC2PassRoleUser" {
  name = "WhistEC2PassRole${var.env}"

  tags = {
    Name = "WhistEC2PassRoleUser${var.env}"
    Description = "This user has the DeploymentRolePolicy attached, and will simply pass those permissions to the DeploymentRole so it is able to manage instances."
    Env = var.env
    Terraform = true
  }
}

# Custom group policies

resource "aws_iam_group_policy" "ForceMFA" {
  name   = "ForceMFA"
  group  = aws_iam_group.Whist2FA.id
  policy = data.aws_iam_policy_document.MFAPolicy.json
}


# AWS managed group policies

resource "aws_iam_group_policy_attachment" "AdminPolicy" {
  group      = aws_iam_group.WhistAdmins.name
  policy_arn = "arn:aws:iam::aws:policy/AdministratorAccess"
}

resource "aws_iam_group_policy_attachment" "CIPolicy" {
  group = aws_iam_group.WhistCI.name
  for_each = var.env == "prod" ? toset([
    "arn:aws:iam::aws:policy/AmazonEC2FullAccess",
    "arn:aws:iam::aws:policy/IAMFullAccess",
    "arn:aws:iam::aws:policy/AmazonS3FullAccess",
  ]) : []
  policy_arn = each.value
}

resource "aws_iam_group_policy_attachment" "EngineeringPolicy" {
  group = aws_iam_group.WhistEngineers.name
  for_each = var.env == "prod" ? toset([
    "arn:aws:iam::aws:policy/AmazonEC2FullAccess",
    "arn:aws:iam::aws:policy/IAMFullAccess",
    "arn:aws:iam::aws:policy/AmazonS3FullAccess",
    "arn:aws:iam::aws:policy/AmazonSSMFullAccess",
    "arn:aws:iam::aws:policy/AWSSupportAccess",
  ]) : []
  policy_arn = each.value
}

# Policy and policy attachment for WhistEC2PassRoleUser

resource "aws_iam_policy" "WhistEC2PassRoleUserPolicy" {
  name = "WhistEC2PassRoleUserPolicy"
  description = "This policy gives the necessary permissions to start, stop and terminate on-demand and spot instances. It is meant to be used by the WhistEC2PassRoleUser."
  policy = data.aws_iam_policy_document.WhistEC2PassRoleUserPolicy.json
}

resource "aws_iam_user_policy_attachment" "WhistEC2PassRoleUserPolicyAttachment" {
  user = aws_iam_user.WhistEC2PassRoleUser.name
  policy_arn = aws_iam_policy.WhistEC2PassRoleUserPolicy.arn
}
