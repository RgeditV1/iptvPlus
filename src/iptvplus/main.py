import argparse
from email import parser
from pathlib import Path

# __init__.py import
from . import *


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
                # Extrae los detalles específicos de cada película
                details = get_movie_details(item["url"])

                save_media_item(
                    title=details.get("title") or item["title"],
                    media_type="movie",
                    url=item["url"],
                    poster=details.get("poster") or item["poster"],
                    description=details.get("description"),
                    rating=details.get("rating"),
                    genres=details.get("genres")
                )
                print(f" - Procesada: {details.get('title') or item['title']}")

            print(f"\nSe guardaron/actualizaron {len(results)} elementos completos en database.db")

    except Exception as error:
        print(f"Error procesando la solicitud: {error}")

if __name__ == "__main__":
    main()