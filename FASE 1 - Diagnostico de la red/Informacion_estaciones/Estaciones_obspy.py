from pathlib import Path
import pandas as pd
from obspy.clients.fdsn import Client
from obspy import Inventory

# 1. Configuración de rutas y límites geográficos (Pacífico Colombiano)
CARPETA_SALIDA = r"c:\Users\ASUS\OneDrive\Documentos\FASE1"
MIN_LAT, MAX_LAT = 0.0, 9.0
MIN_LON, MAX_LON = -80.0, -76.0

COLUMNAS_ESTACIONES = [
    "red",
    "codigo_estacion",
    "nombre_estacion",
    "departamento",
    "municipio",
    "latitud",
    "longitud",
    "elevacion_m",
    "tipo_sensor",
    "fecha_instalacion",
    "estado_operativo",
    "frecuencia_muestreo_hz",
    "resolucion",
    "ficha_o_metadatos_disponible",
    "observaciones_derecho_peticion",
]


def descargar_inventarios() -> tuple[Inventory, Inventory]:
    """Descarga estaciones de RSNC (SGC/CM) y busca redes asociadas en EarthScope."""
    inv_rsnc = Inventory(networks=[], source="SGC_RSNC")
    inv_osso = Inventory(networks=[], source="OSSO")

    client_earthscope = Client("EARTHSCOPE")

    # 2A. Intentar nodo FDSN directo del SGC (puerto 8080, SeisComP FDSNWS)
    try:
        print("Consultando nodo FDSN directo del SGC (http://sismo.sgc.gov.co:8080)...")
        client_sgc = Client("http://sismo.sgc.gov.co:8080", timeout=20)
        inv_rsnc = client_sgc.get_stations(
            minlatitude=MIN_LAT,
            maxlatitude=MAX_LAT,
            minlongitude=MIN_LON,
            maxlongitude=MAX_LON,
            level="channel",
        )
        print(f" -> Estaciones descargadas desde SGC directo: {len(inv_rsnc.get_contents()['stations'])}")
    except Exception as e:
        print(f" -> Nodo directo SGC no accesible ({e}). Usando respaldo en EarthScope (red CM)...")
        # 2B. Respaldo oficial: Red CM (Servicio Geológico Colombiano) en EarthScope
        try:
            inv_rsnc = client_earthscope.get_stations(
                network="CM",
                minlatitude=MIN_LAT,
                maxlatitude=MAX_LAT,
                minlongitude=MIN_LON,
                maxlongitude=MAX_LON,
                level="channel",
            )
            print(f" -> Estaciones RSNC (red CM) descargadas de EarthScope: {len(inv_rsnc.get_contents()['stations'])}")
        except Exception as e2:
            print(f" -> Error consultando red CM en EarthScope: {e2}")

    # 3. Consultar red OSSO (código 'OS') en EarthScope
    try:
        print("Consultando red OSSO (código OS) en EarthScope...")
        inv_osso = client_earthscope.get_stations(
            network="OS",
            minlatitude=MIN_LAT,
            maxlatitude=MAX_LAT,
            minlongitude=MIN_LON,
            maxlongitude=MAX_LON,
            level="channel",
        )
        print(f" -> Estaciones OSSO descargadas: {len(inv_osso.get_contents()['stations'])}")
    except Exception as e:
        print(f" -> Sin estaciones públicas bajo código 'OS' en FDSN (HTTP 204): {e}")
        print("    Nota: El catálogo de OSSO requiere solicitud directa o cruce con estaciones integradas en CM.")

    return inv_rsnc, inv_osso


def inventario_a_dataframe(inventario: Inventory) -> pd.DataFrame:
    """Convierte un objeto Inventory de ObsPy al esquema de columnas definido."""
    filas = []

    for net in inventario:
        for sta in net:
            sensores = sorted({
                cha.sensor.description.strip()
                for cha in sta
                if cha.sensor and cha.sensor.description
            })
            frecuencias = sorted({
                str(cha.sample_rate)
                for cha in sta
                if cha.sample_rate is not None
            })

            estado = "Cerrada / Histórica" if sta.end_date else "Activa (según FDSN)"
            fecha_inst = sta.start_date.strftime("%Y-%m-%d") if sta.start_date else ""

            filas.append({
                "red": net.code,
                "codigo_estacion": sta.code,
                "nombre_estacion": sta.site.name if sta.site else "",
                "departamento": "",
                "municipio": "",
                "latitud": round(sta.latitude, 6),
                "longitud": round(sta.longitude, 6),
                "elevacion_m": sta.elevation,
                "tipo_sensor": " | ".join(sensores) if sensores else "No especificado",
                "fecha_instalacion": fecha_inst,
                "estado_operativo": estado,
                "frecuencia_muestreo_hz": " | ".join(frecuencias),
                "resolucion": "",
                "ficha_o_metadatos_disponible": "Sí (StationXML FDSN)",
                "observaciones_derecho_peticion": "",
            })

    if not filas:
        return pd.DataFrame(columns=COLUMNAS_ESTACIONES)

    df = pd.DataFrame(filas, columns=COLUMNAS_ESTACIONES)
    df = df.drop_duplicates(subset=["red", "codigo_estacion"]).sort_values(
        by=["red", "codigo_estacion"]
    )
    return df


def generar_catalogos(carpeta_salida: str) -> None:
    """Ejecuta la descarga y guarda los archivos CSV y StationXML."""
    base = Path(carpeta_salida)
    base.mkdir(parents=True, exist_ok=True)

    inv_rsnc, inv_osso = descargar_inventarios()

    catalogos = {
        "rsnc": inv_rsnc,
        "osso": inv_osso,
    }

    for nombre_red, inventario in catalogos.items():
        ruta_csv = base / f"catalogo_estaciones_{nombre_red}.csv"
        ruta_xml = base / f"metadatos_{nombre_red}.xml"

        df = inventario_a_dataframe(inventario)
        df.to_csv(ruta_csv, index=False, encoding="utf-8-sig")
        print(f"Guardado CSV ({len(df)} estaciones): {ruta_csv}")

        if len(inventario) > 0:
            inventario.write(str(ruta_xml), format="STATIONXML")
            print(f"Guardado StationXML: {ruta_xml}")


if __name__ == "__main__":
    generar_catalogos(CARPETA_SALIDA)