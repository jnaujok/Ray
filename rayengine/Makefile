# RayEngine - PVM ray tracing engine (Unix).
#
# Usage:  make [PVM_INCLUDE=/path/to/pvm3/include] [PVM_LIB=/path/to/pvm3/lib/ARCH]

PVM_INCLUDE ?= ../third_party/pvm3/include
PVM_LIB     ?= /usr/local/lib/pvm3/lib/ALPHA

CDEFS     = -DUNIX
CXXFLAGS ?= -O
CXXFLAGS += -Iinclude -I$(PVM_INCLUDE) $(CDEFS)
LDLIBS    = -lm -L$(PVM_LIB) -lpvm3 -llibgcc

VPATH = src

SRCS = AmbientMaterial.cpp Camera.cpp Color.cpp ColorArray.cpp CString.cpp DiffuseMaterial.cpp \
        FileStream.cpp ImageFile.cpp Light.cpp LightArray.cpp main.cpp Material.cpp \
        MaterialList.cpp Matrix.cpp Objects.cpp ObjectsArray.cpp Plane.cpp PtLight.cpp \
        RayArray.cpp Rays.cpp RaySettings.cpp ReflectiveMaterial.cpp Scene.cpp \
        SpecularMaterial.cpp Sphere.cpp Transform.cpp TransparentMaterial.cpp Triangle.cpp \
        Vector.cpp
OBJS = $(SRCS:.cpp=.o)

all: rayengine

rayengine: $(OBJS)
	$(CXX) -o $@ $(CXXFLAGS) $(OBJS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) rayengine

cleandir: clean
	rm -f tags .depend

depend: $(SRCS)
	mkdep $(CXXFLAGS) $^

tags: $(SRCS)
	ctags $^

.PHONY: all clean cleandir depend tags
