# This variable represents the Whist environment (i.e. dev, staging, prod)
variable "env" {
  type = string
}

# This variable is a list of the buckets that will be included
# in the multi-region access point.
variable "buckets" {
  type = map(string)
}
