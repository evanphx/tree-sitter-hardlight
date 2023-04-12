#include <tree_sitter/parser.h>
#include <wchar.h>
#include <wctype.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

// For explanation of the tokens see grammar.js
typedef enum {
    FRONTMATTER,
} TokenType;

typedef struct {
    int blah;
} Scanner;

static bool scan(Scanner *s, TSLexer *lexer, const bool *valid_symbols) {
    if (valid_symbols[FRONTMATTER]) {
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

    return false;
}

void *tree_sitter_hardlight_external_scanner_create() {
    Scanner *s = (Scanner *)malloc(sizeof(Scanner));

    return s;
}

bool tree_sitter_hardlight_external_scanner_scan(
    void *payload,
    TSLexer *lexer,
    const bool *valid_symbols
) {
    Scanner *scanner = (Scanner *)payload;
    return scan(scanner, lexer, valid_symbols);
}

unsigned tree_sitter_hardlight_external_scanner_serialize(
    void *payload,
    char* buffer
) {
    return 0;
}

void tree_sitter_hardlight_external_scanner_deserialize(
    void *payload,
    char* buffer,
    unsigned length
) {
    return;
}

void tree_sitter_hardlight_external_scanner_destroy(void *payload) {
    Scanner *scanner = (Scanner *)payload;
    // free(scanner);
}
