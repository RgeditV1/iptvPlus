# Lo Nuevo!

## [0.2.2]

# Añadido

- filtrado por paises
- Texto Live en modo TV

## Cambio

- Rework Scrap.exe, ahora utiliza imdb, tmdb y torrentio para conseguir los detalles de las peliculas
- Se le agrego un marco a los posters en `Movie Detail`
- Se cambio el modo de descarga de torrents, ahora se monta un servidor http para las url de las peliculas

## Corregido

- Antes el `Movie Detail` estaba desorganizado, ahora estan alineados a la izquierda
- Antes el Timeline no funcionaba, ahora el timeline y los labels de timepo funcionan correctamente

## [0.2.1] -> HOTFIX

### Correcciones de Errores (Fixes)

- **CPack / Instalador:** Se corrigió la inclusión de `scrap.exe` en el instalador NSIS usando la directiva `install(PROGRAMS ...)` opcional y se configuró la generación automática del acceso directo en el escritorio.
- **Interfaz (Z-Index / Renderizado):** Se solucionó el problema por el cual la barra lateral de canales quedaba superpuesta por debajo de los controles del reproductor al salir del modo pantalla completa.
- **Reproductor:** Se añadió la limpieza automática de las opciones de los menús de audio y subtítulos al detener la reproducción (`stop()`) o al presentarse un error de red.
- **Reproductor:** Se implementaron grupos de acciones exclusivas (`QActionGroup`) en las pistas de audio y subtítulos para evitar la selección múltiple simultánea de opciones.

## [0.2]

### Cambio

- Rework de la GUI completa y reestructuracion del programa
- seccion de pelicula y busqueda añadida (Experimental)
- Titulo de la ventana principal

### Añadido
- Icono de Aplicacion
- Selector de Magnets (Experimental)
- Selector de Pista de Audio y Subtitulos (Experimental)

## [0.1.2]

### Añadido
- Boton de Pantalla Completa (Full Screen)
- Spinner de Carga para el reproductor

### Cambio
- color oscuro al widget principal en ``mainwindow``
- texto de boton stop a icono

### Revertido
- Icono antiguo de Menu

## [0.1.1]

### Añadido
- Transparencia a los botones de Reproduccion y Menu
- Nuevo Icono de Menu
- Nuevo Icono Play-Circle

### Corregido
- Centrado de botones de reproduccion
- Eliminar contorno de boton de volumen
- Ajuste de botones en la barra de control de reproduccion

## [0.1]

### Añadido
- Soporte para URLs .m3u: Ahora es posible cargar y reproducir listas de reproducción remotas directamente a través de enlaces HTTP/HTTPS.
- Lista de canales propia: Se ha integrado una vista de lista de canales independiente para mejorar la navegación y organización del contenido dentro de la aplicación.
- Nuevos iconos para los botones de Reproducción
