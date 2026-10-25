#!/usr/bin/env python3
"""Validate untrusted PR titles as data, never as executable shell input."""
import os
import re


def valid_title(title: str) -> bool:
    return bool(re.fullmatch(
        r'(?:feat|fix|refactor|perf|docs|test|build|ci|chore|style|revert)'
        r'(?:\([a-z0-9][a-z0-9_./-]*\))?!?: [^\s].*', title
    )) and '\n' not in title and '\r' not in title


if __name__ == '__main__':
    if os.environ.get('EVENT_NAME') != 'pull_request':
        print('PR title check does not apply to this event.')
    elif not valid_title(os.environ.get('PR_TITLE', '')):
        raise SystemExit('Use a Conventional Commit title, e.g. feat: add a sensor or refactor!: change the API')
    else:
        print('PR title follows Conventional Commits.')
