TARGET = torrent
ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)
TEMPLATE = lib
CONFIG += staticlib warn_off

INCLUDEPATH += include ../boost ed25519/src

DEFINES *= BOOST_ASIO_DISABLE_SERIAL_PORT BOOST_ASIO_SEPARATE_COMPILATION \
           TORRENT_DISABLE_RESOLVE_COUNTRIES TORRENT_DISABLE_GEO_IP \
           TORRENT_DISABLE_ENCRYPTION TORRENT_DISABLE_POOL_ALLOCATOR \
           TORRENT_NO_DEPRECATE
CONFIG(release, debug|release) {
  DEFINES *= NDEBUG TORRENT_DISABLE_INVARIANT_CHECKS TORRENT_DISABLE_FULL_STATS
} else {
  DEFINES *= TORRENT_EXPENSIVE_INVARIANT_CHECKS TORRENT_DHT_VERBOSE_LOGGING TORRENT_STATS \
             TORRENT_UPNP_LOGGING TORRENT_LOGGING TORRENT_VERBOSE_LOGGING
}

HEADERS += \
  include/libtorrent/ConvertUTF.h \
  include/libtorrent/GeoIP.h \
  include/libtorrent/add_torrent_params.hpp \
  include/libtorrent/address.hpp \
  include/libtorrent/alert.hpp \
  include/libtorrent/alert_dispatcher.hpp \
  include/libtorrent/alert_manager.hpp \
  include/libtorrent/alert_types.hpp \
  include/libtorrent/alloca.hpp \
  include/libtorrent/allocator.hpp \
  include/libtorrent/assert.hpp \
  include/libtorrent/aux_/session_impl.hpp \
  include/libtorrent/bandwidth_limit.hpp \
  include/libtorrent/bandwidth_manager.hpp \
  include/libtorrent/bandwidth_queue_entry.hpp \
  include/libtorrent/bandwidth_socket.hpp \
  include/libtorrent/bencode.hpp \
  include/libtorrent/bitfield.hpp \
  include/libtorrent/bloom_filter.hpp \
  include/libtorrent/broadcast_socket.hpp \
  include/libtorrent/bt_peer_connection.hpp \
  include/libtorrent/buffer.hpp \
  include/libtorrent/build_config.hpp \
  include/libtorrent/chained_buffer.hpp \
  include/libtorrent/config.hpp \
  include/libtorrent/connection_queue.hpp \
  include/libtorrent/copy_ptr.hpp \
  include/libtorrent/create_torrent.hpp \
  include/libtorrent/deadline_timer.hpp \
  include/libtorrent/debug.hpp \
  include/libtorrent/disk_buffer_holder.hpp \
  include/libtorrent/disk_buffer_pool.hpp \
  include/libtorrent/disk_io_thread.hpp \
  include/libtorrent/entry.hpp \
  include/libtorrent/enum_net.hpp \
  include/libtorrent/error.hpp \
  include/libtorrent/error_code.hpp \
  include/libtorrent/escape_string.hpp \
  include/libtorrent/extensions/logger.hpp \
  include/libtorrent/extensions/lt_trackers.hpp \
  include/libtorrent/extensions/metadata_transfer.hpp \
  include/libtorrent/extensions/smart_ban.hpp \
  include/libtorrent/extensions/ut_metadata.hpp \
  include/libtorrent/extensions/ut_pex.hpp \
  include/libtorrent/extensions.hpp \
  include/libtorrent/file.hpp \
  include/libtorrent/file_pool.hpp \
  include/libtorrent/file_storage.hpp \
  include/libtorrent/fingerprint.hpp \
  include/libtorrent/gzip.hpp \
  include/libtorrent/hasher.hpp \
  include/libtorrent/http_connection.hpp \
  include/libtorrent/http_parser.hpp \
  include/libtorrent/http_seed_connection.hpp \
  include/libtorrent/http_stream.hpp \
  include/libtorrent/http_tracker_connection.hpp \
  include/libtorrent/i2p_stream.hpp \
  include/libtorrent/identify_client.hpp \
  include/libtorrent/instantiate_connection.hpp \
  include/libtorrent/intrusive_ptr_base.hpp \
  include/libtorrent/invariant_check.hpp \
  include/libtorrent/io.hpp \
  include/libtorrent/io_service.hpp \
  include/libtorrent/io_service_fwd.hpp \
  include/libtorrent/ip_filter.hpp \
  include/libtorrent/ip_voter.hpp \
  include/libtorrent/kademlia/dht_observer.hpp \
  include/libtorrent/kademlia/dht_tracker.hpp \
  include/libtorrent/kademlia/find_data.hpp \
  include/libtorrent/kademlia/get_item.hpp \
  include/libtorrent/kademlia/get_peers.hpp \
  include/libtorrent/kademlia/item.hpp \
  include/libtorrent/kademlia/logging.hpp \
  include/libtorrent/kademlia/msg.hpp \
  include/libtorrent/kademlia/node.hpp \
  include/libtorrent/kademlia/node_entry.hpp \
  include/libtorrent/kademlia/node_id.hpp \
  include/libtorrent/kademlia/observer.hpp \
  include/libtorrent/kademlia/refresh.hpp \
  include/libtorrent/kademlia/routing_table.hpp \
  include/libtorrent/kademlia/rpc_manager.hpp \
  include/libtorrent/kademlia/traversal_algorithm.hpp \
  include/libtorrent/lazy_entry.hpp \
  include/libtorrent/lsd.hpp \
  include/libtorrent/magnet_uri.hpp \
  include/libtorrent/max.hpp \
  include/libtorrent/natpmp.hpp \
  include/libtorrent/packet_buffer.hpp \
  include/libtorrent/parse_url.hpp \
  include/libtorrent/pch.hpp \
  include/libtorrent/pe_crypto.hpp \
  include/libtorrent/peer.hpp \
  include/libtorrent/peer_connection.hpp \
  include/libtorrent/peer_id.hpp \
  include/libtorrent/peer_info.hpp \
  include/libtorrent/peer_request.hpp \
  include/libtorrent/piece_block_progress.hpp \
  include/libtorrent/piece_picker.hpp \
  include/libtorrent/policy.hpp \
  include/libtorrent/proxy_base.hpp \
  include/libtorrent/ptime.hpp \
  include/libtorrent/puff.hpp \
  include/libtorrent/random.hpp \
  include/libtorrent/rss.hpp \
  include/libtorrent/session.hpp \
  include/libtorrent/session_settings.hpp \
  include/libtorrent/session_status.hpp \
  include/libtorrent/settings.hpp \
  include/libtorrent/sha1_hash.hpp \
  include/libtorrent/size_type.hpp \
  include/libtorrent/sliding_average.hpp \
  include/libtorrent/socket.hpp \
  include/libtorrent/socket_io.hpp \
  include/libtorrent/socket_type.hpp \
  include/libtorrent/socket_type_fwd.hpp \
  include/libtorrent/socks5_stream.hpp \
  include/libtorrent/ssl_stream.hpp \
  include/libtorrent/stat.hpp \
  include/libtorrent/storage.hpp \
  include/libtorrent/storage_defs.hpp \
  include/libtorrent/string_util.hpp \
  include/libtorrent/thread.hpp \
  include/libtorrent/time.hpp \
  include/libtorrent/timestamp_history.hpp \
  include/libtorrent/tommath.h \
  include/libtorrent/tommath_class.h \
  include/libtorrent/tommath_superclass.h \
  include/libtorrent/torrent.hpp \
  include/libtorrent/torrent_handle.hpp \
  include/libtorrent/torrent_info.hpp \
  include/libtorrent/tracker_manager.hpp \
  include/libtorrent/udp_socket.hpp \
  include/libtorrent/udp_tracker_connection.hpp \
  include/libtorrent/union_endpoint.hpp \
  include/libtorrent/upnp.hpp \
  include/libtorrent/utf8.hpp \
  include/libtorrent/utp_socket_manager.hpp \
  include/libtorrent/utp_stream.hpp \
  include/libtorrent/version.hpp \
  include/libtorrent/web_connection_base.hpp \
  include/libtorrent/web_peer_connection.hpp \
  include/libtorrent/xml_parse.hpp \

SOURCES += \
  src/ConvertUTF.cpp \
  src/alert.cpp \
  src/alert_manager.cpp \
  src/allocator.cpp \
  src/asio.cpp \
  #src/asio_ssl.cpp \
  src/assert.cpp \
  src/bandwidth_limit.cpp \
  src/bandwidth_manager.cpp \
  src/bandwidth_queue_entry.cpp \
  src/bloom_filter.cpp \
  src/broadcast_socket.cpp \
  src/bt_peer_connection.cpp \
  src/chained_buffer.cpp \
  src/connection_queue.cpp \
  src/create_torrent.cpp \
  src/disk_buffer_holder.cpp \
  src/disk_buffer_pool.cpp \
  src/disk_io_thread.cpp \
  src/entry.cpp \
  src/enum_net.cpp \
  src/error_code.cpp \
  src/escape_string.cpp \
  src/file.cpp \
  src/file_pool.cpp \
  src/file_storage.cpp \
  src/gzip.cpp \
  src/hasher.cpp \
  src/http_connection.cpp \
  src/http_parser.cpp \
  src/http_seed_connection.cpp \
  src/http_stream.cpp \
  src/http_tracker_connection.cpp \
  src/i2p_stream.cpp \
  src/identify_client.cpp \
  src/instantiate_connection.cpp \
  src/ip_filter.cpp \
  src/ip_voter.cpp \
  src/lazy_bdecode.cpp \
  src/logger.cpp \
  src/lsd.cpp \
  src/lt_trackers.cpp \
  src/magnet_uri.cpp \
  src/metadata_transfer.cpp \
  src/natpmp.cpp \
  src/packet_buffer.cpp \
  src/parse_url.cpp \
  src/pe_crypto.cpp \
  src/peer_connection.cpp \
  src/piece_picker.cpp \
  src/policy.cpp \
  src/puff.cpp \
  src/random.cpp \
  src/rss.cpp \
  src/session.cpp \
  src/session_impl.cpp \
  src/settings.cpp \
  src/sha1.cpp \
  src/smart_ban.cpp \
  src/socket_io.cpp \
  src/socket_type.cpp \
  src/socks5_stream.cpp \
  src/stat.cpp \
  src/storage.cpp \
  src/string_util.cpp \
  src/thread.cpp \
  src/time.cpp \
  src/timestamp_history.cpp \
  src/torrent.cpp \
  src/torrent_handle.cpp \
  src/torrent_info.cpp \
  src/tracker_manager.cpp \
  src/udp_socket.cpp \
  src/udp_tracker_connection.cpp \
  src/upnp.cpp \
  src/ut_metadata.cpp \
  src/ut_pex.cpp \
  src/utf8.cpp \
  src/utp_socket_manager.cpp \
  src/utp_stream.cpp \
  src/web_connection_base.cpp \
  src/web_peer_connection.cpp \
  src/kademlia/dht_tracker.cpp \
  src/kademlia/find_data.cpp \
  src/kademlia/get_item.cpp \
  src/kademlia/get_peers.cpp \
  src/kademlia/item.cpp \
  src/kademlia/logging.cpp \
  src/kademlia/node.cpp \
  src/kademlia/node_id.cpp \
  src/kademlia/refresh.cpp \
  src/kademlia/routing_table.cpp \
  src/kademlia/rpc_manager.cpp \
  src/kademlia/traversal_algorithm.cpp \
  ed25519/src/sign.c \
  ed25519/src/verify.c \
  ed25519/src/sha512.c \
  ed25519/src/sc.c \
  ed25519/src/ge.c \
  ed25519/src/fe.c \
  boost_error_code.cpp \
