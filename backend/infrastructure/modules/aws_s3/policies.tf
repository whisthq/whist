# ------------------------------ Policy documents for public access ------------------------------ #

# The following policies allow access for all principals (public access), to all
# objects in the buckets. Only read access is given via the `GetObject` action.
# This policy will only work if the public access block settings of the bucket
# are set to `false`.

data "aws_iam_policy_document" "whist-browser-macos-arm64-public-access-policy" {
  statement {
    principals {
      # This block will be rendered as
      # "Principal": "*"
      identifiers = ["*"]
      type        = "*"
    }

    actions = [
      "s3:GetObject",
    ]

    effect = "Allow"
    resources = [
      "${aws_s3_bucket.whist-browser-macos-arm64.arn}/*",
    ]
  }
}

data "aws_iam_policy_document" "whist-browser-macos-x64-public-access-policy" {
  statement {
    principals {
      # This block will be rendered as
      # "Principal": "*"
      identifiers = ["*"]
      type        = "*"
    }

    actions = [
      "s3:GetObject",
    ]

    effect = "Allow"
    resources = [
      "${aws_s3_bucket.whist-browser-macos-x64.arn}/*",
    ]
  }
}

data "aws_iam_policy_document" "whist-browser-policies-public-access-policy" {
  statement {
    principals {
      # This block will be rendered as
      # "Principal": "*"
      identifiers = ["*"]
      type        = "*"
    }

    actions = [
      "s3:GetObject",
    ]

    effect = "Allow"
    resources = [
      "${aws_s3_bucket.whist-browser-policies.arn}/*",
    ]
  }
}

# ------------------------------ Policies for macOS Whist Chromium Browser ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-browser-macos-arm64" {
  bucket                  = aws_s3_bucket.whist-browser-macos-arm64.id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

resource "aws_s3_bucket_public_access_block" "whist-browser-macos-x64" {
  bucket                  = aws_s3_bucket.whist-browser-macos-x64.id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

# The buckets containing the Whist Browser applications that users download have to be 
# publicly accessible in all environments so that auto-update works correctly. We keep
# the buckets for server-side Whist Browser, which runs in our mandelboxes, private since
# those get autoupdated at deploys.

resource "aws_s3_bucket_policy" "whist-browser-macos-arm64-public-access-policy-attachment" {
  bucket = aws_s3_bucket.whist-browser-macos-arm64.id
  policy = data.aws_iam_policy_document.whist-browser-macos-arm64-public-access-policy.json
}

resource "aws_s3_bucket_policy" "whist-browser-macos-x64-public-access-policy-attachment" {
  bucket = aws_s3_bucket.whist-browser-macos-x64.id
  policy = data.aws_iam_policy_document.whist-browser-macos-x64-public-access-policy.json
}

# ------------------------------ Access Block for Whist Browser Policies ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-browser-policies" {
  bucket                  = aws_s3_bucket.whist-browser-policies.id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

# ------------------------- Policy for Linux Server-side Whist Chromium Browser ------------------------- #

resource "aws_s3_bucket_public_access_block" "whist-server-browser-linux-x64" {
  bucket                  = aws_s3_bucket.whist-server-browser-linux-x64.id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

# ------------------------------ Policies for assets ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-brand-assets" {
  count                   = var.env == "dev" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-brand-assets[0].id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

resource "aws_s3_bucket_public_access_block" "whist-test-assets" {
  count                   = var.env == "dev" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-test-assets[0].id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

resource "aws_s3_bucket_public_access_block" "whist-fonts" {
  count                   = var.env == "dev" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-fonts[0].id
  block_public_acls       = true
  block_public_policy     = true
  restrict_public_buckets = true
  ignore_public_acls      = true
}

# ------------------------------ Policies for protocol ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-e2e-protocol-test-logs" {
  count                   = var.env == "dev" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-e2e-protocol-test-logs[0].id
  block_public_acls       = true
  block_public_policy     = true
  restrict_public_buckets = true
  ignore_public_acls      = true
}

resource "aws_s3_bucket_public_access_block" "whist-protocol-dependencies" {
  count                   = var.env == "dev" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-protocol-dependencies[0].id
  block_public_acls       = false
  block_public_policy     = false
  restrict_public_buckets = false
  ignore_public_acls      = false
}

# ------------------------------ Policies for other data ------------------------------ #

# Policy for dev secrets

resource "aws_s3_bucket_public_access_block" "whist-dev-secrets" {
  count                   = var.env == "dev" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-dev-secrets[0].id
  block_public_acls       = true
  block_public_policy     = true
  restrict_public_buckets = true
  ignore_public_acls      = true
}

resource "aws_s3_bucket_public_access_block" "whist-terraform-state" {
  count                   = var.env == "dev" ? 1 : 0
  bucket                  = aws_s3_bucket.whist-terraform-state[0].id
  block_public_acls       = true
  block_public_policy     = true
  restrict_public_buckets = true
  ignore_public_acls      = true
}

# ------------------------------ Configure server side encryption ------------------------------ #

# macOS Chromium Browser buckets encryption

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-browser-macos-arm64-encryption" {
  bucket = aws_s3_bucket.whist-browser-macos-arm64.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-browser-macos-x64-encryption" {
  bucket = aws_s3_bucket.whist-browser-macos-x64.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

# Whist Browser Policies bucket encryption

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-browser-policies-encryption" {
  bucket = aws_s3_bucket.whist-browser-policies.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}


# Linux Server-side Chromium Browser bucket encryption

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-server-browser-linux-x64-encryption" {
  bucket = aws_s3_bucket.whist-server-browser-linux-x64.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

# Assets buckets encryption

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-brand-assets-encryption" {
  count  = var.env == "dev" ? 1 : 0
  bucket = aws_s3_bucket.whist-brand-assets[0].id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-test-assets-encryption" {
  count  = var.env == "dev" ? 1 : 0
  bucket = aws_s3_bucket.whist-test-assets[0].id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

# Protocol buckets encryption

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-e2e-protocol-test-logs-encryption" {
  count  = var.env == "dev" ? 1 : 0
  bucket = aws_s3_bucket.whist-e2e-protocol-test-logs[0].id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-protocol-dependencies-encryption" {
  count  = var.env == "dev" ? 1 : 0
  bucket = aws_s3_bucket.whist-protocol-dependencies[0].id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

# Other resources encryption

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-dev-secrets-encryption" {
  count  = var.env == "dev" ? 1 : 0
  bucket = aws_s3_bucket.whist-dev-secrets[0].id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "whist-terraform-state-encryption" {
  count  = var.env == "dev" ? 1 : 0
  bucket = aws_s3_bucket.whist-terraform-state[0].id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

# ------------------------------ Configure bucket versioning ------------------------------ #

# We version the Terraform state bucket, so that we can "revert" to a previous state of our Terraform
# infrastructure, in the case of a configuration or manual mistake.
resource "aws_s3_bucket_versioning" "whist-terraform-state-versioning" {
  count  = var.env == "dev" ? 1 : 0
  bucket = aws_s3_bucket.whist-terraform-state[0].id
  versioning_configuration {
    status = "Enabled"
  }
}

# ------------------------------ Lifecycle policies for protocol E2E logs bucket  --------------------- #
resource "aws_s3_bucket_lifecycle_configuration" "whist-e2e-protocol-test-logs-lifecycle" {
  count  = var.env == "dev" ? 1 : 0
  bucket = aws_s3_bucket.whist-e2e-protocol-test-logs[0].id

  # Remove logs older than 7 days
  rule {
    id     = "removeObsoleteLogs"
    status = "Enabled"

    expiration {
      days = 7
    }
  }
}
