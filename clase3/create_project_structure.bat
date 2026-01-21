md main\common
md main\cyclone_tcp
md main\cyclone_tcp\core
md main\cyclone_tcp\drivers
md main\cyclone_tcp\drivers\wifi
md main\cyclone_tcp\ipv4
md main\cyclone_tcp\igmp
md main\cyclone_tcp\nat
md main\cyclone_tcp\ipv6
md main\cyclone_tcp\mld
md main\cyclone_tcp\ppp
md main\cyclone_tcp\mibs
md main\cyclone_tcp\dns
md main\cyclone_tcp\mdns
md main\cyclone_tcp\dns_sd
md main\cyclone_tcp\netbios
md main\cyclone_tcp\llmnr
md main\cyclone_tcp\dhcp
md main\cyclone_tcp\dhcpv6
md main\cyclone_tcp\http
md main\cyclone_tcp\web_socket

REM Common Sources
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\cpu_endian.c main\common
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\os_port_freertos.c main\common
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\date_time.c main\common
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\str.c main\common
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\path.c main\common
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\resource_manager.c main\common
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\debug.c main\common

REM Common Headers
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\cpu_endian.h main\common
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\compiler_port.h main\common
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\os_port.h main\common
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\os_port_freertos.h main\common
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\date_time.h main\common
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\str.h main\common
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\path.h main\common
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\resource_manager.h main\common
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\error.h main\common
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\common\debug.h main\common

REM CycloneTCP Sources
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\drivers\wifi\esp32_wifi_driver.c main\cyclone_tcp\drivers\wifi
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\core\*.c main\cyclone_tcp\core
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\ipv4\*.c main\cyclone_tcp\ipv4
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\igmp\*.c main\cyclone_tcp\igmp
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\ipv6\*.c main\cyclone_tcp\ipv6
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\mld\*.c main\cyclone_tcp\mld
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\dns\*.c main\cyclone_tcp\dns
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\mdns\*.c main\cyclone_tcp\mdns
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\netbios\*.c main\cyclone_tcp\netbios
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\llmnr\*.c main\cyclone_tcp\llmnr
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\dhcp\*.c main\cyclone_tcp\dhcp
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\http\*.c main\cyclone_tcp\http

REM CycloneTCP Headers
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\drivers\wifi\esp32_wifi_driver.h main\cyclone_tcp\drivers\wifi
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\core\*.h main\cyclone_tcp\core
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\ipv4\*.h main\cyclone_tcp\ipv4
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\igmp\*.h main\cyclone_tcp\igmp
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\nat\*.h main\cyclone_tcp\nat
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\ipv6\*.h main\cyclone_tcp\ipv6
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\mld\*.h main\cyclone_tcp\mld
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\ppp\*.h main\cyclone_tcp\ppp
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\mibs\*.h main\cyclone_tcp\mibs
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\dns\*.h main\cyclone_tcp\dns
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\mdns\*.h main\cyclone_tcp\mdns
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\dns_sd\*.h main\cyclone_tcp\dns_sd
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\netbios\*.h main\cyclone_tcp\netbios
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\llmnr\*.h main\cyclone_tcp\llmnr
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\dhcp\*.h main\cyclone_tcp\dhcp
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\dhcpv6\*.h main\cyclone_tcp\dhcpv6
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\http\*.h main\cyclone_tcp\http
copy \home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\cyclone_tcp\web_socket\*.h main\cyclone_tcp\web_socket

REM Compile resources
\home\tecna-smart-lab\Documentos\CODAERUS\PROTOCOLOS\CycloneTCP_SSL_SSH_IPSEC_EAP_CRYPTO_Open_2_5_4\utils\ResourceCompiler\bin\rc.exe resources main\res.c
