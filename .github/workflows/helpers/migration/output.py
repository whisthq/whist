#!/usr/bin/env python
from functools import wraps


def title(text):
    print(f"## :construction: {text} :construction:")


def alert(text):
    print(f"## :rotating_light: {text} :rotating_light:")


def body(text):
    print(text)


def sep():
    print("---")


def collapse(func):
    @wraps(func)
    def collapse_wrapper(*args, **kwargs):
        print("<details>")
        print("<summary>Click to expand</summary>\n")
        func(*args, **kwargs)
        print("</details>")

    return collapse_wrapper


@collapse
def sql(text):
    print("```sql")
    print(text)
    print("```")
