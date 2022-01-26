# IAM roles

resource "aws_iam_role" "AmazonAppStreamServiceAccess" {
  name = "AmazonAppStreamServiceAccess" 
  managed_policy_arns = [
    "arn:aws:iam::aws:policy/service-role/AmazonAppStreamServiceAccess"
  ]
}

resource "aws_iam_role" "ApplicationAutoScalingForAmazonAppStreamAccess" {
  name = "ApplicationAutoScalingForAmazonAppStreamAccess"
  managed_policy_arns = [
    "arn:aws:iam::aws:policy/service-role/ApplicationAutoScalingForAmazonAppStreamAccess"
  ]
}


resource "aws_iam_role" "aws-ec2-spot-fleet-autoscale-role" {
  name = "aws-ec2-spot-fleet-autoscale-role"
  managed_policy_arns = [
    "arn:aws:iam::aws:policy/service-role/AmazonEC2SpotFleetAutoscaleRole"
  ]
}


resource "aws_iam_role" "aws-ec2-spot-fleet-tagging-role" {
  name = "aws-ec2-spot-fleet-tagging-role"
    managed_policy_arns = [
    "arn:aws:iam::aws:policy/service-role/AmazonEC2SpotFleetTaggingRole"
  ]
}


resource "aws_iam_role" "AWSServiceRoleForAmazonSSM" {
  name = "AWSServiceRoleForAmazonSSM"
  managed_policy_arns = [
    "arn:aws:iam::aws:policy/aws-service-role/AmazonSSMServiceRolePolicy"
  ]
}

resource "aws_iam_role" "AWSServiceRoleForComputeOptimizer" {
  name = "AWSServiceRoleForComputeOptimizer"
  managed_policy_arns = [
    "arn:aws:iam::aws:policy/aws-service-role/ComputeOptimizerServiceRolePolicy"
  ]
}


resource "aws_iam_role" "AWSServiceRoleForEC2Spot" {
  name = "AWSServiceRoleForEC2Spot"
  managed_policy_arns = [
    "arn:aws:iam::aws:policy/aws-service-role/AWSEC2SpotServiceRolePolicy"
  ]
}

resource "aws_iam_role" "AWSServiceRoleForEC2SpotFleet" {
  name = "AWSServiceRoleForEC2SpotFleet"
  managed_policy_arns = [
    "arn:aws:iam::aws:policy/aws-service-role/AWSEC2SpotFleetServiceRolePolicy"
  ]
}

resource "aws_iam_role" "AWSServiceRoleForServiceQuotas" {
  name = "AWSServiceRoleForServiceQuotas"
  managed_policy_arns = [
    "arn:aws:iam::aws:policy/aws-service-role/ServiceQuotasServiceRolePolicy"
  ]
}

resource "aws_iam_role" "AWSServiceRoleForSupport" {
  name = "AWSServiceRoleForSupport"
  managed_policy_arns = [
    "arn:aws:iam::aws:policy/aws-service-role/AWSSupportServiceRolePolicy"
  ]
}

resource "aws_iam_role" "AWSServiceRoleForTrustedAdvisor" {
  name = "AWSServiceRoleForTrustedAdvisor"
  managed_policy_arns = [
    "arn:aws:iam::aws:policy/aws-service-role/AWSTrustedAdvisorServiceRolePolicy"
  ]
}

resource "aws_iam_role" "PackerAMIBuilder" {
  name = "PackerAMIBuilder"
  managed_policy_arns = [
    "arn:aws:iam::aws:policy/AmazonS3ReadOnlyAccess"
  ]
}

resource "aws_iam_role" "TestDeploymentRole" {
  name = "TestDeploymentRole"
  managed_policy_arns = [
    "arn:aws:iam::aws:policy/AmazonEC2FullAccess",
    "arn:aws:iam::aws:policy/AmazonS3FullAccess",
    "arn:aws:iam::aws:policy/AmazonEC2ReadOnlyAccess",
    "arn:aws:iam::aws:policy/AmazonSSMFullAccess"
  ]
}

resource "aws_iam_role" "container-admin" {
  name = format("container-admin-%s", var.env)
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

resource "aws_iam_group_policy" "2FA-policy" {
  name   = "Force_MFA"
  group  = aws_iam_group.whist-2FA
  policy = jsonencode
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
