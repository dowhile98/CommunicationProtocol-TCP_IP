"""
coap_client.py  —  Cliente CoAP interactivo para demostración en clase
=====================================================================
Requiere:  pip install aiocoap

Recursos disponibles en el servidor ESP32:
  GET  /.well-known/core   -> Descubrimiento de recursos
  GET  /test               -> Texto plano "Hello World!"
  GET  /info               -> JSON con uptime, heap y chip
  GET  /led                -> JSON estado del LED simulado
  PUT  /led                -> Modifica LED  {"state": "on" | "off"}
  GET  /counter            -> JSON contador (auto-incrementa en cada GET)
  POST /counter/reset      -> Reinicia contador a 0
  DELETE /counter          -> Reinicia contador a 0 (semantica REST)
  POST /echo               -> Devuelve el mismo payload que envias
"""

import asyncio
import json
import logging
import sys

from aiocoap import Context, Message
from aiocoap import GET, PUT, POST, DELETE
from aiocoap.numbers.contentformat import ContentFormat

# Configuracion ----------------------------------------------------------------
SERVER_IP   = "192.168.1.36"
SERVER_PORT = 5683
BASE_URI    = f"coap://{SERVER_IP}:{SERVER_PORT}"

# Silenciar logs de aiocoap para que la salida sea mas limpia.
# Cambiar a logging.DEBUG para ver el trafico interno.
logging.basicConfig(level=logging.WARNING)

# Colores ANSI -----------------------------------------------------------------
RESET  = "\033[0m"
BOLD   = "\033[1m"
CYAN   = "\033[96m"
GREEN  = "\033[92m"
YELLOW = "\033[93m"
RED    = "\033[91m"
GRAY   = "\033[90m"
BLUE   = "\033[94m"


# Helpers ----------------------------------------------------------------------

def banner():
    print(f"""
{CYAN}{BOLD}+====================================================+
|       CLIENTE CoAP  -  Demo ESP32 + CycloneTCP    |
|  Servidor: {SERVER_IP}:{SERVER_PORT:<5}                         |
+====================================================+{RESET}
""")


def menu():
    print(f"{BOLD}----  MENU DE OPERACIONES  ----{RESET}")
    ops = [
        ("1", "GET",    "/.well-known/core", "Descubrimiento de recursos"),
        ("2", "GET",    "/test",             "Hello World! (texto plano)"),
        ("3", "GET",    "/info",             "Info del dispositivo (JSON)"),
        ("4", "GET",    "/led",              "Leer estado del LED simulado"),
        ("5", "PUT",    "/led",              "Cambiar estado del LED  {state: on|off}"),
        ("6", "GET",    "/counter",          "Leer contador (auto-incrementa)"),
        ("7", "POST",   "/counter/reset",    "Reiniciar contador"),
        ("8", "DELETE", "/counter",          "Borrar (reiniciar) contador"),
        ("9", "POST",   "/echo",             "Echo  -  envia un mensaje y lo recibe"),
        ("0", " -- ",   " -- ",              "Salir"),
    ]
    for key, method, path, desc in ops:
        method_color = {
            "GET": GREEN, "PUT": YELLOW, "POST": BLUE, "DELETE": RED, " -- ": GRAY
        }.get(method, RESET)
        print(f"  {BOLD}{key}{RESET}  {method_color}{method:<6}{RESET}  "
              f"{CYAN}{path:<22}{RESET}  {GRAY}{desc}{RESET}")
    print()


def pretty_payload(raw: bytes) -> str:
    """Intenta formatear JSON; si falla devuelve el texto crudo."""
    text = raw.decode("utf-8", errors="replace")
    try:
        parsed = json.loads(text)
        return json.dumps(parsed, indent=2, ensure_ascii=False)
    except json.JSONDecodeError:
        return text


def print_response(response, sent_uri: str):
    code_str = str(response.code)
    ok = response.code.is_successful()
    color = GREEN if ok else RED
    print(f"\n{BOLD}<  Respuesta{RESET}")
    print(f"   URI     : {CYAN}{sent_uri}{RESET}")
    print(f"   Codigo  : {color}{code_str}{RESET}")
    if response.payload:
        print(f"   Payload :")
        for line in pretty_payload(response.payload).splitlines():
            print(f"     {line}")
    else:
        print(f"   Payload : {GRAY}(vacio){RESET}")
    print()


async def send_request(protocol: Context, method, uri: str,
                       payload: bytes = b"",
                       content_format=None) -> None:
    """Construye, envia y muestra un request CoAP."""
    msg = Message(code=method, uri=uri)
    if payload:
        msg.payload = payload
    if content_format is not None:
        msg.opt.content_format = content_format

    # Mostrar lo que se envia
    method_name = {GET: "GET", PUT: "PUT", POST: "POST", DELETE: "DELETE"}.get(method, "?")
    print(f"\n{BOLD}>  Enviando{RESET}")
    print(f"   Metodo  : {YELLOW}{method_name}{RESET}")
    print(f"   URI     : {CYAN}{uri}{RESET}")
    if payload:
        print(f"   Payload : {payload.decode('utf-8', errors='replace')}")

    try:
        response = await protocol.request(msg).response
        print_response(response, uri)
    except Exception as exc:
        print(f"{RED}X  Error de red: {exc}{RESET}\n")


# Operaciones ------------------------------------------------------------------

async def op_discover(proto):
    await send_request(proto, GET, f"{BASE_URI}/.well-known/core")

async def op_test(proto):
    await send_request(proto, GET, f"{BASE_URI}/test")

async def op_info(proto):
    await send_request(proto, GET, f"{BASE_URI}/info")

async def op_led_get(proto):
    await send_request(proto, GET, f"{BASE_URI}/led")

async def op_led_put(proto):
    raw = input(f"  Estado del LED [{GREEN}on{RESET}/{RED}off{RESET}]: ").strip().lower()
    if raw not in ("on", "off"):
        print(f"{RED}  Valor invalido. Usa 'on' u 'off'.{RESET}")
        return
    body = json.dumps({"state": raw}).encode()
    await send_request(proto, PUT, f"{BASE_URI}/led",
                       payload=body,
                       content_format=ContentFormat.JSON)

async def op_counter_get(proto):
    await send_request(proto, GET, f"{BASE_URI}/counter")

async def op_counter_reset(proto):
    await send_request(proto, POST, f"{BASE_URI}/counter/reset")

async def op_counter_delete(proto):
    await send_request(proto, DELETE, f"{BASE_URI}/counter")

async def op_echo(proto):
    msg = input("  Escribe el mensaje a enviar: ").strip()
    if not msg:
        print(f"{GRAY}  (mensaje vacio, cancelado){RESET}")
        return
    body = msg.encode()
    await send_request(proto, POST, f"{BASE_URI}/echo",
                       payload=body,
                       content_format=ContentFormat.TEXT)


OPERATIONS = {
    "1": op_discover,
    "2": op_test,
    "3": op_info,
    "4": op_led_get,
    "5": op_led_put,
    "6": op_counter_get,
    "7": op_counter_reset,
    "8": op_counter_delete,
    "9": op_echo,
}


# Bucle principal --------------------------------------------------------------

async def main():
    banner()

    print(f"{YELLOW}Conectando al servidor CoAP...{RESET}")
    try:
        protocol = await Context.create_client_context()
    except Exception as exc:
        print(f"{RED}No se pudo crear el contexto CoAP: {exc}{RESET}")
        sys.exit(1)

    print(f"{GREEN}Contexto listo.{RESET}  Escribe el numero de la operacion.\n")

    while True:
        menu()
        try:
            choice = input(f"  {BOLD}Opcion > {RESET}").strip()
        except (EOFError, KeyboardInterrupt):
            choice = "0"

        if choice == "0":
            print(f"\n{CYAN}Hasta luego!{RESET}\n")
            break

        op = OPERATIONS.get(choice)
        if op is None:
            print(f"{RED}  Opcion desconocida. Selecciona del 0 al 9.{RESET}\n")
            continue

        await op(protocol)

        # Pausa para que el alumno pueda leer la respuesta
        try:
            input(f"{GRAY}  [ENTER para continuar]{RESET}")
        except (EOFError, KeyboardInterrupt):
            break


if __name__ == "__main__":
    asyncio.run(main())