# All non-global resources should be declared as modules inside this file.
# We use the multi-region module to "inject" the resources into the regions
# we want, instead of manually enabling them one by one on each.

terraform {
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 4.11.0"
    }
  }

  required_version = ">= 0.14.9"
}

# Here we call the modules needed across every AWS region

module "vpc" {
  source     = "../aws_vpc"
  env        = var.env
  cidr_block = var.cidr_block
}
