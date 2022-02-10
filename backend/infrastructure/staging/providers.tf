# This is the provider used to enable terraform resources 
# on the us-east-1 region. This is the default region so it
# doesn't need an alias.
provider "aws" {
  region = "us-east-1"
}

# This is the provider used to enable terraform resources 
# on the us-east-2 region. The alias "use2" is short for
# us-east-2.
provider "aws" {
  alias  = "use2"
  region = "us-east-2"
}

# This is the provider used to enable terraform resources 
# on the us-west-1 region. The alias "usw1" is short for
# us-west-1.
provider "aws" {
  alias  = "usw1"
  region = "us-west-1"
}


# This is the provider used to enable terraform resources 
# on the us-west-2 region. The alias "usw2" is short for
# us-west-2.
provider "aws" {
  alias  = "usw2"
  region = "us-west-2"
}


# This is the provider used to enable terraform resources 
# on the us-ca-central-1 region. The alias "usca1" is short for
# us-ca-central-1.
provider "aws" {
  alias  = "usca1"
  region = "us-ca-central-1"
}
