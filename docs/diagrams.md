# Diagrams

List of sequence/UML diagrams for design and/or research reasons. Could may be used in a whitepaper if still relevant.

## High-level Design

```plantuml
(*) --> "Fetch source document from a P2P network"
--> "Parse markdown document"
-->[AST model output] "Convert AST to X & Y coords with text/drawing items"
--> "Add items to a drawing scene"
--> "Render the scene on screen"
-->[User sees the output on screen] (*)
```

*P2P* = Peer-to-Peer (decentralized networks)

*AST* = Abstract Syntax Tree

## Text on Screen Flowcharts

Folowing diagrams are only applicable when you need to draw text on the screen without supporting library.

However, most GUI libraries does support you in drawing text on a screen.

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
-->[AST model output] "Convert AST to Bitmap struct"
-->[Ready for painter] "Paint text on Viewport"
-->[Text visable on Screen] (*)
```

*ttf* = TrueType Font

*AST* = Abstract Syntax Tree