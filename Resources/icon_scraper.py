import os
import requests
from bs4 import BeautifulSoup
import time

BASE_URL = "https://wiki.bloodontheclocktower.com"
SAVE_DIR = "botc_icons"
os.makedirs(SAVE_DIR, exist_ok=True)

HEADERS = {"User-Agent": "Mozilla/5.0"}

# Character type pages
CHAR_TYPES = [
    "/Trouble_Brewing",
    "/Sects_%26_Violets",
    "/Bad_Moon_Rising",
    "/Travellers",
    "/Fabled",
    "/Loric",
    "/Experimental"
]

def get_character_links(page_suffix):
    """Get all character page links from a character type page."""
    url = BASE_URL + page_suffix
    resp = requests.get(url, headers=HEADERS)
    soup = BeautifulSoup(resp.text, "html.parser")
    links = set()
    
    # Find all character links in the main content (exclude category links)
    for a in soup.select("#mw-content-text a[href^='/']"):
        href = a.get("href")
        if href and not any(x in href for x in [":", "Main_Page"]):
            links.add(BASE_URL + href)
    return list(links)

def download_icon(char_url):
    """Download the main character icon."""
    resp = requests.get(char_url, headers=HEADERS)
    soup = BeautifulSoup(resp.text, "html.parser")
    
    img_tag = soup.select_one("a.image img")  # main image inside the page
    if img_tag:
        img_url = BASE_URL + img_tag.get("src")
        # Extract character name from URL for filename
        name = char_url.split("/")[-1].replace("_", " ")
        filename = os.path.join(SAVE_DIR, f"{name}.png")
        img_data = requests.get(img_url, headers=HEADERS).content
        with open(filename, "wb") as f:
            f.write(img_data)
        print(f"Downloaded: {filename}")
    else:
        print(f"No icon found for {char_url}")

# Main script
all_links = []
for char_type in CHAR_TYPES:
    print(f"Fetching links from {char_type} ...")
    links = get_character_links(char_type)
    print(f"Found {len(links)} characters.")
    all_links.extend(links)
    time.sleep(1)

all_links = list(set(all_links))
print(all_links[0])
print(f"Total unique character pages: {len(all_links)}")

for char_url in all_links:
    download_icon(char_url)
    time.sleep(0.5)

print("All character icons downloaded!")
