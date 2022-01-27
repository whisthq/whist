terraform {
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 3.27"
    }
  }

  required_version = ">= 0.14.9"
}

# Here we call the modules needed across every region

module "vpc" {
  source          = "../aws_vpc"
  env             = var.env
}

module "ec2" {
  source          = "../aws_ec2"
  env             = var.env
}