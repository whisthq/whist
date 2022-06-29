# This variable represents the Whist environment (i.e. dev, staging, prod)
variable "env" {
  description = "The name of the environment a resource belongs to."
}

# This variable specifies the regions where the multi-region access point
# should look for user config buckets
variable "regions" {
  type = list(string)
}
