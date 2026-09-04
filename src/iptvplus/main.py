import sys
import io
import argparse
from pathlib import Path

# Evita que Windows falle al imprimir caracteres
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')

package_dir = Path(__file__).resolve().parent.parent
if str(package_dir) not in sys.path:
    sys.path.insert(0, str(package_dir))

# __init__.py import
from iptvplus import (
    init_db,
    save_media_item,
    get_movie_details,
    get_genres,
    get_by_genre,
    search_movies,
)


def main():
    
    init_db()
    
    parser = argparse.ArgumentParser(
        description="Buscador y explorador de películas de Cinecalidad"
    )

    parser.add_argument(
        "-s",
        "--search",
        type=str,
        metavar="BUSCAR",
        help="Nombre de la película que quieres buscar",
    )

    parser.add_argument(
        "-l",
        "--limit",
        type=int,
        default=10,
        metavar="LIMITE",
        help="Número máximo de resultados a mostrar",
    )

    parser.add_argument(
        "--get-genres",
        action="store_true",
        help="Obtener la lista de géneros disponibles",
    )

    parser.add_argument(
        "--genre",
        type=str,
        metavar="GENERO",
        help="Filtrar películas por género",
    )

    args = parser.parse_args()

    if not any([args.search, args.get_genres, args.genre]):
        parser.print_help()
        return

    try:
        if args.get_genres:
            results = get_genres()
            print(f"Géneros disponibles ({len(results)}):")
            for g in results:
                print(f" - {g['name']} ({g['slug']})")
        else:
            if args.genre:
                results = get_by_genre(args.genre, limit=args.limit)
            elif args.search:
                results = search_movies(args.search, limit=args.limit)

            print(f"Obteniendo detalles de {len(results)} películas...")
            
            for item in results:
                details = get_movie_details(item["url"])

                save_media_item(
                    title=details.get("title") or item["title"],
                    media_type="movie",
                    url=item["url"],
                    poster=details.get("poster") or item["poster"],
                    description=details.get("description"),
                    rating=details.get("rating"),
                    genres=details.get("genres"),
                    streams=details.get("streams"),
                    trailer=details.get("trailer")
                )
                
                print(f" - Procesada: {details.get('title') or item['title']}")
                if details.get("trailer"):
                    print(f"    └ Tráiler: {details['trailer']}")
                if details.get("streams"):
                    for s in details["streams"]:
                        print(f"    └ Reproductor ({s['server']}): {s['url']}")

    except Exception as error:
        print(f"Error procesando la solicitud: {error}")

if __name__ == "__main__":
    main()