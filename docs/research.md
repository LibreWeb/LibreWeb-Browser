# Research topics


## Graphical frameworks & renderers

*Goal:* We want to draw text on the screen as fast as possible. Skipping the HTML parser step, meaning: `Markdown` -> `screen` instead of: `Markdown` -> `HTML` / `CSS` -> `screen`.

## Skia / QtSkia

Skia is a 2D graphics library. Using for drwaing text, geometries, and images. Support backends CPU raster, OpenGL/Vulkan. Both for display and SVG/PDF (can also read those files). 
Skia is also used for Google Chrome/Chrome OS, Firefox, Android, LibreOffice and more.

[Studies](https://www.facebook.com/notes/beagleboardorg-foundation/comparing-html-rendering-performance-with-qtwebkit-and-qt-native-classes/439968524361/) has shown with the Matrix GUI (developed by Texas Instruments for ARM based SoCs) that (Qt)WebKit is much much faster in rendering then the built-in Qt renderer.

Skia supports several (platform-dependent) back-ends, including one for CPU-based software rasterization, one for Portable Document Format (PDF) output, and one for GPU-accelerated OpenGL, OpenGL ES, Vulkan, and Metal one.

Doing advanced text manipulation in Skia can be achieved by combining [`SkTextBlob`](https://api.skia.org/classSkTextBlob.html#details), [`SkPaint`](https://skia.org/user/api/skpaint_overview) and [`SkTypeface`](https://api.skia.org/classSkFont.html#details) / [`SkFontStyle`](https://api.skia.org/classSkFontStyle.html).

* [Pre-build Skia library](https://github.com/aseprite/skia/releases)
* [Desktop Library using Skia](https://github.com/aseprite/laf)
* [Maybe look into RichTextKit](https://github.com/toptensoftware/RichTextKit) (although using SkiaSharp bleh)
* [API overview](https://skia.org/user/api/)

Skia is ported to Qt, called QtSkia. The Qt WebEgine renders web pages by using Skia and is not using QPainter or Qt for this purpose. 

**Conclusion:** *No Conclusion yet* - Looks like a big no.

Skia project is a mess to setup and build against. Beining one of the most used graphical libaries, its also the one with the biggest mess. There are no packages delivered, not static or dynamically linked. Meaning building an application and link to Skia is very hard.

Even the Skia "Hello World" example code is building and including their own private header files (including but not limited by tools/sk_app folder which has includes to private headers). Bottom line: there is no clear separation of concerns. It's really hard to get something compiled with a static library from aseprite binaries. The static library is properbly build against a different GLIBC: undefined reference to symbol 'pthread_getspecific@@GLIBC_2.2.5'.

There is code in Skia which is not even C++10 lot alone C++14 complient.

## GTK+ / Cairo

GTK+ is using the Cairo 2D graphics renderer using rasterization (CPU). But Cairo also including backends for acceleration and for vector output formats. Can be used for both display and export to PDF/SVG.

**Conclusion:** *No Conclusion yet* - Under investigation

## NanoVG

[NanoVG](https://github.com/memononen/nanovg) is a 2D vector drawing library on top of OpenGL for UI and visualization. Its very small, with a lean API.

**Conclusion:** *No Conclusion yet* - Under investigation

### Qt

Qt has many Qt-classes, features and APIs for drawing and rendering. See the following sub-items:

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

**Conclusion:** Will NOT be used.


### Qt examples/links

* [Drawing text](https://github.com/radekp/qt/blob/master/src/gui/text/qtextlayout.cpp#L1114) with QPainter.
* [Mifit Text render](https://github.com/mifit/mifit/blob/master/libs/opengl/Text.cpp) for a code example. 
* [Drawing Text/line using QPainter](https://www.youtube.com/watch?v=tc3nlNEAdig) (video).
By `baysmith`: 

    It generates image atlas dynamically using a QPainter to draw to a texture which is displayed with quads. I don't know how much less efficient it is to draw the characters to the image on demand rather than prebaking, but I need the flexibility to change the font to anything the system provides.
* [Calligra](https://github.com/KDE/calligra) Word processor using Qt, maybe also creating their own text painting as well?

## ImGui

For some inspiration; there exists [Text Editor #1](https://github.com/BalazsJako/ImGuiColorTextEdit), [Text Editor #2](https://github.com/Rezonality/zep) created with Imgui.

* [Fonts documentation](https://github.com/ocornut/imgui/blob/master/docs/FONTS.md) of Imgui.

After testing the Imgui demo window using GLFW library + Vulkan backend, I discover a lot of mouse lag when dragging windows over the screen.
Also the application suffered quite some screen tearing (even with VSYNC on).

My expectation is that there are some huge synchronization issues with Imgui and OpenGL/Vulkan backends. Therefor is my advice is not to use Imgui for text rendering applications until futher notice.

**Conclusion:** Will NOT be used.

