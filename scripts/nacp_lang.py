#!/usr/bin/env python3
"""Patch per-language title entries in a Switch NACP.

nacptool writes the same name/author into all 16 language slots. This script
overrides individual slots, e.g. to show the original Japanese title only when
the console's system language is Japanese.

Usage:
    nacp_lang.py in.nacp out.nacp --lang Japanese --name "..." --author "..."
"""

import argparse
import shutil
import sys

# NacpLanguageEntry order, as defined by libnx / nn.
LANGUAGES = [
    "AmericanEnglish",
    "BritishEnglish",
    "Japanese",
    "French",
    "German",
    "LatinAmericanSpanish",
    "Spanish",
    "Italian",
    "Dutch",
    "CanadianFrench",
    "Portuguese",
    "Russian",
    "Korean",
    "TraditionalChinese",
    "SimplifiedChinese",
    "BrazilianPortuguese",
]

ENTRY_SIZE = 0x300
NAME_SIZE = 0x200
AUTHOR_SIZE = 0x100


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("input")
    ap.add_argument("output")
    ap.add_argument("--lang", action="append", required=True, choices=LANGUAGES)
    ap.add_argument("--name", action="append", required=True)
    ap.add_argument("--author", action="append", required=True)
    ap.add_argument("--fill-empty-name", help="fill language slots nacptool left blank")
    ap.add_argument("--fill-empty-author", help="fill language slots nacptool left blank")
    args = ap.parse_args()

    if not (len(args.lang) == len(args.name) == len(args.author)):
        ap.error("--lang/--name/--author must be given the same number of times")

    shutil.copyfile(args.input, args.output)
    with open(args.output, "r+b") as f:
        data = bytearray(f.read())

        for lang, name, author in zip(args.lang, args.name, args.author):
            idx = LANGUAGES.index(lang)
            off = idx * ENTRY_SIZE

            name_b = name.encode("utf-8")
            author_b = author.encode("utf-8")
            if len(name_b) >= NAME_SIZE:
                sys.exit(f"name too long for {lang}")
            if len(author_b) >= AUTHOR_SIZE:
                sys.exit(f"author too long for {lang}")

            data[off:off + NAME_SIZE] = name_b.ljust(NAME_SIZE, b"\0")
            data[off + NAME_SIZE:off + ENTRY_SIZE] = author_b.ljust(AUTHOR_SIZE, b"\0")
            print(f"[nacp_lang] {lang}: {name} / {author}")

        # nacptool only populates the first 12 language slots; consoles set to
        # Korean/Chinese/pt-BR would otherwise show a blank title.
        if args.fill_empty_name and args.fill_empty_author:
            name_b = args.fill_empty_name.encode("utf-8")
            author_b = args.fill_empty_author.encode("utf-8")
            for idx, lang in enumerate(LANGUAGES):
                off = idx * ENTRY_SIZE
                if data[off] != 0:
                    continue
                data[off:off + NAME_SIZE] = name_b.ljust(NAME_SIZE, b"\0")
                data[off + NAME_SIZE:off + ENTRY_SIZE] = author_b.ljust(AUTHOR_SIZE, b"\0")
                print(f"[nacp_lang] {lang}: filled with default title")

        f.seek(0)
        f.write(data)
        f.truncate()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
