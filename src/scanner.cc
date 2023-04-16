#include <tree_sitter/parser.h>
#include <vector>
#include <cwctype>
#include <cstring>
#include <cassert>
#include <stdio.h>
namespace {

using std::vector;
using std::iswspace;
using std::memcpy;

enum TokenType {
  FRONTMATTER,
  STRING_START,
  STRING_CONTENT,
  STRING_END,
  SCAN_ERROR,
};

struct Delimiter {
  enum {
    SingleQuote = 1 << 0,
    DoubleQuote = 1 << 1,
    BackQuote = 1 << 2,
    Raw = 1 << 3,
    Format = 1 << 4,
    Triple = 1 << 5,
    Bytes = 1 << 6,
  };

  Delimiter() : flags(0) {}

  bool is_format() const {
    return flags & Format;
  }

  bool is_raw() const {
    return flags & Raw;
  }

  bool is_triple() const {
    return flags & Triple;
  }

  bool is_bytes() const {
    return flags & Bytes;
  }

  int32_t end_character() const {
    if (flags & SingleQuote) return '\'';
    if (flags & DoubleQuote) return '"';
    if (flags & BackQuote) return '`';
    return 0;
  }

  void set_format() {
    flags |= Format;
  }

  void set_raw() {
    flags |= Raw;
  }

  void set_triple() {
    flags |= Triple;
  }

  void set_bytes() {
    flags |= Bytes;
  }

  void set_end_character(int32_t character) {
    switch (character) {
      case '\'':
        flags |= SingleQuote;
        break;
      case '"':
        flags |= DoubleQuote;
        break;
      case '`':
        flags |= BackQuote;
        break;
      default:
        assert(false);
    }
  }

  char flags;
};

struct Scanner {
  Scanner() {
    assert(sizeof(Delimiter) == sizeof(char));
    deserialize(NULL, 0);
  }

  unsigned serialize(char *buffer) {
    size_t i = 0;

    size_t delimiter_count = delimiter_stack.size();
    if (delimiter_count > UINT8_MAX) delimiter_count = UINT8_MAX;
    buffer[i++] = delimiter_count;

    if (delimiter_count > 0) {
      memcpy(&buffer[i], delimiter_stack.data(), delimiter_count);
    }
    i += delimiter_count;

    return i;
  }

  void deserialize(const char *buffer, unsigned length) {
    delimiter_stack.clear();

    if (length > 0) {
      size_t i = 0;

      size_t delimiter_count = (uint8_t)buffer[i++];
      delimiter_stack.resize(delimiter_count);
      if (delimiter_count > 0) {
        memcpy(delimiter_stack.data(), &buffer[i], delimiter_count);
      }
      i += delimiter_count;
    }
  }

  void advance(TSLexer *lexer) {
    lexer->advance(lexer, false);
  }

  void skip(TSLexer *lexer) {
    lexer->advance(lexer, true);
  }

  bool scan(TSLexer *lexer, const bool *valid_symbols) {
    if (valid_symbols[SCAN_ERROR]) {
      // printf("scan_error: %c (%x) %s\n", lexer->lookahead, lexer->lookahead, valid_symbols[STRING_END] ? "+end" : "-end");
    }

    if (valid_symbols[STRING_CONTENT] && !delimiter_stack.empty() && !valid_symbols[SCAN_ERROR]) {
      // printf("sting_content: %c (%x) %s\n", lexer->lookahead, lexer->lookahead, valid_symbols[STRING_END] ? "+end" : "-end");
      Delimiter delimiter = delimiter_stack.back();
      int32_t end_character = delimiter.end_character();
      bool has_content = false;
      while (lexer->lookahead) {
        if ((lexer->lookahead == '{' || lexer->lookahead == '}') && delimiter.is_format()) {
          lexer->mark_end(lexer);
          lexer->result_symbol = STRING_CONTENT;
          return has_content;
        } else if (lexer->lookahead == '\\') {
          if (delimiter.is_raw()) {
            // Step over the backslash.
            lexer->advance(lexer, false);
            // Step over any escaped quotes.
            if (lexer->lookahead == delimiter.end_character() || lexer->lookahead == '\\') {
              lexer->advance(lexer, false);
            }
            continue;
          } else if (delimiter.is_bytes()) {
              lexer->mark_end(lexer);
              lexer->advance(lexer, false);
              if (lexer->lookahead == 'N' || lexer->lookahead == 'u' || lexer->lookahead == 'U') {
                // In bytes string, \N{...}, \uXXXX and \UXXXXXXXX are not escape sequences
                // https://docs.python.org/3/reference/lexical_analysis.html#string-and-bytes-literals
                lexer->advance(lexer, false);
              } else {
                  lexer->result_symbol = STRING_CONTENT;
                  return has_content;
              }
          } else {
            lexer->mark_end(lexer);
            lexer->result_symbol = STRING_CONTENT;
            return has_content;
          }
        } else if (lexer->lookahead == end_character) {
          if (delimiter.is_triple()) {
            lexer->mark_end(lexer);
            lexer->advance(lexer, false);
            if (lexer->lookahead == end_character) {
              lexer->advance(lexer, false);
              if (lexer->lookahead == end_character) {
                if (has_content) {
                  lexer->result_symbol = STRING_CONTENT;
                } else {
                  lexer->advance(lexer, false);
                  lexer->mark_end(lexer);
                  delimiter_stack.pop_back();
                  lexer->result_symbol = STRING_END;
                }
                return true;
              } else {
                lexer->mark_end(lexer);
                lexer->result_symbol = STRING_CONTENT;
                return true;
              }
            } else {
              lexer->mark_end(lexer);
              lexer->result_symbol = STRING_CONTENT;
              return true;
            }
          } else {
            if (has_content) {
              lexer->result_symbol = STRING_CONTENT;
            } else {
        // printf("string end\n");
              lexer->advance(lexer, false);
              delimiter_stack.pop_back();
              lexer->result_symbol = STRING_END;
            }
            lexer->mark_end(lexer);
            return true;
          }
        } else if (lexer->lookahead == '\n' && has_content && !delimiter.is_triple()) {
          return false;
        }
        advance(lexer);
        has_content = true;
      }

      return false;
    }

    if (valid_symbols[FRONTMATTER] && !valid_symbols[SCAN_ERROR]) {
        for (;;) {
            if (lexer->lookahead == '-') {
                int minus_count = 0;

                // We don't consume the ---\n as part of the frontmatter
                // token so that it can get highlighted differently.
                lexer->mark_end(lexer);

                while (lexer->lookahead == '-') {
                    lexer->advance(lexer, false);
                    minus_count++;
                }

                if (minus_count == 3) {
                    // if exactly 3 check if next symbol (after eventual
                    // whitespace) is newline
                    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
                        lexer->advance(lexer, false);
                    }

                    if (lexer->lookahead == '\r' || lexer->lookahead == '\n') {
                        // if so also consume newline
                        if (lexer->lookahead == '\r') {
                            lexer->advance(lexer, false);
                            if (lexer->lookahead == '\n') {
                                lexer->advance(lexer, false);
                            }
                        } else {
                            lexer->advance(lexer, false);
                        }
                        return true;
                    }
                }
            }

            // otherwise consume rest of line
            while (lexer->lookahead != '\n' && lexer->lookahead != '\r' && !lexer->eof(lexer)) {
                lexer->advance(lexer, false);
            }

            if (lexer->eof(lexer)) {
                return false;
            }

            // advance over newline
            if (lexer->lookahead == '\r') {
                lexer->advance(lexer, false);
                if (lexer->lookahead == '\n') {
                    lexer->advance(lexer, false);
                }
            } else {
                lexer->advance(lexer, false);
            }
        }
    }

    lexer->mark_end(lexer);

    // Skip whitespace (we're likely in error recovery, searching for a string start
    for (;;) {
      switch (lexer->lookahead) {
        case '\n':
        case ' ':
        case '\r':
        case '\t':
          skip(lexer);
          break;
        default:
          goto done;
      }
    }

done:

    if (valid_symbols[STRING_START]) {
      // printf("sting_start: %c (%x)\n", lexer->lookahead, lexer->lookahead);
      Delimiter delimiter;

      if (lexer->lookahead == '`') {
        delimiter.set_end_character('`');
        advance(lexer);
        lexer->mark_end(lexer);
      } else if (lexer->lookahead == '\'') {
        delimiter.set_end_character('\'');
        advance(lexer);
        lexer->mark_end(lexer);
        if (lexer->lookahead == '\'') {
          advance(lexer);
          if (lexer->lookahead == '\'') {
            advance(lexer);
            lexer->mark_end(lexer);
            delimiter.set_triple();
          }
        }
      } else if (lexer->lookahead == '"') {
        delimiter.set_end_character('"');
        advance(lexer);
        lexer->mark_end(lexer);
        if (lexer->lookahead == '"') {
          advance(lexer);
          if (lexer->lookahead == '"') {
            advance(lexer);
            lexer->mark_end(lexer);
            delimiter.set_triple();
          }
        }
      }

      if (delimiter.end_character()) {
        // printf("string start\n");
        delimiter_stack.push_back(delimiter);
        lexer->result_symbol = STRING_START;
        return true;
      }
    }

    return false;
  }

  vector<Delimiter> delimiter_stack;
};

}

extern "C" {

void *tree_sitter_hardlight_external_scanner_create() {
  return new Scanner();
}

bool tree_sitter_hardlight_external_scanner_scan(void *payload, TSLexer *lexer,
                                            const bool *valid_symbols) {
  Scanner *scanner = static_cast<Scanner *>(payload);
  return scanner->scan(lexer, valid_symbols);
}

unsigned tree_sitter_hardlight_external_scanner_serialize(void *payload, char *buffer) {
  Scanner *scanner = static_cast<Scanner *>(payload);
  return scanner->serialize(buffer);
}

void tree_sitter_hardlight_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  Scanner *scanner = static_cast<Scanner *>(payload);
  scanner->deserialize(buffer, length);
}

void tree_sitter_hardlight_external_scanner_destroy(void *payload) {
  Scanner *scanner = static_cast<Scanner *>(payload);
  delete scanner;
}

}
