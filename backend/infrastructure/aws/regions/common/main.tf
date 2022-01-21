provider "aws" {
  region = var.region
}

module "networking-infra" {
  source="../../resources/networking"
  region = var.region
}
