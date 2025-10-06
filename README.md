# I believe in sexpr supremacy

# In short

It sucks that such formats as `csv` are popular, yet there is no popular strictly tree-based format with minimal syntax. But this format exists and its called [s-expressions](https://stackoverflow.com/a/74172551)! The problem, however, is that this format is pretty unspecified and tied to a particular language. Also, there is a way to extend it to be usable, but it requires adding built-in types like `string` or `date` that remove beauty of the generality of the format alltogether!

This take on the format preserves basic sexpr as a main building block for the tree. Instead of built-in types it uses special *lexing directives* that allow to parse particular character sequences differently. This way, you encode any character sequence as you like without such fuss as commas, quotes or unnecessary escapes.


# About

**What it is**: minimal structured format based on [s-expressions](https://stackoverflow.com/a/74172551) with configurable lexing.

**What it isn't**: JSON/YAML/XML/INI/whatever replacement, not a programming language, not a ready-out-of-the-box solution for all your problems

**Why**: because I felt a need for universal, polyglot structured format, that will be semantically simpler than JSON, yet more powerful and open for interpretation.

**Goals (importance descending)**:
- Simplicity for both spec and implementation
- Flexibility: "porous" design lets you decide how you interpretet every each token
- Durability: implementation is simple enough and simplest C89 without stdlib dependency, drop this in every project you want or implement 1:1 alternative in your favorite imperative language
- Control: there is no "cut corners", you can encode anything you want, usually with less fuss compared to other formats

**Non-goals**:
- Writability (and to the lesser extent readability): sepxr generally doesn't read good, but we sacrifice this for generality and homogeneity
- Expressiveness: format is simple, but not easy; format is flexible, but not expressive out of the box, you need stuff => you interpret/encode it yourself
- Uniqueness: I really want to be this as general and boring as possible since this helps with existing tooling

# Examples

## Simple

The simple tree with the root node containing two words `Hello` and `world`:
```lisp
(Hello world)
```

More complex tree:
```lisp
(
    (version 1.0)
    (debug true)
    (timeout 30)
)
```

If you need a string, use an `s` [directive](#directives):
```lisp
(
    (title (`s|Lord of the rings))
    (author (`s|J. R. R. Tolkien))
    (publication (`s|29 July 1954))
)
```

## JSON-like

Consider the encoding of an arbitrary configuration as a list of pairs `key-value`:
```lisp
(
    (user: (
        (name: Alice)
        (roles: (admin tester))
        (active: true)
    ))
    (max_sessions: 5)
)
```

Corresponding JSON:
```json
{
    "user": {
        "name": "Alice",
        "roles": ["admin", "tester"],
        "active": true
    },
    "maxSessions": 5
}
```

As you see, there is no much difference. However, the same JSON could be encoded differently:

```lisp
(`skobki braces: "{([" "})]" |
    user: { 
        name: Alice
        roles: [admin tester]
        active: true
    }
    max_sessions: 5
)
```

Looks much more clean, isn't it? Thanks to `skobki` [directive](#directives), we are able to use different syntax to aid readability.

Also see [more examples](#more-examples) below.

# Spec

## Terminology

1. Sexpr -- a list, each containing payload or other sexpr
1. Payload -- data encoded in particular format with certain length
1. Token -- a view into payload; a token can be control or text
1. Character -- a part of the payload that is not further divisible in the current encoding.
1. Control set -- set of characters that dictates the structure of the document (the contents of resulting tree)
1. Delimiters -- set of characters that delimits text tokens
1. Braces -- set of characters that is used to contruct document tree nodes
1. Escape -- a character that is used to escape other characters from control set
1. Directive -- a character sequence with special syntax that is used to configure lexing in a particular area of sexpr
1. Node -- a metadata about payload, primarily used to link text tokens into graph
1. Document tree -- a collection of nodes

## Encoding

The primary encoding (as a most practical one) is UTF-8. The document is expected to have this encoding. For practical reasons, the control set of characters is assumed to be ASCII (`\u0000-\u007F`). No utf-8 normalization is performed.

Hovewer, the format itself doesn't restrict the use of other (even UTF-32) encodings, but this will complexify the implementation. The characters are not restricted to be encoded as multiple bytes, nor the control set or directives.

## Lexer

The lexical structure of the document is defined by the following EBNF
```EBNF
TEXT ::= CHARACTER+

Tree ::= DELIMITER* OPEN_BRACE Token* CLOSE_BRACE DELIMITER*
Token ::= 
    | TEXT
    | DELIMITER
    | ESCAPE CHARACTER
    | OPEN_BRACE Token* CLOSE_BRACE
```
Where `DELIMITER` is any character from delimiters control set, `ESCAPE` is a character from escape control and `OPEN_BRACE` and `CLOSE_BRACE` are pair of characters from braces control set.

### Control set

Control set is defined as follows
```ts
ControlSet = {
    delimiters: List C
    braces: List (C, C)
    escape: C
}
```

There are following restrictions on the set:
1. `C` is a character from (`\u0000-\u007F`)
1. Sets that are formed from these lists are disjoint
1. The total number of distinct control characters (|delimeters| + |braces| + 1 escape) MUST NOT exceed an implementation-defined constant N (e.g. 32)

Default configuration of the control set expected to be the following:
```ts
control_set: ControlSet = {
    delimiters = [
        0x20, // space
        0x09, // tab
        0x0a, // LF
        0x0d, // CR
    ],
    braces = [
        0x28, // open round brace
        0x29, // closed round brace
    ],
    escape = 0x60 // backtick
}
```

### Lexing process

The lexing process can be described as informal state-machine:
1. On EOF => flush
2. On character
    - If character in delimiters => flush, emit delimiter token
    - If character in braces and it is open and immediately next character is an escape => switch to directive parsing, then match `|`
    - If character in braces => flush, emit brace token
    - If character in escape => skip the current character and append the next character to current token
    - If character is non of the above => append it to the current token
Where `flush` is to emit the current token and clear its contents.

If directive is encountered and successfully parsed, parser executes an action that is [custom to every directive](#directives) and then switches back to general mode.

Note that in any unexpected case an error is reported and lexing stops.

### Directives

Directive is a sequence of characters, that starts with escape from the control set followed by the body of the directive, ending by the pipe symbol `|`.

By default, directive has the following grammar:
```EBNF
WS ::= 0x20 | 0x09 | 0x0a | 0x0d
HEX ::= [0-9a-f]
ESCAPED ::= "\\" ( "\\" | "\"" | "'" | "n" | "r" | "t" | "f" | "u" HEX HEX HEX HEX )
CHAR ::= 0x00-0x7F # excluding ESCAPED ascii range
C_STR ::= '"' (ESCAPED | CHAR)* '"'
IDENT ::= [a-z_]+
NUM ::=  [0-9] | [1-9][0-9]+

Directive ::= ESCAPE DirectiveBody '|'
DirectiveBody ::= 
    | IDENT
    | IDENT WS+ (CHAR | C_STR | NUM | WS)*
```

The main purpose of directives is to configure the lexer in some way for the rest of enclosing sexpr or till the EOF. The directive is in effect after pipe symbol of the directive and until the next matching brace that syntactically matches the beginning brace of enclosing sexpr. Note that this closing brace is expected immediately after the directive effect ends.

There are a number of different directives. To represent the currently used escape, the symbol `\` will be used in the following description of the directives. Note that the default symbol that is used for escape is backtick \`, but backslash is used primarily for illustrative purposes.

1. `\s|`. String directive. Directive forces the lexer to include every character that is not a matching closing brace of the enclosing sexpr. The escape could be used to escape the escape itself or the matching closing brace to continue consuming the input. Example:
```lisp
(\s|This is a string)
(\s| this also a string, but with \) being escaped)
(\s|As you could
    tell, there are no real
                    difficulties
writing a multiline string verbatim too)
```
Grammar:
```EBNF
Str ::= ESCAPE s WS* '|'
```

2. `\s SENTINEL|`. String with sentinel. Directive forces the lexer to include every character that doesn't match a sentinel character sequence or EOF. The escape could be used to escape the escape itself or the sentinel character sequence to continue consuming the input. Sentinel must not represent a character from a current control set. Example:
```lisp
(\s "EOF"|This string can contain anything: braces () and unbalanced ones ). Also some 
curlies-squares like {[with escapes \\ in them]} EOF)
(\s "\""|This string will go a long way until double quote is encountered")
(\s "\u0053\u004c\u004f\u0050"| The 0x53 0x4c 0x4f 0x50 is for \SLOP in UTF-8. Notice that we
can't use \SLOP as a word directly since it is a sentinel, but ) or 🍰 can be used SLOP)
```
Grammar:
```EBNF
StrSentinel ::= ESCAPE s WS+ C_STR WS* '|'
```

3. `\raw N|`. Raw payload. Directive forces the lexer to skip the next N bytes. N could be represented as an natural of particular size and can have an upper limit that is implementation defined. Escaping doesn't work while the directive is in effect, since there is no need for it. Example:
```lisp
(\raw 91|This woman 👩‍🚒 wears red hat. Wow! Also, anyone knows what `str` in JS? What about `\aboba`?)
```
Grammar:
```EBNF
Raw ::= ESCAPE raw WS+ NUM WS* '|'
```

4. `\skobki ...|`. Configuration directive. Directive forces to replace character class (delimiters, braces or escape) in the control set up until matching closing brace. When delimiters are changed, matching control brace still expected to be the a counterpart of beginning brace that has formed the enclosing sexpr. The replaced character class must obey [the rules](#control-set) for control set. Any clause from (`delimiters:`, `braces:`, `escape:`) must appear at most once. Configuration directive cannot be empty. Example:
```lisp
(\skobki braces: "{" "}"|
    Now we only accept curlies!
    {foo bar} {baz}
)
(\skobki braces: "{(" "})" escape: "$"| We can use $$ to represent escape and mask the
    closing matching brace $) now. Also, other sepxr: ({a b}{c d})
)
(\skobki delimiters: "-"|------The dashes are nonimportant and will be-----used to delimit, notice
that we don't have newline here so this token will contain it too)
```
Grammar:
```EBNF
Skobki ::= ESCAPE skobki WS+ (
    | delimiters: WS+ C_STR
    | braces: WS+ C_STR WS+ C_STR
    | escape: '"' CHAR '"'
)+ WS* '|'
```

5. `\ver N M K|`. Version directive. Upon encountering the directive, check if specified version in form of N.M.K (major, minor and patch) is less than or equal to
the version of skobki library. If the version is greater, an error is issued. Example:
```lisp
(\ver 1 0 0|
    something....
)
```
Grammar:
```EBNF
Ver ::= ESCAPE ver WS+ NUM WS+ NUM WS+ NUM WS* '|'
```

Other directive names are reserved for future use.

Directives (if any) must appear as an immediate character sequence after beginning brace of the sexpr. There can only be one directive in the sexpr, since there is only one beginning brace. However, each nested sexpr can have its own directive.

Directives can stack. That is, if some directive allows parsing in general mode after it is encountered (like \`skobki or \`ver directives), the next directive encountered
in the child sexpr can stack up with effect from parent directive. Effect of every directive is lexically scoped, that is, when scope of the directive ends on closing brace, its effect cancels. This means, that \`skobki configuration can be used locally
to parse some arbitrary part of document tree differently, preserving the configuration for other parts of the tree.

## Parser

Since lexical tokens are insufficient to properly manipulate the tree, the concept of node is introduced. Nodes are used to link text tokens into a tree. Node has the following structure:

```ts
Node = {
    start: index
    end: index
    count: natural
    parent: index
    next_sibling: index
}
```
where `index` is a descriptor that is semantically used reference to the element of tokens collection. Each `index` can have a default invalid value such as `-1`, `null` or others.

Indices `start` and `end` encode a span of tokens that compose the node. `count` is used to encode how many children given nodes has. `parent` is used to reference parent node of the current node. `next_sibling` is used to reference next sibling on the same level as the current node.

The children of a particular node need to go immediately after the node itself. "Immediately after" specifies correponding location in the node collection that composes the document tree. This is reminiscent of the in-order tree traversal of a parsed tree. This way, no children in the node itself is stored.

TODO: be explicit about errors

## Diagrams

TODO: lexer and parser as a diagrams with explicit links on a particular example

# More examples

TODO:
