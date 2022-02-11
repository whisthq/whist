# This is the provider used to enable terraform resources 
# on the us-east-1 region. This is the default region so it
# doesn't need an alias.
provider "aws" {
  region = "us-east-1"
}

# This is the provider used to enable terraform resources 
# on the us-west-1 region. The alias "usw1" is short for
# us-west-1.
provider "aws" {
  alias  = "usw1"
  region = "us-west-1"
}
