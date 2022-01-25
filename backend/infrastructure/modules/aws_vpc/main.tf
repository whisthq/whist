# Create default VPCs

resource "aws_default_vpc" "default-vpcs" {
    for_each = var.enabled_regions
    enable_dns_hostnames = true
    enable_dns_support = true
    tags = {
        Name = format("Whist %s Main VPC", each.value)
    }
}

# Create default subnets

resource "aws_default_subnet" "default-subnets" {
    for_each = var.enabled_regions
    availability_zone = each.value

  tags = {
    Name = format("default-subnet-%s", each.value)
    Region = format("%s", each.value)
  }
}

# Create default Internet Gateways
resource "aws_internet_gateway" "default-gateways" {
    for_each = aws_default_vpc.default-vpcs
    vpc_id = each.value.id

    tags = {
        Name = format("whist-%s-internet-gateway", each.value.tags_all["Region"])
    }
}

locals {  
    vpc_ids = [for vpc in aws_vpc.default-vpcs : vpc.default_route_table_id]
    gw_ids = [for gw in aws_internet_gateway.default-gateways : gw.id]
}

# Create default route tables
resource "aws_default_route_table" "whist-route-tables" {
  count = length(local.vpc_ids)
  default_route_table_id = local.vpc_ids[count.index]

  route {
    cidr_block = "0.0.0.0/0"
    gateway_id =local.gw_ids[count.index]
  }

  tags = {
    Name = format("Route table for VPC: %s", local.vpc_ids[count.index])
  }
}