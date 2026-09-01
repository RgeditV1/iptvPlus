import argparse
import json
from pathlib import Path

# __init__.py import
from . import *


def main():
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
    path = Path(__file__).parent / "results.json"

    if not any([args.search, args.get_genres, args.genre]):
        parser.print_help()
        return

    output = {}

    try:
        if args.get_genres:
            results = get_genres()
        elif args.genre:
            results = get_by_genre(args.genre, limit=args.limit)
        elif args.search:
            results = search_movies(args.search, limit=args.limit)

        output["results"] = results

    except Exception as error:
        output = {"results": [], "error": str(error)}

    # Escritura centralizada del JSON
    with open(path, "w", encoding="utf-8") as file:
        json.dump(output, file, ensure_ascii=False, indent=4)


if __name__ == "__main__":
    main()