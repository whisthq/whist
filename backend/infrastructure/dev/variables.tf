variable "env" {
  type    = string
  default = ""
}

variable "enabled_regions" {
  type    = set(string)
  default = []
}
