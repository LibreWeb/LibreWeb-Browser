# LibreWeb Browser

<!-- Add badge: ![Matrix](https://img.shields.io/matrix/libreweb:matrix.melroy.org) -->

Decentralized Web-Browser; a revolution of the WWW.

What would you do different; if you could **reinvent** The Internet in 21st century? With all the knowledge and new technologies available today.

I was inspired by Douglas Engelbart, Tim Berners-Lee and Ted Nelson as well as projects like IPFS, Jekyll, ARPANET, and more.

*Note:* Project is still in development!

## Download

* [Download the latest release](https://gitlab.melroy.org/libreweb/browser/-/releases)

## Screens

![Browser Screenshot](./misc/browser_screenshot.png)  
![Browser Markdown Editor](./misc/browser_screenshot_2.png)

## Documentation

Visit the [dedicated documentation site](https://docs.libreweb.org) for user documentation.

## Ideas/Features

The current success criteria:

* Everyone should be able to easily **read** and **create** a site/blog/news page and publish the content online (without minimal technical knowledge);
* Built-in easy-to-use **editor** (whenever you want to publish some content without programming language knowledge);
* **Decentralized** (no single-point of failure or censorship), like: P2P, DHT and IPFS;
* *No* client-server approach (the client is also the server and visa versa) - think **mesh network**.
* **Encrypted** transfers;
* Data is stored **redundantly** within the network (no single-point of failure);
* **Versioning**/revisions of content and documenents (automatically solves broken 'links', that can't happy anymore);
* Publisher user should be able to add additional information about the document/page, eg. title or path (similar in how Jekyll is using the `YML` format for meta data)
* Human-readable source-code (eg. `Markdown`, could be extended as well);
* End-user is in control about the layout and styling (just like with e-books);
* Content is King;
* Fast and Extensible!

*Note:* Since HyperText (so is HTML) is not used, you can even ditch the HTTP protocol. However TLS, for encryption, can still be used.

## Developers

Decentralized Browser is written C++ together with some [libraries](/lib). It's using the [cmark-gfm](https://github.com/github/cmark-gfm) library for example, which is used for CommonMark (markdown) parsing.

We're using markdown as the source-code of the content/site. No HTML and JavaScript anymore, content is king after all.

LibreWeb Browser is using [Gnome GTK3](https://developer.gnome.org/gtk3/stable/) as UI framework.

### Development Environment

I'm using VSCodium editor, with the following extensions installed: `C/C++`, `CMake`, `CMake Tools`, `PlantUML`, `Markdown All in One`, `vscode-icons` and `GitLab Workflow`.

### Build Dependencies

For the build you need at least:

* GCC 9 or higher (GCC 8 should also work, but not adviced. Package: `build-essential`)
* CMake (Package: `cmake`)
* Ninja build system (Package: `ninja-build`)
* GTK & Pango (including C++ bindings):
  * Package: `libgtkmm-3.0-dev` under Debian based distros

### Developer Docs

See latest [Developer Docs](https://gitlab.melroy.org/libreweb/browser/-/jobs/artifacts/master/file/build/docs/html/index.html?job=doxygen).

### Research

For [research document](https://gitlab.melroy.org/libreweb/research_lab/-/blob/master/research.md) plus findings including explanation (like [diagrams](https://gitlab.melroy.org/libreweb/research_lab/-/blob/master/diagrams.md)) see the:

* [LibreWeb Research Lab Project](https://gitlab.melroy.org/libreweb/research_lab/-/tree/master)
