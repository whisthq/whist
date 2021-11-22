provider "aws" {
  region = var.region
}


// add some tags to the DHCP options to show developers, hey, this isn't useless
// so for the love of all that is holy, do not delete it
resource "aws_default_vpc_dhcp_options" "whist-dhcp-options" {
  tags = {
    Description = "Default Whist DHCP options set for VPCs in this region"
  }
}

// We need a separate VPC for each region... 
// for prod, staging, dev, localdev
resource "aws_vpc" "whist-vpcs" {
  for_each             = var.environments
  cidr_block           = var.vpc-cidrs
  enable_dns_hostnames = true
  enable_dns_support   = true
  instance_tenancy     = "default"
  tags = {
    Name        = format("whist-%s-vpc", each.key)
    Environment = format("%s", each.key)
  }
}

// configure the necessary security groups
resource "aws_security_group" "whist-non-prod-security-groups" {
  # Applies to all VPCs that are NOT prod
  for_each = { for key, val in aws_vpc.whist-vpcs : key => val if val.tags_all["Environment"] != "prod" }
  vpc_id   = each.value.id

  name        = format("whist-%s-security-group", each.value.tags_all["Environment"])
  description = format("Security group for %s", each.value.tags_all["Name"])


  ingress {
    description = "whist-http-rule"
    protocol    = "tcp"
    from_port   = 80
    to_port     = 80
    cidr_blocks = ["0.0.0.0/0"]
  }

  ingress {
    description = "whist-udp-rule"
    protocol    = "udp"
    from_port   = 1025
    to_port     = 49150
    cidr_blocks = ["0.0.0.0/0"]
  }

  ingress {
    description = "whist-tcp-rule"
    protocol    = "tcp"
    from_port   = 1025
    to_port     = 49150
    cidr_blocks = ["0.0.0.0/0"]
  }

  ingress {
    description = "whist-ssh-rule"
    to_port     = 22
    from_port   = 22
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }

  egress {
    description = "whist-ipv4-rule"
    protocol    = "-1"
    to_port     = 0
    from_port   = 0
    cidr_blocks = ["0.0.0.0/0"]
  }

  # egress {
  #   description = "whist-ipv6-rule"
  #   protocol = "-1"
  #   to_port = 0
  #   from_port = 0
  #   cidr_blocks = ["::/0"]
  # }

  tags = {
    Name        = format("whist-%s-security-group", each.value.tags_all["Environment"])
    Environment = format("%s", each.value.tags_all["Environment"])
  }
}

resource "aws_security_group" "whist-prod-security-group" {
  # Applies to all VPCs in prod
  for_each = { for key, val in aws_vpc.whist-vpcs : key => val if val.tags_all["Environment"] == "prod" }
  vpc_id   = each.value.id

  name        = format("whist-%s-security-group", each.value.tags_all["Environment"])
  description = format("Security group for %s", each.value.tags_all["Name"])


  ingress {
    description = "whist-http-rule"
    protocol    = "tcp"
    from_port   = 80
    to_port     = 80
    cidr_blocks = ["0.0.0.0/0"]
  }

  ingress {
    description = "whist-udp-rule"
    protocol    = "udp"
    from_port   = 1025
    to_port     = 49150
    cidr_blocks = ["0.0.0.0/0"]
  }

  ingress {
    description = "whist-tcp-rule"
    protocol    = "tcp"
    from_port   = 1025
    to_port     = 49150
    cidr_blocks = ["0.0.0.0/0"]
  }

  egress {
    description = "whist-ipv4-rule"
    protocol    = "-1"
    to_port     = 0
    from_port   = 0
    cidr_blocks = ["0.0.0.0/0"]
  }

  tags = {
    Name        = format("whist-%s-security-group", each.value.tags_all["Environment"])
    Environment = format("%s", each.value.tags_all["Environment"])
  }

}

// Make the internet gateways for each environment VPC
resource "aws_internet_gateway" "whist-internet-gateways" {
  for_each = aws_vpc.whist-vpcs
  vpc_id   = each.value.id

  tags = {
    Environment = format("%s", each.value.tags_all["Environment"])
    Name        = format("whist-%s-internet-gateway", each.value.tags_all["Environment"])
    Description = format("Internet gateway for %s", each.value.tags_all["Name"])
  }
}

locals {
  vpc_ids = tolist([for vpc in aws_vpc.whist-vpcs : vpc.default_route_table_id])
  gw_ids = tolist([for gw in aws_internet_gateway.whist-internet-gateways : gw.id])
  pairs = zipmap(local.vpc_ids, local.gw_ids)
}

# resource "aws_default_route_table" "whist-route-tables" {
#   for_each = local.pairs # zipmap(local.vpc_ids, local.gw_ids)
#   default_route_table_id = each.key

#   route {
#     cidr_block = "0.0.0.0/0"
#     gateway_id = each.value
#   }
# }

// make the subnets for each environment
resource "aws_subnet" "whist-subnets" {
  for_each = aws_vpc.whist-vpcs
  vpc_id   = each.value.id

  cidr_block              = "172.31.0.0/20"
  map_public_ip_on_launch = true

  tags = {
    Environment = format("%s", each.value.tags_all["Environment"])
    Name        = format("whist-%s-subnet", each.value.tags_all["Environment"])
    Description = format("Subnet for %s", each.value.tags_all["Name"])
  }

}
