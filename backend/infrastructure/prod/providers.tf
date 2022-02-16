# ------------------------------ North America region providers ------------------------------ #

# This is the provider used to enable terraform resources 
# on the us-east-1 region. This is the default region so it
# doesn't need an alias. Location: Northern Virginia. 
provider "aws" {
  region = "us-east-1"
}

# This is the provider used to enable terraform resources 
# on the us-east-2 region. The alias "use2" is short for
# us-east-2. Location: Ohio.
provider "aws" {
  alias  = "use2"
  region = "us-east-2"
}

# This is the provider used to enable terraform resources 
# on the us-west-1 region. The alias "usw1" is short for
# us-west-1. Location: Northern California.
provider "aws" {
  alias  = "usw1"
  region = "us-west-1"
}


# This is the provider used to enable terraform resources 
# on the us-west-2 region. The alias "usw2" is short for
# us-west-2. Location: Oregon.
provider "aws" {
  alias  = "usw2"
  region = "us-west-2"
}


# This is the provider used to enable terraform resources 
# on the us-ca-central-1 region. The alias "cac1" is short for
# ca-central-1. Location: Central Canada.
provider "aws" {
  alias  = "cac1"
  region = "ca-central-1"
}

# ------------------------------ South America region providers ------------------------------ #

# This is the provider used to enable terraform resources 
# on the sa-east-1 region. The alias "sae1" is short for
# sa-east-1. Location: Brazil.
provider "aws" {
  alias  = "sae1"
  region = "sa-east-1"
}

# ------------------------------ Europe region providers ------------------------------ #

# This is the provider used to enable terraform resources 
# on the eu-central-1 region. The alias "euc1" is short for
# eu-central-1. Location: Frankfurt. 
provider "aws" {
  alias  = "euc1"
  region = "eu-central-1"
}

# This is the provider used to enable terraform resources 
# on the eu-west-1 region. The alias "euw1" is short for
# eu-west-1. Location: Ireland.
provider "aws" {
  alias  = "euw1"
  region = "eu-west-1"
}

# This is the provider used to enable terraform resources 
# on the eu-west-2 region. The alias "euw2" is short for
# eu-west-2. Location: London.
provider "aws" {
  alias  = "euw2"
  region = "eu-west-2"
}

# This is the provider used to enable terraform resources 
# on the eu-west-3 region. The alias "euw3" is short for
# eu-west-3. Location: Paris.
provider "aws" {
  alias  = "euw3"
  region = "eu-west-3"
}

# This is the provider used to enable terraform resources 
# on the eu-south-1 region. The alias "eus1" is short for
# eu-south-1. Location: Milan.
provider "aws" {
  alias  = "eus1"
  region = "eu-south-1"
}

# This is the provider used to enable terraform resources 
# on the eu-north-1 region. The alias "eun1" is short for
# eu-north-1. Location: Stockholm.
provider "aws" {
  alias  = "eun1"
  region = "eu-north-1"
}

# ------------------------------ Africa region providers ------------------------------ #

# This is the provider used to enable terraform resources 
# on the af-south-1 region. The alias "afs1" is short for
# af-south-1. Location: Cape Town.
provider "aws" {
  alias  = "afs1"
  region = "af-south-1"
}

# ------------------------------ Middle East region providers ------------------------------ #

# This is the provider used to enable terraform resources 
# on the me-south-1 region. The alias "mes1" is short for
# me-south-1. Location: Bahrain.
provider "aws" {
  alias  = "mes1"
  region = "me-south-1"
}

# ------------------------------ Asia region providers ------------------------------ #

# This is the provider used to enable terraform resources 
# on the ap-east-1 region. The alias "ape1" is short for
# ap-east-1. Location: Hong Kong.
provider "aws" {
  alias  = "ape1"
  region = "ap-east-1"
}

# This is the provider used to enable terraform resources 
# on the ap-south-1 region. The alias "aps1" is short for
# ap-south-1. Location: Mumbai.
provider "aws" {
  alias  = "aps1"
  region = "ap-south-1"
}

# This is the provider used to enable terraform resources 
# on the ap-southeast-1 region. The alias "apse1" is short for
# ap-southeast-1. Location: Singapore.
provider "aws" {
  alias  = "apse1"
  region = "ap-southeast-1"
}

# This is the provider used to enable terraform resources 
# on the ap-southeast-2 region. The alias "apse2" is short for
# ap-southeast-2. Location: Sydney.
provider "aws" {
  alias  = "apse2"
  region = "ap-southeast-2"
}

# This is the provider used to enable terraform resources 
# on the ap-southeast-3 region. The alias "apse3" is short for
# ap-southeast-3. Location: Jakarta.
provider "aws" {
  alias                  = "apse3"
  region                 = "ap-southeast-3"
  skip_region_validation = true
}

# This is the provider used to enable terraform resources 
# on the ap-northeast-1 region. The alias "apne1" is short for
# ap-northeast-1. Location: Tokyo.
provider "aws" {
  alias  = "apne1"
  region = "ap-northeast-1"
}

# This is the provider used to enable terraform resources 
# on the ap-northeast-2 region. The alias "apne2" is short for
# ap-northeast-2. Location: Seoul.
provider "aws" {
  alias  = "apne2"
  region = "ap-northeast-2"
}

# This is the provider used to enable terraform resources 
# on the ap-northeast-3 region. The alias "apne3" is short for
# ap-northeast-3. Location: Osaka.
provider "aws" {
  alias                  = "apne3"
  region                 = "ap-northeast-3"
  skip_region_validation = true
}
