#define SKOBKI_IMPL

#define INVALID_ESCAPE -10

#ifdef SKOBKI_IMPL

typedef unsigned long size_t;
typedef size_t bool;
#define true  1
#define false 0

#define MAX_PUNCT_TYPES 8

struct config_t {
  char delimiters[MAX_PUNCT_TYPES];
  size_t delimiters_len;
  char escapes[MAX_PUNCT_TYPES];
  size_t escapes_len;
};

struct span_t {
  const char* start;
  size_t len;
};

enum token_type_t {
  token_type_char,
  token_type_delimiter,
  token_type_escape
};

struct token_t {
  size_t type;
  struct span_t contents;
};

struct lexer_t {
  struct span_t input;
  size_t cur;

  size_t is_eof;
  int error;
  struct config_t config;
};

struct config_t default_config(void) {
  struct config_t r = {0};
  r.delimiters[0] = ' ';
  r.delimiters[1] = '\n';
  r.delimiters[2] = '{';
  r.delimiters[3] = '}';
  r.delimiters_len = 4;
  r.escapes[0] = '\\';
  r.escapes_len = 1;
  return r;
}

void advance(struct lexer_t* l) {
  l->cur++;
  if (l->cur >= l->input.len) {
    l->is_eof = true;
    return;
  }
}

bool peek_token_char_recognizable(
    struct lexer_t l, const char* recognized_chars, size_t recognized_len) {
  size_t i = 0;
  char c;
  if (l.is_eof || l.error != 0) {
    return false;
  }
  c = l.input.start[l.cur];
  for (; i < recognized_len; ++i) {
    if (c == recognized_chars[i]) {
      return true;
    }
  }
  return false;
}

enum token_type_t peek_token_char(struct lexer_t l) {
  if (l.is_eof || l.error != 0) {
    return token_type_char;
  }
  if (peek_token_char_recognizable(l, l.config.delimiters, l.config.delimiters_len)) {
    return token_type_delimiter;
  }
  if (peek_token_char_recognizable(l, l.config.escapes, l.config.escapes_len)) {
    return token_type_escape;
  }
  return token_type_char;
}

struct token_t next_token_char(struct lexer_t* l, enum token_type_t type) {
  struct token_t token = {0};

  if (l->is_eof || l->error != 0) {
    return token;
  }

  if (type == token_type_escape) {
    advance(l);
    if (l->is_eof) {
      l->error = INVALID_ESCAPE;
      return token;
    }

    token.type = token_type_char;
    goto fill_token;
  }

fill_token:
  token.type = type;
  token.contents.start = &l->input.start[l->cur];
  token.contents.len = 1;
  advance(l);

  return token;
}

struct token_t next_token(struct lexer_t* l) {
  struct token_t token = {0};
  enum token_type_t type;

  if (l->is_eof || l->error != 0) {
    return token;
  }
  type = peek_token_char(*l);
  if (type != token_type_char) {
    return next_token_char(l, type);
  }

  token.contents.start = &l->input.start[l->cur];
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

#endif
