CXXFLAGS += -Ilibraries/libral/include
CXXFLAGS += -fPIC -muclibc -march=mips32r2 -D_GNU_SOURCE

LDFLAGS += -Llibraries/libral/lib
LDLIBS += -Xlinker "-(" -I include -lfptcamera -lXMI_AVSYS -limp -Xlinker "-)"

# LDFLAGS += -Xlinker "-(" -I include
# LDLIBS += -lfptcamera -lXMI_AVSYS -limp -Xlinker "-)"

# LDLIBS += libraries/libral-270105/lib/libfptcamera.a
# LDLIBS += libraries/libral-270105/lib/libXMI_AVSYS.a
# LDLIBS += libraries/libral-270105/lib/libimp.a