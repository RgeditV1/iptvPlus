import yt_dlp
import base64
from urllib.parse import urljoin
import requests
from bs4 import BeautifulSoup

BASE_URL = "https://www.cinecalidad.am"

HEADERS = {
    "User-Agent": (
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/139.0 Safari/537.36"
    )
}

GENRE_MAP = {
    "accion": "accion",
    "acción": "accion",
    "animacion": "animacion",
    "animación": "animacion",
    "anime": "anime",
    "aventura": "aventura",
    "belico": "belica",
    "bélico": "belica",
    "belica": "belica",
    "ciencia ficcion": "ciencia-ficcion",
    "ciencia ficción": "ciencia-ficcion",
    "ciencia-ficcion": "ciencia-ficcion",
    "crimen": "crimen",
    "comedia": "comedia",
    "documental": "documental",
    "drama": "drama",
    "familiar": "familia",
    "familia": "familia",
    "fantasia": "fantasia",
    "fantasía": "fantasia",
    "historia": "historia",
    "musica": "musica",
    "música": "musica",
    "misterio": "misterio",
    "terror": "terror",
    "suspenso": "suspense",
    "suspense": "suspense",
    "romance": "romance",
    "dc comics": "peliculas-de-dc-comics-online-cinecalidad",
    "marvel": "universo-marvel",
}


def resolve_stream_url(embed_url: str, referer_url: str = None) -> str:
    """
    Intenta extraer la URL directa de reproducción (.m3u8 / .mp4).
    Si yt-dlp no soporta el servidor, recurre a inspecionar el HTML.
    """
    ydl_opts = {
        'format': 'best',
        'quiet': True,
        'no_warnings': True,
    }
    
    if referer_url:
        ydl_opts['http_headers'] = {'Referer': referer_url}

    # 1. Intentar extracción con yt-dlp (Funciona para YouTube, Vimeo oficial, etc.)
    try:
        with yt_dlp.YoutubeDL(ydl_opts) as ydl:
            info = ydl.extract_info(embed_url, download=False)
            if 'url' in info:
                return info['url']
    except Exception:
        pass

    # 2. Fallback para servidores iframe clon (ej. vimeos.net, voe clones)
    try:
        headers = {
            "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/139.0",
            "Referer": referer_url or "https://www.cinecalidad.am/"
        }
        resp = requests.get(embed_url, headers=headers, timeout=10)
        
        # Buscar en el HTML si hay un iframe anidado o un enlace m3u8
        soup = BeautifulSoup(resp.text, "html.parser")
        
        # Si vimeos.net redirige internamente a otro iframe (como Voe o Fembed)
        nested_iframe = soup.select_one("iframe[src]")
        if nested_iframe and nested_iframe.get("src"):
            return nested_iframe["src"]

    except Exception as e:
        print(f"Error resolviendo enlace: {e}")

    # Retorna la URL embebida original si no se pudo desofuscar
    return embed_url

def _decode_url_if_needed(raw_url: str) -> str:
    """Intenta decodificar URLs si vienen en Base64 o limpia espacios."""
    raw_url = raw_url.strip()
    # Si parece una cadena Base64 sin espacios
    if not raw_url.startswith("http") and len(raw_url) > 10 and " " not in raw_url:
        try:
            decoded = base64.b64decode(raw_url).decode("utf-8")
            if decoded.startswith("http"):
                return decoded
        except Exception:
            pass
    return raw_url


def extract_media_links(soup: BeautifulSoup) -> dict:
    """Extrae enlaces de reproductores (Voe, Vimeo) y tráiler del objeto BeautifulSoup."""
    streams = []
    trailer = None

    for iframe in soup.select("iframe[src], iframe[data-src]"):
        src = iframe.get("data-src") or iframe.get("src", "")
        src = _decode_url_if_needed(src)
        if not src or src.startswith(("javascript:", "data:")):
            continue

        full_url = urljoin(BASE_URL, src)

        if "voe" in full_url.lower():
            streams.append({"server": "Voe", "url": full_url})
        elif "vimeo" in full_url.lower():
            streams.append({"server": "Vimeo", "url": full_url})
        elif "youtube.com" in full_url.lower() or "youtu.be" in full_url.lower():
            if not trailer:
                trailer = full_url

    player_options = soup.select(".option, .do-play, [data-link], [data-option], .nav-tabs li, .player-option")
    for opt in player_options:
        raw_target = (
            opt.get("data-link")
            or opt.get("data-option")
            or opt.get("data-url")
            or opt.get("href")
            or ""
        )
        if not raw_target or raw_target.startswith("#"):
            continue

        url = _decode_url_if_needed(raw_target)
        if not url.startswith("http"):
            url = urljoin(BASE_URL, url)

        opt_text = opt.get_text(strip=True).lower()

        if "voe" in url.lower() or "voe" in opt_text:
            if not any(s["url"] == url for s in streams):
                streams.append({"server": "Voe", "url": url})
        elif "vimeo" in url.lower() or "vimeo" in opt_text:
            if not any(s["url"] == url for s in streams):
                streams.append({"server": "Vimeo", "url": url})
        elif "youtube.com" in url.lower() or "youtu.be" in url.lower():
            if not trailer:
                trailer = url

    if not trailer:
        trailer_el = soup.select_one("#trailer, .trailer-link, a[href*='youtube.com'], a[href*='youtu.be']")
        if trailer_el:
            t_url = trailer_el.get("href") or trailer_el.get("data-url") or trailer_el.get("data-link")
            if t_url:
                trailer = _decode_url_if_needed(t_url)

    for anchor in soup.select("a[href]"):
        href = anchor.get("href", "")
        decoded_href = _decode_url_if_needed(href)
        
        if "voe.sx" in decoded_href.lower() or "/e/" in decoded_href.lower() and "voe" in decoded_href.lower():
            if not any(s["url"] == decoded_href for s in streams):
                streams.append({"server": "Voe", "url": decoded_href})
        elif "vimeo.com" in decoded_href.lower():
            if not any(s["url"] == decoded_href for s in streams):
                streams.append({"server": "Vimeo", "url": decoded_href})
        elif ("youtube.com" in decoded_href.lower() or "youtu.be" in decoded_href.lower()) and not trailer:
            trailer = decoded_href

    return {
        "streams": streams,
        "trailer": trailer,
    }


def _fetch_and_parse_movies(url: str, params: dict = None, limit: int = 10) -> list[dict]:
    response = requests.get(url, params=params, timeout=15, headers=HEADERS)
    response.raise_for_status()

    soup = BeautifulSoup(response.text, "html.parser")
    results = []

    for article in soup.select("article.item.movies"):
        if "naadb" in article.get("class", []):
            continue

        type_element = article.select_one(".selt")
        if type_element and "serie" in type_element.get_text(strip=True).lower():
            continue

        title_element = article.select_one(".in_title")
        if not title_element:
            continue
        title = title_element.get_text(strip=True)

        link_element = article.select_one("a[href]")
        if not link_element:
            continue
        movie_url = urljoin(BASE_URL, link_element.get("href"))

        image_element = article.select_one("img")
        if not image_element:
            continue

        poster = (
            image_element.get("data-src")
            or image_element.get("data-lazy-src")
            or image_element.get("src")
        )
        if not poster:
            continue

        results.append({
            "title": title,
            "poster": urljoin(BASE_URL, poster),
            "url": movie_url,
        })

        if len(results) >= limit:
            break

    return results


def get_movie_details(url: str) -> dict:
    """Extrae la información detallada de una película desde su página individual."""
    response = requests.get(url, timeout=15, headers=HEADERS)
    response.raise_for_status()

    soup = BeautifulSoup(response.text, "html.parser")
    single_left = soup.select_one(".single_left")

    if not single_left:
        return {}

    title_el = single_left.select_one("h1")
    title = title_el.get_text(strip=True) if title_el else ""

    poster_el = single_left.select_one("td img")
    poster = ""
    if poster_el:
        poster_src = poster_el.get("data-src") or poster_el.get("src")
        if poster_src:
            poster = urljoin(BASE_URL, poster_src)

    desc_el = single_left.select_one("td[style*='justify'] > p")
    description = desc_el.get_text(strip=True) if desc_el else None

    rating = None
    rating_b = single_left.select_one("span b")
    if rating_b:
        try:
            rating = float(rating_b.get_text(strip=True))
        except ValueError:
            pass

    genres = [
        g_anchor.get_text(strip=True)
        for g_anchor in single_left.select("span a[href*='/genero-de-la-pelicula/']")
    ]

    # Extracción de reproductores y tráiler
    media_data = extract_media_links(soup)

    return {
        "title": title,
        "url": url,
        "poster": poster,
        "description": description,
        "rating": rating,
        "genres": genres,
        "streams": media_data["streams"],
        "trailer": media_data["trailer"],
    }


def get_genres() -> list[dict]:
    seen_slugs = set()
    genres_list = []

    for key, slug in GENRE_MAP.items():
        if slug not in seen_slugs:
            seen_slugs.add(slug)
            genres_list.append({"name": key.title(), "slug": slug})

    return genres_list


def get_by_genre(genre_input: str, limit: int = 10) -> list[dict]:
    clean_input = genre_input.strip().lower()
    slug = GENRE_MAP.get(clean_input, clean_input)

    target_url = f"{BASE_URL}/genero-de-la-pelicula/{slug}/"
    return _fetch_and_parse_movies(target_url, limit=limit)


def search_movies(query: str, limit: int = 10) -> list[dict]:
    return _fetch_and_parse_movies(BASE_URL, params={"s": query}, limit=limit)