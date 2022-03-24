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
# Create Route Tables
#

resource "aws_default_route_table" "MainRouteTable" {
  default_route_table_id = aws_vpc.MainVPC.default_route_table_id

  # Local VPC route
  route {
    cidr_block = aws_vpc.MainVPC.cidr_block
  }

  # Allow route to the internet gateway
  route {
    cidr_block = "0.0.0.0/0"
    gateway_id = aws_internet_gateway.MainInternetGateway.id
  }

  tags = {
    Name      = "MainRouteTable${var.env}"
    Env       = var.env
    Terraform = true
  }
}

#
# Create Security groups
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
    cidr_blocks = ["0.0.0.0/0"]
  }

  ingress {
    description = "whist-tcp-rule"
    protocol    = "tcp"
    from_port   = 1025
    to_port     = 49150
    cidr_blocks = ["0.0.0.0/0"]
  }

  # Allow inbound traffic on port 443 so the instance
  # can use SSM
  dynamic "ingress" {
    for_each = var.env != "prod" ? [1] : []
    content {
      description = "whist-tcp-rule"
      protocol    = "tcp"
      from_port   = 443
      to_port     = 443
      cidr_blocks = ["0.0.0.0/0"]
    }
  }

  # Allow all outgoing IPv4 traffic
  egress {
    description      = "whist-ipv4-rule"
    protocol         = "-1"
    to_port          = 0
    from_port        = 0
    cidr_blocks      = ["0.0.0.0/0"]
    ipv6_cidr_blocks = ["::/0"]
  }

  tags = {
    Name      = "MandelboxesSecurityGroup${var.env}"
    Env       = var.env
    Terraform = true
  }
}

# This security group is used for development instances, so it will
# only exist on the dev environment.
resource "aws_security_group" "MandelboxesDevelopmentGroup" {
  count       = var.env == "dev" ? 1 : 0
  name        = "MandelboxesDevelopmentGroup"
  description = "This security group is used by WhistEngineers development instances. It opens the necessary ports for developing Whist, such as SSH, and ports assigned to mandelboxes."
  vpc_id      = aws_vpc.MainVPC.id

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

  # Allow incoming SSH traffic
  ingress {
    description = "whist-ssh-rule"
    protocol    = "tcp"
    from_port   = 22
    to_port     = 22
    cidr_blocks = ["0.0.0.0/0"]
  }

  # Allow incoming HTTP traffic
  ingress {
    description = "whist-http-rule"
    protocol    = "tcp"
    from_port   = 22
    to_port     = 22
    cidr_blocks = ["0.0.0.0/0"]
  }

  # Allow all outgoing IPv4 traffic
  egress {
    description      = "whist-ipv4-rule"
    protocol         = "-1"
    to_port          = 0
    from_port        = 0
    cidr_blocks      = ["0.0.0.0/0"]
    ipv6_cidr_blocks = ["::/0"]
  }

  tags = {
    Name      = "MandelboxesDevelopmentGroup"
    Env       = var.env
    Terraform = true
  }
}