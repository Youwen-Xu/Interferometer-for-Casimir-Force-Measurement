from pathlib import Path
import sys

import pdfplumber


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit("usage: extract_pdfs.py INPUT OUTPUT")
    input_path = Path(sys.argv[1])
    output_path = Path(sys.argv[2])
    chunks: list[str] = []
    with pdfplumber.open(input_path) as pdf:
        for page_number, page in enumerate(pdf.pages, start=1):
            text = page.extract_text(layout=True) or "[NO EXTRACTABLE TEXT]"
            chunks.append(f"\n===== PAGE {page_number} / {len(pdf.pages)} =====\n{text}\n")
    output_path.write_text("".join(chunks), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
