/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf --output-file src/protvar.gen.c src/protvar.gperf  */
/* Computed positions: -k'' */

#line 8 "src/protvar.gperf"

#include <mbstring.h>
#include <string.h>
/* maximum key range = 2, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
/*ARGSUSED*/
static unsigned int
hash (register const char *str, register size_t len)
{
  return len;
}

const char *
in_protvar_word_set (register const char *str, register size_t len)
{
  enum
    {
      TOTAL_KEYWORDS = 2,
      MIN_WORD_LENGTH = 5,
      MAX_WORD_LENGTH = 6,
      MIN_HASH_VALUE = 5,
      MAX_HASH_VALUE = 6
    };

  static const unsigned char lengthtable[] =
    {
       0,  0,  0,  0,  0,  5,  6
    };
  static const char * const wordlist[] =
    {
      "", "", "", "", "",
      "shell",
      "status"
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        if (len == lengthtable[key])
          {
            register const char *s = wordlist[key];

            if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
              return s;
          }
    }
  return 0;
}
#line 14 "src/protvar.gperf"

bool
isprotvar(struct u8view sv)
{
	return in_protvar_word_set(sv.p, sv.len) != nullptr;
}
