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

# Gloabl resources

module "iam" {
  source          = "./resources/iam"
  region          = "us-east-2" # The region here does not matter. IAM is global
  employee_emails = var.employee_emails
  admin_emails    = var.admin_emails
}

module "s3" {
  source = "./resources/buckets"
  region = "us-east-2" # The region here does not matter. S3 is in a global namespace
}

# Region specific resources

# Below shows how to deploy to two separate regions
module "us-west-1-infra" {
  source    = "./regions/common"
  region    = "us-west-1"
  vpc-cidrs = var.vpc_cidr
}

module "us-east-2-infra" {
  source    =  "./regions/common"
  region    = "us-east-2"
  vpc-cidrs = var.vpc_cidr
}
