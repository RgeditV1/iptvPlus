import os
import requests

TMDB_API_KEY = os.environ.get("TMDB_API_KEY", "123b8afffe2428799cc508f55848a789")
BASE_URL = "https://api.themoviedb.org/3"
TMDB_IMG_BASE = "https://image.tmdb.org/t/p"

GENRE_MAP = {
    "acción": 28, "accion": 28, "action": 28,
    "aventura": 12, "adventure": 12,
    "animación": 16, "animacion": 16, "animation": 16,
    "comedia": 35, "comedy": 35,
    "crimen": 80, "crime": 80,
    "documental": 99, "documentary": 99,
    "drama": 18,
    "familiar": 10751, "family": 10751,
    "fantasía": 14, "fantasia": 14, "fantasy": 14,
    "historia": 36, "history": 36,
    "terror": 27, "horror": 27,
    "música": 10402, "musica": 10402, "music": 10402,
    "misterio": 9648, "mystery": 9648,
    "romance": 10749,
    "ciencia ficción": 878, "ciencia ficcion": 878, "sci-fi": 878,
    "suspenso": 53, "thriller": 53,
    "bélico": 10752, "belico": 10752, "war": 10752,
    "western": 37
}


def _get_trailer_url(tmdb_id: int) -> str | None:
    """Obtiene el enlace del tráiler en YouTube desde TMDB."""
    try:
        url = f"{BASE_URL}/movie/{tmdb_id}/videos"
        params = {"api_key": TMDB_API_KEY, "language": "es-ES"}
        resp = requests.get(url, params=params, timeout=10)
        
        # Si no hay tráiler en español, reintentar en inglés
        if resp.status_code == 200:
            results = resp.json().get("results", [])
            if not results:
                params["language"] = "en-US"
                resp = requests.get(url, params=params, timeout=10)
                results = resp.json().get("results", []) if resp.status_code == 200 else []

            for vid in results:
                if vid.get("site") == "YouTube" and vid.get("type") in ["Trailer", "Teaser"]:
                    return f"https://www.youtube.com/watch?v={vid.get('key')}"
    except Exception as e:
        print(f"Error obteniendo tráiler para TMDB ID {tmdb_id}: {e}")
    return None


def _parse_tmdb_results(results: list, limit: int = 10) -> list[dict]:
    parsed = []
    for item in results[:limit]:
        tmdb_id = item.get("id")
        title = item.get("title") or item.get("original_title") or "Sin título"
        poster_path = item.get("poster_path")
        poster = f"{TMDB_IMG_BASE}/w342{poster_path}" if poster_path else None
        
        release_date = item.get("release_date") or ""
        year_str = release_date[:4] if release_date else ""
        year = int(year_str) if year_str.isdigit() else None

        trailer = _get_trailer_url(tmdb_id) if tmdb_id else None

        parsed.append({
            "id": tmdb_id,
            "title": title,
            "poster": poster,
            "url": f"https://www.themoviedb.org/movie/{tmdb_id}",
            "description": item.get("overview", ""),
            "rating": item.get("vote_average", 0.0),
            "year": year,
            "release_year": year,
            "trailer": trailer
        })
    return parsed

def get_imdb_id(tmdb_id: int) -> str | None:
    url = f"{BASE_URL}/movie/{tmdb_id}/external_ids"

    params = {
        "api_key": TMDB_API_KEY
    }

    try:
        response = requests.get(url, params=params, timeout=10)
        response.raise_for_status()

        data = response.json()
        return data.get("imdb_id")

    except requests.RequestException as e:
        print(f"Error obteniendo IMDb ID para TMDB {tmdb_id}: {e}")
        return None

def search_tmdb_movies(query: str, limit: int = 10) -> list[dict]:
    """Busca películas directamente en TMDB en español."""
    url = f"{BASE_URL}/search/movie"
    params = {
        "api_key": TMDB_API_KEY,
        "query": query,
        "language": "es-ES",
        "page": 1,
        "include_adult": "false"
    }
    response = requests.get(url, params=params, timeout=15)
    response.raise_for_status()
    data = response.json()
    return _parse_tmdb_results(data.get("results", []), limit=limit)


def get_tmdb_by_genre(genre_input: str, limit: int = 10) -> list[dict]:
    """Descubre películas de un género específico en TMDB."""
    clean_input = genre_input.strip().lower()
    genre_id = GENRE_MAP.get(clean_input, 28)

    url = f"{BASE_URL}/discover/movie"
    params = {
        "api_key": TMDB_API_KEY,
        "with_genres": genre_id,
        "language": "es-ES",
        "sort_by": "popularity.desc",
        "page": 1,
        "include_adult": "false"
    }
    response = requests.get(url, params=params, timeout=15)
    response.raise_for_status()
    data = response.json()
    return _parse_tmdb_results(data.get("results", []), limit=limit)


def get_genres_list() -> list[dict]:
    """Lista todos los géneros soportados."""
    genres_list = []
    seen = set()
    for name, g_id in GENRE_MAP.items():
        if g_id not in seen:
            seen.add(g_id)
            genres_list.append({"name": name.title(), "slug": str(g_id)})
    return genres_list