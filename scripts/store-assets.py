"""Resize the existing logo and enforce the WACK 200 KiB image limit.

Uses Pillow from the project's existing GUI tooling, only on the build machine.
"""
import argparse
from pathlib import Path

from PIL import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source = Path(__file__).resolve().parents[1] / "public/logo/logo.png"
    args.output.mkdir(parents=True, exist_ok=True)
    sizes = {
        "StoreLogo.png": 50,
        "Square44x44Logo.png": 44,
        "Square150x150Logo.png": 150,
        "StoreLogo.scale-200.png": 100,
        "Square44x44Logo.scale-200.png": 88,
        "Square150x150Logo.scale-200.png": 300,
        "StoreLogo.scale-400.png": 200,
        "Square44x44Logo.scale-400.png": 176,
        "Square150x150Logo.scale-400.png": 600,
        "StoreListingLogo.png": 300,
    }
    with Image.open(source) as original:
        for name, size in sizes.items():
            image = original.resize((size, size), Image.Resampling.LANCZOS)
            output = args.output / name
            image.save(output, optimize=True)
            if output.stat().st_size >= 204800:
                # Adaptive palette followed by error diffusion preserves gradients.
                if image.mode == "RGB":
                    palette = image.quantize(colors=256, method=Image.Quantize.MEDIANCUT)
                    image = image.quantize(palette=palette, dither=Image.Dither.FLOYDSTEINBERG)
                else:
                    image = image.convert("RGBA").quantize(colors=256, method=Image.Quantize.FASTOCTREE)
                image.save(output, optimize=True)
            if output.stat().st_size >= 204800:
                raise ValueError(f"WACK image size limit exceeded: {name}")
            with Image.open(output) as check:
                if check.format != "PNG" or check.size != (size, size):
                    raise ValueError(f"Invalid Store image: {name}")


if __name__ == "__main__":
    main()
