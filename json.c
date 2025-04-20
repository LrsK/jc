#include <stdio.h>
#include <stdlib.h>

typedef int bool;

typedef struct {
  char *data;
  unsigned int capacity;
  unsigned int len;
} String;

typedef struct {
  char *data;
  unsigned int len;
} StringSlice;

char *StringSliceGet(StringSlice *slice) {
  char *realStr = malloc(slice->len + 1);
  for (unsigned int i = 0; i < slice->len; i++) {
    realStr[i] = slice->data[i];
  }
  realStr[slice->len] = '\0';
  return realStr;
}

String *StringNew(unsigned int capacity) {
  String *s = malloc(sizeof(String));
  s->data = malloc(capacity + 1);
  s->data[0] = '\0';
  s->capacity = capacity;
  s->len = 0;
  return s;
}

void StringAppend(String *s, char c) {
  if (s->len >= s->capacity - 1) {
    s->capacity *= 2;
    s->data = realloc(s->data, s->capacity);
  }
  s->data[s->len] = c;
  s->len++;
  s->data[s->len] = '\0';
}

char *StringGet(String *s) { return s->data; }

// Define JSON tokens
enum JSONTokenKind {
  OBJECT_START,
  OBJECT_END,
  ARRAY_START,
  ARRAY_END,
  COMMA,
  COLON,
  STRING,
  NUMBER,
  TRUE,
  FALSE,
  NONE,
  END,
  ILLEGAL
};

/*
  union {
    char *string;
    bool boolean;
    int integer;
    float real;
  };
*/

typedef struct JSONToken {
  enum JSONTokenKind kind;
  const char *value;
  unsigned int pos;
} JSONToken;

typedef struct JSONLexer {
  const char *data;
  unsigned int len;
  unsigned int pos;
  unsigned int read_pos;
  char current_char;
} JSONLexer;

static char json_lexer_peek(JSONLexer *lexer) {
  if (lexer->read_pos >= lexer->len) {
    return EOF;
  }
  return lexer->data[lexer->read_pos];
}

static char json_lexer_read(JSONLexer *lexer) {
  lexer->current_char = json_lexer_peek(lexer);
  lexer->pos = lexer->read_pos;
  lexer->read_pos++;
  return lexer->current_char;
}

static void json_lexer_skip_whitespace(JSONLexer *lexer) {
  while (lexer->current_char == ' ' || lexer->current_char == '\t' ||
         lexer->current_char == '\n' || lexer->current_char == '\r') {
    json_lexer_read(lexer);
  }
}

static int json_lexer_is_literal(JSONLexer *lexer, const char *literal) {
  for (int i = 0; literal[i] != '\0'; i++) {
    if (lexer->current_char != literal[i]) {
      return 0;
    }
    json_lexer_read(lexer);
  }
  return 1;
}

int lexer_init(JSONLexer *lexer, const char *data, unsigned int len) {
  lexer->data = data;
  lexer->len = len;
  lexer->pos = 0;
  lexer->read_pos = 0;
  lexer->current_char = 0;
  json_lexer_read(lexer);
  return 0;
}

JSONToken json_lexer_next(JSONLexer *lexer) {
  json_lexer_skip_whitespace(lexer);

  unsigned int pos = lexer->pos;
  if (lexer->current_char == EOF) {
    json_lexer_read(lexer);
    return (JSONToken){.kind = END, .value = NULL, .pos = pos};
  } else if (lexer->current_char == '{') {
    json_lexer_read(lexer);
    return (JSONToken){.kind = OBJECT_START, .value = NULL, .pos = pos};
  } else if (lexer->current_char == '}') {
    json_lexer_read(lexer);
    return (JSONToken){.kind = OBJECT_END, .value = NULL, .pos = pos};
  } else if (lexer->current_char == '[') {
    json_lexer_read(lexer);
    return (JSONToken){.kind = ARRAY_START, .value = NULL, .pos = pos};
  } else if (lexer->current_char == ']') {
    json_lexer_read(lexer);
    return (JSONToken){.kind = ARRAY_END, .value = NULL, .pos = pos};
  } else if (lexer->current_char == ',') {
    json_lexer_read(lexer);
    return (JSONToken){.kind = COMMA, .value = NULL, .pos = pos};
  } else if (lexer->current_char == ':') {
    json_lexer_read(lexer);
    return (JSONToken){.kind = COLON, .value = NULL, .pos = pos};
  } else if (lexer->current_char == '"') {
    // Read string
    StringSlice slice = {.data = (char *)lexer->data + pos, .len = 0};
    json_lexer_read(lexer);
    bool escape_mode = 0;
    while (lexer->current_char != '"' || escape_mode) {
      if (lexer->current_char == '\\') {
        escape_mode = 1;
      } else {
        escape_mode = 0;
      }
      slice.len++;
      json_lexer_read(lexer);
    }
    // Eat closing quote
    json_lexer_read(lexer);
    return (JSONToken){
        .kind = STRING, .value = StringSliceGet(&slice), .pos = pos};
  } else if (lexer->current_char == '-' ||
             (lexer->current_char >= '0' && lexer->current_char <= '9')) {
    // Read number, can be int or float and negative
    String *string = StringNew(2);
    while (lexer->current_char >= '0' && lexer->current_char <= '9') {
      StringAppend(string, lexer->current_char);
      json_lexer_read(lexer);
    }
    if (lexer->current_char == '.') {
      StringAppend(string, lexer->current_char);
      json_lexer_read(lexer);
      while (lexer->current_char >= '0' && lexer->current_char <= '9') {
        StringAppend(string, lexer->current_char);
        json_lexer_read(lexer);
      }
    }

    return (JSONToken){.kind = NUMBER, .value = StringGet(string), .pos = pos};

  } else if (lexer->current_char == 't') {
    // Read true
    if (!json_lexer_is_literal(lexer, "true")) {
      return (JSONToken){.kind = ILLEGAL, .value = NULL, .pos = pos};
    }

    return (JSONToken){.kind = TRUE, .value = "true", .pos = pos};

  } else if (lexer->current_char == 'f') {
    // Read false
    if (!json_lexer_is_literal(lexer, "false")) {
      return (JSONToken){.kind = ILLEGAL, .value = NULL, .pos = pos};
    }
    return (JSONToken){.kind = FALSE, .value = "false", .pos = pos};

  } else if (lexer->current_char == 'n') {
    // Read null
    if (!json_lexer_is_literal(lexer, "null")) {
      return (JSONToken){.kind = ILLEGAL, .value = NULL, .pos = pos};
    }
    return (JSONToken){.kind = NONE, .value = NULL, .pos = pos};
  }

  return (JSONToken){.kind = END, .value = NULL, .pos = pos};
}

int main(int argc, char *argv[]) {
  // Parse arguments
  if (argc < 2) {
    printf("Usage: %s <JSON file>\n", argv[0]);
    return 1;
  }

  // Load data from file in args
  FILE *file = fopen("test.json", "r"); // argv[1], "r");
  if (file == NULL) {
    printf("Failed to open file: %s\n", argv[1]);
    return 1;
  }

  // Read data from file
  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *data = malloc(file_size + 1);
  fread(data, 1, file_size, file);
  data[file_size] = '\0';

  JSONLexer lexer = {0};
  JSONToken token = {0};

  printf("JSON file size: %zu bytes\n", file_size);
  printf("JSON data: %s\n", data);

  // Close file
  fclose(file);
  // exit(1);

  // Initialize lexer
  lexer_init(&lexer, data, file_size);

  // Parse JSON data
  do {
    token = json_lexer_next(&lexer);
    switch (token.kind) {
    case OBJECT_START:
      printf("{\n");
      break;
    case OBJECT_END:
      printf("}\n");
      break;
    case ARRAY_START:
      printf("[\n");
      break;
    case ARRAY_END:
      printf("]\n");
      break;
    case COMMA:
      printf(",\n");
      break;
    case COLON:
      printf(":\n");
      break;
    case STRING:
      printf("STRING: %s\n", token.value);
      break;
    case NUMBER:
      printf("NUMBER: %s\n", token.value);
      break;
    case TRUE:
      printf("TRUE\n");
      break;
    case FALSE:
      printf("FALSE\n");
      break;
    case NONE:
      printf("NONE\n");
      break;
    case ILLEGAL:
      printf("ILLEGAL\n");
      break;
    case END:
      break;
    }
  } while (token.kind != END);

  return 0;
}
