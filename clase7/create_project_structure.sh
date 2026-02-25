#!/bin/bash
set -e

SRC="/home/tecna-smart-lab/Documentos/CODAERUS/PROTOCOLOS/CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4"
DST="/home/tecna-smart-lab/GitHub/CommunicationProtocol-TCP_IP/clase7/main"

echo "=== Creando estructura de directorios ==="
mkdir -p "$DST/common"
mkdir -p "$DST/cyclone_tcp/core"
mkdir -p "$DST/cyclone_tcp/drivers/wifi"
mkdir -p "$DST/cyclone_tcp/ipv4"
mkdir -p "$DST/cyclone_tcp/igmp"
mkdir -p "$DST/cyclone_tcp/nat"
mkdir -p "$DST/cyclone_tcp/ipv6"
mkdir -p "$DST/cyclone_tcp/mld"
mkdir -p "$DST/cyclone_tcp/ppp"
mkdir -p "$DST/cyclone_tcp/mibs"
mkdir -p "$DST/cyclone_tcp/dns"
mkdir -p "$DST/cyclone_tcp/mdns"
mkdir -p "$DST/cyclone_tcp/dns_sd"
mkdir -p "$DST/cyclone_tcp/netbios"
mkdir -p "$DST/cyclone_tcp/llmnr"
mkdir -p "$DST/cyclone_tcp/dhcp"
mkdir -p "$DST/cyclone_tcp/dhcpv6"
mkdir -p "$DST/cyclone_tcp/http"
mkdir -p "$DST/cyclone_tcp/web_socket"
mkdir -p "$DST/cyclone_tcp/mqtt"
mkdir -p "$DST/cyclone_tcp/coap"
echo "OK"

copy_file() {
    local src="$1"
    local dst_dir="$2"
    if [ -f "$src" ]; then
        cp "$src" "$dst_dir/"
        echo "  OK: $(basename $src)"
    else
        echo "  SKIP (no existe): $src"
    fi
}

copy_glob() {
    local pattern="$1"
    local dst_dir="$2"
    local count=0
    for f in $pattern; do
        [ -f "$f" ] || continue
        cp "$f" "$dst_dir/"
        count=$((count+1))
    done
    echo "  -> $dst_dir: $count archivos copiados"
}

echo ""
echo "=== Common Sources ==="
copy_file "$SRC/common/cpu_endian.c"       "$DST/common"
copy_file "$SRC/common/os_port_freertos.c" "$DST/common"
copy_file "$SRC/common/date_time.c"        "$DST/common"
copy_file "$SRC/common/str.c"              "$DST/common"
copy_file "$SRC/common/path.c"             "$DST/common"
copy_file "$SRC/common/resource_manager.c" "$DST/common"
copy_file "$SRC/common/debug.c"            "$DST/common"

echo ""
echo "=== Common Headers ==="
copy_file "$SRC/common/cpu_endian.h"       "$DST/common"
copy_file "$SRC/common/compiler_port.h"    "$DST/common"
copy_file "$SRC/common/os_port.h"          "$DST/common"
copy_file "$SRC/common/os_port_freertos.h" "$DST/common"
copy_file "$SRC/common/date_time.h"        "$DST/common"
copy_file "$SRC/common/str.h"              "$DST/common"
copy_file "$SRC/common/path.h"             "$DST/common"
copy_file "$SRC/common/resource_manager.h" "$DST/common"
copy_file "$SRC/common/error.h"            "$DST/common"
copy_file "$SRC/common/debug.h"            "$DST/common"

echo ""
echo "=== CycloneTCP Sources ==="
copy_file "$SRC/cyclone_tcp/drivers/wifi/esp32_wifi_driver.c" "$DST/cyclone_tcp/drivers/wifi"
copy_glob "$SRC/cyclone_tcp/core/*.c"     "$DST/cyclone_tcp/core"
copy_glob "$SRC/cyclone_tcp/ipv4/*.c"     "$DST/cyclone_tcp/ipv4"
copy_glob "$SRC/cyclone_tcp/igmp/*.c"     "$DST/cyclone_tcp/igmp"
copy_glob "$SRC/cyclone_tcp/ipv6/*.c"     "$DST/cyclone_tcp/ipv6"
copy_glob "$SRC/cyclone_tcp/mld/*.c"      "$DST/cyclone_tcp/mld"
copy_glob "$SRC/cyclone_tcp/dns/*.c"      "$DST/cyclone_tcp/dns"
copy_glob "$SRC/cyclone_tcp/mdns/*.c"     "$DST/cyclone_tcp/mdns"
copy_glob "$SRC/cyclone_tcp/netbios/*.c"  "$DST/cyclone_tcp/netbios"
copy_glob "$SRC/cyclone_tcp/llmnr/*.c"    "$DST/cyclone_tcp/llmnr"
copy_glob "$SRC/cyclone_tcp/dhcp/*.c"     "$DST/cyclone_tcp/dhcp"
copy_glob "$SRC/cyclone_tcp/http/*.c"     "$DST/cyclone_tcp/http"
copy_glob "$SRC/cyclone_tcp/mqtt/*.c"     "$DST/cyclone_tcp/mqtt"
copy_glob "$SRC/cyclone_tcp/coap/*.c"     "$DST/cyclone_tcp/coap"
echo ""
echo "=== CycloneTCP Headers ==="
copy_file "$SRC/cyclone_tcp/drivers/wifi/esp32_wifi_driver.h" "$DST/cyclone_tcp/drivers/wifi"
copy_glob "$SRC/cyclone_tcp/core/*.h"     "$DST/cyclone_tcp/core"
copy_glob "$SRC/cyclone_tcp/ipv4/*.h"     "$DST/cyclone_tcp/ipv4"
copy_glob "$SRC/cyclone_tcp/igmp/*.h"     "$DST/cyclone_tcp/igmp"
copy_glob "$SRC/cyclone_tcp/nat/*.h"      "$DST/cyclone_tcp/nat"
copy_glob "$SRC/cyclone_tcp/ipv6/*.h"     "$DST/cyclone_tcp/ipv6"
copy_glob "$SRC/cyclone_tcp/mld/*.h"      "$DST/cyclone_tcp/mld"
copy_glob "$SRC/cyclone_tcp/ppp/*.h"      "$DST/cyclone_tcp/ppp"
copy_glob "$SRC/cyclone_tcp/mibs/*.h"     "$DST/cyclone_tcp/mibs"
copy_glob "$SRC/cyclone_tcp/dns/*.h"      "$DST/cyclone_tcp/dns"
copy_glob "$SRC/cyclone_tcp/mdns/*.h"     "$DST/cyclone_tcp/mdns"
copy_glob "$SRC/cyclone_tcp/dns_sd/*.h"   "$DST/cyclone_tcp/dns_sd"
copy_glob "$SRC/cyclone_tcp/netbios/*.h"  "$DST/cyclone_tcp/netbios"
copy_glob "$SRC/cyclone_tcp/llmnr/*.h"    "$DST/cyclone_tcp/llmnr"
copy_glob "$SRC/cyclone_tcp/dhcp/*.h"     "$DST/cyclone_tcp/dhcp"
copy_glob "$SRC/cyclone_tcp/dhcpv6/*.h"   "$DST/cyclone_tcp/dhcpv6"
copy_glob "$SRC/cyclone_tcp/http/*.h"     "$DST/cyclone_tcp/http"
copy_glob "$SRC/cyclone_tcp/web_socket/*.h" "$DST/cyclone_tcp/web_socket"
copy_glob "$SRC/cyclone_tcp/mqtt/*.h"     "$DST/cyclone_tcp/mqtt"
copy_glob "$SRC/cyclone_tcp/coap/*.h"     "$DST/cyclone_tcp/coap"
echo ""
echo "=== Compilar recursos (pack.py via wine) ==="

# RC="$SRC/utils/ResourceCompiler/bin/rc.exe"
# RESOURCES_DIR="/home/tecna-smart-lab/GitHub/CommunicationProtocol-TCP_IP/clase6/resources"
# OUT_FILE="$DST/res.c"
# if [ -f "$RC" ]; then
#     wine "$RC" "$RESOURCES_DIR" "$OUT_FILE" && echo "OK: res.c generado" || echo "ERROR al generar res.c"
# else
#     echo "SKIP: rc.exe no encontrado en $RC"
# fi

echo ""
echo "=== COMPLETADO ==="
