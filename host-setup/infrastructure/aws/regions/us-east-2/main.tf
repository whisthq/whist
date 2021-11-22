provider "aws" {
  region = var.region
}

module "networking-1" {
  source="../../infrastructure/networking"
  vpc-cidrs=var.vpc-cidrs
  region = var.region
}