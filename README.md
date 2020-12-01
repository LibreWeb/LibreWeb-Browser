# Browser for DWWW

Decentralized Browser; a revolution of the WWW.

What would you do different; if you could **reinvent** The Internet in 21st century? With all the knowledge and new technologies available today.

I was inspired by Douglas Engelbart, Tim Berners-Lee and Ted Nelson as well as projects like IPFS, Jekyll, ARPANET, and more.

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

## Devs

Decentralized Browser written in C++20 with C libraries. And using the [cmark-gfm](https://github.com/github/cmark-gfm) library, used for CommonMark (markdown) parsing.

Browser is using GTK as UI library including Pango & Cairo for text drawing and manipulation.

For now we will use markdown as the source of the site. No HTML and JavaScript anymore, content is king after all.

### Development Environment

I'm using VSCodium editor, with the following extensions installed: `C/C++`, `Cmake` and `Cmake Tools` .

### Depedencies

For the build you need at least:

* GCC 9 or higher (GCC 8 should also work)
* CMake
* GTK & Cairo & Pango (including C++ bindings):
    - Install: `libgtkmm-3.0-dev` under Debian based distros

For release packages you also need:

* CPack

### Diagrams

There existing several design and/or research diagrams of Browser by using PlantUML. Sometimes words aren't enough to explain yourself.

[Check-out the diagrams page](docs/diagrams.md).

### Drawing/Graphics rendering engine/GUI

We are currently in an exploration phase about which 2D (vector) graphics rendering library we should use.

*Currently:* GTK + Cairo + Pango, which works fine and is fast!

See [research document](docs/research.md) (also in markdown) for more information and conclusions based on facts.

### Decentralized Web

We are also currently in an exploration page about which kind of P2P or decentralized network solution is the best fit.

See [research document](docs/research.md) (also in markdown) for more information and conclusions based on facts.