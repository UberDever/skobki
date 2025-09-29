#define SKOBKI_IMPL

typedef unsigned long skobki_uint;

enum skobki_error_t {
  SKOBKI_ERROR_INVALID_ESCAPE = -1,
  SKOBKI_ERROR_NOMEM = -2,
  SKOBKI_ERROR_INVALID_DIRECTIVE = -3,
  SKOBKI_ERROR_TOO_MUCH_PUNCT = -4
};

enum skobki_type_t {
  SKOBKI_TYPE_CHUNK,
  SKOBKI_TYPE_SEXPR
};

struct skobki_token_t {
  enum skobki_type_t type;
  skobki_uint start;
  skobki_uint end;
  skobki_uint count;
  skobki_uint parent;
  skobki_uint next_sibling;
};

#define SKOBKI_MAX_PUNCT_TYPES 16

struct skobki_span_t {
  skobki_uint start;
  skobki_uint len;
};

struct skobki_config_t {
  unsigned char punct[SKOBKI_MAX_PUNCT_TYPES];
  struct skobki_span_t delimiters;
  struct skobki_span_t brackets;
  struct skobki_span_t escape;
};

struct skobki_lexer_t {
  const char* input;
  struct skobki_span_t view;
  skobki_uint cur;

  skobki_uint is_eof;
  enum skobki_error_t error;
  struct skobki_config_t config;
};

void skobki_init_lexer(struct skobki_lexer_t* l);

skobki_uint skobki_tokenize(
    struct skobki_lexer_t* l,
    const char* input,
    struct skobki_span_t input_view,
    struct skobki_token_t* out_tokens,
    skobki_uint out_tokens_max_len);

#ifdef SKOBKI_IMPL

typedef skobki_uint size_t;
typedef size_t bool;
#define true  1
#define false 0

enum token_type_t {
  token_type_char,
  token_type_delimiter,
  token_type_brackets,
  token_type_escape
};

struct token_t {
  size_t type;
  struct skobki_span_t contents;
};

struct skobki_config_t default_config(void) {
  struct skobki_config_t r = {0};
  size_t i = 0;
  size_t cur_len = 0;
  r.delimiters.start = i;
  r.punct[i++] = ' ';
  cur_len++;
  r.punct[i++] = '\n';
  cur_len++;
  r.delimiters.len = cur_len;

  r.brackets.start = i;
  cur_len = 0;
  r.punct[i++] = '{';
  cur_len++;
  r.punct[i++] = '}';
  cur_len++;
  r.brackets.len = cur_len;

  r.escape.start = i;
  cur_len = 0;
  r.punct[i++] = '\\';
  cur_len++;
  r.escape.len = cur_len;
  return r;
}

bool strings_equal(
    const char* input_lhs,
    struct skobki_span_t lhs,
    const char* input_rhs,
    struct skobki_span_t rhs) {
  size_t i = 0;
  if (lhs.len != rhs.len) {
    return false;
  }
  for (i = 0; i < lhs.len; ++i) {
    if (input_lhs[lhs.start + i] != input_rhs[rhs.start + i]) {
      return false;
    }
  }
  return true;
}

void advance(struct skobki_lexer_t* l) {
  l->cur++;
  if (l->cur >= l->view.len) {
    l->is_eof = true;
    return;
  }
}

char hex_char_to_value(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  } else if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  } else {
    return -1;
  }
}

bool hex_string_to_byte(const char* contents, struct skobki_span_t str, unsigned char* byte) {
  int high, low;
  if (str.len != 2) {
    return false;
  }
  high = hex_char_to_value(contents[str.start + 0]);
  low = hex_char_to_value(contents[str.start + 1]);
  if (high == -1 || low == -1) {
    return false;
  }
  *byte = (unsigned char)((high << 4) | low);
  return true;
}

bool peek_token_char_recognizable(struct skobki_lexer_t l, struct skobki_span_t range) {
  size_t i = 0;
  char c;
  if (l.is_eof || l.error != 0) {
    return false;
  }
  c = l.input[l.view.start + l.cur];
  for (i = 0; i <= range.len; ++i) {
    if (c == l.config.punct[range.start + i]) {
      return true;
    }
  }
  return false;
}

enum token_type_t peek_token_char(struct skobki_lexer_t l) {
  if (l.is_eof || l.error != 0) {
    return token_type_char;
  }
  if (peek_token_char_recognizable(l, l.config.delimiters)) {
    return token_type_delimiter;
  }
  if (peek_token_char_recognizable(l, l.config.brackets)) {
    return token_type_brackets;
  }
  if (peek_token_char_recognizable(l, l.config.escape)) {
    return token_type_escape;
  }
  return token_type_char;
}

struct token_t next_token_char(struct skobki_lexer_t* l, enum token_type_t type) {
  struct token_t token = {0};

  if (l->is_eof || l->error != 0) {
    return token;
  }

  if (type == token_type_escape) {
    advance(l);
    if (l->is_eof) {
      l->error = SKOBKI_ERROR_INVALID_ESCAPE;
      return token;
    }

    token.type = token_type_char;
    goto fill_token;
  }

fill_token:
  token.type = type;
  token.contents.start = l->view.start + l->cur;
  token.contents.len = 1;
  advance(l);

  return token;
}

struct token_t next_token(struct skobki_lexer_t* l) {
  struct token_t token = {0};
  enum token_type_t type;

  if (l->is_eof || l->error != 0) {
    return token;
  }
  type = peek_token_char(*l);
  if (type != token_type_char) {
    return next_token_char(l, type);
  }

  token.contents.start = l->view.start + l->cur;
  token.type = token_type_char;
  while (1) {
    type = peek_token_char(*l);
    if (l->is_eof || l->error != 0) {
      return token;
    }
    if (type == token_type_char) {
      token.contents.len++;
      advance(l);
      continue;
    }

    return token;
  }
}

struct skobki_span_t update_lexer_directive(
    struct skobki_lexer_t* l, unsigned char* prev_directive, size_t prev_directive_place) {
  struct skobki_span_t new_directive = {0};
  struct token_t t;
  size_t len = 0;
  const char* NEWLINE = "\n";
  static struct skobki_span_t NEWLINE_VIEW = {0, 1};

  while (1) {
    unsigned char byte;
    t = next_token(l);
    if (l->is_eof || l->error != 0) {
      return new_directive;
    }
    if (t.type == token_type_delimiter
        && strings_equal(l->input, t.contents, NEWLINE, NEWLINE_VIEW)) {
      new_directive.start = prev_directive_place;
      new_directive.len = len;
      return new_directive;
    }
    if (t.type == token_type_delimiter) {
      continue;
    }
    if (t.type == token_type_brackets) {
      return new_directive;
    }
    if (t.type == token_type_escape) {
      l->error = SKOBKI_ERROR_INVALID_ESCAPE;
      return new_directive;
    }
    if (!hex_string_to_byte(l->input, t.contents, &byte)) {
      l->error = SKOBKI_ERROR_INVALID_DIRECTIVE;
      return new_directive;
    }
    prev_directive[len++] = byte;
  }

  return new_directive;
}

struct directive_sink_t {
  struct skobki_span_t expected;
  struct skobki_span_t* to_fill;
};

void setup_lexer_from_file(struct skobki_lexer_t* l) {
  struct skobki_span_t delimiters;
  struct skobki_span_t brackets;
  struct skobki_span_t escape;
  struct skobki_config_t config;
  struct token_t t;
  size_t punct_len = 0;
  size_t i = 0;
  size_t prev_punct_place = 0;
  struct skobki_span_t new_directive;
  const size_t directives_count = 3;
  struct directive_sink_t directives[3];

  const char* all_directives = "delimiters:brackets:escape:";

  delimiters.start = 0;
  delimiters.len = 11;
  brackets.start = 11;
  brackets.len = 9;
  escape.start = 20;
  escape.len = 7;
  directives[0].expected = delimiters;
  directives[0].to_fill = &config.delimiters;
  directives[1].expected = brackets;
  directives[1].to_fill = &config.brackets;
  directives[2].expected = escape;
  directives[2].to_fill = &config.escape;

  while (1) {
    enum token_type_t type = peek_token_char(*l);
    if (l->is_eof || l->error != 0) {
      return;
    }
    if (type == token_type_escape) {
      l->error = SKOBKI_ERROR_INVALID_ESCAPE;
      return;
    }
    if (type == token_type_char) {
      t = next_token(l);
      if (l->is_eof || l->error != 0) {
        return;
      }
      for (i = 0; i < directives_count; ++i) {
        if (directives[i].to_fill == 0) {
          continue;
        }
        if (strings_equal(all_directives, directives[i].expected, l->input, t.contents)) {
          break;
        }
      }
      if (i == directives_count) {
        l->error = SKOBKI_ERROR_INVALID_DIRECTIVE;
        return;
      }

      new_directive = update_lexer_directive(l, config.punct + prev_punct_place, prev_punct_place);
      if (new_directive.len == 0) {
        l->error = SKOBKI_ERROR_INVALID_DIRECTIVE;
        return;
      }
      punct_len += new_directive.len;
      if (punct_len >= SKOBKI_MAX_PUNCT_TYPES) {
        l->error = SKOBKI_ERROR_TOO_MUCH_PUNCT;
        return;
      }
      *directives[i].to_fill = new_directive;
      directives[i].to_fill = 0;
      prev_punct_place += new_directive.len;
      continue;
    }
    if (type == token_type_brackets) {
      l->config = config;
      return;
    }
    advance(l);
    continue;
  }
}

void skobki_init_lexer(struct skobki_lexer_t* l) {
  l->input = 0;
  l->view.start = 0;
  l->view.len = 0;
  l->cur = 0;
  l->is_eof = false;
  l->error = 0;
  l->config = default_config();
}

skobki_uint skobki_tokenize(
    struct skobki_lexer_t* l,
    const char* input,
    struct skobki_span_t input_view,
    struct skobki_token_t* out_tokens,
    size_t out_tokens_max_len) {
  l->input = input;
  l->view = input_view;
  setup_lexer_from_file(l);

  return 0;
}

#endif
