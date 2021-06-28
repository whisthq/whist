from app.helpers.blueprint_helpers.aws.aws_mandelbox_assign_post import is_user_active


def test_inactive(bulk_instance):
    """
    tests that a user is correctly ruled inactive
    """
    bulk_instance()
    assert not is_user_active("rando_user")


def test_inactive_with_others(bulk_instance):
    """
    tests that even with containers for some user, a different user isn't active
    """
    bulk_instance(associated_mandelboxes=1)
    assert not is_user_active("rando_user")


def test_active(bulk_instance):
    """
    tests that a given user is detected as active
    """
    bulk_instance(associated_mandelboxes=1, user_for_containers="rando_user")
    assert is_user_active("rando_user")
