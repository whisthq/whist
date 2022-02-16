variable "env" {
  type    = string
  default = ""
}

variable "enabled_regions" {
  type    = set(string)
  default = []
}

variable "cidr_block" {
  type = string
  default = ""
}
