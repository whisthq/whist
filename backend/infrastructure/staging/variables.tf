variable "env" {
  type    = string
  default = ""
}

variable "enabled_regions" {
  type    = set(string)
  default = []
}

variable "whist-env-managed-policies" {
  type = set(string)
}
