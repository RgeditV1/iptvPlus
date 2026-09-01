import requests
from bs4 import BeautifulSoup
from urllib.parse import urljoin


BASE_URL = "https://www.cinecalidad.am"


def search_movies(query, limit=10):
    """
    Busca películas en Cinecalidad y devuelve como máximo `limit` resultados.
    """

    response = requests.get(
        BASE_URL,
        params={"s": query},
        timeout=15,
        headers={
            "User-Agent": (
                "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                "AppleWebKit/537.36 (KHTML, like Gecko) "
                "Chrome/139.0 Safari/537.36"
            )
        },
    )

    response.raise_for_status()

    soup = BeautifulSoup(response.text, "html.parser")

    results = []

    for article in soup.select("article.item.movies"):

        # Ignorar publicidad
        if "naadb" in article.get("class", []):
            continue

        # Buscar el indicador de tipo
        type_element = article.select_one(".selt")

        if type_element:
            content_type = type_element.get_text(strip=True).lower()

            # Ignorar series
            if "serie" in content_type:
                continue

        # Título
        title_element = article.select_one(".in_title")

        if not title_element:
            continue

        title = title_element.get_text(strip=True)

        # Enlace
        link_element = article.select_one("a[href]")

        if not link_element:
            continue

        movie_url = urljoin(BASE_URL, link_element.get("href"))

        # Portada
        image_element = article.select_one("img")

        if not image_element:
            continue

        poster = (
            image_element.get("data-src")
            or image_element.get("data-lazy-src")
            or image_element.get("src")
        )

        if not poster:
            continue

        poster = urljoin(BASE_URL, poster)

        results.append({
            "title": title,
            "poster": poster,
            "url": movie_url,
        })

        # Limitar después de filtrar
        if len(results) >= limit:
            break

    return results