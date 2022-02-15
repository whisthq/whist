# Create default VPCs

resource "aws_vpc" "MainVPC" {
  count  = var.env == "prod" ? 1 : 0
  cidr_block = "172.31.0.0/16"
  enable_dns_hostnames = true
  enable_dns_support   = true
  tags = {
    Name      = "MainVPC"
    Env       = var.env
    Terraform = true
  }
}

# Create default subnets

resource "aws_subnet" "DefaultSubnet" {
  count  = var.env == "prod" ? 1 : 0
  vpc_id = aws_vpc.MainVPC[0].id
  cidr_block = "172.31.0.0/16"
  tags = {
    Name      = "DefaultSubnet"
    Description = "Default subnet for the MainVPC."
    Env       = var.env
    Terraform = true
  }
}

# Security groups

resource "aws_default_security_group" "DefaultSecurityGroup" {
  count  = var.env == "prod" ? 1 : 0
  vpc_id = aws_vpc.MainVPC[0].id

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
    cidr_blocks = [aws_vpc.MainVPC[0].cidr_block]
  }

  tags = {
    Name      = "DefaultSecurityGroup"
    Env       = var.env
    Terraform = true
  }
}

resource "aws_security_group" "MandelboxesSecurityGroup" {
  count  = var.env == "prod" ? 1 : 0
  name        = "MandelboxesSecurityGroup"
  description = "The security group used for instances which run mandelboxes. The ingress rules are the ports that can be allocated by Docker, and the egress rules allows all traffic."
  vpc_id      = aws_vpc.MainVPC[0].id

  # Port ranges that can be allocated to mandelboxes by Docker
  ingress {
    description = "whist-udp-rule"
    protocol    = "udp"
    from_port   = 1025
    to_port     = 49150
    cidr_blocks = [aws_vpc.MainVPC[0].cidr_block]
  }

  ingress {
    description = "whist-tcp-rule"
    protocol    = "tcp"
    from_port   = 1025
    to_port     = 49150
    cidr_blocks = [aws_vpc.MainVPC[0].cidr_block]
  }

  # We allow all outgoing traffic
  egress {
    description = "whist-ipv4-rule"
    protocol    = "-1"
    to_port     = 0
    from_port   = 0
    cidr_blocks = [aws_vpc.MainVPC[0].cidr_block]
  }

  tags = {
    Name      = "MandelboxesSecurityGroup"
    Env       = var.env
    Terraform = true
  }
}
