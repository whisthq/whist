# This is the provider used to enable terraform resources 
# on the us-east-1 region. This is the default region so it
# doesn't need an alias.
provider "aws" {
  access_key = "AKIAR5GLDA2KXBNIVLU7"
  secret_key = "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"
  region     = "us-east-1"
}

# This is the provider used to enable terraform resources 
# on the us-east-2 region. The alias "use2" is short for
# us-east-2.
provider "aws" {
  access_key = "AKIAR5GLDA2KXBNIVLU7"
  secret_key = "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"
  alias      = "use2"
  region     = "us-east-2"
}
