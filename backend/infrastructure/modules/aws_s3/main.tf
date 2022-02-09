# ------------------------------ Buckets for Chromium builds ------------------------------ #

resource "aws_s3_bucket" "whist-browser-chromium-macos-arm64" {
  bucket = "whist-browser-macos-arm64-${var.env}"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-browser-chromium-macos-arm64-${var.env}"
    Env       = var.env
    Terraform = true
  }
}

resource "aws_s3_bucket" "whist-browser-chromium-macos" {
  bucket = "whist-browser-macos-${var.env}"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-browser-macos-${var.env}"
    Env       = var.env
    Terraform = true
  }
}

resource "aws_s3_bucket" "whist-browser-chromium-windows" {
  bucket = "whist-browser-windows-${var.env}"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-browser-windows-${var.env}"
    Env       = var.env
    Terraform = true
  }
}

# Bucket for Chromium debug build

resource "aws_s3_bucket" "whist-browser-chromium-debug-build" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-browser-chromium-debug-build"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-browser-chromium-debug-build"
    Env       = var.env
    Terraform = true
  }
}


# ------------------------------ Buckets for Brave builds ------------------------------ #

resource "aws_s3_bucket" "whist-browser-brave-linux-x64" {
  bucket = "whist-browser-brave-linux-x64-${var.env}"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-browser-brave-linux-x64-${var.env}"
    Env       = var.env
    Terraform = true
  }
}

resource "aws_s3_bucket" "whist-browser-brave-macos-arm64" {
  bucket = "whist-browser-brave-macos-arm64-${var.env}"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-browser-windows-${var.env}"
    Env       = var.env
    Terraform = true
  }
}

resource "aws_s3_bucket" "whist-browser-brave-macos-x64" {
  bucket = "whist-browser-brave-macos-arm64-${var.env}"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-browser-windows-${var.env}"
    Env       = var.env
    Terraform = true
  }
}

# Bucket for Brave cache

resource "aws_s3_bucket" "whist-brave-sscache" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-brave-sscache"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-brave-sscache"
    Env       = var.env
    Terraform = true
  }
}

# ------------------------------ Buckets for user app configs ------------------------------ #

resource "aws_s3_bucket" "whist-user-app-config" {
  bucket = "whist-user-app-configs-${var.env}"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-user-app-configs-${var.env}"
    Env       = var.env
    Terraform = true
  }
}

# ------------------------------ Buckets for assets ------------------------------ #

resource "aws_s3_bucket" "whist-brand-assets" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-brand-assets"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-brand-assets"
    Env       = var.env
    Terraform = true
  }
}

resource "aws_s3_bucket" "whist-website-assets" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-website-assets"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-website-assets"
    Env       = var.env
    Terraform = true
  }
}

resource "aws_s3_bucket" "whist-test-assets" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-test-assets"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-test-assets"
    Env       = var.env
    Terraform = true
  }
}

# ------------------------------ Buckets for protocol ------------------------------ #

resource "aws_s3_bucket" "whist-e2e-protocol-test-logs" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-e2e-protocol-test-logs"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-e2e-protocol-test-logs"
    Env       = var.env
    Terraform = true
  }
}

resource "aws_s3_bucket" "whist-protocol-shared-libs" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-protocol-shared-libs"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-protocol-shared-libs"
    Env       = var.env
    Terraform = true
  }
}

# ------------------------------ Buckets for other data ------------------------------ #

resource "aws_s3_bucket" "whist-technical-interview-data" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-technical-interview-data"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-technical-interview-data"
    Env       = var.env
    Terraform = true
  }
}

# Bucket for dev secrets

resource "aws_s3_bucket" "whist-dev-secrets" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-dev-secrets"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-dev-secrets"
    Env       = var.env
    Terraform = true
  }
}