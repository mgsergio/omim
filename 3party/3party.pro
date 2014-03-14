# Project that includes all third party projects.

TEMPLATE = subdirs

SUBDIRS = freetype fribidi minizip jansson tomcrypt protobuf osrm expat torrent \
    succinct \

!linux* {
SUBDIRS += opening_hours \

}

!iphone*:!tizen*:!android* {
  SUBDIRS += gflags   \
             libtess2  \
             gmock    \
}
