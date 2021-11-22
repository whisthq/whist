variable "regions" {
  type = set(string)
  default = ["us-west-2",
  "us-east-2"]

}

### Variables for IAM related infrastructure ###
# For modifying actual policy statements, head on over to the infrastructure/iam directory
variable "employee_emails" {
  type    = set(string)
  default = ["value"]
}

variable "admin_emails" {
  type    = set(string)
  default = ["boss"]
}

### Variables for VPC related infrastructure ###

variable "vpc_cidr" {
  type    = string
  default = "172.31.0.0/16"
}
