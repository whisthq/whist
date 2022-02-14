# ------------------------------ Buckets for Chromium builds ------------------------------ #

resource "aws_s3_bucket" "whist-browser-chromium-macos-arm64" {
  bucket = "whist-browser-chromium-macos-arm64-${var.env}"

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
    Description = "Bucket for storing Chromium builds for MacOS Arm64"
    Env       = var.env
    Terraform = true
  }
}

resource "aws_s3_bucket" "whist-browser-chromium-macos-x64" {
  bucket = "whist-browser-chromium-macos-x64-${var.env}"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-browser-chromium-macos-${var.env}"
    Description = "Bucket for storing Chromium builds for MacOS x64"
    Env       = var.env
    Terraform = true
  }
}

resource "aws_s3_bucket" "whist-browser-chromium-windows" {
  bucket = "whist-browser-chromium-windows-${var.env}"

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }
  tags = {
    Name      = "whist-browser-chromium-windows-${var.env}"
    Description = "Bucket for storing Chromium builds for Windows"
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
    Description = "Bucket for storing Chromium debug builds"
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
    Description = "Bucket for storing user configurations"
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
    Description = "Bucket for storing Whist brand assets"
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
    Description = "Bucket for storing Whist website assets"
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
    Description = "Bucket for storing Whist test assets"
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
    Description = "Bucket for storing protocol E2E test logs"
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
    Description = "Bucket for storing protocol shared libs"
    Env       = var.env
    Terraform = true
  }
}

# ------------------------------ Buckets for other data ------------------------------ #

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
    Description = "Bucket for storing Whist dev secrets"
    Env       = var.env
    Terraform = true
  }
}

resource "aws_s3_bucket" "whist-terraform-state" {
  count  = var.env == "prod" ? 1 : 0
  bucket = "whist-terraform-state"

  versioning {
    enabled = true
  }

  server_side_encryption_configuration {
    rule {
      apply_server_side_encryption_by_default {
        sse_algorithm = "AES256"
      }
      bucket_key_enabled = false
    }
  }

  tags = {
    Name      = "whist-terraform-state"
    Description = "Bucket for storing Terraform state files."
    Env       = var.env
    Terraform = true
  }
}
