#
# Create default VPCs
#

resource "aws_vpc" "MainVPC" {
  cidr_block           = var.cidr_block
  enable_dns_hostnames = true
  enable_dns_support   = true

  tags = {
    Name      = "MainVPC${var.env}"
    Env       = var.env
    Terraform = true
  }
}

#
# Create default Subnets
#

resource "aws_subnet" "DefaultSubnet" {
  vpc_id                  = aws_vpc.MainVPC.id
  cidr_block              = var.cidr_block
  map_public_ip_on_launch = true

  tags = {
    Name        = "DefaultSubnet${var.env}"
    Description = "Default subnet for the MainVPC."
    Env         = var.env
    Terraform   = true
  }
}

#
# Create default Internet Gateway
#

resource "aws_internet_gateway" "MainInternetGateway" {
  vpc_id = aws_vpc.MainVPC.id

  tags = {
    Name      = "MainInternetGateway${var.env}"
    Env       = var.env
    Terraform = true
  }
}

#
# Create default Security groups
#

resource "aws_security_group" "MandelboxesSecurityGroup" {
  name        = "MandelboxesSecurityGroup"
  description = "The security group used for instances which run mandelboxes. The ingress rules are the ports that can be allocated by Docker, and the egress rules allows all traffic."
  vpc_id      = aws_vpc.MainVPC.id

  # Port ranges that can be allocated to mandelboxes by Docker
  ingress {
    description = "whist-udp-rule"
    protocol    = "udp"
    from_port   = 1025
    to_port     = 49150
    cidr_blocks = [aws_vpc.MainVPC.cidr_block]
  }

  ingress {
    description = "whist-tcp-rule"
    protocol    = "tcp"
    from_port   = 1025
    to_port     = 49150
    cidr_blocks = [aws_vpc.MainVPC.cidr_block]
  }

  # Allow inbound traffic on port 443 so the instance
  # can reach SSM
  dynamic "ingress" {
    for_each    = var.env != "prod" ? [1] : []
    description = "whist-tcp-rule"
    protocol    = "tcp"
    from_port   = 443
    to_port     = 443
    cidr_blocks = [aws_vpc.MainVPC.cidr_block]
  }

  # We allow all outgoing traffic
  egress {
    description = "whist-ipv4-rule"
    protocol    = "-1"
    to_port     = 0
    from_port   = 0
    cidr_blocks = [aws_vpc.MainVPC.cidr_block]
  }

  tags = {
    Name      = "MandelboxesSecurityGroup${var.env}"
    Env       = var.env
    Terraform = true
  }
}
