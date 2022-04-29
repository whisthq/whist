# ------------------------------ Policies for macOS Electron application ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-electron-macos-arm64" {
  bucket                  = aws_s3_bucket.whist-electron-macos-arm64.id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

resource "aws_s3_bucket_public_access_block" "whist-electron-macos-x64" {
  bucket                  = aws_s3_bucket.whist-electron-macos-x64.id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

# ------------------------------ Policies for Windows Electron application ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-electron-windows" {
  bucket                  = aws_s3_bucket.whist-electron-windows.id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

resource "aws_s3_bucket_public_access_block" "whist-electron-windows-base" {
  bucket                  = aws_s3_bucket.whist-electron-windows-base.id
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

resource "aws_s3_bucket_public_access_block" "whist-test-assets" {
  count                   = var.env == "prod" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-test-assets[0].id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

resource "aws_s3_bucket_public_access_block" "whist-fonts" {
  count                   = var.env == "prod" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-fonts[0].id
  block_public_acls       = true
  block_public_policy     = true
  restrict_public_buckets = true
  ignore_public_acls      = true
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

# macOS Electron application buckets encryption

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-electron-macos-arm64-encryption" {
  bucket = aws_s3_bucket.whist-electron-macos-arm64.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-electron-macos-x64-encryption" {
  bucket = aws_s3_bucket.whist-electron-macos-x64.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

# Windows Electron application buckets encryption

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-electron-windows-encryption" {
  bucket = aws_s3_bucket.whist-electron-windows.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-electron-windows-base-encryption" {
  bucket = aws_s3_bucket.whist-electron-windows-base.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
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

# Assets buckets encryption

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-brand-assets-encryption" {
  count  = var.env == "prod" ? 1 : 0
  bucket = aws_s3_bucket.whist-brand-assets[0].id

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

# Protocol buckets encryption

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-e2e-protocol-test-logs-encryption" {
  count  = var.env == "prod" ? 1 : 0
  bucket = aws_s3_bucket.whist-e2e-protocol-test-logs[0].id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-protocol-dependencies-encryption" {
  count  = var.env == "prod" ? 1 : 0
  bucket = aws_s3_bucket.whist-protocol-dependencies[0].id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-protocol-client-shared-lib-encryption" {
  count  = var.env == "dev" ? 1 : 0
  bucket = aws_s3_bucket.whist-protocol-client-shared-lib[0].id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

# Other resources encryption

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

# We version the user config buckets, so that we can "revert" to a previous state of a user's
# browser session in case their session data somehow got wiped out.
resource "aws_s3_bucket_versioning" "whist-user-app-configs-versioning" {
  bucket = aws_s3_bucket.whist-user-app-configs.id
  versioning_configuration {
    status = "Enabled"
  }
}

# We version the Terraform state bucket, so that we can "revert" to a previous state of our Terraform
# infrastructure, in the case of a configuration or manual mistake.
resource "aws_s3_bucket_versioning" "whist-terraform-state-versioning" {
  count  = var.env == "prod" ? 1 : 0
  bucket = aws_s3_bucket.whist-terraform-state[0].id
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
