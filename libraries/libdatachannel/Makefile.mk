CXXFLAGS += -Ilibraries/libdatachannel/include

LDLIBS += libraries/libdatachannel/lib/libdatachannel.a
LDLIBS += libraries/libdatachannel/lib/libjuice.a
LDLIBS += libraries/libdatachannel/lib/libsrtp2.a
LDLIBS += libraries/libdatachannel/lib/libusrsctp.a