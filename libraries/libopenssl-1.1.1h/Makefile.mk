CXXFLAGS += -Ilibraries/libopenssl-1.1.1h/include

LDFLAGS += -Llibraries/libopenssl-1.1.1h/lib

LDLIBS += -lcrypto -lssl