provider "aws" {
  region = var.region
}

module "networking-2" {
  source="../../infrastructure/networking"
  vpc-cidrs=var.vpc-cidrs
  region = var.region
}