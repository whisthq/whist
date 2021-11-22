provider "aws" {
  region = var.region
}

module "netwroking-infra" {
  source="../../infrastructure/networking"
  vpc-cidrs=var.vpc-cidrs
  region = var.region
}