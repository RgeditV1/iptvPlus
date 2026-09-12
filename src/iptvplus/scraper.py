import urllib.parse
import requests
import re

BASE_URL = "https://en.yts.lu/"

HEADERS = {
    "User-Agent": (
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/139.0 Safari/537.36"
    ),
    "Accept": "application/json, text/javascript, */*; q=0.01",
}

# Mapeo de géneros según el script JS de la página
GENRE_MAP = {
    "action": 28,
    "acción": 28,
    "adventure": 12,
    "aventura": 12,
    "animation": 16,
    "animacion": 16,
    "animación": 16,
    "comedy": 35,
    "comedia": 35,
    "crime": 80,
    "crimen": 80,
    "documentary": 99,
    "documental": 99,
    "drama": 18,
    "family": 10751,
    "familiar": 10751,
    "fantasy": 14,
    "fantasía": 14,
    "history": 36,
    "historia": 36,
    "horror": 27,
    "terror": 27,
    "music": 10402,
    "música": 10402,
    "mystery": 9648,
    "misterio": 9648,
    "romance": 10749,
    "sci-fi": 878,
    "ciencia ficcion": 878,
    "ciencia ficción": 878,
    "thriller": 53,
    "suspenso": 53,
    "war": 10752,
    "belico": 10752,
    "western": 37
}

TMDB_IMG_BASE = "https://image.tmdb.org/t/p"

def get_movie_details(title: str, tmdb_id: int = None, year: str = "") -> dict:
    """
    Consulta los torrents y reproductores disponibles para la película.
    """
    streams = []
    
    # Reproductor iframe por defecto (VidSrc)
    if tmdb_id:
        vidsrc_url = f"https://vidsrc.mov/embed/movie/{tmdb_id}"
        streams.append({"server": "VidSrc", "url": vidsrc_url})

    # Obtener Torrents / Magnets desde la API interna
    try:
        params = {
            "api": "torrents",
            "mode": "movie",
            "name": title,
            "year": str(year) if year else "",
            "quality": "all"
        }
        resp = requests.get(BASE_URL, params=params, headers=HEADERS, timeout=15)
        if resp.status_code == 200:
            data = resp.json()
            hits = data.get("hits", [])
            for torrent in hits:
                magnet = torrent.get("magnetUrl")
                t_title = torrent.get("title", "Torrent")
                if magnet:
                    streams.append({
                        "server": f"Magnet ({torrent.get('source', 'Torrent')}) - {t_title}",
                        "url": magnet
                    })
    except Exception as e:
        print(f"Error al obtener torrents para {title}: {e}")

    return {"streams": streams}


def _parse_api_results(results: list, limit: int = 10) -> list[dict]:
    parsed = []
    for item in results[:limit]:
        if item.get("media_type") == "person":
            continue

        tmdb_id = item.get("id")
        title = item.get("title") or item.get("name") or "Desconocido"
        poster_path = item.get("poster_path")
        poster = f"{TMDB_IMG_BASE}/w342{poster_path}" if poster_path else None
        
        release_date = item.get("release_date") or item.get("first_air_date") or ""
        year = release_date[:4] if release_date else ""

        detail_url = f"{BASE_URL}movie/{tmdb_id}" if tmdb_id else BASE_URL

        parsed.append({
            "id": tmdb_id,
            "title": title,
            "poster": poster,
            "url": detail_url,
            "description": item.get("overview"),
            "rating": item.get("vote_average"),
            "year": year
        })
    return parsed


def search_movies(query: str, limit: int = 10) -> list[dict]:
    """Busca películas utilizando la API interna del sitio."""
    params = {
        "api": "search",
        "mode": "movie",
        "q": query,
        "page": 1
    }
    response = requests.get(BASE_URL, params=params, headers=HEADERS, timeout=15)
    response.raise_for_status()
    data = response.json()
    return _parse_api_results(data.get("results", []), limit=limit)


def get_by_genre(genre_input: str, limit: int = 10) -> list[dict]:
    """Obtiene películas filtradas por género."""
    clean_input = genre_input.strip().lower()
    genre_id = GENRE_MAP.get(clean_input, 0)

    params = {
        "api": "discover",
        "mode": "movie",
        "page": 1,
        "sort": "popularity.desc",
        "genre": genre_id
    }
    response = requests.get(BASE_URL, params=params, headers=HEADERS, timeout=15)
    response.raise_for_status()
    data = response.json()
    return _parse_api_results(data.get("results", []), limit=limit)



def get_genres() -> list[dict]:
    """Retorna la lista de géneros soportados."""
    genres_list = []
    seen = set()
    for name, g_id in GENRE_MAP.items():
        if g_id not in seen:
            seen.add(g_id)
            genres_list.append({"name": name.title(), "slug": str(g_id)})
    return genres_list