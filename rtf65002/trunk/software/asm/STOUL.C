#include <ctype.h>

/* ---------------------------------------------------------------------------
   Description :
      Convert string to long based on prefix characters in string. See
   table below. Conversion stops on encountering the first character which
   is not a digit in the indicated radix. *outstr is updated to point past
   the end of the number. NULL may be passed for outstr to indicate its
   not used. Leading spaces are skipped in the input string.

   Prefix   type of number
      0x - hexidecimal
      0X
      0  - octal
      0o
      0O
      0b - binary
      0B
      1-9  decimal

--------------------------------------------------------------------------- */

unsigned long stoul(char *instr, char **outstr)
{
   unsigned long num = 0;
   char *str = instr;

   while(isspace(*str))
      ++*str;

   if (*str != '0')
   {
      while(isdigit(*str))
         num = (num * 10) + (*str++ - '0');
   }
   else
   {
      ++str;
      switch(*str)
      {
         case 'x':
         case 'X':
            for(++str; isxdigit(*str); ++str)
               num = (num * 16) + (isdigit(*str) ? *str - '0' : toupper(*str) - 'A' + 10);
            break;

         case 'o':
         case 'O':
            for(++str; *str >= '0' && *str <= '7'; ++str)
               num = (num * 8) + *str - '0';
            break;

         case 'b':
         case 'B':
            for(++str; *str == '0' || *str == '1'; ++str)
               num = (num + num) + *str - '0';
            break;

         default:
         {
            while('0' <= *str && *str <= '7')
            {
               num *= 8;
               num += *str++ -'0';
            }
         }
      }
   }
   if (outstr)
      *outstr = str;
   return (num);
}

