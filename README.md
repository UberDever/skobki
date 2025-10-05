# I believe in sexpr supremacy

# Spec

## About

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

## Terminology

1. Sexpr -- a list, each containing payload or other sexpr
1. Payload -- data encoded in particular format with certain length
1. Token -- a view on the payload described by the pair of positions (start, end); can be control or text
1. Character -- a part of the payload that is not furhter divisible in the current encoding.
1. Control set -- set of characters that dictates the structure of the document (the contents of resuling tree)
1. Delimiters -- set of characters that delimits text tokens
1. Braces -- set of characters that is used to contruct document tree nodes
1. Escape -- a character that is used to escape other characters from control set
1. Directive -- a clause with special syntax that is used to configure lexing in a particular area of sexpr
1. Node -- a metadata about payload, primarily used to link text tokens into graph
1. Document tree -- a collection of nodes

## Encoding

The primary encoding (as a most practical one) is UTF-8. The document is expected to have this encoding. For practical reasons, the control set of characters is assumed to be ASCII (`\u0000-\u007F`). No utf-8 normalization is performed.

Hovewer, the format itself doesn't restrict the use of other (even UTF-32) encodings, but this will complexify the implementation. The characters are not restricted to be encoded as multiple bytes, nor the control set or directives.

## Lexer

### Control set

Control set is defined as follows
```
    ControlSet = {
        delimiters: List C
        braces: List (C, C)
        Escape: C
    }
```

There are following restrictions on the set:
1. `C` is a character from (`\u0000-\u007F`)
1. Sets that are formed from these lists are disjoint
1. Sum of lengths of every set is expected to be less than some arbitrary N that is implementation-defined

Default configuration of the control set expected to be the following:
```
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
    - If character in braces => flush, emit brace token
    - If character in escape => skip the current character and append the next character to current token
    - If character is `[` and followed by escape => switch to directive parsing, then match `]`
    - If character is non of the above => append it to the current token
Where `flush` is to emit the current token and clear its contents.

If directive is encountered and successfully parsed, parser executes an action that is [custom to every directive](#directives) and then switches back to general mode.

Note that in any unexpected case an error is reported and lexing stops.

### Directives

Directive is a sequence of characters, enclosed in square brackets `[`, `]` with the open bracket followed immediately by escape from the control set and a sequence of characters designating identifier.

By default, directive has the following grammar:
```
WS ::= 0x20 | 0x09 | 0x0a | 0x0d
HEX ::= [0-9a-f]
ESCAPED ::= "\\" ( "\\" | "\"" | "'" | "n" | "r" | "t" | "f" | "u" HEX HEX HEX HEX )
CHAR ::= 0x00-0x7F # excluding ESCAPED ascii range
C_STR ::= '"' (ESCAPED | CHAR)* '"'
IDENT ::= [a-bA-B_]+
NUM ::=  [0-9] | [1-9][0-9]+

Directive ::= '[' ESCAPE IDENT WS+ (CHAR | C_STR | NUM | WS)* ']'
```

The main purpose of directives is to configure the lexer in some way for the rest of enclosing sexpr or till the EOF. The directive is in effect after closing bracket of the directive and until the next matching brace that syntactically matches the beginning brace of enclosing sexpr. Note that this closing brace is expected immediately after the directive effect ends.

There are a number of different directives. To represent the currently used escape, the symbol `\` will be used in the following description of the directives. Note that the default symbol that is used for escape is backtick \`

1. `[\str]`. String directive. Directive forces the lexer to include every character that is not a matching closing brace of the enclosing sexpr. The escape could be used to escape the escape itself or the matching closing brace to continue consuming the input. Example:
```
    ([\str]This is a string)
    ([\str] this also a string, but with \) being escaped)
    ([\str]As you could
        tell, there are no real
                        difficulties
    writing a multiline string verbatim too)
```
Grammar:
```
    Str ::= '[' ESCAPE str WS* ']'
```

2. `[\str SENTINEL]`. String with sentinel. Directive forces the lexer to include every character that doesn't match a sentinel character sequence or EOF. The escape could be used to escape the escape itself or the sentinel character sequence to continue consuming the input. Sentinel must not represent a character from a current control set. Example:
```
    ([\str "EOF"]This string can contain anything: braces () and unbalanced ones ). Also some 
    curlies-squares like {[with escapes \\ in them]} EOF)
    ([\str "\""]This string will go a long way until double quote is encountered")
    ([\str "\u0053\u004c\u004f\u0050"] The 0x53 0x4c 0x4f 0x50 is for \SLOP in UTF-8. Notice that we
    can't use \SLOP as a word directly since it is a sentinel, but ) or 🍰 can be used SLOP)
```
Grammar:
```
    StrSentinel ::= '[' ESCAPE str WS+ C_STR WS* ']'
```

3. `[\raw N]`. Raw payload. Directive forces the lexer to include every character up until certain count N, where N is a count of characters in selected encoding. N could be represented as an natural of particular size and can have an upper limit that is implementation defined. For UTF-8 it is the count of codepoints. Escaping doesn't work while the directive is in effect, since there is no need for it. Example:
```
    ([\raw 91]This woman 👩‍🚒 wears red hat. Wow! Also, anyone knows what `str` in JS? What about `\aboba`?)
```
Grammar:
```
    Raw ::= '[' ESCAPE raw WS+ NUM WS* ']'
```

4. `[\skobki ...]`. Configuration directive. Directive forces to replace character class (delimiters, braces or escape) in the control set up until matching closing brace. When delimiters are changed, matching control brace still expected to be the a counterpart of begining brace that has formed the enclosing sexpr. The replaced character class must obey [the rules](#control-set) for control set. Any clause from (`delimiters:`, `braces:`, `escape:`) must appear at most once. Configuration directive cannot be empty. Example:
```
    (
        [\skobki braces: "{" "}"]
        Now we only accept curlies!
        {foo bar} {baz}
    )
    ([\skobki braces: "{(" "})" escape: "$"] We can use $$ to represent escape and mask the
        closing matching brace $) now. Also, other sepxr: ({a b}{c d})
    )
    ([\skobki delimiters: "-"]------The dashes are nonimportant and will be-----used to delimit, notice
    that we don't have newline here so this token will contain it too)
```
Grammar:
```
    Skobki ::= '[' ESCAPE skobki WS+ (
        | delimiters: WS+ C_STR
        | braces: WS+ C_STR WS+ C_STR
        | escape: '"' CHAR '"'
    )+ WS+ ']'
```

Other directive names are reserved for future use.

TODO: directive position and count; stacking
TODO: versioning

## Parser

TODO: document ebnf