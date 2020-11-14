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

## Top level design

```plantuml
(*) --> "Get file online /\nRead from disk"
-->[Doc in memory] "Parse document (CommonMark)"
-->[AST output] "Render"
-->[Layout] "Draw on screen"
-->[Output visable in GUI] (*)
```

## Devs

Decentralized Browser written in C and C++20. And using the [cmark-gfm](https://github.com/github/cmark-gfm) library, used for CommonMark (markdown) parsing.

The GUI-toolkit or 2D/3D engine used displaying the content is not yet decided. Can be anything really, like: Qt, wxWidgets or Imgui.

We can also still change the language of the source code (iso markdown). Atleast no HTML and JavaScript anymore, content is king after all.

### GUIs

#### Qt

Qt [Rich Text Processing](https://doc.qt.io/qt-5/richtext.html) can't be used, since that only supports HTML to rich text. Or you need to use the built-in markdown parser, in both cases doesn't give use the right flexibility we need.

See [Mifit Text render](https://github.com/mifit/mifit/blob/master/libs/opengl/Text.cpp) example. 

From `baysmith`: It generates image atlas dynamically using a QPainter to draw to a texture which is displayed with quads. I don't know how much less efficient it is to draw the characters to the image on demand rather than prebaking, but I need the flexibility to change the font to anything the system provides.

See also [Calligra](https://github.com/KDE/calligra) Word processor using Qt, maybe also creating their own text as well?

#### Dear Imgui

Imgui is used for Games but also applications. For example the Unity Editor is using Imgui!

For some inspiration; there exists [Text Editor #1](https://github.com/BalazsJako/ImGuiColorTextEdit), [Text Editor #2](https://github.com/Rezonality/zep) created with Imgui.

But there are many more demos and projects out there using Imgui!
