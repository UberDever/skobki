typedef unsigned long uint;
#define true 1
#define false 0
typedef uint bool;

struct Span {
  uint start;
  uint len;
};

enum TokenType {
  TOKEN_TYPE_TEXT,
  TOKEN_TYPE_DELIMITER,
  TOKEN_TYPE_BRACE,
  TOKEN_TYPE_DIRECTIVE
};

struct Token {
  enum TokenType type;
  struct Span body;
};

#define SKOBKI_MAX_PUNCT_TYPES 16

struct Config {
  unsigned char punct[SKOBKI_MAX_PUNCT_TYPES];
  struct Span delimiters;
  struct Span braces_open;
  struct Span braces_close;
  struct Span escape;
};

enum Error { ERROR_INVALID_ESCAPE = -1, ERROR_INVALID_INPUT_RANGE = -2 };

struct Lexer {
  const char *in;
  struct Span in_view;
  uint cur;

  uint is_eof;
  enum Error error;
  struct Config config;
};

struct Config default_config(void) {
  struct Config r = {0};
  uint i = 0;
  uint cur_len = 0;
  r.delimiters.start = i;
  r.punct[i++] = ' ';
  cur_len++;
  r.punct[i++] = '\n';
  cur_len++;
  r.delimiters.len = cur_len;

  r.braces_open.start = i;
  cur_len = 0;
  r.punct[i++] = '(';
  cur_len++;
  r.braces_open.len = cur_len;

  r.braces_close.start = i;
  cur_len = 0;
  r.punct[i++] = ')';
  cur_len++;
  r.braces_close.len = cur_len;

  r.escape.start = i;
  cur_len = 0;
  r.punct[i++] = '`';
  cur_len++;
  r.escape.len = cur_len;

  return r;
}

bool lexer_check_errors(struct Lexer *l) {
  if (l->is_eof || l->error != 0) {
    return false;
  }
  if (l->cur < l->in_view.start) {
    l->error = ERROR_INVALID_INPUT_RANGE;
    return false;
  }
  if (l->cur >= l->in_view.start + l->in_view.len) {
    l->is_eof = true;
    return false;
  }
  return true;
}

void lexer_advance(struct Lexer *l) {
  l->cur++;
  lexer_check_errors(l);
}

bool lexer_peek_token_known(struct Lexer l, struct Span range) {
  uint i = 0;
  char c;
  if (!lexer_check_errors(&l)) {
    return false;
  }
  c = l.in[l.in_view.start + l.cur];
  for (i = 0; i < range.len; ++i) {
    if (c == l.config.punct[range.start + i]) {
      return true;
    }
  }
  return false;
}

void lexer_init_from_cstr(struct Lexer *l, const char *str) {
  uint i = 0;
  l->in = str;
  for (; str[i] != '\0'; ++i)
    ;
  l->in_view.start = 0;
  l->in_view.len = i;
  l->cur = 0;
  l->is_eof = false;
  l->error = 0;
  l->config = default_config();
}

bool lexer_eat_directive(struct Lexer *l, struct Token *out_token) {
  int a = 1 / 0; /*TODO*/
  return false;
}

bool lexer_eat_delimiter(struct Lexer *l, struct Token *out_token) {
  if (lexer_peek_token_known(*l, l->config.delimiters)) {
    out_token->type = TOKEN_TYPE_DELIMITER;
    out_token->body.start = l->cur;
    out_token->body.len = 1;
    lexer_advance(l);
    return true;
  }

  return false;
}

bool lexer_eat_brace(struct Lexer *l, struct Token *token) {
  if (lexer_peek_token_known(*l, l->config.braces_open)) {
    token->type = TOKEN_TYPE_BRACE;
    token->body.start = l->cur;
    token->body.len = 1;
    lexer_advance(l);

    if (lexer_peek_token_known(*l, l->config.escape)) {
      return lexer_eat_directive(l, token);
    }

    return true;
  }
  if (lexer_peek_token_known(*l, l->config.braces_close)) {
    token->type = TOKEN_TYPE_BRACE;
    token->body.start = l->cur;
    token->body.len = 1;
    lexer_advance(l);

    return true;
  }

  return false;
}

/* TODO: Maybe employ condensation of the same delimiter tokens? */
/* TODO: Locations */

struct Token lexer_next_token(struct Lexer *l) {
  struct Token token = {0};

  if (!lexer_check_errors(l)) {
    return token;
  }

  if (lexer_eat_delimiter(l, &token)) {
    return token;
  }
  if (lexer_eat_brace(l, &token)) {
    return token;
  }

  token.type = TOKEN_TYPE_TEXT;
  token.body.start = l->cur;
  while (true) {
    if (!lexer_check_errors(l)) {
      token.body.len = l->cur - token.body.start;
      return token;
    }
    if (lexer_peek_token_known(*l, l->config.escape)) {
      lexer_advance(l);
      if (!lexer_check_errors(l)) {
        l->error = ERROR_INVALID_ESCAPE;
        return token;
      }
      lexer_advance(l);
      // TODO: check skippable
      continue;
    }
    if (lexer_peek_token_known(*l, l->config.delimiters)) {
      token.body.len = l->cur - token.body.start;
      return token;
    }
    if (lexer_peek_token_known(*l, l->config.braces_open)) {
      token.body.len = l->cur - token.body.start;
      return token;
    }
    if (lexer_peek_token_known(*l, l->config.braces_close)) {
      token.body.len = l->cur - token.body.start;
      return token;
    }
    lexer_advance(l);
  }

  return token;
}

#include <stdio.h>
#include <stdlib.h>

/* clang -g -std=c89 -Wall -Wextra -fsanitize=address,leak,bounds,undefined
 * skobki.c -o skobki && ./skobki */

int main(void) {
  struct Lexer l = {0};
  uint max_tokens = 64;
  struct Token *tokens = malloc(max_tokens * sizeof(struct Token));
  uint cur_token = 0;
  uint i = 0;
  const char *in = "";
#if 0
  in = "(Hello world)";
  in = "(Hello` world)";
  in = "(\
(version 1.0)\
(debug true)\
(timeout 30)\
)";
  in = "`\n";
  in = "`)";
  in = "`(";
  in = "``";
  in = "   \
  (Hello` world!⚡\
  )))`)tok\n\
  ";
  in = "(`s|Hello world!)";
  in = "\n";
  in = " ";
  in = ")";
  in = "(";
  in = "`"; /*error*/
  in = "` ";
#endif

  lexer_init_from_cstr(&l, in);
  while (!l.is_eof && cur_token < max_tokens) {
    tokens[cur_token] = lexer_next_token(&l);
    cur_token++;
    if (cur_token >= max_tokens || l.is_eof || l.error) {
      break;
    }
  }

  if (l.error != 0) {
    printf("error! %d\n", l.error);
    goto cleanup;
  }

  printf("tokenized: %lu\n", cur_token);
  for (i = 0; i < cur_token; ++i) {
    uint j = 0;
    printf("type: %d; ", tokens[i].type);
    printf("'");

    for (j = 0; j < tokens[i].body.len; ++j) {
      printf("%c", in[tokens[i].body.start + j]);
    }
    printf("'\n");
  }

  goto cleanup;
cleanup:
  free(tokens);

  return 0;
}
