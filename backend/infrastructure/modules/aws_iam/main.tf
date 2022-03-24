#
# IAM service-linked roles
#

# This role is linked to the Systems Manager service, and includes all of the permissions required by that service.
resource "aws_iam_service_linked_role" "ServiceRoleForSSM" {
  count            = var.env == "prod" ? 1 : 0
  aws_service_name = "ssm.amazonaws.com"
  description      = "Provides access to AWS Resources managed or used by Amazon SSM"
  tags = {
    Name      = "ServiceRoleForSSM"
    Env       = var.env
    Terraform = true
  }
}

# This role is linked to the Compute Optimizer service, and includes all of the permissions required by that service.
resource "aws_iam_service_linked_role" "ServiceRoleForComputeOptimizer" {
  count            = var.env == "prod" ? 1 : 0
  aws_service_name = "compute-optimizer.amazonaws.com"
  description      = "Allows ComputeOptimizer to call AWS services and collect workload details on your behalf"
  tags = {
    Name      = "ServiceRoleForComputeOptimizer"
    Env       = var.env
    Terraform = true
  }
}

# This role is linked to the EC2 Spot service, and includes all of the permissions required by that service.
resource "aws_iam_service_linked_role" "ServiceRoleForEC2Spot" {
  count            = var.env == "prod" ? 1 : 0
  aws_service_name = "spot.amazonaws.com"
  description      = "Default EC2 Spot Service Linked Role"
  tags = {
    Name      = "ServiceRoleForEC2Spot"
    Env       = var.env
    Terraform = true
  }
}

# This role is linked to the Service Quotas service, and includes all of the permissions required by that service.
resource "aws_iam_service_linked_role" "ServiceRoleForServiceQuotas" {
  count            = var.env == "prod" ? 1 : 0
  aws_service_name = "servicequotas.amazonaws.com"
  description      = "A service-linked role is required for Service Quotas to access your service limits"
  tags = {
    Name      = "ServiceRoleForServiceQuotas"
    Env       = var.env
    Terraform = true
  }
}

#
# IAM Whist-created roles
#

# This role is used by Packer to build AMIs, its policy has the minimum amount of permissions to operate.
resource "aws_iam_role" "PackerAMIBuilder" {
  count              = var.env == "dev" ? 1 : 0
  name               = "PackerAMIBuilder"
  description        = "This role is used by Packer to build AMIs, its policy has the minimum amount of permissions to operate"
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

# This role is used by the scaling service and webserver to manage instances.
resource "aws_iam_role" "EC2DeploymentRole" {
  name               = "EC2DeploymentRole${var.env}"
  description        = "This role is used by the scaling service and webserver to manage instances"
  assume_role_policy = data.aws_iam_policy_document.EC2AssumeRolePolicy.json

  inline_policy {
    name   = "WhistEC2DeploymentRolePolicy"
    policy = data.aws_iam_policy_document.WhistEC2DeploymentRolePolicy.json
  }

  tags = {
    Name      = "EC2DeploymentRole${var.env}"
    Env       = var.env
    Terraform = true
  }
}

# We create an instance profile for the deployment role because 
# Terraform doesn't create it by default. We use a different instance
# profile for each environment.
resource "aws_iam_instance_profile" "EC2DeploymentRoleInstanceProfile" {
  name = "EC2DeploymentRoleInstanceProfile${var.env}"
  role = aws_iam_role.EC2DeploymentRole.name
}

#
# IAM User groups
#

# All Whist employees are in this group. It forces 2-FA for all AWS console users.
resource "aws_iam_group" "Whist2FA" {
  count = var.env == "prod" ? 1 : 0
  name  = "Whist2FA"
}

# This group confers full AWS administrative privileges to its members.
resource "aws_iam_group" "WhistAdmins" {
  count = var.env == "prod" ? 1 : 0
  name  = "WhistAdmins"
}

# This group contains the Whist IAM role (s) used in our GitHub Actions pipelines.
resource "aws_iam_group" "WhistCI" {
  count = var.env == "prod" ? 1 : 0
  name  = "WhistCI"
}

# This is the general user group for Whist engineers, containing the necessary 
# permissions for developing Whist.
resource "aws_iam_group" "WhistEngineers" {
  count = var.env == "prod" ? 1 : 0
  name  = "WhistEngineers"
}

#
# IAM Users
#

# Employee users are created/deleted manually by the Whist team when an employee joins/leaves.

# This user has the DeploymentRolePolicy attached, and will simply pass those permissions to the
# DeploymentRole so it is able to manage user EC2 instances.
resource "aws_iam_user" "WhistEC2PassRoleUser" {
  name = "WhistEC2PassRole${var.env}"

  tags = {
    Name      = "WhistEC2PassRoleUser${var.env}"
    Env       = var.env
    Terraform = true
  }
}

#
# IAM User access keys
#

resource "aws_iam_access_key" "WhistEC2PassRoleUserAccessKey" {
  user   = aws_iam_user.WhistEC2PassRoleUser.name
  status = "Active"
}

#
# IAM Custom group policies
#

# This policy forces 2-FA for all Whist engineers on AWS, and is attached to the 
# Whist2FA group.
resource "aws_iam_group_policy" "ForceMFA" {
  count  = var.env == "prod" ? 1 : 0
  name   = "ForceMFA"
  group  = aws_iam_group.Whist2FA[0].id
  policy = data.aws_iam_policy_document.MFAPolicy.json
}

#
# IAM AWS-managed group policies
#

# This policy gives WhistAdmins full AWS permissions.
resource "aws_iam_group_policy_attachment" "AdminPolicy" {
  count      = var.env == "prod" ? 1 : 0
  group      = aws_iam_group.WhistAdmins[0].name
  policy_arn = "arn:aws:iam::aws:policy/AdministratorAccess"
}

# This policy gives WhistCI the basic permissions required for our GitHub Actions pipelines to function.
resource "aws_iam_group_policy_attachment" "CIPolicy" {
  group = aws_iam_group.WhistCI[0].name
  for_each = var.env == "prod" ? toset([
    "arn:aws:iam::aws:policy/AmazonEC2FullAccess",
    "arn:aws:iam::aws:policy/AmazonS3FullAccess",
    "arn:aws:iam::aws:policy/AmazonVPCFullAccess",
    "arn:aws:iam::aws:policy/IAMFullAccess",
    "arn:aws:iam::aws:policy/SecretsManagerReadWrite",
  ]) : []
  policy_arn = each.value
}

# This policy gives WhistEngineers the basic permissions required for our developing Whist.
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

#
# IAM Policy and policy attachment for WhistEC2PassRoleUser
#

resource "aws_iam_policy" "WhistEC2PassRoleUserPolicy" {
  name        = "WhistEC2PassRoleUserPolicy${var.env}"
  description = "This policy grants the PassRoleUser the permissions to pass the DeploymentRole to instances."
  policy      = data.aws_iam_policy_document.WhistEC2PassRoleUserPolicy.json
}

resource "aws_iam_user_policy_attachment" "WhistEC2PassRoleUserPolicyAttachment" {
  user       = aws_iam_user.WhistEC2PassRoleUser.name
  policy_arn = aws_iam_policy.WhistEC2PassRoleUserPolicy.arn
}
