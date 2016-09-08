# How to use this Dockerfile

This Dockerfile can be used to build an image of the FIWARE Synchronization GE server lightweight version,
aka realXtend Tundra-Urho3D.

It is based on Ubuntu 14.04. Building requires a 64bit .deb image of Tundra, which can be created using the
tools/Linux/Ubuntu/Package.bash script. A prebuilt .deb image is also provided.

The image will expose ports 2345 (real-time communications default port) and 2346 (for http communication
if wanted)

To build the image:

    docker build -t synchronization-tundra-urho3d .

To run the image in interactive mode (shell), with the container's ports exposed:

    docker run -P -it synchronization-tundra-urho3d

Running Tundra-Urho3D inside the image requires headless mode, for example:

    cd /opt/realxtend-tundra-urho3d
    ./Tundra --file Scenes/Physics/scene.txml --server --headless
