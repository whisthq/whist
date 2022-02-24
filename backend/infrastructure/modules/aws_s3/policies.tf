# ------------------------------ Policies for Chromium builds ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-chromium-macos-arm64" {
  bucket                  = aws_s3_bucket.whist-chromium-macos-arm64.id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

resource "aws_s3_bucket_public_access_block" "whist-chromium-macos-x64" {
  bucket                  = aws_s3_bucket.whist-chromium-macos-x64.id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

resource "aws_s3_bucket_public_access_block" "whist-chromium-windows" {
  bucket                  = aws_s3_bucket.whist-chromium-windows.id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

# ------------------------------ Policies for user app configs ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-user-app-configs" {
  bucket                  = aws_s3_bucket.whist-user-app-configs.id
  block_public_acls       = true
  block_public_policy     = true
  restrict_public_buckets = true
  ignore_public_acls      = true
}

# ------------------------------ Policies for assets ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-brand-assets" {
  count                   = var.env == "prod" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-brand-assets[0].id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

resource "aws_s3_bucket_public_access_block" "whist-website-assets" {
  count                   = var.env == "prod" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-website-assets[0].id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

resource "aws_s3_bucket_public_access_block" "whist-test-assets" {
  count                   = var.env == "prod" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-test-assets[0].id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

# This policy will allow all objects in the whist-website-assets to
# be accessible by anyone on the internet.
data "aws_iam_policy_document" "whist-website-assets-policy" {
  count = var.env == "prod" ? 1 : 0
  statement {
    sid    = "AllowPublicAccess"
    effect = "Allow"

    principals {
      type = "AWS"
      identifiers = [
        "*"
      ]
    }

    actions = [
      "s3:GetObject"
    ]
    resources = [
      "${aws_s3_bucket.whist-website-assets[0].arn}/*",
    ]
  }
}

# ------------------------------ Policies for protocol ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-e2e-protocol-test-logs" {
  count                   = var.env == "prod" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-e2e-protocol-test-logs[0].id
  block_public_acls       = true
  block_public_policy     = true
  restrict_public_buckets = true
  ignore_public_acls      = true
}

resource "aws_s3_bucket_public_access_block" "whist-protocol-dependencies" {
  count                   = var.env == "prod" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-protocol-dependencies[0].id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

resource "aws_s3_bucket_public_access_block" "whist-protocol-client-shared-lib" {
  count                   = var.env == "dev" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-protocol-client-shared-lib[0].id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

# ------------------------------ Policies for other data ------------------------------ #

# Policy for dev secrets

resource "aws_s3_bucket_public_access_block" "whist-dev-secrets" {
  count                   = var.env == "prod" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-dev-secrets[0].id
  block_public_acls       = true
  block_public_policy     = true
  restrict_public_buckets = true
  ignore_public_acls      = true
}

resource "aws_s3_bucket_public_access_block" "whist-terraform-state" {
  count                   = var.env == "prod" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-terraform-state[0].id
  block_public_acls       = true
  block_public_policy     = true
  restrict_public_buckets = true
  ignore_public_acls      = true
}

# ------------------------------ Configure server side encryption ------------------------------ #

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-chromium-macos-arm64-encryption" {
  bucket = aws_s3_bucket.whist-chromium-macos-arm64.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-chromium-macos-x64-encryption" {
  bucket = aws_s3_bucket.whist-chromium-macos-x64.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-chromium-windows-encryption" {
  bucket = aws_s3_bucket.whist-chromium-windows.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-user-app-configs-encryption" {
  bucket = aws_s3_bucket.whist-user-app-configs.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-brand-assets-encryption" {
  count  = var.env == "prod" ? 1 : 0
  bucket = aws_s3_bucket.whist-brand-assets[0].id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-website-assets-encryption" {
  count  = var.env == "prod" ? 1 : 0
  bucket = aws_s3_bucket.whist-website-assets[0].id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-test-assets-encryption" {
  count  = var.env == "prod" ? 1 : 0
  bucket = aws_s3_bucket.whist-test-assets[0].id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-e2e-protocol-test-logs-encryption" {
  count  = var.env == "prod" ? 1 : 0
  bucket = aws_s3_bucket.whist-e2e-protocol-test-logs[0].id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-protocol-shared-libs-encryption" {
  count  = var.env == "prod" ? 1 : 0
  bucket = aws_s3_bucket.whist-protocol-shared-libs[0].id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-dev-secrets-encryption" {
  count  = var.env == "prod" ? 1 : 0
  bucket = aws_s3_bucket.whist-dev-secrets[0].id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-terraform-state-encryption" {
  count  = var.env == "prod" ? 1 : 0
  bucket = aws_s3_bucket.whist-terraform-state[0].id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

# ------------------------------ Configure bucket versioning ------------------------------ #

resource "aws_s3_bucket_versioning" "whist-user-app-configs-versioning" {
  bucket = aws_s3_bucket.whist-user-app-configs.id
  versioning_configuration {
    status = "Enabled"
  }
}

resource "aws_s3_bucket_versioning" "whist-terraform-state-versioning" {
  count  = var.env == "prod" ? 1 : 0
  bucket = aws_s3_bucket.whist-terraform-state[0].id
  versioning_configuration {
    status = "Enabled"
  }
}

# ------------------------------ Lifecycle policies for bucket versioning ------------------------------ #

resource "aws_s3_bucket_lifecycle_configuration" "whist-user-app-configs-lifecycle" {
  bucket = aws_s3_bucket.whist-user-app-configs.id

  # This rule keeps only the 3 most recent nonexpired objects
  # on the bucket. This applies to objects 5 days after becoming
  # noncurrent. It is also necessary to provide a filter or AWS will
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
        object_size_less_than    = 1000000000
      }
    }

    noncurrent_version_expiration {
      newer_noncurrent_versions = 3
      noncurrent_days           = 5
    }
  }
}
