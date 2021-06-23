#!/usr/bin/env python
from hypothesis import settings, Verbosity

settings.register_profile("dev", max_examples=20, verbosity=Verbosity.verbose)
settings.load_profile("dev")
