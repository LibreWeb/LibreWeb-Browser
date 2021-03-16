# LibreWeb Browser

[![Join the chat at https://gitter.im/LibreWeb/Browser](https://badges.gitter.im/LibreWeb/Browser.svg)](https://gitter.im/LibreWeb/Browser?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Decentralized Web-Browser; a revolution of the WWW.

What would you do different; if you could **reinvent** The Internet in 21st century? With all the knowledge and new technologies available today.

I was inspired by Douglas Engelbart, Tim Berners-Lee and Ted Nelson as well as projects like IPFS, Jekyll, ARPANET, and more.

*Note:* Project is still in Alpha phase!

## Download

* [Download the latest release](https://gitlab.melroy.org/libreweb/browser/-/releases)

## Screens

![Browser Screenshot](./misc/browser_screenshot.png)  
![Browser Markdown Editor](./misc/browser_screenshot_2.png)

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

Decentralized Browser written in C++20 with C libraries. And using the [cmark-gfm](https://github.com/github/cmark-gfm) library, used for CommonMark (markdown) parsing.

Browser is using GTK 3 as UI library including Pango & Cairo for text drawing and manipulation.

For now we will use markdown as the source of the site. No HTML and JavaScript anymore, content is king after all.

### Development Environment

I'm using VSCodium editor, with the following extensions installed: `C/C++`, `CMake`, `CMake Tools`, `PlantUML`, `Markdown All in One`, `vscode-icons` and `GitLab Workflow`.

### Build Dependencies

For the build you need at least:

* GCC 9 or higher (GCC 8 should also work, but not adviced. Package: `build-essential`)
* CMake (Package: `cmake`)
* Ninja build system (Package: `ninja-build`)
* GTK & Cairo & Pango (including C++ bindings):
  * Package: `libgtkmm-3.0-dev` under Debian based distros

### Documentation

See latest [Developer Docs](https://gitlab.melroy.org/libreweb/browser/-/jobs/artifacts/master/file/build/docs/html/index.html?job=doxygen).

### Research

For [research document](https://gitlab.melroy.org/libreweb/research_lab/-/blob/master/research.md) plus findings including explanation (like [diagrams](https://gitlab.melroy.org/libreweb/research_lab/-/blob/master/diagrams.md)) see the:

* [LibreWeb Research Lab Project](https://gitlab.melroy.org/libreweb/research_lab/-/tree/master)
