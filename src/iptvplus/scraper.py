import re
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


def _fetch_and_parse_movies(url: str, params: dict = None, limit: int = 10) -> list[dict]:
    """Función auxiliar para hacer GET a una URL y extraer las películas del HTML."""
    response = requests.get(
        url,
        params=params,
        timeout=15,
        headers=HEADERS,
    )
    response.raise_for_status()

    soup = BeautifulSoup(response.text, "html.parser")
    results = []

    for article in soup.select("article.item.movies"):
        # Ignorar publicidad
        if "naadb" in article.get("class", []):
            continue

        # Buscar el indicador de tipo e ignorar series
        type_element = article.select_one(".selt")
        if type_element and "serie" in type_element.get_text(strip=True).lower():
            continue

        # Título
        title_element = article.select_one(".in_title")
        if not title_element:
            continue
        title = title_element.get_text(strip=True)

        # Enlace
        link_element = article.select_one("a[href]")
        if not link_element:
            continue
        movie_url = urljoin(BASE_URL, link_element.get("href"))

        # Portada
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

    # Título principal
    title_el = single_left.select_one("h1")
    title = title_el.get_text(strip=True) if title_el else ""

    # Poster en la vista detallada
    poster_el = single_left.select_one("td img")
    poster = ""
    if poster_el:
        poster_src = (
            poster_el.get("data-src")
            or poster_el.get("src")
        )
        if poster_src:
            poster = urljoin(BASE_URL, poster_src)

    # Sinopsis / Descripción
    desc_el = single_left.select_one("td[style*='justify'] > p")
    description = desc_el.get_text(strip=True) if desc_el else None

    # Rating / Puntuación (TMDB)
    rating = None
    rating_b = single_left.select_one("span b")
    if rating_b:
        try:
            rating = float(rating_b.get_text(strip=True))
        except ValueError:
            pass

    genres = []
    for g_anchor in single_left.select("span a[href*='/genero-de-la-pelicula/']"):
        genres.append(g_anchor.get_text(strip=True))

    return {
        "title": title,
        "url": url,
        "poster": poster,
        "description": description,
        "rating": rating,
        "genres": genres,
    }

def get_genres() -> list[dict]:
    """Retorna la lista de géneros disponibles basados en GENRE_MAP."""
    seen_slugs = set()
    genres_list = []

    for key, slug in GENRE_MAP.items():
        if slug not in seen_slugs:
            seen_slugs.add(slug)
            genres_list.append({"name": key.title(), "slug": slug})

    return genres_list


def get_by_genre(genre_input: str, limit: int = 10) -> list[dict]:
    """Filtra y realiza la búsqueda de películas por el género indicado."""
    clean_input = genre_input.strip().lower()
    slug = GENRE_MAP.get(clean_input, clean_input)

    target_url = f"{BASE_URL}/genero-de-la-pelicula/{slug}/"
    return _fetch_and_parse_movies(target_url, limit=limit)


def search_movies(query: str, limit: int = 10) -> list[dict]:
    """Busca películas en Cinecalidad utilizando la barra de búsqueda."""
    return _fetch_and_parse_movies(BASE_URL, params={"s": query}, limit=limit)