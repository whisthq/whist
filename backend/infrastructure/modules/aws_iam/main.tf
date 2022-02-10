# IAM service-linked roles

resource "aws_iam_service_linked_role" "ServiceRoleForSSM" {
  aws_service_name = "ssm.amazonaws.com"
  tags = {
    Name      = "ServiceRoleForSSM"
    Env       = var.env
    Terraform = true
  }
}

resource "aws_iam_service_linked_role" "ServiceRoleForComputeOptimizer" {
  aws_service_name = "compute-optimizer.amazonaws.com"
  tags = {
    Name      = "ServiceRoleForComputeOptimizer"
    Env       = var.env
    Terraform = true
  }
}

resource "aws_iam_service_linked_role" "ServiceRoleForEC2Spot" {
  aws_service_name = "spot.amazonaws.com"
  tags = {
    Name      = "ServiceRoleForEC2Spot"
    Env       = var.env
    Terraform = true
  }
}

resource "aws_iam_service_linked_role" "ServiceRoleForServiceQuotas" {
  aws_service_name = "servicequotas.amazonaws.com"
  tags = {
    Name      = "ServiceRoleForServiceQuotas"
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
    Terraform = true
  }
}

resource "aws_iam_role" "EC2DeploymentRole" {
  name               = "EC2DeploymentRole"
  assume_role_policy = data.aws_iam_policy_document.EC2AssumeRolePolicy.json

  inline_policy {
    name   = "deployment-role-policy"
    policy = data.aws_iam_policy_document.DeploymentRoleInlinePolicy.json
  }

  tags = {
    Name      = "EC2DeploymentRole"
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
