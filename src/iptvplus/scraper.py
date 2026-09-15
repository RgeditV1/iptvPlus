from tmdb_scraper import (
    search_tmdb_movies,
    get_tmdb_by_genre,
    get_genres_list,
    get_imdb_id
)
from torrentio import get_torrentio_streams
from db import get_db_connection


def search_movies(query: str, limit: int = 10) -> list[dict]:
    movies = search_tmdb_movies(query, limit=limit)

    for movie in movies:
        tmdb_id = movie.get("id")

        if tmdb_id:
            imdb_id = get_imdb_id(tmdb_id)
            movie["imdb_id"] = imdb_id
            movie["streams"] = (
                get_torrentio_streams(imdb_id)
                if imdb_id
                else []
            )
        else:
            movie["streams"] = []

    return movies


def get_by_genre(genre_input: str, limit: int = 10) -> list[dict]:
    """Filtra películas por género en TMDB y vincula sus streams desde Torrentio."""
    movies = get_tmdb_by_genre(genre_input, limit=limit)

    for movie in movies:
        tmdb_id = movie.get("id")

        if tmdb_id:
            imdb_id = get_imdb_id(tmdb_id)
            movie["imdb_id"] = imdb_id

            movie["streams"] = (
                get_torrentio_streams(imdb_id)
                if imdb_id
                else []
            )
        else:
            movie["imdb_id"] = None
            movie["streams"] = []

    return movies

def get_genres() -> list[dict]:
    """Obtiene los géneros disponibles."""
    return get_genres_list()


def get_movie_details(title: str, tmdb_id: int = None, year: str = "") -> dict:
    """Obtiene los detalles de la película guardada en la BD local."""
    with get_db_connection() as conn:
        cursor = conn.cursor()
        
        target_url = f"https://www.themoviedb.org/movie/{tmdb_id}" if tmdb_id else None
        
        if target_url:
            cursor.execute("""
                SELECT id, title, type, url, poster, description, release_year, rating, trailer
                FROM media WHERE url = ? AND type = 'movie';
            """, (target_url,))
        else:
            cursor.execute("""
                SELECT id, title, type, url, poster, description, release_year, rating, trailer
                FROM media WHERE title LIKE ? AND type = 'movie';
            """, (f"%{title}%",))

        row = cursor.fetchone()
        if not row:
            return {"streams": []}

        movie_data = dict(row)

        cursor.execute("""
            SELECT server, url, language, quality FROM streams WHERE media_id = ?;
        """, (movie_data["id"],))
        movie_data["streams"] = [dict(s) for s in cursor.fetchall()]

        cursor.execute("""
            SELECT g.name FROM genres g
            JOIN media_genres mg ON g.id = mg.genre_id
            WHERE mg.media_id = ?;
        """, (movie_data["id"],))
        movie_data["genres"] = [g["name"] for g in cursor.fetchall()]

        return movie_data