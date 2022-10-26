INSERT INTO dev VALUES
    ('DESIRED_FREE_MANDELBOXES_US_EAST_1', '2'),
    ('ENABLED_REGIONS', '["us-east-1"]'),
    ('MANDELBOX_LIMIT_PER_USER', '3');

INSERT INTO desktop_app_version(id, major, minor, micro, dev_rc, staging_rc, dev_commit_hash, staging_commit_hash, prod_commit_hash)
    VALUES
        (1, 3, 0, 0, 0, 0, 'dummy_commit_hash', 'dummy_commit_hash', 'dummy_commit_hash');
