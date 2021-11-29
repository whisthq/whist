variable "region" {}

variable "environments" {
  description = "Whist environments"
  type        = set(string)
  default     = ["prod", "staging", "dev", "localdev"]
}

variable "vpc-cidrs" {}
