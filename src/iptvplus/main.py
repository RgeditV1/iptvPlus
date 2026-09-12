import argparse
import sys
import io

from pathlib import Path

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')

if getattr(sys, 'frozen', False):
    # Ejecutando desde el ejecutable de PyInstaller
    BASE_DIR = Path(sys._MEIPASS)
else:
    BASE_DIR = Path(__file__).resolve().parent

# Añadir el directorio al PATH de búsqueda de módulos de Python
if str(BASE_DIR) not in sys.path:
    sys.path.insert(0, str(BASE_DIR))

from scraper import search_movies, get_by_genre, get_movie_details, get_genres
from db import init_db, clear_db, save_movie, get_saved_movies, get_streams_by_language


def main():
    parser = argparse.ArgumentParser(
        description="CLI Tool para buscar y guardar información de películas."
    )
    
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "-s", "--search",
        type=str,
        help="Busca películas por título."
    )
    group.add_argument(
        "-g", "--genre",
        type=str,
        help="Filtra películas por género (ej. Action, Comedy, Horror)."
    )
    group.add_argument(
        "--get-genres",
        action="store_true",
        help="Lista todos los géneros disponibles."
    )
    group.add_argument(
        "--list-saved",
        action="store_true",
        help="Muestra las películas guardadas en la base de datos."
    )
    group.add_argument(
        "--clear-db",
        action="store_true",
        help="Limpia la base de datos de películas guardadas."
    )

    parser.add_argument(
        "--get-streams", 
        type=str, 
        metavar="ID_O_TITULO", 
        help="Obtiene los enlaces/magnets de una película por su ID o Título de la DB."
    )
    parser.add_argument(
        "--lang", 
        type=str, 
        choices=["LATINO", "ESP", "ENG", "DUAL/MULTI", "SUB"], 
        help="Filtra los enlaces por idioma (usar junto a --get-streams)."
    )
    parser.add_argument(
        "-l", "--limit",
        type=int,
        default=10,
        help="Número máximo de resultados a mostrar (por defecto 10)."
    )
    parser.add_argument(
        "--save",
        action="store_true",
        help="Guarda automáticamente en SQLite los resultados encontrados."
    )

    args = parser.parse_args()

    # Inicializar base de datos
    init_db()

    # Opción: Limpiar la base de datos
    if args.clear_db:
        clear_db()
        print("\nBase de datos limpiada con éxito.")
        return

    # Opción: Consultar streams/magnets por ID o Título desde la DB
    if args.get_streams:
        streams = get_streams_by_language(args.get_streams, target_lang=args.lang)
        if not streams:
            lang_str = f" en idioma [{args.lang}]" if args.lang else ""
            print(f"\nNo se encontraron enlaces para '{args.get_streams}'{lang_str}.")
            return

        print(f"\n--- Fuentes encontradas para '{args.get_streams}' ({len(streams)}) ---")
        for st in streams:
            print(f"[{st['language']}] [{st['quality']}] {st['server']}: {st['url']}")
        return

    # Opción: Listar géneros
    if args.get_genres:
        genres = get_genres()
        print("\n--- Géneros Disponibles ---")
        for g in genres:
            print(f"- {g['name']} (slug/ID: {g['slug']})")
        return

    # Opción: Listar películas guardadas en la DB
    if args.list_saved:
        movies = get_saved_movies()
        if not movies:
            print("\nNo hay películas guardadas en la base de datos.")
            return
        
        print(f"\n--- Películas Guardadas en DB ({len(movies)}) ---")
        for m in movies:
            print(f"\n[ID: {m['id']}] {m['title']} ({m.get('year', 'N/A')})")
            print(f"  Rating: {m.get('rating', 'N/A')}")
            print(f"  Poster: {m.get('poster', 'N/A')}")
            print(f"  URL: {m.get('url', 'N/A')}")
            if m.get("streams"):
                print("  Fuentes / Torrents:")
                for st in m["streams"]:
                    lang = st.get('language', 'UNK')
                    qual = st.get('quality', '1080p')
                    print(f"    - [{lang}] [{qual}] [{st.get('server')}]: {st.get('url')}")
        return

    # Opción: Búsqueda o filtrado por género desde Scraper
    movies = []
    if args.search:
        print(f"\nBuscando '{args.search}'...")
        movies = search_movies(args.search, limit=args.limit)
    elif args.genre:
        print(f"\nObteniendo películas del género '{args.genre}'...")
        movies = get_by_genre(args.genre, limit=args.limit)
    else:
        parser.print_help()
        sys.exit(0)

    if not movies:
        print("No se encontraron resultados.")
        return

    print(f"\nSe encontraron {len(movies)} resultados:\n")

    for idx, movie in enumerate(movies, 1):
        print(f"{idx}. {movie['title']} ({movie.get('year', 'N/A')})")
        print(f"   TMDB ID: {movie.get('id')}")
        print(f"   Rating: {movie.get('rating', 'N/A')}")
        print(f"   Poster: {movie.get('poster', 'N/A')}")
        
        details = get_movie_details(
            title=movie["title"],
            tmdb_id=movie.get("id"),
            year=movie.get("year")
        )
        
        movie["streams"] = details.get("streams", [])

        if args.save:
            save_movie(movie)
            print("   -> Guardada en DB.")

        print("-" * 50)


if __name__ == "__main__":
    main()