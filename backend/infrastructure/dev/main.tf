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

}

module "s3" {
  source = "../modules/aws_s3"
  env    = "dev"
}
