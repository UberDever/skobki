typedef unsigned long uint;
#define true  1
#define false 0
typedef uint bool;

struct Span {
  uint start;
  uint len;
};

struct Span span_from_cstr(const char* str) {
  uint i = 0;
  struct Span s;
  for (i = 0; str[i] != '\0'; ++i)
    ;
  s.start = 0;
  s.len = i;
  return s;
}

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

enum Error {
  ERROR_INVALID_ESCAPE = -1,
  ERROR_INVALID_INPUT_RANGE = -2
};

struct Lexer {
  const char* in;
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

bool lexer_check_errors(struct Lexer* l) {
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

void lexer_advance(struct Lexer* l) {
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

void lexer_init_from_cstr(struct Lexer* l, const char* str) {
  l->in = str;
  l->in_view = span_from_cstr(str);
  l->cur = 0;
  l->is_eof = false;
  l->error = 0;
  l->config = default_config();
}

bool lexer_eat_directive(struct Lexer* l, struct Token* out_token) {
  int a = 1 / 0; /*TODO*/
  return false;
}

bool lexer_eat_delimiter(struct Lexer* l, struct Token* out_token) {
  if (lexer_peek_token_known(*l, l->config.delimiters)) {
    out_token->type = TOKEN_TYPE_DELIMITER;
    out_token->body.start = l->cur;
    out_token->body.len = 1;
    lexer_advance(l);
    return true;
  }

  return false;
}

bool lexer_eat_brace(struct Lexer* l, struct Token* token) {
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

struct Token lexer_next_token(struct Lexer* l) {
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
  if (lexer_peek_token_known(*l, l->config.escape)) {
    lexer_advance(l);
    if (!lexer_check_errors(l)) {
      l->error = ERROR_INVALID_ESCAPE;
      return token;
    }
    token.type = TOKEN_TYPE_TEXT;
    if (lexer_peek_token_known(*l, l->config.delimiters)) {
      token.body.start = l->cur;
      token.body.len = 1;
      lexer_advance(l);
      return token;
    }
    if (lexer_peek_token_known(*l, l->config.braces_open)) {
      token.body.start = l->cur;
      token.body.len = 1;
      lexer_advance(l);
      return token;
    }
    if (lexer_peek_token_known(*l, l->config.braces_close)) {
      token.body.start = l->cur;
      token.body.len = 1;
      lexer_advance(l);
      return token;
    }
    if (lexer_peek_token_known(*l, l->config.escape)) {
      token.body.start = l->cur;
      token.body.len = 1;
      lexer_advance(l);
      return token;
    }

    l->error = ERROR_INVALID_ESCAPE;
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
      token.body.len = l->cur - token.body.start;
      return token;
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
#include <assert.h>

/* clang -g -std=c89 -Wall -Wextra -fsanitize=address,leak,bounds,undefined
 * skobki.c -o skobki && ./skobki */

struct ExpectedToken {
  enum TokenType type;
  const char* body;
};

struct ExpectedTokens {
  struct ExpectedToken* xs;
  uint len;
};

struct Expectation {
  bool error;

  union ExpectationUnion {
    struct ExpectedTokens tokens;
    enum Error error;
  } as;
};

bool strings_equal(const char* input_lhs, struct Span lhs, const char* input_rhs, struct Span rhs) {
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

int lexer_test_general(const char* in, struct Expectation expected) {
  struct Lexer l = {0};
  uint max_tokens = 64;
  struct Token* tokens = malloc(max_tokens * sizeof(struct Token));
  uint cur_token = 0;
  uint tokens_len = 0;
  uint i = 0;
  int result = 0;

  lexer_init_from_cstr(&l, in);
  while (!l.is_eof) {
    tokens[cur_token] = lexer_next_token(&l);
    cur_token++;
    if (cur_token >= max_tokens) {
      printf("warning: reallocating tokens buf\n");
      max_tokens *= 2;
      tokens = realloc(tokens, max_tokens * sizeof(struct Token));
    }
    if (l.is_eof || l.error) {
      break;
    }
  }
  tokens_len = cur_token;

  if (l.error != 0) {
    if (!expected.error) {
      printf("lexer error: %d\n", l.error);
      result = l.error;
      goto cleanup;
    }
    result = !(l.error == expected.as.error);
    if (result != false) {
      printf("errors %d != %d\n", l.error, expected.as.error);
      goto cleanup;
    }
    goto cleanup;
  }

#ifdef TEST_DUMP_EXPECTATION

  printf("struct Expectation e = {0};\n");
  printf("struct ExpectedTokens expected = {0};\n");
  printf("uint len = %lu;\n", tokens_len);
  printf("struct ExpectedToken arr[%lu] = {\n", tokens_len);
  for (i = 0; i < tokens_len; ++i) {
    uint j = 0;
    printf("{ ");
    if (tokens[i].type == TOKEN_TYPE_TEXT) {
      printf("TOKEN_TYPE_TEXT");
    }
    if (tokens[i].type == TOKEN_TYPE_DELIMITER) {
      printf("TOKEN_TYPE_DELIMITER");
    }
    if (tokens[i].type == TOKEN_TYPE_BRACE) {
      printf("TOKEN_TYPE_BRACE");
    }
    if (tokens[i].type == TOKEN_TYPE_DIRECTIVE) {
      printf("TOKEN_TYPE_DIRECTIVE");
    }

    printf(",\"");
    for (j = 0; j < tokens[i].body.len; ++j) {
      printf("%c", in[tokens[i].body.start + j]);
    }
    printf("\" },\n");
  }
  printf("};\n");
  printf("expected.len = len;\n");
  printf("expected.xs = arr;\n");
  printf("e.as.tokens = expected;\n");

  printf("\n");

#else
  assert(expected.error == false);

  if (tokens_len != expected.as.tokens.len) {
    printf("token len %lu != %lu\n", tokens_len, expected.as.tokens.len);
    result = -1;
    goto cleanup;
  }
  for (i = 0; i < expected.as.tokens.len; ++i) {
    struct Span expected_tokens_view = span_from_cstr(expected.as.tokens.xs[i].body);
    if (tokens[i].type != expected.as.tokens.xs[i].type) {
      printf("token types %d != %d\n", tokens[i].type, expected.as.tokens.xs[i].type);
      result = -1;
      goto cleanup;
    }
    if (!strings_equal(in, tokens[i].body, expected.as.tokens.xs[i].body, expected_tokens_view)) {
      uint j = 0;
      printf("'");
      for (j = 0; j < tokens[i].body.len; ++j) {
        printf("%c", in[tokens[i].body.start + j]);
      }
      printf("'\n");

      printf("'");
      for (j = 0; j < expected_tokens_view.len; ++j) {
        printf("%c", expected.as.tokens.xs[i].body[tokens[i].body.start + j]);
      }
      printf("'\n");
    }
  }

#endif

cleanup:
  free(tokens);

  return result;
}

#define TEST_LEXER(s, e)                                                                           \
  if (!lexer_test_general(s, e)) {                                                                 \
    result = -1;                                                                                   \
  }

int main(void) {
  int result = 0;

  {
    struct Expectation e = {0};
    struct ExpectedTokens expected = {0};
    uint len = 5;
    struct ExpectedToken arr[5] = {
        {TOKEN_TYPE_BRACE, "("},
        {TOKEN_TYPE_TEXT, "Hello"},
        {TOKEN_TYPE_TEXT, " "},
        {TOKEN_TYPE_TEXT, "world"},
        {TOKEN_TYPE_BRACE, ")"},
    };
    expected.len = len;
    expected.xs = arr;
    e.as.tokens = expected;
    TEST_LEXER("(Hello` world)", e);
  }

  {
    struct Expectation e = {0};
    struct ExpectedTokens expected = {0};
    uint len = 5;
    struct ExpectedToken arr[5] = {
        {TOKEN_TYPE_BRACE, "("},
        {TOKEN_TYPE_TEXT, "Hello"},
        {TOKEN_TYPE_DELIMITER, " "},
        {TOKEN_TYPE_TEXT, "world"},
        {TOKEN_TYPE_BRACE, ")"},
    };
    expected.len = len;
    expected.xs = arr;
    e.as.tokens = expected;
    TEST_LEXER("(Hello world)", e);
  }

  {
    struct Expectation e = {0};
    struct ExpectedTokens expected = {0};
    uint len = 17;
    struct ExpectedToken arr[17] = {
        {TOKEN_TYPE_BRACE, "("},
        {TOKEN_TYPE_BRACE, "("},
        {TOKEN_TYPE_TEXT, "version"},
        {TOKEN_TYPE_DELIMITER, " "},
        {TOKEN_TYPE_TEXT, "1.0"},
        {TOKEN_TYPE_BRACE, ")"},
        {TOKEN_TYPE_BRACE, "("},
        {TOKEN_TYPE_TEXT, "debug"},
        {TOKEN_TYPE_DELIMITER, " "},
        {TOKEN_TYPE_TEXT, "true"},
        {TOKEN_TYPE_BRACE, ")"},
        {TOKEN_TYPE_BRACE, "("},
        {TOKEN_TYPE_TEXT, "timeout"},
        {TOKEN_TYPE_DELIMITER, " "},
        {TOKEN_TYPE_TEXT, "30"},
        {TOKEN_TYPE_BRACE, ")"},
        {TOKEN_TYPE_BRACE, ")"},
    };
    expected.len = len;
    expected.xs = arr;
    e.as.tokens = expected;
    TEST_LEXER(
        "("
        "(version 1.0)"
        "(debug true)"
        "(timeout 30)"
        ")",
        e);
  }

  {
    struct Expectation e = {0};
    struct ExpectedTokens expected = {0};
    uint len = 1;
    struct ExpectedToken arr[1] = {
        {TOKEN_TYPE_TEXT, "\n"},
    };
    expected.len = len;
    expected.xs = arr;
    e.as.tokens = expected;
    TEST_LEXER("`\n", e);
  }

  {
    struct Expectation e = {0};
    struct ExpectedTokens expected = {0};
    uint len = 1;
    struct ExpectedToken arr[1] = {
        {TOKEN_TYPE_TEXT, ")"},
    };
    expected.len = len;
    expected.xs = arr;
    e.as.tokens = expected;
    TEST_LEXER("`)", e);
  }

  {
    struct Expectation e = {0};
    struct ExpectedTokens expected = {0};
    uint len = 1;
    struct ExpectedToken arr[1] = {
        {TOKEN_TYPE_TEXT, "("},
    };
    expected.len = len;
    expected.xs = arr;
    e.as.tokens = expected;
    TEST_LEXER("`(", e);
  }

  {
    struct Expectation e = {0};
    struct ExpectedTokens expected = {0};
    uint len = 1;
    struct ExpectedToken arr[1] = {
        {TOKEN_TYPE_TEXT, "`"},
    };
    expected.len = len;
    expected.xs = arr;
    e.as.tokens = expected;
    TEST_LEXER("``", e);
  }

  {
    struct Expectation e = {0};
    struct ExpectedTokens expected = {0};
    uint len = 14;
    struct ExpectedToken arr[14] = {
        {TOKEN_TYPE_DELIMITER, " "},
        {TOKEN_TYPE_DELIMITER, " "},
        {TOKEN_TYPE_DELIMITER, " "},
        {TOKEN_TYPE_DELIMITER, " "},
        {TOKEN_TYPE_BRACE, "("},
        {TOKEN_TYPE_TEXT, "Hello"},
        {TOKEN_TYPE_TEXT, " "},
        {TOKEN_TYPE_TEXT, "world!⚡"},
        {TOKEN_TYPE_BRACE, ")"},
        {TOKEN_TYPE_BRACE, ")"},
        {TOKEN_TYPE_BRACE, ")"},
        {TOKEN_TYPE_TEXT, ")"},
        {TOKEN_TYPE_TEXT, "tok"},
        {TOKEN_TYPE_DELIMITER, "\n"}};
    expected.len = len;
    expected.xs = arr;
    e.as.tokens = expected;
    TEST_LEXER(
        "    "
        "(Hello` world!⚡"
        ")))`)tok\n",
        e);
  }

#if 0
    TEST_LEXER("(`s|Hello world!)", expected);
#endif

  {
    struct Expectation e = {0};
    struct ExpectedTokens expected = {0};
    uint len = 1;
    struct ExpectedToken arr[1] = {
        {TOKEN_TYPE_DELIMITER, "\n"},
    };
    expected.len = len;
    expected.xs = arr;
    e.as.tokens = expected;
    TEST_LEXER("\n", e);
  }

  {
    struct Expectation e = {0};
    struct ExpectedTokens expected = {0};
    uint len = 1;
    struct ExpectedToken arr[1] = {
        {TOKEN_TYPE_DELIMITER, " "},
    };
    expected.len = len;
    expected.xs = arr;
    e.as.tokens = expected;
    TEST_LEXER(" ", e);
  }

  {
    struct Expectation e = {0};
    struct ExpectedTokens expected = {0};
    uint len = 1;
    struct ExpectedToken arr[1] = {
        {TOKEN_TYPE_BRACE, ")"},
    };
    expected.len = len;
    expected.xs = arr;
    e.as.tokens = expected;
    TEST_LEXER(")", e);
  }

  {
    struct Expectation e = {0};
    struct ExpectedTokens expected = {0};
    uint len = 1;
    struct ExpectedToken arr[1] = {
        {TOKEN_TYPE_BRACE, "("},
    };
    expected.len = len;
    expected.xs = arr;
    e.as.tokens = expected;
    TEST_LEXER("(", e);
  }

  {
    struct Expectation e = {0};
    e.error = true;
    e.as.error = ERROR_INVALID_ESCAPE;
    TEST_LEXER("`", e);
  }

  {
    struct Expectation e = {0};
    e.error = true;
    e.as.error = ERROR_INVALID_ESCAPE;
    TEST_LEXER("`!", e);
  }

  {
    struct Expectation e = {0};
    struct ExpectedTokens expected = {0};
    uint len = 1;
    struct ExpectedToken arr[1] = {
        {TOKEN_TYPE_TEXT, " "},
    };
    expected.len = len;
    expected.xs = arr;
    e.as.tokens = expected;
    TEST_LEXER("` ", e);
  }

  return result;
}
