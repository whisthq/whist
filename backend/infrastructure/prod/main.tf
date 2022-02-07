terraform {
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 3.27"
    }
  }

  required_version = ">= 0.14.9"
}

# Global modules, use default provider

module "iam" {
  source                     = "../modules/aws_iam"
  env                        = var.env
  whist-env-managed-policies = var.whist-env-managed-policies
}

module "s3" {
  source = "../modules/aws_s3"
  env    = var.env
}

module "secrets-manager" {
  source = "../modules/aws_secrets_manager"
  env    = var.env
}

# Region specific modules

module "us-east-1" {
  source = "../modules/aws_multi_region"
  env    = var.env
}

module "us-east-2" {
  source = "../modules/aws_multi_region"
  env    = var.env
  providers = {
    aws = aws.use2
  }
}