#!/usr/bin/env python
from functools import wraps

def newline(func):
    @wraps(func)
    def newline_wrapper(*args, **kwargs):
        return func(*args, **kwargs) + "\n"
    return newline_wrapper

def join_newline(*args):
    identity = lambda: ""
    newlined = newline(identity)
    return newlined().join(i for i in args)

def collapse(func):
    @wraps(func)
    def collapse_wrapper(*args, **kwargs):
        return join_newline(
            "<details>",
            "<summary>Click to expand</summary>\n",
            func(*args, **kwargs),
            "</details>"
        )
    return collapse_wrapper

def title(text):
    return f"## {text}"

def notice(text):
    return f"## :construction: {text} :construction:"

def error(text):
    return f"## :x: {text} :x:"

def valid(text):
    return f"## {text} :thumbsup:"

def alert(text):
    return f"## :rotating_light: {text} :rotating_light:"

def body(text):
    return text

def sep():
    return "---"

def comment(text):
    return f"<!-- {text} -->"

@collapse
def sql(text):
    return join_newline("```sql", text, "````")

@collapse
def python(text):
    return join_newline("```python", text, "````")
