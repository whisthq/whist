#!/usr/bin/env python3

import os
import sys
import subprocess

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(-1)
    language_name = sys.argv[1].replace('"', "").replace("'", "")

    # Get list of available languages
    subproc_handle = subprocess.Popen("locale -a", shell=True, stdout=subprocess.PIPE)
    subprocess_stdout = subproc_handle.stdout.readlines()
    locales = [line.decode("utf-8").strip() for line in subprocess_stdout]

    # 1. If the language is in the list, done
    if language_name in locales:
        print(language_name)
        sys.exit(0)

    # 2. Check if there is a case-insensitive match
    lower_case_locales = [x.lower() for x in locales]
    language_name = language_name.lower()
    if language_name in lower_case_locales:
        print(locales[lower_case_locales.index(language_name)])
        sys.exit(0)

    # 3. Check if there is a match using _ vs -
    locales_no_dashes = [x.replace("-", "_") for x in lower_case_locales]
    language_name = language_name.replace("-", "_")
    if language_name in locales_no_dashes:
        print(locales[locales_no_dashes.index(language_name)])
        sys.exit(0)

    # 4. Check if there is a match after removing script names
    script_names = ["Arab", "Cyrl", "Guru", "Latn", "Hans", "Hant", "Vaii"]
    # To remove duplicate "-" characters in string a: "-".join([x for x in a.split("-") if len(x) > 0])
    locales_no_script_names = locales_no_dashes
    for script in script_names:
        language_name = language_name.replace(script.lower(), "")
        locales_no_script_names = [x.replace(script.lower(), "") for x in locales_no_script_names]
        # remove potentially repeated underscores or dashes (which are now underscores)
        language_name = "_".join([x for x in language_name.split("_") if len(x) > 0])
        locales_no_script_names = [
            "_".join([x for x in a.split("_") if len(x) > 0]) for a in locales_no_script_names
        ]
    if language_name in locales_no_script_names:
        print(locales[locales_no_script_names.index(language_name)])
        sys.exit(0)

    # 5. Check if there is a match after removing the encoding suffix (part to the right of the ".")
    language_name = language_name.split(".")[0]
    locales_no_suffixes = [x.split(".")[0] for x in locales_no_script_names]
    if language_name in locales_no_suffixes:
        print(locales[locales_no_suffixes.index(language_name)])
        sys.exit(0)

    # 6. Check if there is a match after peeling off the region suffix (part to the right of the first "_")
    language_name = language_name.split("_")[0]
    locales_no_region = [x.split("_")[0] for x in locales_no_suffixes]
    if language_name in locales_no_region:
        print(locales[locales_no_region.index(language_name)])
        sys.exit(0)
