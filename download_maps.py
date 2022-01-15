#!/usr/bin/env python3
import math
import sys
import urllib.request
import os


def deg2num(lat_deg, lon_deg, zoom):
    lat_rad = math.radians(lat_deg)
    n = 2.0 ** zoom
    x = int((lon_deg + 180.0) / 360.0 * n)
    y = int((1.0 - math.asinh(math.tan(lat_rad)) / math.pi) / 2.0 * n)
    return x, y


def main(lon, lat, zoom=14, rad=10):
    """
    Download maps from the command line.
    """
    x, y = deg2num(float(lat), float(lon), int(zoom))
    z = int(zoom)
    print("Downloading tiles...", end="")
    for i in range(-rad, rad + 1):
        os.makedirs(f"{z}/{x + i}/", exist_ok=True)
        for j in range(-rad, rad + 1):
            print(".", end="")
            urllib.request.urlretrieve(f"https://a.tile.opentopomap.org/{z}/{x + i}/{y + j}.png", f"{z}/{x + i}/{y + j}.png")
    print("\nDone.")


if __name__ == "__main__":
    main(*sys.argv[1:])
