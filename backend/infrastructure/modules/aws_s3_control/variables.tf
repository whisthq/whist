# This variable represents the Whist environment (i.e. dev, staging, prod)
variable "env" {
  type = string
}

# This variable specifies the regions where the multi-region access point
# should look for user config buckets
variable "regions" {
  type = list(string)
}
