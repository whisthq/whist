# IAM service-linked roles

# This role is linked to Systems Manager service, and includes all of the permissions required by that service.
resource "aws_iam_service_linked_role" "ServiceRoleForSSM" {
  count  = var.env == "prod" ? 1 : 0
  aws_service_name = "ssm.amazonaws.com"
  tags = {
    Name      = "ServiceRoleForSSM"
    Env       = var.env
    Terraform = true
  }
}

# This role is linked to Compute Optimizer service, and includes all of the permissions required by that service.
resource "aws_iam_service_linked_role" "ServiceRoleForComputeOptimizer" {
  count  = var.env == "prod" ? 1 : 0
  aws_service_name = "compute-optimizer.amazonaws.com"
  tags = {
    Name      = "ServiceRoleForComputeOptimizer"
    Env       = var.env
    Terraform = true
  }
}

# This role is linked to EC2 Spot service, and includes all of the permissions required by that service.
resource "aws_iam_service_linked_role" "ServiceRoleForEC2Spot" {
  count  = var.env == "prod" ? 1 : 0
  aws_service_name = "spot.amazonaws.com"
  tags = {
    Name      = "ServiceRoleForEC2Spot"
    Env       = var.env
    Terraform = true
  }
}

# This role is linked to Service Quotas service, and includes all of the permissions required by that service.
resource "aws_iam_service_linked_role" "ServiceRoleForServiceQuotas" {
  count  = var.env == "prod" ? 1 : 0
  aws_service_name = "servicequotas.amazonaws.com"
  tags = {
    Name      = "ServiceRoleForServiceQuotas"
    Env       = var.env
    Terraform = true
  }
}

# IAM roles

# This role is used by Packer to build AMIs, its policy has the minimum amount of permissions to operate.
resource "aws_iam_role" "PackerAMIBuilder" {
  count  = var.env == "dev" ? 1 : 0
  name               = "PackerAMIBuilder"
  assume_role_policy = data.aws_iam_policy_document.EC2AssumeRolePolicy.json

  inline_policy {
    name   = "packer-policy"
    policy = data.aws_iam_policy_document.PackerAMIBuilderInlinePolicy.json
  }

  tags = {
    Name      = "PackerAMIBuilder"
    Env       = var.env
    Terraform = true
  }
}

# This role is used by the scaling service and webserver to manage instances. It gets its permissions from the WhistEC2PassRoleUser.
resource "aws_iam_role" "EC2DeploymentRole" {
  name               = "EC2DeploymentRole${var.env}"
  assume_role_policy = data.aws_iam_policy_document.EC2AssumeRolePolicy.json

  tags = {
    Name      = "EC2DeploymentRole${var.env}"
    Env       = var.env
    Terraform = true
  }
}

#IAM User groups

resource "aws_iam_group" "Whist2FA" {
  count  = var.env == "prod" ? 1 : 0
  name = "Whist2FA"
}

resource "aws_iam_group" "WhistAdmins" {
  count  = var.env == "prod" ? 1 : 0
  name = "WhistAdmins"
}

resource "aws_iam_group" "WhistCI" {
  count  = var.env == "prod" ? 1 : 0
  name = "WhistCI"
}

resource "aws_iam_group" "WhistEngineers" {
  count  = var.env == "prod" ? 1 : 0
  name = "WhistEngineers"
}

# Users

# This user has the DeploymentRolePolicy attached, and will simply pass those permissions to the DeploymentRole so it is able to manage instances.
resource "aws_iam_user" "WhistEC2PassRoleUser" {
  name = "WhistEC2PassRole${var.env}"

  tags = {
    Name = "WhistEC2PassRoleUser${var.env}"
    Env = var.env
    Terraform = true
  }
}

# Custom group policies

resource "aws_iam_group_policy" "ForceMFA" {
  count  = var.env == "prod" ? 1 : 0
  name   = "ForceMFA"
  group  = aws_iam_group.Whist2FA[0].id
  policy = data.aws_iam_policy_document.MFAPolicy.json
}


# AWS managed group policies

resource "aws_iam_group_policy_attachment" "AdminPolicy" {
  count  = var.env == "prod" ? 1 : 0
  group      = aws_iam_group.WhistAdmins[0].name
  policy_arn = "arn:aws:iam::aws:policy/AdministratorAccess"
}

resource "aws_iam_group_policy_attachment" "CIPolicy" {
  group = aws_iam_group.WhistCI[0].name
  for_each = var.env == "prod" ? toset([
    "arn:aws:iam::aws:policy/AmazonEC2FullAccess",
    "arn:aws:iam::aws:policy/IAMFullAccess",
    "arn:aws:iam::aws:policy/AmazonS3FullAccess",
  ]) : []
  policy_arn = each.value
}

resource "aws_iam_group_policy_attachment" "EngineeringPolicy" {
  group = aws_iam_group.WhistEngineers[0].name
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
  name = "WhistEC2PassRoleUserPolicy${var.env}"
  description = "This policy gives the necessary permissions to start, stop and terminate on-demand and spot instances. It is meant to be used by the WhistEC2PassRoleUser."
  policy = data.aws_iam_policy_document.WhistEC2PassRoleUserPolicy.json
}

resource "aws_iam_user_policy_attachment" "WhistEC2PassRoleUserPolicyAttachment" {
  user = aws_iam_user.WhistEC2PassRoleUser.name
  policy_arn = aws_iam_policy.WhistEC2PassRoleUserPolicy.arn
}
