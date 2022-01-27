# Get name of the current region

data "aws_region" "current" {}

# Create default VPCs

resource "aws_default_vpc" "main" {
  enable_dns_hostnames = true
  enable_dns_support   = true
  tags = {
    Name = format("Whist %s Main VPC", data.aws_region.current.name)
  }
}

# Create default subnets

resource "aws_default_subnet" "default-subnet" {
  availability_zone = data.aws_region.current.name

  tags = {
    Name   = format("default-subnet-%s", data.aws_region.current.name)
  }
}

# Security groups

resource "aws_default_security_group" "default" {
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
}

resource "aws_security_group" "container-tester" {
  name = "container-tester"
  vpc_id = aws_default_vpc.main.id

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
}