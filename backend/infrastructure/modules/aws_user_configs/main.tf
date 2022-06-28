data "aws_region" "current" {}

# ------------------------------ Buckets for user app configs ------------------------------ #

# Buckets for storing user configurations, per environment
resource "aws_s3_bucket" "whist-user-app-configs" {
  bucket = "whist-user-app-configs-${data.aws_region.current.name}-${var.env}"

  # Ignore the internal lifecycle rules because we have
  # a separate lifecycle resource.
  # See: https://registry.terraform.io/providers/hashicorp/aws/latest/docs/resources/s3_bucket_lifecycle_configuration#usage-notes
  lifecycle {
    ignore_changes = [
      lifecycle_rule
    ]
  }

  tags = {
    Name      = "whist-user-app-configs-${data.aws_region.current.name}-${var.env}"
    Env       = var.env
    Terraform = true
  }
}

# ------------------------------ Policies for user app configs ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-user-app-configs" {
  bucket                  = aws_s3_bucket.whist-user-app-configs.id
  block_public_acls       = true
  block_public_policy     = true
  restrict_public_buckets = true
  ignore_public_acls      = true
}

# User config buckets encryption

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-user-app-configs-encryption" {
  bucket = aws_s3_bucket.whist-user-app-configs.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

# ------------------------------ Configure bucket versioning ------------------------------ #

# We version the user config buckets, so that we can "revert" to a previous state of a user's
# browser session in case their session data somehow got wiped out.
resource "aws_s3_bucket_versioning" "whist-user-app-configs-versioning" {
  bucket = aws_s3_bucket.whist-user-app-configs.id
  versioning_configuration {
    status = "Enabled"
  }
}

# ------------------------------ Lifecycle policies for bucket versioning ------------------------------ #

# We only keep the last 3 versions of a user's browser session data, so that our buckets don't grow too
# big (which affects storage costs and retrieval times).
resource "aws_s3_bucket_lifecycle_configuration" "whist-user-app-configs-lifecycle" {
  bucket = aws_s3_bucket.whist-user-app-configs.id

  # This rule keeps only the 3 most recent non-expired objects
  # on the bucket. This applies to objects 5 days after becoming
  # non-current. It is also necessary to provide a filter or AWS will
  # return an error.
  rule {
    id     = "userConfigCleanRule"
    status = "Enabled"

    # To apply the rule to all user configs, add a filter using a range
    # of size up to 1Gb.
    filter {
      # Represented in bytes
      and {
        object_size_greater_than = 0
        object_size_less_than    = 1000000000 # 1Gb
      }
    }

    noncurrent_version_expiration {
      newer_noncurrent_versions = 3
      noncurrent_days           = 5
    }
  }
}
