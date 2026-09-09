import os
import re
import sys
import sqlite3
from pathlib import Path
from contextlib import contextmanager


def get_app_data_dir(app_name: str = "iptvPlus") -> Path:
    """
    Retorna el directorio en %LOCALAPPDATA% para Windows,
    o ~/.local/share para Linux/macOS. 
    """
    if sys.platform == "win32":
        base_path = Path(os.environ.get("LOCALAPPDATA", Path.home() / "AppData" / "Local"))
    else:
        base_path = Path.home() / ".local" / "share"

    data_dir = base_path / app_name
    data_dir.mkdir(parents=True, exist_ok=True)
    return data_dir


DB_PATH = get_app_data_dir("iptvPlus") / "database.db"


def parse_magnet_metadata(magnet_url: str, title: str = "") -> dict:
    """Extrae idioma y calidad detectados desde el magnet o su título descriptivo."""
    text_to_search = f"{title} {magnet_url}".lower()

    # Detección de idioma por patrones de texto común
    language = "ENG"  # Por defecto la mayoría de torrents
    
    if re.search(r"\b(esp\.latino|latino|lat|dual\.latino)\b", text_to_search):
        language = "LATINO"
    elif re.search(r"\b(castellano|esp|spanish)\b", text_to_search):
        language = "ESP"
    elif re.search(r"\b(eng\.and\.esp|dual|multi)\b", text_to_search):
        language = "DUAL/MULTI"
    elif re.search(r"\b(subbed|sub|softsub)\b", text_to_search):
        language = "SUB"

    # Detección de resolución
    quality = "Unknown"
    if "2160p" in text_to_search or "4k" in text_to_search:
        quality = "2160p"
    elif "1080p" in text_to_search:
        quality = "1080p"
    elif "720p" in text_to_search:
        quality = "720p"

    return {"language": language, "quality": quality}


@contextmanager
def get_db_connection():
    """Garantiza la apertura, configuración y cierre seguro de la conexión a la BD."""
    conn = sqlite3.connect(DB_PATH, timeout=30.0)
    conn.execute("PRAGMA journal_mode = WAL;")
    conn.execute("PRAGMA foreign_keys = ON;")
    conn.row_factory = sqlite3.Row
    try:
        yield conn
    finally:
        conn.close()


def _slugify(text: str) -> str:
    """Genera un slug limpio eliminando caracteres especiales."""
    text = text.lower().strip()
    return re.sub(r"[^\w\s-]", "", text).replace(" ", "-")


def init_db():
    """Inicializa las tablas e índices de la base de datos."""
    with get_db_connection() as conn:
        with conn:
            conn.executescript("""
                CREATE TABLE IF NOT EXISTS media (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    title TEXT NOT NULL,
                    type TEXT CHECK(type IN ('movie', 'series', 'anime')) NOT NULL,
                    url TEXT UNIQUE NOT NULL,
                    poster TEXT,
                    description TEXT,
                    release_year INTEGER,
                    rating REAL,
                    trailer TEXT,
                    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                );

                CREATE TABLE IF NOT EXISTS genres (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    name TEXT UNIQUE NOT NULL,
                    slug TEXT UNIQUE NOT NULL
                );

                CREATE TABLE IF NOT EXISTS media_genres (
                    media_id INTEGER,
                    genre_id INTEGER,
                    PRIMARY KEY (media_id, genre_id),
                    FOREIGN KEY (media_id) REFERENCES media (id) ON DELETE CASCADE,
                    FOREIGN KEY (genre_id) REFERENCES genres (id) ON DELETE CASCADE
                );

                CREATE TABLE IF NOT EXISTS streams (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    media_id INTEGER NOT NULL,
                    server TEXT NOT NULL,
                    url TEXT NOT NULL,
                    language TEXT DEFAULT 'UNKNOWN',
                    quality TEXT DEFAULT '1080p',
                    UNIQUE(media_id, url),
                    FOREIGN KEY (media_id) REFERENCES media (id) ON DELETE CASCADE
                );

                CREATE TABLE IF NOT EXISTS episodes (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    media_id INTEGER NOT NULL,
                    season INTEGER DEFAULT 1,
                    episode_number INTEGER NOT NULL,
                    title TEXT,
                    stream_url TEXT,
                    FOREIGN KEY (media_id) REFERENCES media (id) ON DELETE CASCADE
                );

                CREATE INDEX IF NOT EXISTS idx_media_type ON media(type);
                CREATE INDEX IF NOT EXISTS idx_episodes_media ON episodes(media_id);
                CREATE INDEX IF NOT EXISTS idx_streams_media ON streams(media_id);
            """)


def save_media_item(
    title: str,
    media_type: str,
    url: str,
    poster: str = None,
    description: str = None,
    release_year: int = None,
    rating: float = None,
    trailer: str = None,
    genres: list[str] = None,
    streams: list[dict] = None
) -> int:
    """Inserta o actualiza un elemento multimedia general."""
    if not url or not title:
        raise ValueError("Se requieren al menos 'title' y 'url' para guardar el contenido.")

    with get_db_connection() as conn:
        with conn:
            cursor = conn.cursor()
            
            cursor.execute("""
                INSERT INTO media (title, type, url, poster, description, release_year, rating, trailer)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(url) DO UPDATE SET
                    title = excluded.title,
                    type = excluded.type,
                    poster = COALESCE(excluded.poster, media.poster),
                    description = COALESCE(excluded.description, media.description),
                    release_year = COALESCE(excluded.release_year, media.release_year),
                    rating = COALESCE(excluded.rating, media.rating),
                    trailer = COALESCE(excluded.trailer, media.trailer)
                RETURNING id;
            """, (title, media_type, url, poster, description, release_year, rating, trailer))
            
            media_id = cursor.fetchone()["id"]

            if genres:
                for genre_name in genres:
                    slug = _slugify(genre_name)
                    cursor.execute("""
                        INSERT INTO genres (name, slug) VALUES (?, ?)
                        ON CONFLICT(slug) DO UPDATE SET name = excluded.name;
                    """, (genre_name, slug))
                    
                    cursor.execute("""
                        INSERT INTO media_genres (media_id, genre_id)
                        SELECT ?, id FROM genres WHERE slug = ?
                        ON CONFLICT DO NOTHING;
                    """, (media_id, slug))

            if streams:
                stream_data = []
                for s in streams:
                    if isinstance(s, dict) and s.get("url"):
                        stream_url = s["url"]
                        server = s.get("server", "Unknown")
                        
                        metadata = parse_magnet_metadata(stream_url, server)
                        language = s.get("language") or metadata["language"]
                        quality = s.get("quality") or metadata["quality"]

                        stream_data.append((media_id, server, stream_url, language, quality))

                if stream_data:
                    cursor.executemany("""
                        INSERT INTO streams (media_id, server, url, language, quality)
                        VALUES (?, ?, ?, ?, ?)
                        ON CONFLICT(media_id, url) DO UPDATE SET
                            language = excluded.language,
                            quality = excluded.quality;
                    """, stream_data)

            return media_id


def save_movie(
    movie_or_title=None,
    url: str = None,
    poster: str = None,
    description: str = None,
    release_year: int = None,
    rating: float = None,
    trailer: str = None,
    genres: list[str] = None,
    streams: list[dict] = None,
    **kwargs
) -> int:
    """Acepta un dict o argumentos por nombre para guardar películas."""
    if isinstance(movie_or_title, dict):
        d = movie_or_title
        return save_media_item(
            title=d.get("title"),
            media_type="movie",
            url=d.get("url"),
            poster=d.get("poster"),
            description=d.get("description"),
            release_year=d.get("release_year") or d.get("year"),
            rating=d.get("rating"),
            trailer=d.get("trailer"),
            genres=d.get("genres"),
            streams=d.get("streams")
        )

    return save_media_item(
        title=movie_or_title,
        media_type="movie",
        url=url,
        poster=poster,
        description=description,
        release_year=release_year,
        rating=rating,
        trailer=trailer,
        genres=genres,
        streams=streams
    )


def get_saved_movies(limit: int = 50, offset: int = 0) -> list[dict]:
    """Obtiene la lista de películas guardadas con sus géneros y reproductores."""
    with get_db_connection() as conn:
        cursor = conn.cursor()
        
        cursor.execute("""
            SELECT id, title, type, url, poster, description, release_year, release_year AS year, rating, trailer, created_at
            FROM media
            WHERE type = 'movie'
            ORDER BY created_at DESC
            LIMIT ? OFFSET ?;
        """, (limit, offset))
        
        rows = cursor.fetchall()
        movies = []

        for row in rows:
            movie = dict(row)
            
            cursor.execute("""
                SELECT g.name FROM genres g
                JOIN media_genres mg ON g.id = mg.genre_id
                WHERE mg.media_id = ?;
            """, (movie["id"],))
            movie["genres"] = [g["name"] for g in cursor.fetchall()]

            cursor.execute("""
                SELECT server, url, language, quality FROM streams
                WHERE media_id = ?;
            """, (movie["id"],))
            movie["streams"] = [dict(s) for s in cursor.fetchall()]

            movies.append(movie)

        return movies


def get_streams_by_language(media_id_or_title: str | int, target_lang: str = None) -> list[dict]:
    """
    Obtiene los reproductores de un contenido filtrados o prioritizados por idioma.
    Acepta el ID numérico o el título de la película/serie.
    """
    with get_db_connection() as conn:
        cursor = conn.cursor()
        
        if isinstance(media_id_or_title, int) or str(media_id_or_title).isdigit():
            where_clause = "m.id = ?"
            param = int(media_id_or_title)
        else:
            where_clause = "m.title LIKE ?"
            param = f"%{media_id_or_title}%"

        query = f"""
            SELECT s.server, s.url, s.language, s.quality, m.title
            FROM streams s
            JOIN media m ON s.media_id = m.id
            WHERE {where_clause}
        """
        params = [param]

        if target_lang:
            query += " AND UPPER(s.language) = UPPER(?)"
            params.append(target_lang)

        query += """
            ORDER BY 
                CASE UPPER(s.language)
                    WHEN 'LATINO' THEN 1
                    WHEN 'DUAL/MULTI' THEN 2
                    WHEN 'ESP' THEN 3
                    WHEN 'SUB' THEN 4
                    ELSE 5
                END, quality DESC;
        """

        cursor.execute(query, params)
        return [dict(row) for row in cursor.fetchall()]


def clear_db():
    """Limpia todos los registros de la base de datos conservando las tablas."""
    with get_db_connection() as conn:
        with conn:
            cursor = conn.cursor()
            cursor.execute("DELETE FROM media_genres;")
            cursor.execute("DELETE FROM streams;")
            cursor.execute("DELETE FROM episodes;")
            cursor.execute("DELETE FROM genres;")
            cursor.execute("DELETE FROM media;")
        
        old_isolation = conn.isolation_level
        try:
            conn.isolation_level = None
            conn.execute("VACUUM;")
        finally:
            conn.isolation_level = old_isolation