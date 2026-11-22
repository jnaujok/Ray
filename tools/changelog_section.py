#!/usr/bin/env python3
"""Prints one release's section of CHANGELOG.md, for its GitHub Release notes.

    changelog_section.py <tag> [CHANGELOG.md] [owner/repo]

The section is everything under "## [<tag>]" up to the next "## [" heading, without the heading
itself. Images and links to files in the repository (docs/images/...) are made absolute, to the
files as they are at the tag, so they show on the release page. owner/repo defaults to
$GITHUB_REPOSITORY, then jnaujok/Ray. Exits with 1 if the changelog has no such section.
"""
import os
import re
import sys


def section(text, tag):
    lines = text.splitlines()
    start = None
    for index, line in enumerate(lines):
        if start is None:
            if re.match(r'^## \[%s\]' % re.escape(tag), line):
                start = index + 1
        elif line.startswith('## ['):
            return '\n'.join(lines[start:index]).strip('\n')
    return None if start is None else '\n'.join(lines[start:]).strip('\n')


def absolute(text, tag, repo):
    raw = 'https://raw.githubusercontent.com/%s/%s/' % (repo, tag)
    blob = 'https://github.com/%s/blob/%s/' % (repo, tag)
    text = re.sub(r'(!\[[^\]]*\]\()(?!https?:|#)([^)\s]+)', lambda m: m.group(1) + raw + m.group(2), text)
    return re.sub(r'((?<!!)\[[^\]]*\]\()(?!https?:|#|mailto:)([^)\s]+)', lambda m: m.group(1) + blob + m.group(2), text)


def main(argv):
    if not argv:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    tag = argv[0]
    path = argv[1] if len(argv) > 1 else 'CHANGELOG.md'
    repo = argv[2] if len(argv) > 2 else os.environ.get('GITHUB_REPOSITORY', 'jnaujok/Ray')
    with open(path, encoding='utf-8') as f:
        found = section(f.read(), tag)
    if found is None:
        print('CHANGELOG has no section for %s' % tag, file=sys.stderr)
        return 1
    print(absolute(found, tag, repo))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
