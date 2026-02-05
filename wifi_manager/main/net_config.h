/**
 * @file net_config.h
 * @brief CycloneTCP configuration file
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2025 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneTCP Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.5.4
 **/

#ifndef _NET_CONFIG_H
#define _NET_CONFIG_H

#include "sdkconfig.h"

// Macro to convert Kconfig boolean to ENABLED/DISABLED
#define CYCLONE_BOOL_TO_ENABLED(x) ((x) ? ENABLED : DISABLED)

// Macro to convert Kconfig trace level to TRACE_LEVEL
#ifdef CONFIG_MEM_TRACE_LEVEL_INFO
#define MEM_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define MEM_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_NIC_TRACE_LEVEL_INFO
#define NIC_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define NIC_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_ETH_TRACE_LEVEL_INFO
#define ETH_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define ETH_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_LLDP_TRACE_LEVEL_INFO
#define LLDP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define LLDP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_ARP_TRACE_LEVEL_INFO
#define ARP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define ARP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_IP_TRACE_LEVEL_INFO
#define IP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define IP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_IPV4_TRACE_LEVEL_INFO
#define IPV4_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define IPV4_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_IPV6_TRACE_LEVEL_INFO
#define IPV6_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define IPV6_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_ICMP_TRACE_LEVEL_INFO
#define ICMP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define ICMP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_IGMP_TRACE_LEVEL_INFO
#define IGMP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define IGMP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_NAT_TRACE_LEVEL_INFO
#define NAT_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define NAT_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_ICMPV6_TRACE_LEVEL_INFO
#define ICMPV6_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define ICMPV6_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_MLD_TRACE_LEVEL_INFO
#define MLD_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define MLD_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_NDP_TRACE_LEVEL_INFO
#define NDP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define NDP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_UDP_TRACE_LEVEL_INFO
#define UDP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define UDP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_TCP_TRACE_LEVEL_INFO
#define TCP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define TCP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_SOCKET_TRACE_LEVEL_INFO
#define SOCKET_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define SOCKET_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_RAW_SOCKET_TRACE_LEVEL_INFO
#define RAW_SOCKET_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define RAW_SOCKET_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_BSD_SOCKET_TRACE_LEVEL_INFO
#define BSD_SOCKET_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define BSD_SOCKET_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_WEB_SOCKET_TRACE_LEVEL_INFO
#define WEB_SOCKET_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define WEB_SOCKET_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_AUTO_IP_TRACE_LEVEL_INFO
#define AUTO_IP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define AUTO_IP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_SLAAC_TRACE_LEVEL_INFO
#define SLAAC_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define SLAAC_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_DHCP_TRACE_LEVEL_INFO
#define DHCP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define DHCP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_DHCPV6_TRACE_LEVEL_INFO
#define DHCPV6_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define DHCPV6_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_DNS_TRACE_LEVEL_INFO
#define DNS_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define DNS_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_MDNS_TRACE_LEVEL_INFO
#define MDNS_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define MDNS_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_NBNS_TRACE_LEVEL_INFO
#define NBNS_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define NBNS_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_LLMNR_TRACE_LEVEL_INFO
#define LLMNR_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define LLMNR_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_ECHO_TRACE_LEVEL_INFO
#define ECHO_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define ECHO_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_COAP_TRACE_LEVEL_INFO
#define COAP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define COAP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_FTP_TRACE_LEVEL_INFO
#define FTP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define FTP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_HTTP_TRACE_LEVEL_INFO
#define HTTP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define HTTP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_MQTT_TRACE_LEVEL_INFO
#define MQTT_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define MQTT_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_MQTT_SN_TRACE_LEVEL_INFO
#define MQTT_SN_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define MQTT_SN_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_SMTP_TRACE_LEVEL_INFO
#define SMTP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define SMTP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_SNMP_TRACE_LEVEL_INFO
#define SNMP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define SNMP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_SNTP_TRACE_LEVEL_INFO
#define SNTP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define SNTP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_NTP_TRACE_LEVEL_INFO
#define NTP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define NTP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_NTS_TRACE_LEVEL_INFO
#define NTS_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define NTS_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_TFTP_TRACE_LEVEL_INFO
#define TFTP_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define TFTP_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

#ifdef CONFIG_MODBUS_TRACE_LEVEL_INFO
#define MODBUS_TRACE_LEVEL TRACE_LEVEL_INFO
#else
#define MODBUS_TRACE_LEVEL TRACE_LEVEL_OFF
#endif

// Number of network adapters
#define NET_INTERFACE_COUNT CONFIG_NET_INTERFACE_COUNT

// Size of the MAC address filter
#define MAC_ADDR_FILTER_SIZE CONFIG_MAC_ADDR_FILTER_SIZE

// IPv4 support (forzado a IPv4-only)
#define IPV4_SUPPORT ENABLED

// Size of the IPv4 multicast filter
#define IPV4_MULTICAST_FILTER_SIZE CONFIG_IPV4_MULTICAST_FILTER_SIZE

// IPv4 fragmentation support
#if CONFIG_IPV4_FRAG_SUPPORT
#define IPV4_FRAG_SUPPORT ENABLED
#else
#define IPV4_FRAG_SUPPORT DISABLED
#endif

// Maximum number of fragmented packets the host will accept
// and hold in the reassembly queue simultaneously
#define IPV4_MAX_FRAG_DATAGRAMS CONFIG_IPV4_MAX_FRAG_DATAGRAMS

// Maximum datagram size the host will accept when reassembling fragments
#define IPV4_MAX_FRAG_DATAGRAM_SIZE CONFIG_IPV4_MAX_FRAG_DATAGRAM_SIZE

// Size of ARP cache
#define ARP_CACHE_SIZE CONFIG_ARP_CACHE_SIZE

// Maximum number of packets waiting for address resolution to complete
#define ARP_MAX_PENDING_PACKETS CONFIG_ARP_MAX_PENDING_PACKETS

// IGMP host support
#if CONFIG_IGMP_HOST_SUPPORT
#define IGMP_HOST_SUPPORT ENABLED
#else
#define IGMP_HOST_SUPPORT DISABLED
#endif

// DHCP server support
#if CONFIG_DHCP_SERVER_SUPPORT
#define DHCP_SERVER_SUPPORT ENABLED
#else
#define DHCP_SERVER_SUPPORT DISABLED
#endif

// IPv6 support
#if CONFIG_IPV6_SUPPORT
#define IPV6_SUPPORT ENABLED
#else
#define IPV6_SUPPORT DISABLED
#endif

// Size of the IPv6 multicast filter
#define IPV6_MULTICAST_FILTER_SIZE CONFIG_IPV6_MULTICAST_FILTER_SIZE

// IPv6 fragmentation support
#if CONFIG_IPV6_FRAG_SUPPORT
#define IPV6_FRAG_SUPPORT ENABLED
#else
#define IPV6_FRAG_SUPPORT DISABLED
#endif

// Maximum number of fragmented packets the host will accept
// and hold in the reassembly queue simultaneously
#define IPV6_MAX_FRAG_DATAGRAMS CONFIG_IPV6_MAX_FRAG_DATAGRAMS

// Maximum datagram size the host will accept when reassembling fragments
#define IPV6_MAX_FRAG_DATAGRAM_SIZE CONFIG_IPV6_MAX_FRAG_DATAGRAM_SIZE

// MLD node support
#if CONFIG_MLD_NODE_SUPPORT
#define MLD_NODE_SUPPORT ENABLED
#else
#define MLD_NODE_SUPPORT DISABLED
#endif

// RA service support
#if CONFIG_NDP_ROUTER_ADV_SUPPORT
#define NDP_ROUTER_ADV_SUPPORT ENABLED
#else
#define NDP_ROUTER_ADV_SUPPORT DISABLED
#endif

// Neighbor cache size
#define NDP_NEIGHBOR_CACHE_SIZE CONFIG_NDP_NEIGHBOR_CACHE_SIZE

// Destination cache size
#define NDP_DEST_CACHE_SIZE CONFIG_NDP_DEST_CACHE_SIZE

// Maximum number of packets waiting for address resolution to complete
#define NDP_MAX_PENDING_PACKETS CONFIG_NDP_MAX_PENDING_PACKETS

// TCP support
#if CONFIG_TCP_SUPPORT
#define TCP_SUPPORT ENABLED
#else
#define TCP_SUPPORT DISABLED
#endif

// Default buffer size for transmission
#define TCP_DEFAULT_TX_BUFFER_SIZE CONFIG_TCP_DEFAULT_TX_BUFFER_SIZE

// Default buffer size for reception
#define TCP_DEFAULT_RX_BUFFER_SIZE CONFIG_TCP_DEFAULT_RX_BUFFER_SIZE

// Default SYN queue size for listening sockets
#define TCP_DEFAULT_SYN_QUEUE_SIZE CONFIG_TCP_DEFAULT_SYN_QUEUE_SIZE

// Maximum number of retransmissions
#define TCP_MAX_RETRIES CONFIG_TCP_MAX_RETRIES

// Selective acknowledgment support
#if CONFIG_TCP_SACK_SUPPORT
#define TCP_SACK_SUPPORT ENABLED
#else
#define TCP_SACK_SUPPORT DISABLED
#endif

// TCP keep-alive support
#if CONFIG_TCP_KEEP_ALIVE_SUPPORT
#define TCP_KEEP_ALIVE_SUPPORT ENABLED
#else
#define TCP_KEEP_ALIVE_SUPPORT DISABLED
#endif

// UDP support
#if CONFIG_UDP_SUPPORT
#define UDP_SUPPORT ENABLED
#else
#define UDP_SUPPORT DISABLED
#endif

// Receive queue depth for connectionless sockets
#define UDP_RX_QUEUE_SIZE CONFIG_UDP_RX_QUEUE_SIZE

// Raw socket support
#if CONFIG_RAW_SOCKET_SUPPORT
#define RAW_SOCKET_SUPPORT ENABLED
#else
#define RAW_SOCKET_SUPPORT DISABLED
#endif

// Receive queue depth for raw sockets
#define RAW_SOCKET_RX_QUEUE_SIZE CONFIG_RAW_SOCKET_RX_QUEUE_SIZE

// BSD socket support
#if CONFIG_BSD_SOCKET_SUPPORT
#define BSD_SOCKET_SUPPORT ENABLED
#else
#define BSD_SOCKET_SUPPORT DISABLED
#endif

// Number of sockets that can be opened simultaneously
#define SOCKET_MAX_COUNT CONFIG_SOCKET_MAX_COUNT

// LLMNR responder support
#if CONFIG_LLMNR_RESPONDER_SUPPORT
#define LLMNR_RESPONDER_SUPPORT ENABLED
#else
#define LLMNR_RESPONDER_SUPPORT DISABLED
#endif

// HTTP server support
#if CONFIG_HTTP_SERVER_SUPPORT
#define HTTP_SERVER_SUPPORT ENABLED
#else
#define HTTP_SERVER_SUPPORT DISABLED
#endif

// Server Side Includes support
#if CONFIG_HTTP_SERVER_SSI_SUPPORT
#define HTTP_SERVER_SSI_SUPPORT ENABLED
#else
#define HTTP_SERVER_SSI_SUPPORT DISABLED
#endif

#endif
