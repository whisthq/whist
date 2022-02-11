terraform {
  # Use an S3 bucket to store state
  # AWS credentials will be filled in by CI.
  backend "s3" {
    bucket = "whist-terraform-state"
    key = "prod/terraform.tfstate"
    region = "us-east-1"
  }

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

# US East
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

# US West
module "us-west-1" {
  source = "../modules/aws_multi_region"
  env    = var.env
  providers = {
    aws = aws.usw1
  }
}

module "us-west-2" {
  source = "../modules/aws_multi_region"
  env    = var.env
  providers = {
    aws = aws.usw2
  }
}

# Canada central
module "us-ca-central-1" {
  source = "../modules/aws_multi_region"
  env    = var.env
  providers = {
    aws = aws.usca1
  }
}
