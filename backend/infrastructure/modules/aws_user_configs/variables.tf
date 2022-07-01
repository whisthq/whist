# This variable represents the Whist environment (i.e. dev, staging, prod)
variable "env" {
  description = "The name of the environment a resource belongs to."
}

# This variable specifies the regions where bucket replication will be enabled.
# The list is in order of proximity to the current module's region.
variable "replication_regions" {
  type = list(string)
}

# The replication role used by S3 to replicate objects.
variable "replication_role_arn" {
  description = "The role assumed by S3 to replicate objects between buckets."
}
