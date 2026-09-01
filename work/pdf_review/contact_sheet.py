from pathlib import Path
import math
import sys

from PIL import Image, ImageDraw


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit("usage: contact_sheet.py IMAGE_DIR OUTPUT")
    image_dir = Path(sys.argv[1])
    output = Path(sys.argv[2])
    paths = sorted(image_dir.glob("page-*.jpg"))
    if not paths:
        raise SystemExit(f"no rendered pages in {image_dir}")
    columns = 3
    thumb_width = 360
    label_height = 28
    pages: list[Image.Image] = []
    for page_no, path in enumerate(paths, start=1):
        with Image.open(path) as source:
            scale = thumb_width / source.width
            thumb = source.convert("RGB").resize(
                (thumb_width, round(source.height * scale)), Image.Resampling.LANCZOS
            )
        canvas = Image.new("RGB", (thumb.width, thumb.height + label_height), "white")
        canvas.paste(thumb, (0, label_height))
        ImageDraw.Draw(canvas).text((8, 6), f"Page {page_no}", fill="black")
        pages.append(canvas)
    cell_height = max(page.height for page in pages)
    rows = math.ceil(len(pages) / columns)
    sheet = Image.new("RGB", (columns * thumb_width, rows * cell_height), "#dddddd")
    for index, page in enumerate(pages):
        x = (index % columns) * thumb_width
        y = (index // columns) * cell_height
        sheet.paste(page, (x, y))
    sheet.save(output, quality=90)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
