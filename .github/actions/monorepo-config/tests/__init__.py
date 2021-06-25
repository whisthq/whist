#!/usr/bin/env python
from hypothesis import settings, Verbosity

settings.register_profile("dev", max_examples=50, verbosity=Verbosity.verbose)
settings.load_profile("dev")
