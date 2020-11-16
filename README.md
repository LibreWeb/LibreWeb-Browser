# Browser for DWWW

Decentralized Browser; a revolution of the WWW.

What would you do, if you could **reinvent** The Internet in 21st century? With all the knowledge and new technologies available today.

I was inspired by Douglas Engelbart, Ted Nelson as well as projects like IPFS, Jekyll, ARPANET, and more.

*Note:* Project is WIP!

## Ideas/Features

The current plan:

* Everyone should be able to easily **read** and **create** a site/blog/news page and publish the content online (without minimal technical knowledge);
* Built-in easy-to-use **editor** (whenever you want to publish some content without programming language knowledge);
* **Decentralized** (no single failure or censorship), like: P2P, DHT and IPFS;
* *No* client-server approach (the client is also the server and visa versa) - think **mesh network**.
* **Encrypted** transfers;
* Data is stored **redundantly** within the network (no single failure);
* **Versioning**/revisions of content and documenents (automatically solves broken 'links', that can't happy anymore);
* Publisher user should be able to add additional information about the document/page, eg. title or path (similar in how Jekyll is using the `YML` format for meta data)
* Human-readable source-code (eg. `Markdown`, could be extended as well);
* End-user is in control about the layout and styling (just like with e-books);
* Content is King;
* Fast and Extensible!

*Note:* Since HyperText (so is HTML) is not used, you can even ditch the HTTP protocol. However TLS, for encryption, can still be used.

## Text on Screen Flowcharts

**Prepare steps**

```plantuml
(*) --> "Load fonts glyphs (ttf)/default font"
--> "Create Font Atlas"
--> "Create Viewport"
-->[Ready for render] (*)
```

**Text rendering**

```plantuml
(*) --> "Get file online /\nRead from disk"
-->[Content in memory] "Parse document"
-->[AST output] "Convert AST to Bitmap struct"
-->[Ready for painter] "Paint text on Viewport"
-->[Text visable on Screen] (*)
```

*ttf* = TrueType Font

*AST* = Abstract Syntax Tree

## Devs

Decentralized Browser written in C and C++20. And using the [cmark-gfm](https://github.com/github/cmark-gfm) library, used for CommonMark (markdown) parsing.

The GUI-toolkit or 2D/3D engine used displaying the content is not yet decided. Can be anything really, like: Qt, wxWidgets or Imgui.

We can also still change the language of the source code (iso markdown). Atleast no HTML and JavaScript anymore, content is king after all.

### GUIs

#### Qt

Qt [Rich Text Processing](https://doc.qt.io/qt-5/richtext.html)/QTextDocument can't be used, since that only supports HTML for rich text. Or the use of built-in markdown parser, in both cases doesn't give us the right flexibility we need. 

Meaning we can try to use low-level [QPainter](https://doc.qt.io/qt-5/qpainter.html) calls on a viewport/paint device (like QPixmap, QWidget, QPicture and QOpenGLPaintDevice), see [Qt GUI](https://doc.qt.io/qt-5/qtgui-index.html) module. We can also use Qt Quick Scene Graph for OpenGL rendering. Or use 2D renderer using either raster paint engine (without OpenGL calls).

But first let's try using [QGraphicsTextItem](https://doc.qt.io/qt-5/qgraphicstextitem.html) on a [QGraphicsScene](https://doc.qt.io/qt-5/qgraphicsscene.html).

Examples:

* [Drawing text](https://github.com/radekp/qt/blob/master/src/gui/text/qtextlayout.cpp#L1114) with QPainter.
* [Mifit Text render](https://github.com/mifit/mifit/blob/master/libs/opengl/Text.cpp) for a code example. 
* [Drawing Text/line using QPainter](https://www.youtube.com/watch?v=tc3nlNEAdig) (video).

By `baysmith`: 

    It generates image atlas dynamically using a QPainter to draw to a texture which is displayed with quads. I don't know how much less efficient it is to draw the characters to the image on demand rather than prebaking, but I need the flexibility to change the font to anything the system provides.

* [Calligra](https://github.com/KDE/calligra) Word processor using Qt, maybe also creating their own text painting as well?

#### Dear Imgui

[Imgui](https://github.com/ocornut/imgui) is used for Games but also applications. For example the Unity Editor is using Imgui!

For some inspiration; there exists [Text Editor #1](https://github.com/BalazsJako/ImGuiColorTextEdit), [Text Editor #2](https://github.com/Rezonality/zep) created with Imgui.

But there are many more demos and projects out there using Imgui!

See also [Fonts documentation](https://github.com/ocornut/imgui/blob/master/docs/FONTS.md) of Imgui.