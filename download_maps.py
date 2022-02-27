#!/usr/bin/env python3
import concurrent.futures
import math
import os
import random
import socket
import time
import urllib.error
import urllib.request
from pathlib import Path
from tempfile import mkdtemp

import click
from tqdm import tqdm

socket.setdefaulttimeout(10)


def deg2num(lat_deg, lon_deg, zoom):
    lat_rad = math.radians(lat_deg)
    n = 2.0**zoom
    x = int((lon_deg + 180.0) / 360.0 * n)
    y = int((1.0 - math.asinh(math.tan(lat_rad)) / math.pi) / 2.0 * n)
    return x, y


def download_file(url_tmpls, x, y, z, root_path):
    """
    Download a file from a URL.
    """

    full_path = root_path / str(z) / str(x)
    os.makedirs(full_path, exist_ok=True)
    file_path = full_path / f"{y}.png"
    i = 0
    while not os.path.exists(file_path):
        url_tmpl = random.choice(url_tmpls)
        url = url_tmpl.format(z=z, x=x, y=y)
        try:
            urllib.request.urlretrieve(url, file_path)
        except (TimeoutError, urllib.error.URLError):
            backoff = 2**i
            if backoff > 64:
                raise
            time.sleep(backoff)
            i += 1
    return url, file_path


def calc_tiles(lon, lat):
    for rad in range(1, 5):
        z = rad + 12
        x, y = deg2num(lat, lon, z)
        for i in range(-(2**rad), (2**rad) + 1):
            for j in range(-(2**rad), (2**rad) + 1):
                yield x + i, y + j, z


@click.command()
@click.argument("lon", nargs=1, type=float)
@click.argument("lat", nargs=1, type=float)
@click.argument(
    "dest", nargs=1, type=click.Path(file_okay=False, dir_okay=True, writable=True)
)
@click.argument("TILE_SRV", nargs=-1, type=str, required=True)
def main(lon, lat, dest, tile_srv):
    dest = Path(dest)
    with open(dest / "zoom", "w") as f:
        f.write("14")
    with open(dest / "longitude_deg", "w") as f:
        f.write(str(lon))
    with open(dest / "latitude_deg", "w") as f:
        f.write(str(lat))

    tmp_path = Path(mkdtemp())
    tiles = list(calc_tiles(lon, lat))

    with concurrent.futures.ThreadPoolExecutor(len(tile_srv)) as executor:
        list(
            tqdm(
                executor.map(
                    lambda args: download_file(*args),
                    ((tile_srv, x, y, z, tmp_path) for x, y, z in tiles),
                ),
                total=len(tiles),
                unit="tiles",
                desc="Downloading tiles",
            )
        )

    cmd = f"mogrify -define bmp:format=bmp3 -type Grayscale -colors 16 -compress none -format bmp {tmp_path / '*' / '*' / '*.png'}"
    print(f"Converting to 4bit 16 color bitmaps: {cmd}")
    os.system(cmd)
    cmd = f"rsync -a --prune-empty-dirs --include '*/' --include '*.bmp' --exclude '*' '{tmp_path}{os.sep}' '{dest}'"
    print(f"Coping to SD card: {cmd}")
    os.system(cmd)

    print("\nDone.")


if __name__ == "__main__":
    main()
