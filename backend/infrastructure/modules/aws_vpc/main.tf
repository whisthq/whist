# Get name of the current region to use across resources

data "aws_region" "current" {}

# Create default VPCs

resource "aws_default_vpc" "MainVPC" {
  enable_dns_hostnames = true
  enable_dns_support   = true
  tags = {
    Name      = "MainVPC"
    Env       = var.env
    Terraform = true
  }
}

# Create default subnets

resource "aws_default_subnet" "DefaultSubnet" {
  availability_zone = data.aws_region.current.name

  tags = {
    Name      = "DefaultSubnet"
    Env       = var.env
    Terraform = true
  }
}

# Security groups

resource "aws_default_security_group" "DefaultSecurityGroup" {
  vpc_id = aws_default_vpc.main.id

  ingress {
    protocol  = -1
    self      = true
    from_port = 0
    to_port   = 0
  }

  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }

  tags = {
    Name      = "DefaultSecurityGroup"
    Env       = var.env
    Terraform = true
  }
}

resource "aws_security_group" "MandelboxesSecurityGroup" {
  name        = "MandelboxesSecurityGroup"
  description = "The security group used for instances which run mandelboxes. The ingress rules are the ports that can be allocated by Docker, and the egress rules allows all traffic."
  vpc_id      = aws_default_vpc.main.id

  # Port ranges that can be allocated to mandelboxes by Docker
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

  # We allow all outgoing traffic
  egress {
    description = "whist-ipv4-rule"
    protocol    = "-1"
    to_port     = 0
    from_port   = 0
    cidr_blocks = ["0.0.0.0/0"]
  }

  tags = {
    Name      = "MandelboxesSecurityGroup"
    Env       = var.env
    Terraform = true
  }
}
