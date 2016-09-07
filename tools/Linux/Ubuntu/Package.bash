#!/bin/bash

PARENT_DIR=$(dirname $(readlink -f $0))
source ${PARENT_DIR}/Paths.bash

viewer=$(dirname $(readlink -f $0))/../..
deps=$DEPS
viewer=$(cd $viewer && pwd)

dest=deb/opt/realxtend-tundra-urho3d

version="1.0.0"
package="realxtend-tundra-urho3d-$version-ubuntu-14.04-amd64.deb"

echo "Creating $package"
echo "to    $dest"
echo ""

# Prepare
rm -r -f deb/opt
mkdir -p $dest

# List
libraries=( 
    src/urho3d/Build/lib/libUrho3D.so.0
)

destinations=(
    .
)

echo "Copying and stripping Tundra third-party libraries..."
for (( i = 0 ; i < ${#libraries[@]} ; i++ )) do
    lib=${libraries[$i]}
    libname=$(basename $lib)
    libsrc=$DEPS/$lib
    libdest=${destinations[$i]}

    echo "$lib "
    echo "  cp..."
    mkdir -p $dest/$libdest
    cp -f -L $libsrc $dest/$libdest
    echo "  strip..."
    strip --strip-debug $dest/$libdest/$libname
done

# Copy Tundra build and data
echo "Copying and stripping Tundra executable, plugins and data..."
cp $viewer/bin/** $dest
strip --strip-debug $dest/Tundra
strip --strip-debug $dest/*.so

tundradirs=(
    Data
    jsmodules
    Plugins
    Scenes
)
for (( i = 0 ; i < ${#tundradirs[@]} ; i++ )) do
    dir=${tundradirs[$i]}
    echo "$dir "
    echo "  cp..."
    mkdir -p $dest/$dir
    cp -f -r $viewer/bin/$dir/** $dest/$dir
    echo "  strip..."
    strip --strip-debug $dest/$dir/*.so
done

# Create Tundra executable script
cat > $dest/RunTundra <<EOF
#!/bin/sh

tundradir=\$(dirname \$(readlink -f \$0))
cd \$tundradir
LD_LIBRARY_PATH=.:\$LD_LIBRARY_PATH ./Tundra --config tundra-client.json
EOF
chmod +x $dest/RunTundra

echo "Building .deb package..."
dpkg --build deb $package

echo ""
echo "---- ALL DONE ----"
echo ""

