#!/usr/bin/env python3
"""Writes docs/gallery/README.md, the gallery as Markdown that GitHub renders,
from the data in docs/gallery/index.html (TORTURE, RELEASES, PLANNED).

    tools/make_gallery_md.py [docs/gallery/index.html] [docs/gallery/README.md]
"""
import json, re, sys

def js_array(text, name):
    """The JavaScript array literal `const NAME = [...]` as Python data."""
    start = text.index('const %s = [' % name) + len('const %s = ' % name)
    depth, i = 0, start
    while True:
        c = text[i]
        if c == '"':
            i += 1
            while text[i] != '"':
                i += 2 if text[i] == '\\' else 1
        elif c == '[' or c == '{':
            depth += 1
        elif c == ']' or c == '}':
            depth -= 1
            if depth == 0:
                break
        i += 1
    src = text[start:i + 1]
    src = re.sub(r'//[^\n]*', '', src)                       # comments
    src = re.sub(r'([{,]\s*)([A-Za-z_]\w*)\s*:', r'\1"\2":', src)   # bare keys
    src = re.sub(r',(\s*[\]}])', r'\1', src)                 # trailing commas
    return json.loads(src)

def main():
    html_path = sys.argv[1] if len(sys.argv) > 1 else 'docs/gallery/index.html'
    out_path = sys.argv[2] if len(sys.argv) > 2 else 'docs/gallery/README.md'
    text = open(html_path, encoding='utf-8').read()
    torture, releases, planned = (js_array(text, n) for n in ('TORTURE', 'RELEASES', 'PLANNED'))
    repo = 'https://github.com/jnaujok/Ray'
    out = ['# Ray release gallery', '',
           'Every release, rendered: what each one changed in the pictures Ray makes, from the 1997',
           'code to today. This page is written by `tools/make_gallery_md.py` from',
           '[`index.html`](index.html), the same gallery as an interactive page with a slider that',
           'compares any two versions of the torture test (open it in a browser, or through GitHub',
           'Pages).', '',
           '## The torture test, version by version', '',
           '| Version | Image | What changed |', '|---|---|---|']
    for t in torture:
        out.append('| alpha %s | <img src="%s" width="320" alt="Torture test at %s"> | %s |' % (t['v'], t['src'], t['v'], t['note']))
    out += ['', '## Releases, newest first', '']
    for r in releases:
        v = r['v']
        tag = ('v' + v) if v.startswith('0.') else v
        title = v.replace('2026-alpha-', 'alpha ') if v.startswith('2026') else v.capitalize()
        out.append('### %s (%s)' % (title, r['date']))
        out.append('')
        if v.startswith('2026') or v.startswith('0.'):
            out.append('[Code at this tag](%s/tree/%s) · [Changelog](%s/blob/%s/CHANGELOG.md)' % (repo, tag, repo, tag))
            out.append('')
        out.append(r['summary'])
        out.append('')
        if not r['shots'] and r.get('none'):
            out += ['*' + r['none'] + '*', '']
        for s in r['shots']:
            width = 720 if s.get('wide') else 360
            out.append('<img src="%s" width="%d" alt="%s">' % (s['src'], width, s['title']))
            out.append('')
            out.append('**%s.** %s' % (s['title'], s['cap']))
            out.append('')
    out += ['## Coming releases', '', '| Version | Plan | State |', '|---|---|---|']
    for p in planned:
        out.append('| alpha %s | %s | %s |' % (p['v'], p['what'], p['state']))
    out.append('')
    open(out_path, 'w', encoding='utf-8').write('\n'.join(out))
    print('wrote', out_path, len(releases), 'releases,', len(torture), 'torture versions')

if __name__ == '__main__':
    main()
