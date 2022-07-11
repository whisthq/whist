# This module includes all configurations for the user config
# S3 buckets. The reason they are not in the `aws_s3` module is
# because we need to control the regions where these buckets will
# be created, and as the userbase grows, gradually add more regions.

terraform {
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 4.21.0"
    }
  }

  required_version = ">= 0.14.9"
}

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

    # The Lifecycle rule applies to all objects in the bucket.
    filter {}

    # If the configs have not been used in two months, transition
    # to the infrequent access tier to minimize storage costs.
    transition {
      days          = 60
      storage_class = "STANDARD_IA"
    }


    # If the configs have not been used in three months, delete them.
    expiration {
      days = 90
    }

    # Keep only the 3 latest noncurrent versions (old versions of the object).
    # The rest of the noncurrent versions will be deleted once they are more than
    # a day old.
    noncurrent_version_expiration {
      newer_noncurrent_versions = 3
      noncurrent_days           = 1
    }
  }
}

output "bucket_name" {
  value = aws_s3_bucket.whist-user-app-configs.id
}

# ------------------------------ Bucket Replication Configuration ------------------------------ #

# This enables Cross-Region Replication (CRR) between the list of buckets.
# It is only enabled in the "prod" environment to save costs.
# resource "aws_s3_bucket_replication_configuration" "UserConfigReplication" {
#   count  = var.env == "prod" ? 1 : 0
#   bucket = aws_s3_bucket.whist-user-app-configs.id
#   role   = var.replication_role_arn

#   dynamic "rule" {
#     for_each = var.replication_regions

#     content {
#       id       = rule.key
#       priority = rule.key

#       # An empty filter is required to declare multiple
#       # destinations with priorities.
#       filter {}

#       delete_marker_replication {
#         status = "Disabled"
#       }

#       status = "Enabled"

#       destination {
#         bucket = "arn:aws:s3:::whist-user-app-configs-${rule.value}-${var.env}"
#       }
#     }
#   }
# }
