import re

def parse_bib_entries(bib_content):
    entries = re.split(r'@\w+\{', bib_content)[1:]
    parsed = []
    for entry in entries:
        key_match = re.match(r'([^,]+),', entry)
        key = key_match.group(1).strip() if key_match else 'unknown'
        title_match = re.search(r'title\s*=\s*[{\"]([^\}"]+)[}\"]', entry, re.IGNORECASE)
        title = title_match.group(1).strip() if title_match else key
        # Remove leading '{' from title if present
        if title.startswith('{'):
            title = title[1:]
        author_match = re.search(r'author\s*=\s*[{\"]([^\}"]+)[}\"]', entry, re.IGNORECASE)
        author = author_match.group(1).strip() if author_match else ''
        parsed.append({'key': key, 'title': title, 'author': author})
    return parsed
