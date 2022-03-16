# This variable represents the Whist environment (i.e. dev, staging, prod) 
variable "env" {
  type    = string
  default = ""
}

# This variable represents a routing identifier for Internet traffic, it must
# be unique for each AWS region
variable "cidr_block" {
  type    = string
  default = ""
}
