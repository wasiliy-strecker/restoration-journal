"""Prüft PDF-Schrift, Wortgeometrie und Seiten mit den externen Poppler-Werkzeugen."""

import argparse
import re
import subprocess
import xml.etree.ElementTree as ET
from pathlib import Path


def run(*args):
    return subprocess.check_output(args, text=True, timeout=30)


def verify(path):
    fonts = run("pdffonts", str(path)).splitlines()[2:]
    assert fonts and all(
        "DejaVuSans" in line and re.search(r"yes\s+(yes|no)\s+yes", line)
        for line in fonts
    ), "Schrift oder Unicode-Zuordnung fehlt"
    root = ET.fromstring(run("pdftotext", "-bbox", str(path), "-"))
    namespace = {"x": "http://www.w3.org/1999/xhtml"}
    pages = root.findall(".//x:page", namespace)
    words = 0
    for page in pages:
        assert abs(float(page.attrib["width"]) - 595) < 1
        assert abs(float(page.attrib["height"]) - 842) < 1
        rectangles = []
        for word in page.findall(".//x:word", namespace):
            x0, y0, x1, y1 = (
                float(word.attrib[key]) for key in ("xMin", "yMin", "xMax", "yMax")
            )
            assert x0 >= 47 and x1 <= 548, (word.text, "Seitenrand")
            assert (28 <= y0 and y1 <= 44) or (79 <= y0 and y1 <= 792) or (808 <= y0 and y1 <= 821), (word.text, "Inhaltsbereich")
            for u0, v0, u1, v1 in rectangles:
                assert not (
                    min(x1, u1) - max(x0, u0) > 0.5 and min(y1, v1) - max(y0, v0) > 0.5
                ), (word.text, "Wortüberlappung")
            rectangles.append((x0, y0, x1, y1))
            words += 1
    assert pages and words
    print(
        f"{path.name}: {len(pages)} A4-Seiten, {words} Wörter, Schrift und Geometrie geprüft"
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pdf", nargs="+", type=Path)
    args = parser.parse_args()
    for path in args.pdf:
        verify(path)


if __name__ == "__main__":
    main()
