
/* #define SKOBKI_IMPL*/
#include "skobki.h"
#include <stdio.h>

int main(void) {
#if 0
const char* s = "";
const char* s = " ";
const char* s = "}";
const char* s = "\\";
const char* s = "\\}";
const char* s = "abc";
#endif
const char* s = "\\}}\nabc}def   booba beeba782";
  struct config_t cfg = default_config();
  struct lexer_t l = {0};
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
