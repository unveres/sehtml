#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

inline void error(const char *text)
{
  fprintf(stderr, "[Error] %s.\n", text);
}

inline void indent(FILE *out, int n)
{
  int i;

  for (i = 0; i < n; ++i)
    fputc('\t', out);
}

#define ESCSEQ_TEST()                     \
if (is_escseq) {                          \
  if (mode == NONE)                       \
    error("Not valid S-Expression file"); \
                                          \
  fputc(ch, out);                         \
  is_escseq = 0;                          \
  break;                                  \
}

#define STRING_INVERT_TYPE(type) \
  ((type) == DOUBLE_QUOTE_STRING ? SINGLE_QUOTE_STRING : DOUBLE_QUOTE_STRING)

#define STRING_TEST()                                                         \
if (token_type == SINGLE_QUOTE_STRING || token_type == DOUBLE_QUOTE_STRING) { \
  fputc(ch, out);                                                             \
  break;                                                                      \
}

#define STRING_SUPPORT(type)                        \
if (token_type == TOKEN_BEGINNING && mode == VALUE) \
  fputc('=', out);                                  \
                                                    \
fputc(ch, out);                                     \
                                                    \
if (token_type == STRING_INVERT_TYPE(type))         \
  break;                                            \
                                                    \
if (token_type == type) {                           \
  token_type = NORMAL;                              \
  break;                                            \
}                                                   \
                                                    \
token_type = type;                                  \
break;

void translate(FILE *out, int convert_depth)
{
  char  tagname[256] = { 0 };
  int   ch,
        is_escseq    = 0,
        taglen       = 0;

  enum {
    NONE,
    TAG,
    PROPERTY,
    VALUE,
    INNER_HTML
  } mode;

  enum {
    NORMAL,
    TOKEN_BEGINNING,
    PROPERTY_TOKEN,
    SINGLE_QUOTE_STRING,
    DOUBLE_QUOTE_STRING,
    COMMENT
  } token_type;

  mode = NONE;
  token_type = TOKEN_BEGINNING;

  if (convert_depth)
    mode = TAG;

  while ((ch = getchar()) != EOF) {
    if (token_type == COMMENT) {
      if (ch == '\n')
        token_type = TOKEN_BEGINNING;

      continue;
    }

    if (isspace(ch)) {
      if (  token_type == SINGLE_QUOTE_STRING
         || token_type == DOUBLE_QUOTE_STRING) {

        fputc(ch, out);
        continue;
      }

      if (token_type != TOKEN_BEGINNING) {
        if (mode == TAG) {
          indent(out, convert_depth);
          fprintf(out, "<%s", tagname);
          ++mode;
        } else if (mode == PROPERTY)
          ++mode;
        else if (mode == VALUE)
          --mode;

        token_type = TOKEN_BEGINNING;
      }
      continue;
    }

    switch (ch) {
      case ':':
        ESCSEQ_TEST();
        STRING_TEST();

        if (mode != PROPERTY) {
          error("':' not escaped");        /* ':' allowed only in properties */
          break;
        }

        if (token_type)
          token_type = PROPERTY_TOKEN;

        fputc(' ', out);
        break;

      case '(':
        ESCSEQ_TEST();
        STRING_TEST();

        if (mode == NONE) {
          fprintf(out, "<!DOCTYPE html>\n");
          mode = TAG;
          break;
        }

        if (mode == PROPERTY || mode == VALUE) {
          mode = INNER_HTML;
          fprintf(out, ">");
        }
        
        fputc('\n', out);
        translate(out, convert_depth + 1);
        break;

      case ')':
        ESCSEQ_TEST();
        STRING_TEST();

        if (mode == NONE) {
          error("')' sign all of a sudden");
          break;
        }

        /* beginning of something that should be commented */

        if (  (mode == PROPERTY && token_type == PROPERTY_TOKEN)
           || (mode == VALUE    && token_type == TOKEN_BEGINNING)) {

          error("Property without value");
          break;
        }

        if (  (mode == INNER_HTML)
           || (mode == PROPERTY && token_type == NORMAL)) {

          fputc('\n', out);
          indent(out, convert_depth);
          fprintf(out, "</%s>", tagname);
        } else if (  (mode == PROPERTY && token_type == TOKEN_BEGINNING)
                  || (mode == VALUE    && token_type == NORMAL)) {

          fprintf(out, " />");
        } else if (mode == TAG && token_type == NORMAL) {
          indent(out, convert_depth);
          fprintf(out, "<%s />", tagname);
        }

        /* I don't like the formatting, refactoring needed?
           (or that type of code that'll always be ugly) */

        /* end of something that should be commented xD */

        return;

      case '\\':
        ESCSEQ_TEST();

        if (mode == INNER_HTML && token_type == TOKEN_BEGINNING)
          fputc(' ', out);                            /* seperate text nodes */
        
        is_escseq = 1;
        break;

      case '\"':
        ESCSEQ_TEST();
        STRING_SUPPORT(DOUBLE_QUOTE_STRING);

      case '\'':
        ESCSEQ_TEST();
        STRING_SUPPORT(SINGLE_QUOTE_STRING);

      case ';':
        ESCSEQ_TEST();
        STRING_TEST();
        token_type = COMMENT;
        break;

      default:
        if (mode == NONE) {
          error("Not valid S-Expression file");
          break;
        } else if (mode == TAG) {
          if (taglen == 255) {
            error("Tagname is too long, maximum of 255 characters.");
            break;
          }

          tagname[taglen] = ch;
          ++taglen;
          break;
        }

        if (token_type == TOKEN_BEGINNING) {
          if (mode == PROPERTY) {
          /* if property is expected but has character other than ':', then it
             means that it's not property but inner html code so we close our
             tag and then display it as inner html character                 */
          
            fprintf(out, ">\n");
            indent(out, convert_depth + 1);
            mode = INNER_HTML;
          } else if (mode == VALUE)
            fputc('=', out);                                 /* property=value */
          else if (mode == INNER_HTML)
            fputc(' ', out);                            /* seperate text nodes */
        }

        fputc(ch, out);
        break;
    }

    if (token_type == TOKEN_BEGINNING)                   /* self-explanatory */
      token_type = NORMAL;
  }

  return;
}

int main()
{
  /*FILE *out;*/    /* temporary file storing stdout, printed when no errors */
  /*int   ch;*/

  /*if (!(out = tmpfile())) {
    perror("tmpfile");
    exit(1);
  }*/

  translate(stdout, 0);
  fputc('\n', stdout);

  /*while ((ch = fgetc(out)) != EOF)
    putchar(ch);*/

  exit(0);
}

/*
code version: v0.2

must-be-done list:

- support of html character entities, now they must be escaped like this:
  &copy\;
- much much better debugging needed
- bugfix: error "property without value" sometimes displayed more than once
- bugfix: when there are white spaces between '(' and tag name,
          and it's the root element it won't work
          (cause first token is parsed differently than others)
- more refined strings rules and debugging them

other things to do:

- fragmentation of "translate" function into some smaller functions
  highly considered
- some refactoring
- php/xml/sgml compatibility
- few predefined special tags
- maybe debugging should be in seperate module?
- string now behave very "preprocessor-like", they're fully pasted into code,
  even with quotation marks, I am considering a change that they would work
  like that only for values of properties

difference between sehtml without and with strings
   - v0.1 code:
     (span :style "display\:inline-block;transform\:rotate\(180deg\);" &copy;)
   - v0.2 code:
     (span :style "display: inline-block; transform: rotate(180deg);" &copy\;)

we weren't allowed to use spaces in our pseudo-string cause it will seperate
pseudo-string into seperate tokens,
every sehtml character must have been escaped before

*/
