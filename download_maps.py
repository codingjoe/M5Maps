#!/usr/bin/env python3
import math
import random
import socket
import sys
import time
import urllib.request
import urllib.error
import os
import concurrent.futures


socket.setdefaulttimeout(10)


def deg2num(lat_deg, lon_deg, zoom):
    lat_rad = math.radians(lat_deg)
    n = 2.0 ** zoom
    x = int((lon_deg + 180.0) / 360.0 * n)
    y = int((1.0 - math.asinh(math.tan(lat_rad)) / math.pi) / 2.0 * n)
    return x, y


def download_file(x, y, z, root_path):
    """
    Download a file from a URL.
    """

    full_path = os.path.join(root_path, f"{z}/{x}/")
    os.makedirs(full_path, exist_ok=True)
    file_path = os.path.join(full_path, f"{y}.png")
    i = 0
    while not os.path.exists(file_path):
        url_tmpl = random.choice([
            "https://opencache.statkart.no/gatekeeper/gk/gk.open_gmaps?layers=topo4&zoom={z}&x={x}&y={y}",
            "https://opencache2.statkart.no/gatekeeper/gk/gk.open_gmaps?layers=topo4&zoom={z}&x={x}&y={y}",
            "https://opencache3.statkart.no/gatekeeper/gk/gk.open_gmaps?layers=topo4&zoom={z}&x={x}&y={y}",
        ])
        url = url_tmpl.format(z=z, x=x, y=y)
        try:
            urllib.request.urlretrieve(url, file_path)
        except (TimeoutError, urllib.error.URLError):
            backoff = 2 ** i
            if backoff > 32:
                raise
            print(f"Timeout error downloading {url} retrying in {backoff} seconds...")
            time.sleep(backoff)
            i += 1
    return url, file_path


def calc_tiles(lon, lat):
    for rad in range(1, 4):
        z = rad + 13
        x, y = deg2num(float(lat), float(lon), z)
        for i in range(-(2 ** rad), (2 ** rad) + 1):

            for j in range(-(2 ** rad), (2 ** rad) + 1):
                yield x + i, y + j, z


def main(lon, lat, path):

    print("Downloading tiles...")
    with open(os.path.join(path, "zoom"), 'w') as f:
        f.write("10")
    with open(os.path.join(path, "longitude_deg"), 'w') as f:
        f.write(lon)
    with open(os.path.join(path, "latitude_deg"), 'w') as f:
        f.write(lat)

    with concurrent.futures.ThreadPoolExecutor() as executor:
        for url, path in executor.map(lambda args: download_file(*args), [(x, y, z, path) for x, y, z in calc_tiles(lon, lat)]):
            print(f"Downloaded {url} to {path}")

    print("\nDone.")


if __name__ == "__main__":
    main(*sys.argv[1:])
