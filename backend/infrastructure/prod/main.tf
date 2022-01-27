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
  access_key = "AKIAR5GLDA2KXBNIVLU7"
  secret_key = "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"
  region = "us-east-1"
}

module "iam" {
  source = "../modules/aws_iam"
  env    = var.env
}

module "vpc" {
  source          = "../modules/aws_vpc"
  env             = var.env
  enabled_regions = var.enabled_regions
}

module "ec2" {
  source          = "../modules/aws_ec2"
  env             = var.env
}

module "s3" {
  source = "../modules/aws_s3"
  env    = var.env
}
