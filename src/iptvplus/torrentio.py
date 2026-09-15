import requests

TORRENTIO_MANIFEST_URL = (
    "https://torrentio.strem.fun/"
    "providers=mejortorrent,wolfmax4k,cinecalidad,nyaasi,yts|"
    "sort=qualitysize|"
    "language=spanish,latino,korean,japanese|"
    "qualityfilter=unknown,cam,scr"
)

HEADERS = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/139.0 Safari/537.36"
}


def get_torrentio_streams(imdb_id: int) -> list[dict]:
    streams = []

    if not imdb_id:
        return streams

    endpoint = f"{TORRENTIO_MANIFEST_URL}/stream/movie/{imdb_id}.json"

    try:
        resp = requests.get(endpoint, headers=HEADERS, timeout=15)
        # Testing
        """print("Torrentio endpoint:", endpoint)
        print("Status:", resp.status_code)
        print("Response:", resp.text[:5000])
        """
        if resp.status_code == 200:
            data = resp.json()

            for item in data.get("streams", []):
                print("STREAM:", item)

                provider = item.get("name", "Torrentio")
                title_details = item.get("title", "")

                stream_url = item.get("url")

                if not stream_url and item.get("infoHash"):
                    stream_url = (
                        f"magnet:?xt=urn:btih:{item['infoHash']}"
                    )

                if stream_url:
                    server_label = (
                        f"{provider} - {title_details.splitlines()[0]}"
                        if title_details
                        else provider
                    )

                    streams.append({
                        "server": server_label,
                        "url": stream_url
                    })

    except Exception as e:
        print(f"Error consultando Torrentio para TMDB ID {tmdb_id}: {e}")

    return streams