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

Following diagram display the high-level flow/activity of drawing text on a screen.

Most GUI libraries do have a text layout/rendering engine built-in.

### Drawing Activity Diagram*

```plantuml
(*) --> "Get file online /\nRead from disk"
-->[Content in memory] "Parse document"
-->[AST model output] "Convert AST to Text/Pango Markup"
-->[Ready for draw] "Add text to TextView buffer"
-->[Text visable on Screen] (*)
```

*AST* = Abstract Syntax Tree