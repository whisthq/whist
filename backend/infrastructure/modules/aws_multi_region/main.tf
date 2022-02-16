terraform {
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 4.0.0"
    }
  }

  required_version = ">= 0.14.9"
}

# Here we call the modules needed across every region

module "vpc" {
  source = "../aws_vpc"
  env    = var.env
  cidr_block = var.cidr_block
}
