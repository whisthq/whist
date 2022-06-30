terraform {
  # We use an S3 bucket to store Terraform state
  # AWS credentials will be filled in by CI.
  backend "s3" {
    bucket = "whist-terraform-state"
    key    = "staging/terraform.tfstate"
    region = "us-east-1"
  }

  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 4.22.0"
    }
  }

  required_version = ">= 0.14.9"
}

# Global modules, use default provider

module "iam" {
  source = "../modules/aws_iam"
  env    = var.env
}

module "s3" {
  source = "../modules/aws_s3"
  env    = var.env
}

module "s3-control" {
  source = "../modules/aws_s3_control"
  env    = var.env
  regions = [
    "us-east-1",
    "us-west-1",
    "eu-central-1",
    "ap-southeast-1",
    "sa-east-1"
  ]
}

# Region-specific modules, these are enabled only on certain regions

# These are the regions where user config bucket replication is currently enabled. They are meant to represent
# a good world coverage so that users in any geography can retrieve their config from a nearby bucket,
# for faster launch/load time. If we want to optimize this further, we can add more regions here to get
# more replication closer to users as the userbase grows.

module "user-configs-us-east-1" {
  source = "../modules/aws_user_configs"
  env    = var.env
  # us-east-1
  # List of regions in order of proximity.
  regions = [
    "us-west-1",
    "sa-east-1",
    "eu-central-1",
    "ap-southeast-1",
  ]
  replication_role_arn = module.iam.replication_role_arn
}

module "user-configs-us-west-1" {
  source = "../modules/aws_user_configs"
  env    = var.env
  providers = {
    # us-west-1
    aws = aws.usw1
  }
  # List of regions in order of proximity.
  regions = [
    "us-east-1",
    "sa-east-1",
    "eu-central-1",
    "ap-southeast-1",
  ]
  replication_role_arn = module.iam.replication_role_arn
}

module "user-configs-eu-central-1" {
  source = "../modules/aws_user_configs"
  env    = var.env
  providers = {
    # eu-central-1
    aws = aws.euc1
  }
  # List of regions in order of proximity.
  regions = [
    "us-east-1",
    "us-west-1",
    "sa-east-1",
    "ap-southeast-1",
  ]
  replication_role_arn = module.iam.replication_role_arn
}

module "user-configs-ap-southeast-1" {
  source = "../modules/aws_user_configs"
  env    = var.env
  providers = {
    # ap-southeast-2
    aws = aws.apse2
  }
  # List of regions in order of proximity.
  regions = [
    "us-east-1",
    "us-west-1",
    "eu-central-1",
    "sa-east-1"
  ]
  replication_role_arn = module.iam.replication_role_arn
}

module "user-configs-sa-east-1" {
  source = "../modules/aws_user_configs"
  env    = var.env
  providers = {
    # sa-east-1
    aws = aws.sae1
  }
  # List of regions in order of proximity.
  regions = [
    "us-east-1",
    "us-west-1",
    "eu-central-1",
    "ap-southeast-1",
  ]
  replication_role_arn = module.iam.replication_role_arn
}

# Enable all AWS regions on Terraform. Doing this will create
# all multi-region resources on each region declared below. See 
# `providers.tf` for the provider abbreviations used below.

# ------------------------------ North America modules ------------------------------ #

module "us-east-1" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
}

module "us-east-2" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.use2
  }
}

module "us-west-1" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.usw1
  }
}

module "us-west-2" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.usw2
  }
}

module "ca-central-1" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.cac1
  }
}

# ------------------------------ South America modules ------------------------------ #

module "sa-east-1" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.sae1
  }
}

# ------------------------------ modules ------------------------------ #

module "eu-central-1" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.euc1
  }
}

module "eu-west-1" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.euw1
  }
}

module "eu-west-2" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.euw2
  }
}

module "eu-west-3" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.euw3
  }
}

module "eu-south-1" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.eus1
  }
}

module "eu-north-1" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.eun1
  }
}

# ------------------------------ Africa modules ------------------------------ #

module "af-south-1" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.afs1
  }
}

# ------------------------------ Middle East modules ------------------------------ #

module "me-south-1" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.mes1
  }
}

# ------------------------------ Asia modules ------------------------------ #

module "ap-east-1" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.ape1
  }
}

module "ap-south-1" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.aps1
  }
}

module "ap-southeast-1" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.apse1
  }
}

module "ap-southeast-2" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.apse2
  }
}

module "ap-southeast-3" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.apse3
  }
}

module "ap-northeast-1" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.apne1
  }
}

module "ap-northeast-2" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.apne2
  }
}

module "ap-northeast-3" {
  source     = "../modules/aws_multi_region"
  env        = var.env
  cidr_block = var.cidr_block
  providers = {
    aws = aws.apne3
  }
}
