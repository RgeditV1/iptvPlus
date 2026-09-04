import os
import sys
import sqlite3
from pathlib import Path


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

def get_connection():
    conn = sqlite3.connect(DB_PATH, timeout=30.0)
    conn.execute("PRAGMA journal_mode = WAL;")
    conn.execute("PRAGMA foreign_keys = ON;")
    conn.row_factory = sqlite3.Row
    return conn


def init_db():
    with get_connection() as conn:
        cursor = conn.cursor()

        cursor.execute("""
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
        """)

        cursor.execute("""
            CREATE TABLE IF NOT EXISTS genres (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT UNIQUE NOT NULL,
                slug TEXT UNIQUE NOT NULL
            );
        """)

        cursor.execute("""
            CREATE TABLE IF NOT EXISTS media_genres (
                media_id INTEGER,
                genre_id INTEGER,
                PRIMARY KEY (media_id, genre_id),
                FOREIGN KEY (media_id) REFERENCES media (id) ON DELETE CASCADE,
                FOREIGN KEY (genre_id) REFERENCES genres (id) ON DELETE CASCADE
            );
        """)

        cursor.execute("""
            CREATE TABLE IF NOT EXISTS streams (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                media_id INTEGER NOT NULL,
                server TEXT NOT NULL,
                url TEXT NOT NULL,
                UNIQUE(media_id, url),
                FOREIGN KEY (media_id) REFERENCES media (id) ON DELETE CASCADE
            );
        """)

        cursor.execute("""
            CREATE TABLE IF NOT EXISTS episodes (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                media_id INTEGER NOT NULL,
                season INTEGER DEFAULT 1,
                episode_number INTEGER NOT NULL,
                title TEXT,
                stream_url TEXT,
                FOREIGN KEY (media_id) REFERENCES media (id) ON DELETE CASCADE
            );
        """)

        conn.commit()


def save_media_item(
    title: str,
    media_type: str,
    url: str,
    poster: str = None,
    description: str = None,
    rating: float = None,
    trailer: str = None,
    genres: list = None,
    streams: list[dict] = None
) -> int:
    """Inserta o actualiza un elemento multimedia, su rating, tráiler, géneros y reproductores."""
    with get_connection() as conn:
        cursor = conn.cursor()
        
        cursor.execute("SELECT id FROM media WHERE url = ?", (url,))
        existing = cursor.fetchone()

        if existing:
            media_id = existing["id"]
            cursor.execute("""
                UPDATE media SET
                    title = ?,
                    type = ?,
                    poster = COALESCE(?, poster),
                    description = COALESCE(?, description),
                    rating = COALESCE(?, rating),
                    trailer = COALESCE(?, trailer)
                WHERE id = ?;
            """, (title, media_type, poster, description, rating, trailer, media_id))
        else:
            cursor.execute("""
                INSERT INTO media (title, type, url, poster, description, rating, trailer)
                VALUES (?, ?, ?, ?, ?, ?, ?);
            """, (title, media_type, url, poster, description, rating, trailer))
            media_id = cursor.lastrowid

        if genres:
            for genre_name in genres:
                slug = genre_name.lower().replace(" ", "-")
                cursor.execute("""
                    INSERT INTO genres (name, slug) VALUES (?, ?)
                    ON CONFLICT(slug) DO NOTHING;
                """, (genre_name, slug))
                
                cursor.execute("SELECT id FROM genres WHERE slug = ?", (slug,))
                genre_row = cursor.fetchone()
                if genre_row:
                    genre_id = genre_row["id"]
                    cursor.execute("""
                        INSERT INTO media_genres (media_id, genre_id)
                        VALUES (?, ?)
                        ON CONFLICT DO NOTHING;
                    """, (media_id, genre_id))

        if streams:
            for stream in streams:
                server = stream.get("server", "Unknown")
                stream_url = stream.get("url")
                if stream_url:
                    cursor.execute("""
                        INSERT INTO streams (media_id, server, url)
                        VALUES (?, ?, ?)
                        ON CONFLICT(media_id, url) DO NOTHING;
                    """, (media_id, server, stream_url))

        conn.commit()
        return media_id