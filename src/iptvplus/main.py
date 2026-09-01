import argparse
import json

from iptvplus.scraper import search_movies
from pathlib import Path
#from scraper import search_movies


def main():
    parser = argparse.ArgumentParser(
        description="Buscador de películas de Cinecalidad"
    )

    parser.add_argument(
        "--search",
        required=True,
        metavar="BUSCAR",
        help="Nombre de la película que quieres buscar"
    )

    parser.add_argument(
        "--limit",
        type=int,
        default=10,
        metavar="LIMITE",
        help="Número máximo de resultados a mostrar (por defecto: 10)"
    )

    args = parser.parse_args()
    path = Path(__file__).parent / "results.json"

    try:
        results = search_movies(args.search, limit=args.limit)

        output = {
            "results": results
        }

        with open(
            path,
            "w",
            encoding="utf-8"
        ) as file:
            json.dump(
                output,
                file,
                ensure_ascii=False,
                indent=4
            )

    except Exception as error:
        output = {
            "results": [],
            "error": str(error)
        }

        with open(
            path,
            "w",
            encoding="utf-8"
        ) as file:
            json.dump(
                output,
                file,
                ensure_ascii=False,
                indent=4
            )


if __name__ == "__main__":
    main()