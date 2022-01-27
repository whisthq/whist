# Data for IAM role assume policies

data "aws_iam_policy_document" "appstream-assume-role-policy" {
  statement {
    actions = ["sts:AssumeRole"]

    principals {
      type        = "Service"
      identifiers = ["appstream.amazonaws.com"]
    }
  }
}

data "aws_iam_policy_document" "autoscaling-assume-role-policy" {
  statement {
    actions = ["sts:AssumeRole"]

    principals {
      type        = "Service"
      identifiers = ["application-autoscaling.amazonaws.com"]
    }
  }
}

data "aws_iam_policy_document" "spotfleet-assume-role-policy" {
  statement {
    actions = ["sts:AssumeRole"]

    principals {
      type        = "Service"
      identifiers = ["spotfleet.amazonaws.com"]
    }
  }
}

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

resource "aws_iam_service_linked_role" "ServiceRoleForEC2SpotFleet" {
  aws_service_name = "spotfleet.amazonaws.com"  
}

resource "aws_iam_service_linked_role" "ServiceRoleForServiceQuotas" {
  aws_service_name = "servicequotas.amazonaws.com"  
}

# IAM roles

resource "aws_iam_role" "AmazonAppStreamServiceAccess" {
  name = "AmazonAppStreamServiceAccess" 
  assume_role_policy  = data.aws_iam_policy_document.appstream-assume-role-policy.json
  managed_policy_arns = [
    "arn:aws:iam::aws:policy/service-role/AmazonAppStreamServiceAccess"
  ]
}

resource "aws_iam_role" "ApplicationAutoScalingForAmazonAppStreamAccess" {
  name = "ApplicationAutoScalingForAmazonAppStreamAccess"
  assume_role_policy  = data.aws_iam_policy_document.autoscaling-assume-role-policy.json
  
  managed_policy_arns = [
    "arn:aws:iam::aws:policy/service-role/ApplicationAutoScalingForAmazonAppStreamAccess"
  ]
}

resource "aws_iam_role" "aws-ec2-spot-fleet-autoscale-role" {
  name = "aws-ec2-spot-fleet-autoscale-role"
  assume_role_policy  = data.aws_iam_policy_document.autoscaling-assume-role-policy.json

  managed_policy_arns = [
    "arn:aws:iam::aws:policy/service-role/AmazonEC2SpotFleetAutoscaleRole"
  ]
}

resource "aws_iam_role" "aws-ec2-spot-fleet-tagging-role" {
  name = "aws-ec2-spot-fleet-tagging-role"
  assume_role_policy  = data.aws_iam_policy_document.spotfleet-assume-role-policy.json

  managed_policy_arns = [
    "arn:aws:iam::aws:policy/service-role/AmazonEC2SpotFleetTaggingRole"
  ]
}

resource "aws_iam_role" "PackerAMIBuilder" {
  name = "PackerAMIBuilder"
  assume_role_policy  = data.aws_iam_policy_document.ec2-assume-role-policy.json

  managed_policy_arns = [
    "arn:aws:iam::aws:policy/AmazonS3ReadOnlyAccess"
  ]
}

resource "aws_iam_role" "TestDeploymentRole" {
  name = "TestDeploymentRole"
  assume_role_policy  = data.aws_iam_policy_document.ec2-assume-role-policy.json

  managed_policy_arns = [
    "arn:aws:iam::aws:policy/AmazonEC2FullAccess",
    "arn:aws:iam::aws:policy/AmazonS3FullAccess",
    "arn:aws:iam::aws:policy/AmazonEC2ReadOnlyAccess",
    "arn:aws:iam::aws:policy/AmazonSSMFullAccess"
  ]
}

resource "aws_iam_role" "container-admin" {
  name = format("container-admin-%s", var.env)
  assume_role_policy  = data.aws_iam_policy_document.ec2-assume-role-policy.json

  managed_policy_arns = [
    "arn:aws:iam::aws:policy/AmazonEC2FullAccess",
    "arn:aws:iam::aws:policy/SecretsManagerReadWrite",
    "arn:aws:iam::aws:policy/IAMFullAccess",
    "arn:aws:iam::aws:policy/AmazonS3FullAccess",
    "arn:aws:iam::aws:policy/AmazonVPCFullAccess"
  ]
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

# Custom group policies

resource "aws_iam_group_policy" "MFA-policy" {
  name   = "Force_MFA"
  group  = aws_iam_group.whist-2FA.id
  policy = jsonencode(var.mfa-policy)
}

# AWS managed group policies

resource "aws_iam_group_policy_attachment" "admin-policy" {
  group      = aws_iam_group.whist-admins.name
  for_each   = var.whist-admins-managed-policies
  policy_arn = each.key
}

resource "aws_iam_group_policy_attachment" "ci-policy" {
  group      = aws_iam_group.whist-engineers.name
  for_each   = var.whist-ci-managed-policies
  policy_arn = each.key
}

resource "aws_iam_group_policy_attachment" "engineering-policy" {
  group      = aws_iam_group.whist-engineers.name
  for_each   = var.whist-engineers-managed-policies
  policy_arn = each.key
}
