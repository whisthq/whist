terraform {
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 3.27"
    }
  }

  required_version = ">= 0.14.9"
}

provider "aws" {
  region = "us-west-1"
}

/*

Section below is for IAM

*/


############ IAM GROUPS ##################

variable "user_emails" {
  description = "Employee Emails"
  type        = set(string)
  default     = ["bezos@whist.com", "musk@whist.com", "branson@whist.com"]
}

resource "aws_iam_user" "employees" {
  for_each = var.user_emails
  name     = each.value
}

resource "aws_iam_user" "admin-users" {
  for_each = var.admins
  name     = each.value
}

resource "aws_iam_group" "FractalEngineering" {
  name = "FractalEngineering"
}

variable "FractalEngineering-managed-policies" {
  description = "FractalEngineering-managed-policies"
  type        = set(string)
  default = [
    "arn:aws:iam::aws:policy/AmazonEC2FullAccess",
    "arn:aws:iam::aws:policy/IAMFullAccess",
    "arn:aws:iam::aws:policy/AmazonS3FullAccess",
    "arn:aws:iam::aws:policy/AmazonSSMFullAccess",
    "arn:aws:iam::aws:policy/AWSSupportAccess"
  ]
}

resource "aws_iam_group_policy_attachment" "FractalEngineering_policy" {
  group      = aws_iam_group.FractalEngineering.name
  for_each   = var.FractalEngineering-managed-policies
  policy_arn = each.key
}

resource "aws_iam_group" "MFA" {
  name = "2FA"
}

resource "aws_iam_group_policy" "MFA-policy" {
  name  = "2FA"
  group = aws_iam_group.MFA.name

  policy = jsonencode(
    {
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

variable "admins" {
  description = "Admin_names"
  type        = set(string)
  default     = ["boss@whist.com"]
}

resource "aws_iam_group" "FractalAWSAdmins" {
  name = "FractalAWSAdmins"
}

variable "FractalAWSAdmins-managed-policies" {
  description = "FractalAWSAdmins-managed-policies"
  type        = set(string)
  default     = ["arn:aws:iam::aws:policy/AdministratorAccess"]
}

resource "aws_iam_group_policy_attachment" "FractalAWSAdmin_policy" {
  group      = aws_iam_group.FractalAWSAdmins.name
  for_each   = var.FractalAWSAdmins-managed-policies
  policy_arn = each.key
}

resource "aws_iam_user_group_membership" "employee-membership" {
  for_each = var.user_emails

  user   = aws_iam_user.employees[each.key].name
  groups = [aws_iam_group.FractalEngineering.name, aws_iam_group.MFA.name]
}

resource "aws_iam_user_group_membership" "admin-membership" {
  for_each = var.admins
  user     = aws_iam_user.admin-users[each.key].name
  groups   = [aws_iam_group.FractalAWSAdmins.name]
}

########## IAM ROLES ##############

# Service linked roles

resource "aws_iam_service_linked_role" "AWSServiceRoleForAmazonSSM" {
  aws_service_name = "ssm.amazonaws.com"
}

resource "aws_iam_service_linked_role" "AWSServiceRoleForAccessAnalyzer" {
  aws_service_name = "access-analyzer.amazonaws.com"
}

resource "aws_iam_service_linked_role" "AWSServiceRoleForSupport" {
  aws_service_name = "support.amazonaws.com"
}

resource "aws_iam_service_linked_role" "AWSServiceRoleForServiceQuotas" {
  aws_service_name = "servicequotas.amazonaws.com"
}

# Custom roles

resource "aws_iam_role" "PackerAMIBuilder" {
  name        = "PackerAMIBuilder"
  description = "Allows EC2 instances to call AWS EC2 Image Builder and S3 services on your behalf. This role is designed for usage with Packer to automate building AMIs."

  # Terraform's "jsonencode" function converts a
  # Terraform expression result to valid JSON syntax.
  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Action = "sts:AssumeRole"
        Effect = "Allow"
        Sid    = ""
        Principal = {
          Service = "ec2.amazonaws.com"
        }
      },
    ]
  })

  managed_policy_arns = ["arn:aws:iam::aws:policy/AmazonS3ReadOnlyAccess"]
}

/*

Section below is for S3

*/

resource "aws_s3_bucket" "fractal-user-app-configs" {
  bucket = "fractal-user-app-configs1"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }

  versioning {
    enabled = true
  }
}

resource "aws_s3_bucket" "fractal-chromium-macos-arm64-prod" {
  bucket = "fractal-chromium-macos-arm64-prod1"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }

}

resource "aws_s3_bucket" "fractal-chromium-macos-arm64-staging" {
  bucket = "fractal-chromium-macos-arm64-staging1"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }

}

resource "aws_s3_bucket" "fractal-chromium-macos-arm64-dev" {
  bucket = "fractal-chromium-macos-arm64-dev1"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }

}

resource "aws_s3_bucket" "fractal-chromium-macos-dev" {
  bucket = "fractal-chromium-macos-dev1"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }

}

resource "aws_s3_bucket" "fractal-chromium-macos-prod" {
  bucket = "fractal-chromium-macos-prod1"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }

}

resource "aws_s3_bucket" "fractal-chromium-macos-staging" {
  bucket = "fractal-chromium-macos-staging1"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }

}

resource "aws_s3_bucket" "fractal-test-assets" {
  bucket = "fractal-test-assets1"
}

resource "aws_s3_bucket" "fractal-shared-libs" {
  bucket = "fractal-shared-libs1"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }

}


/*

Section below is for VPC

*/

variable "environments" {
  description = "Whist environments"
  type        = set(string)
  default     = ["prod", "staging", "dev", "localdev"]
}

// add some tags to the DHCP options to show developers, hey, this isn't useless
// so for the love of all that is holy, do not delete it
resource "aws_default_vpc_dhcp_options" "whist-dhcp-options" {
  tags = {
    Description = "Default Whist DHCP options set for VPCs in this region"
  }
}

// We need a separate VPC for each region... 
// for prod, staging, dev, localdev
resource "aws_vpc" "whist-vpcs" {
  for_each             = var.environments
  cidr_block           = "172.31.0.0/16"
  enable_dns_hostnames = true
  enable_dns_support   = true
  instance_tenancy     = "default"
  tags = {
    Name        = format("whist-%s-vpc", each.key)
    Environment = format("%s", each.key)
  }
}

// configure the necessary security groups
resource "aws_security_group" "whist-non-prod-security-groups" {
  # Applies to all VPCs that are NOT prod
  for_each = { for key, val in aws_vpc.whist-vpcs : key => val if val.tags_all["Environment"] != "prod" }
  vpc_id   = each.value.id

  name        = format("whist-%s-security-group", each.value.tags_all["Environment"])
  description = format("Security group for %s", each.value.tags_all["Name"])


  ingress {
    description = "whist-http-rule"
    protocol    = "tcp"
    from_port   = 80
    to_port     = 80
    cidr_blocks = ["0.0.0.0/0"]
  }

  ingress {
    description = "whist-udp-rule"
    protocol    = "udp"
    from_port   = 1025
    to_port     = 49150
    cidr_blocks = ["0.0.0.0/0"]
  }

  ingress {
    description = "whist-tcp-rule"
    protocol    = "tcp"
    from_port   = 1025
    to_port     = 49150
    cidr_blocks = ["0.0.0.0/0"]
  }

  ingress {
    description = "whist-ssh-rule"
    to_port     = 22
    from_port   = 22
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }

  egress {
    description = "whist-ipv4-rule"
    protocol    = "-1"
    to_port     = 0
    from_port   = 0
    cidr_blocks = ["0.0.0.0/0"]
  }

  # egress {
  #   description = "whist-ipv6-rule"
  #   protocol = "-1"
  #   to_port = 0
  #   from_port = 0
  #   cidr_blocks = ["::/0"]
  # }

  tags = {
    Name        = format("whist-%s-security-group", each.value.tags_all["Environment"])
    Environment = format("%s", each.value.tags_all["Environment"])
  }
}

resource "aws_security_group" "whist-prod-security-group" {
  # Applies to all VPCs in prod
  for_each = { for key, val in aws_vpc.whist-vpcs : key => val if val.tags_all["Environment"] == "prod" }
  vpc_id   = each.value.id

  name        = format("whist-%s-security-group", each.value.tags_all["Environment"])
  description = format("Security group for %s", each.value.tags_all["Name"])


  ingress {
    description = "whist-http-rule"
    protocol    = "tcp"
    from_port   = 80
    to_port     = 80
    cidr_blocks = ["0.0.0.0/0"]
  }

  ingress {
    description = "whist-udp-rule"
    protocol    = "udp"
    from_port   = 1025
    to_port     = 49150
    cidr_blocks = ["0.0.0.0/0"]
  }

  ingress {
    description = "whist-tcp-rule"
    protocol    = "tcp"
    from_port   = 1025
    to_port     = 49150
    cidr_blocks = ["0.0.0.0/0"]
  }

  egress {
    description = "whist-ipv4-rule"
    protocol    = "-1"
    to_port     = 0
    from_port   = 0
    cidr_blocks = ["0.0.0.0/0"]
  }

  tags = {
    Name        = format("whist-%s-security-group", each.value.tags_all["Environment"])
    Environment = format("%s", each.value.tags_all["Environment"])
  }

}

// Make the internet gateways for each environment VPC
resource "aws_internet_gateway" "whist-internet-gateways" {
  for_each = aws_vpc.whist-vpcs
  vpc_id   = each.value.id

  tags = {
    Environment = format("%s", each.value.tags_all["Environment"])
    Name        = format("whist-%s-internet-gateway", each.value.tags_all["Environment"])
    Description = format("Internet gateway for %s", each.value.tags_all["Name"])
  }
}

locals {
  vpc_ids = tolist([for vpc in aws_vpc.whist-vpcs : vpc.default_route_table_id])
  gw_ids = tolist([for gw in aws_internet_gateway.whist-internet-gateways : gw.id])
}

resource "aws_default_route_table" "whist-route-tables" {
  for_each = zipmap(local.vpc_ids, local.gw_ids)
  default_route_table_id = each.key

  route {
    cidr_block = "0.0.0.0/0"
    gateway_id = each.value
  }
}

// make the subnets for each environment
resource "aws_subnet" "whist-subnets" {
  for_each = aws_vpc.whist-vpcs
  vpc_id   = each.value.id

  cidr_block              = "172.31.0.0/20"
  map_public_ip_on_launch = true

  tags = {
    Environment = format("%s", each.value.tags_all["Environment"])
    Name        = format("whist-%s-subnet", each.value.tags_all["Environment"])
    Description = format("Subnet for %s", each.value.tags_all["Name"])
  }

}

/* 

  Secret Manager

*/

resource "aws_secretsmanager_secret" "github-secret" {
  name        = "ghcrDockerAuthentication"
  description = "Phil's GitHub Personal Access Token and Username, used for authenticating \"Read\" permissions with GitHub Packages for pulling container images from ghcr.io	"
}