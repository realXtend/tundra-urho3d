# Synchronization - User and Programmer's Guide

## Introduction

This document describes the use of the Synchronization Generic Enabler.

The typical use of this GE is creating multi-user networked scenes, which may include 3D visualization (for example a virtual world or a multiplayer game). A client connects, using the WebSocket protocol, to a server where the multi-user scene is hosted, after which they will receive the scene content and any updates to it, for example objects' position updates under a physics simulation calculated by the server. The clients interact with the scene by either directly manipulating the scene content (this may not be feasible in all cases and can be prevented by security scripts running on the server) or by sending Entity Actions, which resemble remote procedure calls, that can be interpreted by scripts running on the server. For example, the movement controls of a client controlled character ("avatar") could be sent as Entity Actions (ie. move forward, stop moving forward, rotate 45 degrees right.)

Note that the Synchronization GE client code itself does not visualize anything, it only updates the internal scene data model according to data from the network. However, the same WebTundra codebase that houses the Synchronization client code also contains an implementation of the 3D-UI GE, which implements 3D visualization on top of the scene data model.

## User guide

The user guide section describes on a high level the data model used by this GE, and how scenes are loaded into the server.

### Data model

The data that is transmitted between the server and the client is based on the Entity-Component-Attribute model as used by realXtend Tundra. It is described here [https://github.com/realXtend/tundra/wiki/Scene-and-EC-Model](https://github.com/realXtend/tundra/wiki/Scene-and-EC-Model)

In particular, the following actions between the server and the client are synchronized bidirectionally through the WebSocket connection:

- Create an entity
- Remove an entity
- Create a component into an entity
- Remove a component from an entity
- Modify an attribute value in a component. This is the bulk of the network traffic. For example when an entity moves, the Transform attribute of its Placeable component is being updated constantly.
- Create an attribute into the DynamicComponent type of component.
- Remove an attribute from a DynamicComponent.
- Send an Entity Action.

When multiple clients are connected, such modifications are sent to all of them.

### Using the Tundra server

The realXtend Tundra SDK is a complex piece of software and to provide a full description of how it can be used is beyond the scope of this document. For more documentation please see [http://doc.meshmoon.com/](http://doc.meshmoon.com/) Note that MeshMoon is a proprietary hosting solution based on Tundra SDK but it currently provides the best, most up-to-date documentation of the Tundra SDK. Parts of the documentation which pertain only to the MeshMoon specific extensions and not the core Tundra SDK itself are marked so.

To see all Tundra's command line options use the command

<pre>
Tundra --help
</pre>

The following command line options are often used:

<pre>
--file scenename.txml  Specify the txml scene file to open on startup. Example scenes exist 
                       in the bin/scenes directory.
--server               Run as a server
--port portnumber      Specify which port the server listens on. This is both for native
                       clients (UDP protocol) and WebSocket clients (TCP protocol)
--headless             Run without graphics rendering
--config configfile    Specify the JSON configuration file(s) to use. If not specified, the
                       default configuration file tundra.json is used.
</pre>

## Programmers guide

The programmer's guide describes the JavaScript client library API and relevant parts of the server code's operation, as well as the binary-level protocol used.

### JavaScript client library

The Synchronization client code is housed in the WebTundra code base [https://github.com/realxtend/webtundra](https://github.com/realxtend/webtundra). See its documentation for full details. Some simple example how to run WebTundra is described here:

#### From binary ####

Download latest WebTundra package from [Catalog](http://catalogue.fiware.org/enablers/3dui-webtundra/downloads) and extract the zip file

Startup the Synchronization server TundraConsole --server --headless --file scenes/Physics/scene.txml

Start apache2 or node.js http-server in folder where you extracted the zip file and open client.html in web browser and press connect.

#### From source ####

Clone WebTundra from GitHub:

<pre>git clone https://github.com/realXtend/WebTundra.git</pre>

Setup [`node.js`](http://nodejs.org/) and [`grunt`](http://gruntjs.com/) for making build:

1. Install node.js
2. Run npm install on the repo root folder to fetch dependencies.
3. Run npm install -g grunt-cli to install the grunt executable as a global tool.

Run grunt in WebTundra root folder:

<pre>grunt build</pre>

Startup the Synchronization server TundraConsole --server --headless --file scenes/Physics/scene.txml

Start apache2 or node.js http-server in folder where you extracted the zip file and open client.html in web browser and press connect.

### Server plugins

The WebSocket server functionality of the Synchronization GE is implemented in the WebSocketServerModule.

It uses the websocketpp library for implementing WebSocket communications [https://github.com/zaphoyd/websocketpp](https://github.com/zaphoyd/websocketpp)

The WebSocketServerModule registers the WebSocket client connections to the Tundra main server class, so that other server-side modules and scripts can treat native (C++ client) and Web client connections as equivalent.

### Synchronization binary protocol

For the description of the byte-level protocol see [https://github.com/realXtend/tundra/wiki/Tundra-protocol](https://github.com/realXtend/tundra/wiki/Tundra-protocol).

- Each message is sent as one binary WebSocket frame, with the message ID encoded as an unsigned little-endian 16-bit value in the beginning.
- Login data (message ID 100) is JSON instead of XML.
- Before the server starts sending scene messages, the client must "authenticate" itself by sending the login message. In a default Tundra server configuration (no scene password, no security scripts) the actual data content sent in the login message does not matter.
