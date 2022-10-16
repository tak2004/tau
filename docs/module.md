# Module

## Format

The module file is a **.zip** file compressed with **deflate/64**. The archive contains a **"module.fb"** and a AST file. The "module.fb" file describes the module content and meta data. It also tells you where the data of the AST file can be located.

Both files are handled by flatbuffer serialization to reduce the dependencies.

You can add additional files on-demand.