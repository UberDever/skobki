
/* #define SKOBKI_IMPL*/
#include "skobki.h"
#include <stdio.h>
#include <stdlib.h>
#if 0
int main(void) {
#if 0
const char* s = "";
const char* s = " ";
const char* s = "}";
const char* s = "\\";
const char* s = "\\}";
const char* s = "abc";
const char* s = "\\}}\nabc}def   booba beeba782";
#endif
const char* s = "delimiters: 0a 20\nbrackets: 7b 7d 28 29\nescape: 5c\n{}";
  struct config_t cfg = default_config();
  struct skobki_lexer_t l = {0};
  size_t len = 0;

  for (; s[len] != '\0'; ++len) {
  }
  printf("input string len: %zu\n", len);

  l.config = cfg;
  l.input.start = s;
  l.input.len = len;

  while (1) {
    struct token_t t = next_token(&l);
    size_t i = 0;
    if (l.error != 0) {
      printf("error: %d\n", l.error);
      return -1;
    }
    printf("type: %zu\n", t.type);
    printf("contents: '");
    for (i = 0; i < t.contents.len; ++i) {
      putc(t.contents.start[i], stdout);
    }
    putc('\'', stdout);
    putc('\n', stdout);
    if (l.is_eof) {
      break;
    }
  }
  return 0;
}
#endif

int main(void) {
  //   const char* input = "delimiters: 0a 20\nbrackets: 7b 7d 28 29\nescape: 5c\n{}";
  const char* input = "delimiters: 42 69\nbrackets: 13 37 14 88\nescape: cc\n{}";
  struct skobki_lexer_t l = {0};
  size_t len = 0;
  struct skobki_span_t view;
  struct skobki_token_t* out_tokens;
  size_t out_max_len = 2;
  size_t count;
  size_t i;
  size_t j;

  out_tokens = malloc(out_max_len * sizeof(*out_tokens));
  for (; input[len] != '\0'; ++len) {
  }
  view.start = 0;
  view.len = len;
  printf("input string len: %zu\n", len);

  skobki_init_lexer(&l);

  while (1) {
    count = skobki_tokenize(&l, input, view, out_tokens, out_max_len);
    if (l.error == SKOBKI_ERROR_NOMEM) {
      out_max_len *= 2;
      out_tokens = realloc(out_tokens, out_max_len * sizeof(*out_tokens));
      continue;
    }
    break;
  }

  if (l.error) {
    printf("error: %d\n", l.error);
    free(out_tokens);
    return -1;
  }

  for (i = 0; i < SKOBKI_MAX_PUNCT_TYPES; ++i) {
    printf("%02X ", l.config.punct[i]);
  }
  putc('\n', stdout);

  for (i = 0; i < count; ++i) {
    struct skobki_token_t t = out_tokens[i];
    printf(
        "type = %u; count = %zu; parent = %zu; next_sibling = %zu\n",
        t.type,
        t.count,
        t.parent,
        t.next_sibling);
    putc('\'', stdout);
    for (j = t.start; j <= t.end; ++j) {
      putc(input[view.start + j], stdout);
    }
    putc('\'', stdout);
  }

  free(out_tokens);
}
