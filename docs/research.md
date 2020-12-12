# Research topics

1. **[Goal #1](#p2pdecentralized-protocolnetwork):** We need a **P2P/decentralized** solution for transferring (markdown) files across the network, **without** a client-server topology. No single-point-of-failure. And no easy way to censorship your site or other sites.
2. **[Goal #2](#gui-libraries):** We need to be able to draw text on the screen as fast and efficient as possible by using some existing **GUI Library**. Skipping the HTML parser step, meaning: `Markdown` -> `screen` instead of: `Markdown` -> `HTML` / `CSS` -> `screen`. Which should give us *full control* about the rendering and drawing sequences.
   - *Conclusion:** GTK + Cairso & Pango


## P2P/Decentralized Protocol/Network

See the list of P2P or decentralized protocols/network solutions below. Also known as Dapps.

Keep in mind that 'Decentralized network' is often not just a single technology, but a collaboration of technologies combined. Like Bittorrent + blockchain + DHT + exchange protocol + protobuf, CAP Theorem, Hypothetical Amnesia Machine,  and more.. All solving a specific piece of the puzzle.

Current status: It looks more and more that the best way might be a mutable Bittorrent solution written in C/C++ or Rust.

### Findings

**Findings #1:** Avoid blockchain solutions here, especially since they are not very scalable. After all each person needs to download the whole blockchain (since we do not want to rely on others). Blockchain is way too heavy. 

**Findings #2:** Instead of blockchain look at alternative solutions to store data in a P2P network.

**Findings #3:** Solutions written in Javascript that runs inside webbrowsers itsn't the solution we need. Avoid those, ideally use an existing (part of a) solution that delivers a API to C/C++. People should really stop making reference designs in Javascript! It doesn't make sense.

**External Links**

* https://sites.google.com/view/sexy-p2p/
* https://news.ycombinator.com/item?id=20162171
* https://www.cse.wustl.edu/~jain/cse570-19/ftp/decentrl/index.html
* https://github.com/gdamdam/awesome-decentralized-web
* [libtorrent](http://www.libtorrent.org/): Feature complete C++ torrent library

### Dat / Hypercore & Hyperdrive

Hypercore is a distributed append-only log. See [Hypercore protocol website](https://hypercore-protocol.org/). And [Hyperdrive GitHub](https://github.com/hypercore-protocol/hyperdrive).

Part of the [Dat Foundation](https://dat.foundation/).

I do like the append-only feature just like git, and inline with Ted vision.

Seriously, why *NodeJS* and not C or C++  or Rust as their reference implementation!?

### GNNnet

Is still a bit too early, and in 'heavy' development.

### Freenet

Freenet is a distributed, Internet-wide peer-to-peer overlay network de-signed to allow anonymized and censorship resistant publication and distribution ofinformation. 

It's 20 years old, and still almost nobody is using it.

Looks a bit too much aimed to corporate organizations somehow.

### IPFS

[IPFS](https://github.com/ipfs/go-ipfs) is a peer-to-peer hypermedia protocol designed to make the web faster, safer, and more open. 

Its really a good candidate to use! Although IPFS duplicates all content, and require pinning of data (but the last one can be abstracted away).

Why are they using the current DNS system? IPNS isn't the full solution.

### Solid

Created by Tim Berners-Lee, which is also the creator of the WWW. It really looks like an extension of the current web, but where the user is in more control about the data.

Looks like its focussed on social sites, rather then fixing the whole WWW.

[Node Solid Server](https://github.com/solid/node-solid-server) can be used to setup your own server and control your data.

[View site](https://solidproject.org/). Visit [Protocol page](https://solid.github.io/specification/) (Draft).

### Secure Scuttlebutt

[Visit SSB website](https://scuttlebot.io/)

Also written in JavaScript? Really?

### BTFS

https://github.com/johang/btfs

### Gun

[Gun](https://gun.eco/) runs inside a web browser, written in Javascript. The technology behind is is very useful, but not their implementation for us specifically.

### LBRY SDK

Implements the LBRY Network protocols. LBRY blockchain is used for storing searchable content (meta-data), right and access rules. SDK includes components:

* Kademlia DHT (Distributed Hash Table)
* Blob exchange protocol for transferring encrypted blobs of content and negotiating payments 
* Protobuf schema for encoding and decoding metadata stored on the blockchain
* Wallet implementation for the LBRY blockchain
* Daemon with a JSON-RPC API to ease building end user applications in any language and for automating various tasks 

Its written in Python. But as stated above, with a JSON-RPC API. However, much features like blockchain is not wanted.

## GUI libraries

See the list of GUI libraries below.

### Qt

Qt has many sub-classes, features and APIs for drawing and rendering. See the sub-items below for more info.

#### Rich Text

Qt [Rich Text Processing](https://doc.qt.io/qt-5/richtext.html)/QTextDocument only accepts HTML format (or directly markdown). But in this project, we are not intressed in HTML and we want to use our own markdown parser in order to extend the markdown language where and when is needed.

**Conclusion:** Will NOT be used.

#### QPainter

Painter is mainly a software renderer also called raster paint engine.

With [QPainter](https://doc.qt.io/qt-5/qpainter.html) you can draw on a viewport/paint device directly (like QPixmap, QWidget, QPicture and QOpenGLPaintDevice), see [Qt GUI](https://doc.qt.io/qt-5/qtgui-index.html) module. Q

**Conclusion:** *No Conclusion yet* - Under investigation

#### QT Quick/OpenGL

Qt as recently switched more and more to Qt Quick Scene features for 2D/OpenGL renderer and used as raster paint engine.

**Conclusion:** *No Conclusion yet* - Under investigation

#### QGraphics Scene

We can try using a [QGraphicsScene](https://doc.qt.io/qt-5/qgraphicsscene.html) and add items to the scene like the [QGraphicsTextItem](https://doc.qt.io/qt-5/qgraphicstextitem.html). The items added will be rendered and displayed automatically.

*Very bad* performance has been discovered during testing, by creating items to the scene. Even with small number of objects and changing the font to bold or italic will cause 20ms.

#### Qt links

* [Drawing text](https://github.com/radekp/qt/blob/master/src/gui/text/qtextlayout.cpp#L1114) with QPainter.
* [Mifit Text render](https://github.com/mifit/mifit/blob/master/libs/opengl/Text.cpp) for a code example. 
* [Drawing Text/line using QPainter](https://www.youtube.com/watch?v=tc3nlNEAdig) (video).
* [Calligra](https://github.com/KDE/calligra) Word processor using Qt, maybe also creating their own text painting as well?

**Conclusion:** No, QGraphics will NOT be used, way too slow. Not performant enough.

### GTK

#### GTK + Cairo

GTK is using the Cairo 2D graphics renderer using rasterization (CPU) bvy default. But Cairo also including backends for acceleration and for vector output formats. Can be used for both display and export to PDF/SVG. GTK is also much more lightweight in comparison with Qt.

Problem is its only raster graphics using the CPU. Which isn't that bad, but I really miss hardware acceleration via OpenGL/Vulkan (cairo-gl will never be stable and is a dead-end). There is no vector generation. Cairo can however be used [together with SDL](https://www.cairographics.org/SDL/) they claim.

Maybe we want to use [GooCanvas](https://wiki.gnome.org/action/show/Projects/GooCanvas), instead of the basic `DrawingArea` or we need to implement our own click handling. See [Stackoverflow question](https://stackoverflow.com/questions/26134840/selecting-drawn-lines-when-clicked-on-in-a-gtk-drawingarea). Although it does require C++ bindings and the project is not maintained anymor last but not least GooCanvasText interface seems very basic as well.

**Conclusion:** Seems like a perfect fit! Ideal for content first apps/viewer.

#### GTK +  OpenGL/Vulkan

Since GTK version 4, they are no longer using Cairo, but directly OpenGL/Vulkan calls. Using `GtkSnapshot`.

**Conclusion:** *No Conclusion yet* - Under investigation

### Skia / QtSkia

Skia is a 2D graphics library. Using for drawing text, geometries, and images. Support backends CPU raster, OpenGL/Vulkan. Both for display and SVG/PDF (can also read those files). 
Skia is also used for Google Chrome/Chrome OS, Firefox, Android, LibreOffice and more.

[Studies](https://www.facebook.com/notes/beagleboardorg-foundation/comparing-html-rendering-performance-with-qtwebkit-and-qt-native-classes/439968524361/) has shown with the Matrix GUI (developed by Texas Instruments for ARM based SoCs) that (Qt)WebKit is much much faster in rendering then the built-in Qt renderer.

Skia supports several (platform-dependent) back-ends, including one for CPU-based software rasterization, one for Portable Document Format (PDF) output, and one for GPU-accelerated OpenGL, OpenGL ES, Vulkan, and Metal one.

Doing advanced text manipulation in Skia can be achieved by combining [`SkTextBlob`](https://api.skia.org/classSkTextBlob.html#details), [`SkPaint`](https://skia.org/user/api/skpaint_overview) and [`SkTypeface`](https://api.skia.org/classSkFont.html#details) / [`SkFontStyle`](https://api.skia.org/classSkFontStyle.html).

* [Pre-build Skia library](https://github.com/aseprite/skia/releases)
* [Desktop Library using Skia](https://github.com/aseprite/laf)
* [Maybe look into RichTextKit](https://github.com/toptensoftware/RichTextKit) (although using SkiaSharp bleh)
* [API overview](https://skia.org/user/api/)

Skia is ported to Qt, called QtSkia. The Qt WebEgine renders web pages by using Skia and is not using QPainter or Qt for this purpose.  Skia is quite a mess, but one of the biggest graphical engines. Too bad Skia doesn't release any static or dynamic libraries what so ever.

Meaning the app and Skia has quite a tight build relationship. Even the Skia "Hello World" example code is building and including their own private header files (including but not limited by tools/sk_app folder which has includes to private headers). Bottom line: there is no clear separation of concerns. 

After 2 full days, I gave up to build of an independent hello world example linking skia as static library. It's really really hard to getting started, this is a red flag.

**Conclusion:** No, too complex, even hard to get started.

## Multimedia libraries / Vector graphics engine

See the list of multimedia libraries below.

### OpenGL / Libraries using OpenGL

Directly using OpenGL calls (which is not ideal). Or use much better alternatives like raylib, SDL or SFML media libraries. [SDL2](http://wiki.libsdl.org/FrontPage) is mature for sure.

SFML project didn't release a new release a long time ago (2 years old). However, SFML has a much better APIs, they claim. And should be easier to use with the native C++ interface/API.

#### SDL

**Conclusion:** *No Conclusion yet*

#### SFML

**Conclusion:** *No Conclusion yet*

#### raylib

[Raylib](https://www.raylib.com/) is a very nice a simple and easy-to-use, well documented library written in C. With no external dependencies.

There is already a seperate [raygui](https://github.com/raysan5/raygui) for GUI development.

**Conclusion:** *No Conclusion yet*

### Blend2D

No really yet mature. But looks very promissing, its a 2D vector graphics engine written in C++! No hardware acceleration yet, which is really bad for a vector engine.

**Conclusion:** *No Conclusion yet*

### OpenVG

OpenVG is part of the Khronos group, which is a plus. But it looks a bit too much enterprise ended. I don't see any packages or source code?  Also the project looks dead. But they claim OpenVG is meant for e-books readers and such.

**Conclusion:** No, dead-ish project.

### NanoVG

[NanoVG](https://github.com/memononen/nanovg) is a 2D vector drawing library on top of OpenGL for UI and visualization. Its very small, with a lean API.

**Conclusion:** *No Conclusion yet*

### ImGui

ImGui is a GUI framework but actually not traditional GUI application. Instead the backends are only hardware accelerated using OpenGL.

For some inspiration; there exists [Text Editor #1](https://github.com/BalazsJako/ImGuiColorTextEdit), [Text Editor #2](https://github.com/Rezonality/zep) created with Imgui.

* [Fonts documentation](https://github.com/ocornut/imgui/blob/master/docs/FONTS.md) of Imgui.

After testing the Imgui demo window using GLFW library + Vulkan backend, I discover a lot of mouse lag when dragging windows over the screen.
Also the application suffered quite some screen tearing (even with VSYNC on).

My expectation is that there are some huge synchronization issues with Imgui and OpenGL/Vulkan backends. Therefor is my advice is not to use Imgui for text rendering applications until futher notice. This is a shame.

**Conclusion:** No, Will NOT be used, some very strange performance issues under Linux (X11). Not production ready as main GUI.
