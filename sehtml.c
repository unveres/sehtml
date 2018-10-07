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
    PROPERTY_TOKEN
  } token_type;

  mode = NONE;
  token_type = TOKEN_BEGINNING;

  if (convert_depth)
    mode = TAG;

  while ((ch = getchar()) != EOF) {
    if (isspace(ch)) {
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

        if (mode != PROPERTY) {
          error("':' not escaped");       /* ':'' allowed only in properties */
          break;
        }

        if (token_type)
          token_type = PROPERTY_TOKEN;

        fputc(' ', out);
        break;

      case '(':
        ESCSEQ_TEST();

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

must-be-done list:

- comments support
- strings support
- much much better debugging needed
- bugfix (error "property without value" sometimes displayed more than once)

other things to do:

- fragmentation of "translate" function into some smaller functions
  highly considered
- some refactoring
- php/xml/sgml compatibility
- few predefined special tags

*/
