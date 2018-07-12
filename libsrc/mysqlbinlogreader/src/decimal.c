/* Copyright (c) 2000, 2011, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#line 18 "decimal.c"

/*
=======================================================================
  NOTE: this library implements SQL standard "exact numeric" type
  and is not at all generic, but rather intentinally crippled to
  follow the standard :)
=======================================================================
  Quoting the standard
  (SQL:2003, Part 2 Foundations, aka ISO/IEC 9075-2:2003)

4.4.2 Characteristics of numbers, page 27:

  An exact numeric type has a precision P and a scale S. P is a positive
  integer that determines the number of significant digits in a
  particular radix R, where R is either 2 or 10. S is a non-negative
  integer. Every value of an exact numeric type of scale S is of the
  form n*10^{-S}, where n is an integer such that �-R^P <= n <= R^P.

  [...]

  If an assignment of some number would result in a loss of its most
  significant digit, an exception condition is raised. If least
  significant digits are lost, implementation-defined rounding or
  truncating occurs, with no exception condition being raised.

  [...]

  Whenever an exact or approximate numeric value is assigned to an exact
  numeric value site, an approximation of its value that preserves
  leading significant digits after rounding or truncating is represented
  in the declared type of the target. The value is converted to have the
  precision and scale of the target. The choice of whether to truncate
  or round is implementation-defined.

  [...]

  All numeric values between the smallest and the largest value,
  inclusive, in a given exact numeric type have an approximation
  obtained by rounding or truncation for that type; it is
  implementation-defined which other numeric values have such
  approximations.

5.3 <literal>, page 143

  <exact numeric literal> ::=
    <unsigned integer> [ <period> [ <unsigned integer> ] ]
  | <period> <unsigned integer>

6.1 <data type>, page 165:

  19) The <scale> of an <exact numeric type> shall not be greater than
      the <precision> of the <exact numeric type>.

  20) For the <exact numeric type>s DECIMAL and NUMERIC:

    a) The maximum value of <precision> is implementation-defined.
       <precision> shall not be greater than this value.
    b) The maximum value of <scale> is implementation-defined. <scale>
       shall not be greater than this maximum value.

  21) NUMERIC specifies the data type exact numeric, with the decimal
      precision and scale specified by the <precision> and <scale>.

  22) DECIMAL specifies the data type exact numeric, with the decimal
      scale specified by the <scale> and the implementation-defined
      decimal precision equal to or greater than the value of the
      specified <precision>.

6.26 <numeric value expression>, page 241:

  1) If the declared type of both operands of a dyadic arithmetic
     operator is exact numeric, then the declared type of the result is
     an implementation-defined exact numeric type, with precision and
     scale determined as follows:

   a) Let S1 and S2 be the scale of the first and second operands
      respectively.
   b) The precision of the result of addition and subtraction is
      implementation-defined, and the scale is the maximum of S1 and S2.
   c) The precision of the result of multiplication is
      implementation-defined, and the scale is S1 + S2.
   d) The precision and scale of the result of division are
      implementation-defined.
*/

#include <my_global.h>
#include "myisampack.h"
#include <m_ctype.h>
#include <my_sys.h> /* for my_alloca */
#include <m_string.h>
#include "decimal.h"

/*
  Internally decimal numbers are stored base 10^9 (see DIG_BASE below)
  So one variable of type decimal_digit_t is limited:

      0 < decimal_digit <= DIG_MAX < DIG_BASE

  in the struct st_decimal_t:

    intg is the number of *decimal* digits (NOT number of decimal_digit_t's !)
         before the point
    frac - number of decimal digits after the point
    buf is an array of decimal_digit_t's
    len is the length of buf (length of allocated space) in decimal_digit_t's,
        not in bytes
*/
typedef decimal_digit_t dec1;
typedef longlong      dec2;

#define DIG_PER_DEC1 9
#define DIG_MASK     100000000
#define DIG_BASE     1000000000
#define DIG_MAX      (DIG_BASE-1)
#define DIG_BASE2    ((dec2)DIG_BASE * (dec2)DIG_BASE)
#define ROUND_UP(X)  (((X)+DIG_PER_DEC1-1)/DIG_PER_DEC1)
static const dec1 powers10[DIG_PER_DEC1+1]={
  1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
static const int dig2bytes[DIG_PER_DEC1+1]={0, 1, 1, 2, 2, 3, 3, 4, 4, 4};
static const dec1 frac_max[DIG_PER_DEC1-1]={
  900000000, 990000000, 999000000,
  999900000, 999990000, 999999000,
  999999900, 999999990 };
static double scaler10[]= {
  1.0, 1e10, 1e20, 1e30, 1e40, 1e50, 1e60, 1e70, 1e80, 1e90
};
static double scaler1[]= {
  1.0, 10.0, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9
};

#ifdef HAVE_purify
#define sanity(d) DBUG_ASSERT((d)->len > 0)
#else
#define sanity(d) DBUG_ASSERT((d)->len >0 && ((d)->buf[0] | \
                              (d)->buf[(d)->len-1] | 1))
#endif

#define FIX_INTG_FRAC_ERROR(len, intg1, frac1, error)                   \
        do                                                              \
        {                                                               \
          if (unlikely(intg1+frac1 > (len)))                            \
          {                                                             \
            if (unlikely(intg1 > (len)))                                \
            {                                                           \
              intg1=(len);                                              \
              frac1=0;                                                  \
              error=E_DEC_OVERFLOW;                                     \
            }                                                           \
            else                                                        \
            {                                                           \
              frac1=(len)-intg1;                                        \
              error=E_DEC_TRUNCATED;                                    \
            }                                                           \
          }                                                             \
          else                                                          \
            error=E_DEC_OK;                                             \
        } while(0)

#define ADD(to, from1, from2, carry)  /* assume carry <= 1 */           \
        do                                                              \
        {                                                               \
          dec1 a=(from1)+(from2)+(carry);                               \
          DBUG_ASSERT((carry) <= 1);                                    \
          if (((carry)= a >= DIG_BASE)) /* no division here! */         \
            a-=DIG_BASE;                                                \
          (to)=a;                                                       \
        } while(0)

#define ADD2(to, from1, from2, carry)                                   \
        do                                                              \
        {                                                               \
          dec2 a=((dec2)(from1))+(from2)+(carry);                       \
          if (((carry)= a >= DIG_BASE))                                 \
            a-=DIG_BASE;                                                \
          if (unlikely(a >= DIG_BASE))                                  \
          {                                                             \
            a-=DIG_BASE;                                                \
            carry++;                                                    \
          }                                                             \
          (to)=(dec1) a;                                                \
        } while(0)

#define SUB(to, from1, from2, carry) /* to=from1-from2 */               \
        do                                                              \
        {                                                               \
          dec1 a=(from1)-(from2)-(carry);                               \
          if (((carry)= a < 0))                                         \
            a+=DIG_BASE;                                                \
          (to)=a;                                                       \
        } while(0)

#define SUB2(to, from1, from2, carry) /* to=from1-from2 */              \
        do                                                              \
        {                                                               \
          dec1 a=(from1)-(from2)-(carry);                               \
          if (((carry)= a < 0))                                         \
            a+=DIG_BASE;                                                \
          if (unlikely(a < 0))                                          \
          {                                                             \
            a+=DIG_BASE;                                                \
            carry++;                                                    \
          }                                                             \
          (to)=a;                                                       \
        } while(0)

/*
  Get maximum value for given precision and scale

  SYNOPSIS
    max_decimal()
    precision/scale - see decimal_bin_size() below
    to              - decimal where where the result will be stored
                      to->buf and to->len must be set.
*/

void max_decimal(int precision, int frac, decimal_t *to)
{
  int intpart;
  dec1 *buf= to->buf;
  DBUG_ASSERT(precision && precision >= frac);

  to->sign= 0;
  if ((intpart= to->intg= (precision - frac)))
  {
    int firstdigits= intpart % DIG_PER_DEC1;
    if (firstdigits)
      *buf++= powers10[firstdigits] - 1; /* get 9 99 999 ... */
    for(intpart/= DIG_PER_DEC1; intpart; intpart--)
      *buf++= DIG_MAX;
  }

  if ((to->frac= frac))
  {
    int lastdigits= frac % DIG_PER_DEC1;
    for(frac/= DIG_PER_DEC1; frac; frac--)
      *buf++= DIG_MAX;
    if (lastdigits)
      *buf= frac_max[lastdigits - 1];
  }
}


static dec1 *remove_leading_zeroes(decimal_t *from, int *intg_result)
{
  int intg= from->intg, i;
  dec1 *buf0= from->buf;
  i= ((intg - 1) % DIG_PER_DEC1) + 1;
  while (intg > 0 && *buf0 == 0)
  {
    intg-= i;
    i= DIG_PER_DEC1;
    buf0++;
  }
  if (intg > 0)
  {
    for (i= (intg - 1) % DIG_PER_DEC1; *buf0 < powers10[i--]; intg--) ;
    DBUG_ASSERT(intg > 0);
  }
  else
    intg=0;
  *intg_result= intg;
  return buf0;
}


/*
  Count actual length of fraction part (without ending zeroes)

  SYNOPSIS
    decimal_actual_fraction()
    from    number for processing
*/

int decimal_actual_fraction(decimal_t *from)
{
  int frac= from->frac, i;
  dec1 *buf0= from->buf + ROUND_UP(from->intg) + ROUND_UP(frac) - 1;

  if (frac == 0)
    return 0;

  i= ((frac - 1) % DIG_PER_DEC1 + 1);
  while (frac > 0 && *buf0 == 0)
  {
    frac-= i;
    i= DIG_PER_DEC1;
    buf0--;
  }
  if (frac > 0)
  {
    for (i= DIG_PER_DEC1 - ((frac - 1) % DIG_PER_DEC1);
         *buf0 % powers10[i++] == 0;
         frac--) ;
  }
  return frac;
}


/*
  Convert decimal to its printable string representation

  SYNOPSIS
    decimal2string()
      from            - value to convert
      to              - points to buffer where string representation
                        should be stored
      *to_len         - in:  size of to buffer (incl. terminating '\0')
                        out: length of the actually written string (excl. '\0')
      fixed_precision - 0 if representation can be variable length and
                        fixed_decimals will not be checked in this case.
                        Put number as with fixed point position with this
                        number of digits (sign counted and decimal point is
                        counted)
      fixed_decimals  - number digits after point.
      filler          - character to fill gaps in case of fixed_precision > 0

  RETURN VALUE
    E_DEC_OK/E_DEC_TRUNCATED/E_DEC_OVERFLOW
*/

int decimal2string(decimal_t *from, char *to, int *to_len,
                   int fixed_precision, int fixed_decimals,
                   char filler)
{
  /* {intg_len, frac_len} output widths; {intg, frac} places in input */
  int len, intg, frac= from->frac, i, intg_len, frac_len, fill;
  /* number digits before decimal point */
  int fixed_intg= (fixed_precision ?
                   (fixed_precision - fixed_decimals) : 0);
  int error=E_DEC_OK;
  char *s=to;
  dec1 *buf, *buf0=from->buf, tmp;

  DBUG_ASSERT(*to_len >= 2+from->sign);

  /* removing leading zeroes */
  buf0= remove_leading_zeroes(from, &intg);
  if (unlikely(intg+frac==0))
  {
    intg=1;
    tmp=0;
    buf0=&tmp;
  }

  if (!(intg_len= fixed_precision ? fixed_intg : intg))
    intg_len= 1;
  frac_len= fixed_precision ? fixed_decimals : frac;
  len= from->sign + intg_len + test(frac) + frac_len;
  if (fixed_precision)
  {
    if (frac > fixed_decimals)
    {
      error= E_DEC_TRUNCATED;
      frac= fixed_decimals;
    }
    if (intg > fixed_intg)
    {
      error= E_DEC_OVERFLOW;
      intg= fixed_intg;
    }
  }
  else if (unlikely(len > --*to_len)) /* reserve one byte for \0 */
  {
    int j= len-*to_len;
    error= (frac && j <= frac + 1) ? E_DEC_TRUNCATED : E_DEC_OVERFLOW;
    if (frac && j >= frac + 1) j--;
    if (j > frac)
    {
      intg-= j-frac;
      frac= 0;
    }
    else
      frac-=j;
    len= from->sign + intg_len + test(frac) + frac_len;
  }
  *to_len=len;
  s[len]=0;

  if (from->sign)
    *s++='-';

  if (frac)
  {
    char *s1= s + intg_len;
    fill= frac_len - frac;
    buf=buf0+ROUND_UP(intg);
    *s1++='.';
    for (; frac>0; frac-=DIG_PER_DEC1)
    {
      dec1 x=*buf++;
      for (i=min(frac, DIG_PER_DEC1); i; i--)
      {
        dec1 y=x/DIG_MASK;
        *s1++='0'+(uchar)y;
        x-=y*DIG_MASK;
        x*=10;
      }
    }
    for(; fill; fill--)
      *s1++=filler;
  }

  fill= intg_len - intg;
  if (intg == 0)
    fill--; /* symbol 0 before digital point */
  for(; fill; fill--)
    *s++=filler;
  if (intg)
  {
    s+=intg;
    for (buf=buf0+ROUND_UP(intg); intg>0; intg-=DIG_PER_DEC1)
    {
      dec1 x=*--buf;
      for (i=min(intg, DIG_PER_DEC1); i; i--)
      {
        dec1 y=x/10;
        *--s='0'+(uchar)(x-y*10);
        x=y;
      }
    }
  }
  else
    *s= '0';
  return error;
}


/*
  Return bounds of decimal digits in the number

  SYNOPSIS
    digits_bounds()
      from         - decimal number for processing
      start_result - index (from 0 ) of first decimal digits will
                     be written by this address
      end_result   - index of position just after last decimal digit
                     be written by this address
*/

static void digits_bounds(decimal_t *from, int *start_result, int *end_result)
{
  int start, stop, i;
  dec1 *buf_beg= from->buf;
  dec1 *end= from->buf + ROUND_UP(from->intg) + ROUND_UP(from->frac);
  dec1 *buf_end= end - 1;

  /* find non-zero digit from number begining */
  while (buf_beg < end && *buf_beg == 0)
    buf_beg++;

  if (buf_beg >= end)
  {
    /* it is zero */
    *start_result= *end_result= 0;
    return;
  }

  /* find non-zero decimal digit from number begining */
  if (buf_beg == from->buf && from->intg)
  {
    start= DIG_PER_DEC1 - (i= ((from->intg-1) % DIG_PER_DEC1 + 1));
    i--;
  }
  else
  {
    i= DIG_PER_DEC1 - 1;
    start= (int) ((buf_beg - from->buf) * DIG_PER_DEC1);
  }
  if (buf_beg < end)
    for (; *buf_beg < powers10[i--]; start++) ;
  *start_result= start; /* index of first decimal digit (from 0) */

  /* find non-zero digit at the end */
  while (buf_end > buf_beg  && *buf_end == 0)
    buf_end--;
  /* find non-zero decimal digit from the end */
  if (buf_end == end - 1 && from->frac)
  {
    stop= (int) (((buf_end - from->buf) * DIG_PER_DEC1 +
           (i= ((from->frac - 1) % DIG_PER_DEC1 + 1))));
    i= DIG_PER_DEC1 - i + 1;
  }
  else
  {
    stop= (int) ((buf_end - from->buf + 1) * DIG_PER_DEC1);
    i= 1;
  }
  for (; *buf_end % powers10[i++] == 0; stop--) ;
  *end_result= stop; /* index of position after last decimal digit (from 0) */
}


/*
  Left shift for alignment of data in buffer

  SYNOPSIS
    do_mini_left_shift()
    dec     pointer to decimal number which have to be shifted
    shift   number of decimal digits on which it should be shifted
    beg/end bounds of decimal digits (see digits_bounds())

  NOTE
    Result fitting in the buffer should be garanted.
    'shift' have to be from 1 to DIG_PER_DEC1-1 (inclusive)
*/

void do_mini_left_shift(decimal_t *dec, int shift, int beg, int last)
{
  dec1 *from= dec->buf + ROUND_UP(beg + 1) - 1;
  dec1 *end= dec->buf + ROUND_UP(last) - 1;
  int c_shift= DIG_PER_DEC1 - shift;
  DBUG_ASSERT(from >= dec->buf);
  DBUG_ASSERT(end < dec->buf + dec->len);
  if (beg % DIG_PER_DEC1 < shift)
    *(from - 1)= (*from) / powers10[c_shift];
  for(; from < end; from++)
    *from= ((*from % powers10[c_shift]) * powers10[shift] +
            (*(from + 1)) / powers10[c_shift]);
  *from= (*from % powers10[c_shift]) * powers10[shift];
}


/*
  Right shift for alignment of data in buffer

  SYNOPSIS
    do_mini_left_shift()
    dec     pointer to decimal number which have to be shifted
    shift   number of decimal digits on which it should be shifted
    beg/end bounds of decimal digits (see digits_bounds())

  NOTE
    Result fitting in the buffer should be garanted.
    'shift' have to be from 1 to DIG_PER_DEC1-1 (inclusive)
*/

void do_mini_right_shift(decimal_t *dec, int shift, int beg, int last)
{
  dec1 *from= dec->buf + ROUND_UP(last) - 1;
  dec1 *end= dec->buf + ROUND_UP(beg + 1) - 1;
  int c_shift= DIG_PER_DEC1 - shift;
  DBUG_ASSERT(from < dec->buf + dec->len);
  DBUG_ASSERT(end >= dec->buf);
  if (DIG_PER_DEC1 - ((last - 1) % DIG_PER_DEC1 + 1) < shift)
    *(from + 1)= (*from % powers10[shift]) * powers10[c_shift];
  for(; from > end; from--)
    *from= (*from / powers10[shift] +
            (*(from - 1) % powers10[shift]) * powers10[c_shift]);
  *from= *from / powers10[shift];
}


/*
  Shift of decimal digits in given number (with rounding if it need)

  SYNOPSIS
    decimal_shift()
    dec       number to be shifted
    shift     number of decimal positions
              shift > 0 means shift to left shift
              shift < 0 meand right shift
  NOTE
    In fact it is multipling on 10^shift.
  RETURN
    E_DEC_OK          OK
    E_DEC_OVERFLOW    operation lead to overflow, number is untoched
    E_DEC_TRUNCATED   number was rounded to fit into buffer
*/

int decimal_shift(decimal_t *dec, int shift)
{
  /* index of first non zero digit (all indexes from 0) */
  int beg;
  /* index of position after last decimal digit */
  int end;
  /* index of digit position just after point */
  int point= ROUND_UP(dec->intg) * DIG_PER_DEC1;
  /* new point position */
  int new_point= point + shift;
  /* number of digits in result */
  int digits_int, digits_frac;
  /* length of result and new fraction in big digits*/
  int new_len, new_frac_len;
  /* return code */
  int err= E_DEC_OK;
  int new_front;

  if (shift == 0)
    return E_DEC_OK;

  digits_bounds(dec, &beg, &end);

  if (beg == end)
  {
    decimal_make_zero(dec);
    return E_DEC_OK;
  }

  digits_int= new_point - beg;
  set_if_bigger(digits_int, 0);
  digits_frac= end - new_point;
  set_if_bigger(digits_frac, 0);

  if ((new_len= ROUND_UP(digits_int) + (new_frac_len= ROUND_UP(digits_frac))) >
      dec->len)
  {
    int lack= new_len - dec->len;
    int diff;

    if (new_frac_len < lack)
      return E_DEC_OVERFLOW; /* lack more then we have in fraction */

    /* cat off fraction part to allow new number to fit in our buffer */
    err= E_DEC_TRUNCATED;
    new_frac_len-= lack;
    diff= digits_frac - (new_frac_len * DIG_PER_DEC1);
    /* Make rounding method as parameter? */
    decimal_round(dec, dec, end - point - diff, HALF_UP);
    end-= diff;
    digits_frac= new_frac_len * DIG_PER_DEC1;

    if (end <= beg)
    {
      /*
        we lost all digits (they will be shifted out of buffer), so we can
        just return 0
      */
      decimal_make_zero(dec);
      return E_DEC_TRUNCATED;
    }
  }

  if (shift % DIG_PER_DEC1)
  {
    int l_mini_shift, r_mini_shift, mini_shift;
    int do_left;
    /*
      Calculate left/right shift to align decimal digits inside our bug
      digits correctly
    */
    if (shift > 0)
    {
      l_mini_shift= shift % DIG_PER_DEC1;
      r_mini_shift= DIG_PER_DEC1 - l_mini_shift;
      /*
        It is left shift so prefer left shift, but if we have not place from
        left, we have to have it from right, because we checked length of
        result
      */
      do_left= l_mini_shift <= beg;
      DBUG_ASSERT(do_left || (dec->len * DIG_PER_DEC1 - end) >= r_mini_shift);
    }
    else
    {
      r_mini_shift= (-shift) % DIG_PER_DEC1;
      l_mini_shift= DIG_PER_DEC1 - r_mini_shift;
      /* see comment above */
      do_left= !((dec->len * DIG_PER_DEC1 - end) >= r_mini_shift);
      DBUG_ASSERT(!do_left || l_mini_shift <= beg);
    }
    if (do_left)
    {
      do_mini_left_shift(dec, l_mini_shift, beg, end);
      mini_shift=- l_mini_shift;
    }
    else
    {
      do_mini_right_shift(dec, r_mini_shift, beg, end);
      mini_shift= r_mini_shift;
    }
    new_point+= mini_shift;
    /*
      If number is shifted and correctly aligned in buffer we can
      finish
    */
    if (!(shift+= mini_shift) && (new_point - digits_int) < DIG_PER_DEC1)
    {
      dec->intg= digits_int;
      dec->frac= digits_frac;
      return err;                 /* already shifted as it should be */
    }
    beg+= mini_shift;
    end+= mini_shift;
  }

  /* if new 'decimal front' is in first digit, we do not need move digits */
  if ((new_front= (new_point - digits_int)) >= DIG_PER_DEC1 ||
      new_front < 0)
  {
    /* need to move digits */
    int d_shift;
    dec1 *to, *barier;
    if (new_front > 0)
    {
      /* move left */
      d_shift= new_front / DIG_PER_DEC1;
      to= dec->buf + (ROUND_UP(beg + 1) - 1 - d_shift);
      barier= dec->buf + (ROUND_UP(end) - 1 - d_shift);
      DBUG_ASSERT(to >= dec->buf);
      DBUG_ASSERT(barier + d_shift < dec->buf + dec->len);
      for(; to <= barier; to++)
        *to= *(to + d_shift);
      for(barier+= d_shift; to <= barier; to++)
        *to= 0;
      d_shift= -d_shift;
    }
    else
    {
      /* move right */
      d_shift= (1 - new_front) / DIG_PER_DEC1;
      to= dec->buf + ROUND_UP(end) - 1 + d_shift;
      barier= dec->buf + ROUND_UP(beg + 1) - 1 + d_shift;
      DBUG_ASSERT(to < dec->buf + dec->len);
      DBUG_ASSERT(barier - d_shift >= dec->buf);
      for(; to >= barier; to--)
        *to= *(to - d_shift);
      for(barier-= d_shift; to >= barier; to--)
        *to= 0;
    }
    d_shift*= DIG_PER_DEC1;
    beg+= d_shift;
    end+= d_shift;
    new_point+= d_shift;
  }

  /*
    If there are gaps then fill ren with 0.

    Only one of following 'for' loops will work becouse beg <= end
  */
  beg= ROUND_UP(beg + 1) - 1;
  end= ROUND_UP(end) - 1;
  DBUG_ASSERT(new_point >= 0);
  
  /* We don't want negative new_point below */
  if (new_point != 0)
    new_point= ROUND_UP(new_point) - 1;

  if (new_point > end)
  {
    do
    {
      dec->buf[new_point]=0;
    } while (--new_point > end);
  }
  else
  {
    for (; new_point < beg; new_point++)
      dec->buf[new_point]= 0;
  }
  dec->intg= digits_int;
  dec->frac= digits_frac;
  return err;
}


/*
  Convert string to decimal

  SYNOPSIS
    internal_str2decl()
      from    - value to convert. Doesn't have to be \0 terminated!
      to      - decimal where where the result will be stored
                to->buf and to->len must be set.
      end     - Pointer to pointer to end of string. Will on return be
		set to the char after the last used character
      fixed   - use to->intg, to->frac as limits for input number

  NOTE
    to->intg and to->frac can be modified even when fixed=1
    (but only decreased, in this case)

  RETURN VALUE
    E_DEC_OK/E_DEC_TRUNCATED/E_DEC_OVERFLOW/E_DEC_BAD_NUM/E_DEC_OOM
    In case of E_DEC_FATAL_ERROR *to is set to decimal zero
    (to make error handling easier)
*/

int
internal_str2dec(const char *from, decimal_t *to, char **end, my_bool fixed)
{
  const char *s= from, *s1, *endp, *end_of_string= *end;
  int i, intg, frac, error, intg1, frac1;
  dec1 x,*buf;
  sanity(to);

  error= E_DEC_BAD_NUM;                         /* In case of bad number */
  while (s < end_of_string && my_isspace(&my_charset_latin1, *s))
    s++;
  if (s == end_of_string)
    goto fatal_error;

  if ((to->sign= (*s == '-')))
    s++;
  else if (*s == '+')
    s++;

  s1=s;
  while (s < end_of_string && my_isdigit(&my_charset_latin1, *s))
    s++;
  intg= (int) (s-s1);
  if (s < end_of_string && *s=='.')
  {
    endp= s+1;
    while (endp < end_of_string && my_isdigit(&my_charset_latin1, *endp))
      endp++;
    frac= (int) (endp - s - 1);
  }
  else
  {
    frac= 0;
    endp= s;
  }

  *end= (char*) endp;

  if (frac+intg == 0)
    goto fatal_error;

  error= 0;
  if (fixed)
  {
    if (frac > to->frac)
    {
      error=E_DEC_TRUNCATED;
      frac=to->frac;
    }
    if (intg > to->intg)
    {
      error=E_DEC_OVERFLOW;
      intg=to->intg;
    }
    intg1=ROUND_UP(intg);
    frac1=ROUND_UP(frac);
    if (intg1+frac1 > to->len)
    {
      error= E_DEC_OOM;
      goto fatal_error;
    }
  }
  else
  {
    intg1=ROUND_UP(intg);
    frac1=ROUND_UP(frac);
    FIX_INTG_FRAC_ERROR(to->len, intg1, frac1, error);
    if (unlikely(error))
    {
      frac=frac1*DIG_PER_DEC1;
      if (error == E_DEC_OVERFLOW)
        intg=intg1*DIG_PER_DEC1;
    }
  }
  /* Error is guranteed to be set here */
  to->intg=intg;
  to->frac=frac;

  buf=to->buf+intg1;
  s1=s;

  for (x=0, i=0; intg; intg--)
  {
    x+= (*--s - '0')*powers10[i];

    if (unlikely(++i == DIG_PER_DEC1))
    {
      *--buf=x;
      x=0;
      i=0;
    }
  }
  if (i)
    *--buf=x;

  buf=to->buf+intg1;
  for (x=0, i=0; frac; frac--)
  {
    x= (*++s1 - '0') + x*10;

    if (unlikely(++i == DIG_PER_DEC1))
    {
      *buf++=x;
      x=0;
      i=0;
    }
  }
  if (i)
    *buf=x*powers10[DIG_PER_DEC1-i];

  /* Handle exponent */
  if (endp+1 < end_of_string && (*endp == 'e' || *endp == 'E'))
  {
    int str_error;
    longlong exponent= my_strtoll10(endp+1, (char**) &end_of_string,
                                    &str_error);

    if (end_of_string != endp +1)               /* If at least one digit */
    {
      *end= (char*) end_of_string;
      if (str_error > 0)
      {
        error= E_DEC_BAD_NUM;
        goto fatal_error;
      }
      if (exponent > INT_MAX/2 || (str_error == 0 && exponent < 0))
      {
        error= E_DEC_OVERFLOW;
        goto fatal_error;
      }
      if (exponent < INT_MIN/2 && error != E_DEC_OVERFLOW)
      {
        error= E_DEC_TRUNCATED;
        goto fatal_error;
      }
      if (error != E_DEC_OVERFLOW)
        error= decimal_shift(to, (int) exponent);
    }
  }
  return error;

fatal_error:
  decimal_make_zero(to);
  return error;
}


/*
  Convert decimal to double

  SYNOPSIS
    decimal2double()
      from    - value to convert
      to      - result will be stored there

  RETURN VALUE
    E_DEC_OK
*/

int decimal2double(decimal_t *from, double *to)
{
  double result= 0.0;
  int i, exp= 0;
  dec1 *buf= from->buf;

  for (i= from->intg; i > 0;  i-= DIG_PER_DEC1)
    result= result * DIG_BASE + *buf++;

  for (i= from->frac; i > 0; i-= DIG_PER_DEC1) {
    result= result * DIG_BASE + *buf++;
    exp+= DIG_PER_DEC1;
  }

  DBUG_PRINT("info", ("interm.: %f %d %f", result, exp,
             scaler10[exp / 10] * scaler1[exp % 10]));

  result/= scaler10[exp / 10] * scaler1[exp % 10];

  *to= from->sign ? -result : result;

  DBUG_PRINT("info", ("result: %f", *to));

  return E_DEC_OK;
}

/*
  Convert double to decimal

  SYNOPSIS
    double2decimal()
      from    - value to convert
      to      - result will be stored there

  RETURN VALUE
    E_DEC_OK/E_DEC_OVERFLOW/E_DEC_TRUNCATED
*/

int double2decimal(double from, decimal_t *to)
{
  /* TODO: fix it, when we'll have dtoa */
  char buff[400], *end;
  int length, res;
  DBUG_ENTER("double2decimal");
  length= sprintf(buff, "%.16G", from);
  DBUG_PRINT("info",("from: %g  from_as_str: %s", from, buff));
  end= buff+length;
  res= string2decimal(buff, to, &end);
  DBUG_PRINT("exit", ("res: %d", res));
  DBUG_RETURN(res);
}


static int ull2dec(ulonglong from, decimal_t *to)
{
  int intg1, error=E_DEC_OK;
  ulonglong x=from;
  dec1 *buf;

  sanity(to);

  for (intg1=1; from >= DIG_BASE; intg1++, from/=DIG_BASE) ;
  if (unlikely(intg1 > to->len))
  {
    intg1=to->len;
    error=E_DEC_OVERFLOW;
  }
  to->frac=0;
  to->intg=intg1*DIG_PER_DEC1;

  for (buf=to->buf+intg1; intg1; intg1--)
  {
    ulonglong y=x/DIG_BASE;
    *--buf=(dec1)(x-y*DIG_BASE);
    x=y;
  }
  return error;
}

int ulonglong2decimal(ulonglong from, decimal_t *to)
{
  to->sign=0;
  return ull2dec(from, to);
}

int longlong2decimal(longlong from, decimal_t *to)
{
  if ((to->sign= from < 0))
    return ull2dec(-from, to);
  return ull2dec(from, to);
}

int decimal2ulonglong(decimal_t *from, ulonglong *to)
{
  dec1 *buf=from->buf;
  ulonglong x=0;
  int intg, frac;

  if (from->sign)
  {
      *to=ULL(0);
      return E_DEC_OVERFLOW;
  }

  for (intg=from->intg; intg > 0; intg-=DIG_PER_DEC1)
  {
    ulonglong y=x;
    x=x*DIG_BASE + *buf++;
    if (unlikely(y > ((ulonglong) ULONGLONG_MAX/DIG_BASE) || x < y))
    {
      *to=ULONGLONG_MAX;
      return E_DEC_OVERFLOW;
    }
  }
  *to=x;
  for (frac=from->frac; unlikely(frac > 0); frac-=DIG_PER_DEC1)
    if (*buf++)
      return E_DEC_TRUNCATED;
  return E_DEC_OK;
}

int decimal2longlong(decimal_t *from, longlong *to)
{
  dec1 *buf=from->buf;
  longlong x=0;
  int intg, frac;

  for (intg=from->intg; intg > 0; intg-=DIG_PER_DEC1)
  {
    longlong y=x;
    /*
      Attention: trick!
      we're calculating -|from| instead of |from| here
      because |LONGLONG_MIN| > LONGLONG_MAX
      so we can convert -9223372036854775808 correctly
    */
    x=x*DIG_BASE - *buf++;
    if (unlikely(y < (LONGLONG_MIN/DIG_BASE) || x > y))
    {
      /*
        the decimal is bigger than any possible integer
        return border integer depending on the sign
      */
      *to= from->sign ? LONGLONG_MIN : LONGLONG_MAX;
      return E_DEC_OVERFLOW;
    }
  }
  /* boundary case: 9223372036854775808 */
  if (unlikely(from->sign==0 && x == LONGLONG_MIN))
  {
    *to= LONGLONG_MAX;
    return E_DEC_OVERFLOW;
  }

  *to=from->sign ? x : -x;
  for (frac=from->frac; unlikely(frac > 0); frac-=DIG_PER_DEC1)
    if (*buf++)
      return E_DEC_TRUNCATED;
  return E_DEC_OK;
}

/*
  Convert decimal to its binary fixed-length representation
  two representations of the same length can be compared with memcmp
  with the correct -1/0/+1 result

  SYNOPSIS
    decimal2bin()
      from    - value to convert
      to      - points to buffer where string representation should be stored
      precision/scale - see decimal_bin_size() below

  NOTE
    the buffer is assumed to be of the size decimal_bin_size(precision, scale)

  RETURN VALUE
    E_DEC_OK/E_DEC_TRUNCATED/E_DEC_OVERFLOW

  DESCRIPTION
    for storage decimal numbers are converted to the "binary" format.

    This format has the following properties:
      1. length of the binary representation depends on the {precision, scale}
      as provided by the caller and NOT on the intg/frac of the decimal to
      convert.
      2. binary representations of the same {precision, scale} can be compared
      with memcmp - with the same result as decimal_cmp() of the original
      decimals (not taking into account possible precision loss during
      conversion).

    This binary format is as follows:
      1. First the number is converted to have a requested precision and scale.
      2. Every full DIG_PER_DEC1 digits of intg part are stored in 4 bytes
         as is
      3. The first intg % DIG_PER_DEC1 digits are stored in the reduced
         number of bytes (enough bytes to store this number of digits -
         see dig2bytes)
      4. same for frac - full decimal_digit_t's are stored as is,
         the last frac % DIG_PER_DEC1 digits - in the reduced number of bytes.
      5. If the number is negative - every byte is inversed.
      5. The very first bit of the resulting byte array is inverted (because
         memcmp compares unsigned bytes, see property 2 above)

    Example:

      1234567890.1234

    internally is represented as 3 decimal_digit_t's

      1 234567890 123400000

    (assuming we want a binary representation with precision=14, scale=4)
    in hex it's

      00-00-00-01  0D-FB-38-D2  07-5A-EF-40

    now, middle decimal_digit_t is full - it stores 9 decimal digits. It goes
    into binary representation as is:


      ...........  0D-FB-38-D2 ............

    First decimal_digit_t has only one decimal digit. We can store one digit in
    one byte, no need to waste four:

                01 0D-FB-38-D2 ............

    now, last digit. It's 123400000. We can store 1234 in two bytes:

                01 0D-FB-38-D2 04-D2

    So, we've packed 12 bytes number in 7 bytes.
    And now we invert the highest bit to get the final result:

                81 0D FB 38 D2 04 D2

    And for -1234567890.1234 it would be

                7E F2 04 37 2D FB 2D
*/
int decimal2bin(decimal_t *from, uchar *to, int precision, int frac)
{
  dec1 mask=from->sign ? -1 : 0, *buf1=from->buf, *stop1;
  int error=E_DEC_OK, intg=precision-frac,
      isize1, intg1, intg1x, from_intg,
      intg0=intg/DIG_PER_DEC1,
      frac0=frac/DIG_PER_DEC1,
      intg0x=intg-intg0*DIG_PER_DEC1,
      frac0x=frac-frac0*DIG_PER_DEC1,
      frac1=from->frac/DIG_PER_DEC1,
      frac1x=from->frac-frac1*DIG_PER_DEC1,
      isize0=intg0*sizeof(dec1)+dig2bytes[intg0x],
      fsize0=frac0*sizeof(dec1)+dig2bytes[frac0x],
      fsize1=frac1*sizeof(dec1)+dig2bytes[frac1x];
  const int orig_isize0= isize0;
  const int orig_fsize0= fsize0;
  uchar *orig_to= to;

  buf1= remove_leading_zeroes(from, &from_intg);

  if (unlikely(from_intg+fsize1==0))
  {
    mask=0; /* just in case */
    intg=1;
    buf1=&mask;
  }

  intg1=from_intg/DIG_PER_DEC1;
  intg1x=from_intg-intg1*DIG_PER_DEC1;
  isize1=intg1*sizeof(dec1)+dig2bytes[intg1x];

  if (intg < from_intg)
  {
    buf1+=intg1-intg0+(intg1x>0)-(intg0x>0);
    intg1=intg0; intg1x=intg0x;
    error=E_DEC_OVERFLOW;
  }
  else if (isize0 > isize1)
  {
    while (isize0-- > isize1)
      *to++= (char)mask;
  }
  if (fsize0 < fsize1)
  {
    frac1=frac0; frac1x=frac0x;
    error=E_DEC_TRUNCATED;
  }
  else if (fsize0 > fsize1 && frac1x)
  {
    if (frac0 == frac1)
    {
      frac1x=frac0x;
      fsize0= fsize1;
    }
    else
    {
      frac1++;
      frac1x=0;
    }
  }

  /* intg1x part */
  if (intg1x)
  {
    int i=dig2bytes[intg1x];
    dec1 x=(*buf1++ % powers10[intg1x]) ^ mask;
    switch (i)
    {
      case 1: mi_int1store(to, x); break;
      case 2: mi_int2store(to, x); break;
      case 3: mi_int3store(to, x); break;
      case 4: mi_int4store(to, x); break;
      default: DBUG_ASSERT(0);
    }
    to+=i;
  }

  /* intg1+frac1 part */
  for (stop1=buf1+intg1+frac1; buf1 < stop1; to+=sizeof(dec1))
  {
    dec1 x=*buf1++ ^ mask;
    DBUG_ASSERT(sizeof(dec1) == 4);
    mi_int4store(to, x);
  }

  /* frac1x part */
  if (frac1x)
  {
    dec1 x;
    int i=dig2bytes[frac1x],
        lim=(frac1 < frac0 ? DIG_PER_DEC1 : frac0x);
    while (frac1x < lim && dig2bytes[frac1x] == i)
      frac1x++;
    x=(*buf1 / powers10[DIG_PER_DEC1 - frac1x]) ^ mask;
    switch (i)
    {
      case 1: mi_int1store(to, x); break;
      case 2: mi_int2store(to, x); break;
      case 3: mi_int3store(to, x); break;
      case 4: mi_int4store(to, x); break;
      default: DBUG_ASSERT(0);
    }
    to+=i;
  }
  if (fsize0 > fsize1)
  {
    uchar *to_end= orig_to + orig_fsize0 + orig_isize0;

    while (fsize0-- > fsize1 && to < to_end)
      *to++= (uchar)mask;
  }
  orig_to[0]^= 0x80;

  /* Check that we have written the whole decimal and nothing more */
  DBUG_ASSERT(to == orig_to + orig_fsize0 + orig_isize0);
  return error;
}

/*
  Restores decimal from its binary fixed-length representation

  SYNOPSIS
    bin2decimal()
      from    - value to convert
      to      - result
      precision/scale - see decimal_bin_size() below

  NOTE
    see decimal2bin()
    the buffer is assumed to be of the size decimal_bin_size(precision, scale)

  RETURN VALUE
    E_DEC_OK/E_DEC_TRUNCATED/E_DEC_OVERFLOW
*/

int bin2decimal(const uchar *from, decimal_t *to, int precision, int scale)
{
  int error=E_DEC_OK, intg=precision-scale,
      intg0=intg/DIG_PER_DEC1, frac0=scale/DIG_PER_DEC1,
      intg0x=intg-intg0*DIG_PER_DEC1, frac0x=scale-frac0*DIG_PER_DEC1,
      intg1=intg0+(intg0x>0), frac1=frac0+(frac0x>0);
  dec1 *buf=to->buf, mask=(*from & 0x80) ? 0 : -1;
  const uchar *stop;
  uchar *d_copy;
  int bin_size= decimal_bin_size(precision, scale);

  sanity(to);
  d_copy= (uchar*) my_alloca(bin_size);
  memcpy(d_copy, from, bin_size);
  d_copy[0]^= 0x80;
  from= d_copy;

  FIX_INTG_FRAC_ERROR(to->len, intg1, frac1, error);
  if (unlikely(error))
  {
    if (intg1 < intg0+(intg0x>0))
    {
      from+=dig2bytes[intg0x]+sizeof(dec1)*(intg0-intg1);
      frac0=frac0x=intg0x=0;
      intg0=intg1;
    }
    else
    {
      frac0x=0;
      frac0=frac1;
    }
  }

  to->sign=(mask != 0);
  to->intg=intg0*DIG_PER_DEC1+intg0x;
  to->frac=frac0*DIG_PER_DEC1+frac0x;

  if (intg0x)
  {
    int i=dig2bytes[intg0x];
    dec1 UNINIT_VAR(x);
    switch (i)
    {
      case 1: x=mi_sint1korr(from); break;
      case 2: x=mi_sint2korr(from); break;
      case 3: x=mi_sint3korr(from); break;
      case 4: x=mi_sint4korr(from); break;
      default: DBUG_ASSERT(0);
    }
    from+=i;
    *buf=x ^ mask;
    if (((ulonglong)*buf) >= (ulonglong) powers10[intg0x+1])
      goto err;
    if (buf > to->buf || *buf != 0)
      buf++;
    else
      to->intg-=intg0x;
  }
  for (stop=from+intg0*sizeof(dec1); from < stop; from+=sizeof(dec1))
  {
    DBUG_ASSERT(sizeof(dec1) == 4);
    *buf=mi_sint4korr(from) ^ mask;
    if (((uint32)*buf) > DIG_MAX)
      goto err;
    if (buf > to->buf || *buf != 0)
      buf++;
    else
      to->intg-=DIG_PER_DEC1;
  }
  DBUG_ASSERT(to->intg >=0);
  for (stop=from+frac0*sizeof(dec1); from < stop; from+=sizeof(dec1))
  {
    DBUG_ASSERT(sizeof(dec1) == 4);
    *buf=mi_sint4korr(from) ^ mask;
    if (((uint32)*buf) > DIG_MAX)
      goto err;
    buf++;
  }
  if (frac0x)
  {
    int i=dig2bytes[frac0x];
    dec1 UNINIT_VAR(x);
    switch (i)
    {
      case 1: x=mi_sint1korr(from); break;
      case 2: x=mi_sint2korr(from); break;
      case 3: x=mi_sint3korr(from); break;
      case 4: x=mi_sint4korr(from); break;
      default: DBUG_ASSERT(0);
    }
    *buf=(x ^ mask) * powers10[DIG_PER_DEC1 - frac0x];
    if (((uint32)*buf) > DIG_MAX)
      goto err;
    buf++;
  }
  my_afree(d_copy);
  return error;

err:
  my_afree(d_copy);
  decimal_make_zero(((decimal_t*) to));
  return(E_DEC_BAD_NUM);
}

/*
  Returns the size of array to hold a decimal with given precision and scale

  RETURN VALUE
    size in dec1
    (multiply by sizeof(dec1) to get the size if bytes)
*/

int decimal_size(int precision, int scale)
{
  DBUG_ASSERT(scale >= 0 && precision > 0 && scale <= precision);
  return ROUND_UP(precision-scale)+ROUND_UP(scale);
}

/*
  Returns the size of array to hold a binary representation of a decimal

  RETURN VALUE
    size in bytes
*/

int decimal_bin_size(int precision, int scale)
{
  int intg=precision-scale,
      intg0=intg/DIG_PER_DEC1, frac0=scale/DIG_PER_DEC1,
      intg0x=intg-intg0*DIG_PER_DEC1, frac0x=scale-frac0*DIG_PER_DEC1;

  DBUG_ASSERT(scale >= 0 && precision > 0 && scale <= precision);
  return intg0*sizeof(dec1)+dig2bytes[intg0x]+
         frac0*sizeof(dec1)+dig2bytes[frac0x];
}

/*
  Rounds the decimal to "scale" digits

  SYNOPSIS
    decimal_round()
      from    - decimal to round,
      to      - result buffer. from==to is allowed
      scale   - to what position to round. can be negative!
      mode    - round to nearest even or truncate

  NOTES
    scale can be negative !
    one TRUNCATED error (line XXX below) isn't treated very logical :(

  RETURN VALUE
    E_DEC_OK/E_DEC_TRUNCATED
*/

int
decimal_round(decimal_t *from, decimal_t *to, int scale,
              decimal_round_mode mode)
{
  int frac0=scale>0 ? ROUND_UP(scale) : scale/DIG_PER_DEC1,
    frac1=ROUND_UP(from->frac), UNINIT_VAR(round_digit),
      intg0=ROUND_UP(from->intg), error=E_DEC_OK, len=to->len,
      intg1=ROUND_UP(from->intg +
                     (((intg0 + frac0)>0) && (from->buf[0] == DIG_MAX)));
  dec1 *buf0=from->buf, *buf1=to->buf, x, y, carry=0;
  int first_dig;

  sanity(to);

  switch (mode) {
  case HALF_UP:
  case HALF_EVEN:       round_digit=5; break;
  case CEILING:         round_digit= from->sign ? 10 : 0; break;
  case FLOOR:           round_digit= from->sign ? 0 : 10; break;
  case TRUNCATE:        round_digit=10; break;
  default: DBUG_ASSERT(0);
  }

  if (unlikely(frac0+intg0 > len))
  {
    frac0=len-intg0;
    scale=frac0*DIG_PER_DEC1;
    error=E_DEC_TRUNCATED;
  }

  if (scale+from->intg < 0)
  {
    decimal_make_zero(to);
    return E_DEC_OK;
  }

  if (to != from || intg1>intg0)
  {
    dec1 *p0= buf0+intg0+max(frac1, frac0);
    dec1 *p1= buf1+intg1+max(frac1, frac0);

    while (buf0 < p0)
      *(--p1) = *(--p0);
    if (unlikely(intg1 > intg0))
      to->buf[0]= 0;

    intg0= intg1;
    buf0=to->buf;
    buf1=to->buf;
    to->sign=from->sign;
    to->intg=min(intg0, len)*DIG_PER_DEC1;
  }

  if (frac0 > frac1)
  {
    buf1+=intg0+frac1;
    while (frac0-- > frac1)
      *buf1++=0;
    goto done;
  }

  if (scale >= from->frac)
    goto done; /* nothing to do */

  buf0+=intg0+frac0-1;
  buf1+=intg0+frac0-1;
  if (scale == frac0*DIG_PER_DEC1)
  {
    int do_inc= FALSE;
    DBUG_ASSERT(frac0+intg0 >= 0);
    switch (round_digit) {
    case 0:
    {
      dec1 *p0= buf0 + (frac1-frac0);
      for (; p0 > buf0; p0--)
      {
        if (*p0)
        {
          do_inc= TRUE;
          break;
        }
      }
      break;
    }
    case 5:
    {
      x= buf0[1]/DIG_MASK;
      do_inc= (x>5) || ((x == 5) &&
                        (mode == HALF_UP || (frac0+intg0 > 0 && *buf0 & 1)));
      break;
    }
    default:
      break;
    }
    if (do_inc)
    {
      if (frac0+intg0>0)
        (*buf1)++;
      else
        *(++buf1)=DIG_BASE;
    }
    else if (frac0+intg0==0)
    {
      decimal_make_zero(to);
      return E_DEC_OK;
    }
  }
  else
  {
    /* TODO - fix this code as it won't work for CEILING mode */
    int pos=frac0*DIG_PER_DEC1-scale-1;
    DBUG_ASSERT(frac0+intg0 > 0);
    x=*buf1 / powers10[pos];
    y=x % 10;
    if (y > round_digit ||
        (round_digit == 5 && y == 5 && (mode == HALF_UP || (x/10) & 1)))
      x+=10;
    *buf1=powers10[pos]*(x-y);
  }
  /*
    In case we're rounding e.g. 1.5e9 to 2.0e9, the decimal_digit_t's inside
    the buffer are as follows.

    Before <1, 5e8>
    After  <2, 5e8>

    Hence we need to set the 2nd field to 0.
    The same holds if we round 1.5e-9 to 2e-9.
   */
  if (frac0 < frac1)
  {
    dec1 *buf= to->buf + ((scale == 0 && intg0 == 0) ? 1 : intg0 + frac0);
    dec1 *end= to->buf + len;

    while (buf < end)
      *buf++=0;
  }
  if (*buf1 >= DIG_BASE)
  {
    carry=1;
    *buf1-=DIG_BASE;
    while (carry && --buf1 >= to->buf)
      ADD(*buf1, *buf1, 0, carry);
    if (unlikely(carry))
    {
      /* shifting the number to create space for new digit */
      if (frac0+intg0 >= len)
      {
        frac0--;
        scale=frac0*DIG_PER_DEC1;
        error=E_DEC_TRUNCATED; /* XXX */
      }
      for (buf1=to->buf+intg0+max(frac0,0); buf1 > to->buf; buf1--)
      {
        buf1[0]=buf1[-1];
      }
      *buf1=1;
      to->intg++;
    }
  }
  else
  {
    for (;;)
    {
      if (likely(*buf1))
        break;
      if (buf1-- == to->buf)
      {
        /* making 'zero' with the proper scale */
        dec1 *p0= to->buf + frac0 + 1;
        to->intg=1;
        to->frac= max(scale, 0);
        to->sign= 0;
        for (buf1= to->buf; buf1<p0; buf1++)
          *buf1= 0;
        return E_DEC_OK;
      }
    }
  }

  /* Here we  check 999.9 -> 1000 case when we need to increase intg */
  first_dig= to->intg % DIG_PER_DEC1;
  if (first_dig && (*buf1 >= powers10[first_dig]))
    to->intg++;

  if (scale<0)
    scale=0;

done:
  to->frac=scale;
  return error;
}

/*
  Returns the size of the result of the operation

  SYNOPSIS
    decimal_result_size()
      from1   - operand of the unary operation or first operand of the
                binary operation
      from2   - second operand of the binary operation
      op      - operation. one char '+', '-', '*', '/' are allowed
                others may be added later
      param   - extra param to the operation. unused for '+', '-', '*'
                scale increment for '/'

  NOTE
    returned valued may be larger than the actual buffer requred
    in the operation, as decimal_result_size, by design, operates on
    precision/scale values only and not on the actual decimal number

  RETURN VALUE
    size of to->buf array in dec1 elements. to get size in bytes
    multiply by sizeof(dec1)
*/

int decimal_result_size(decimal_t *from1, decimal_t *from2, char op, int param)
{
  switch (op) {
  case '-':
    return ROUND_UP(max(from1->intg, from2->intg)) +
           ROUND_UP(max(from1->frac, from2->frac));
  case '+':
    return ROUND_UP(max(from1->intg, from2->intg)+1) +
           ROUND_UP(max(from1->frac, from2->frac));
  case '*':
    return ROUND_UP(from1->intg+from2->intg)+
           ROUND_UP(from1->frac)+ROUND_UP(from2->frac);
  case '/':
    return ROUND_UP(from1->intg+from2->intg+1+from1->frac+from2->frac+param);
  default: DBUG_ASSERT(0);
  }
  return -1; /* shut up the warning */
}

static int do_add(decimal_t *from1, decimal_t *from2, decimal_t *to)
{
  int intg1=ROUND_UP(from1->intg), intg2=ROUND_UP(from2->intg),
      frac1=ROUND_UP(from1->frac), frac2=ROUND_UP(from2->frac),
      frac0=max(frac1, frac2), intg0=max(intg1, intg2), error;
  dec1 *buf1, *buf2, *buf0, *stop, *stop2, x, carry;

  sanity(to);

  /* is there a need for extra word because of carry ? */
  x=intg1 > intg2 ? from1->buf[0] :
    intg2 > intg1 ? from2->buf[0] :
    from1->buf[0] + from2->buf[0] ;
  if (unlikely(x > DIG_MAX-1)) /* yes, there is */
  {
    intg0++;
    to->buf[0]=0; /* safety */
  }

  FIX_INTG_FRAC_ERROR(to->len, intg0, frac0, error);
  if (unlikely(error == E_DEC_OVERFLOW))
  {
    max_decimal(to->len * DIG_PER_DEC1, 0, to);
    return error;
  }

  buf0=to->buf+intg0+frac0;

  to->sign=from1->sign;
  to->frac=max(from1->frac, from2->frac);
  to->intg=intg0*DIG_PER_DEC1;
  if (unlikely(error))
  {
    set_if_smaller(to->frac, frac0*DIG_PER_DEC1);
    set_if_smaller(frac1, frac0);
    set_if_smaller(frac2, frac0);
    set_if_smaller(intg1, intg0);
    set_if_smaller(intg2, intg0);
  }

  /* part 1 - max(frac) ... min (frac) */
  if (frac1 > frac2)
  {
    buf1=from1->buf+intg1+frac1;
    stop=from1->buf+intg1+frac2;
    buf2=from2->buf+intg2+frac2;
    stop2=from1->buf+(intg1 > intg2 ? intg1-intg2 : 0);
  }
  else
  {
    buf1=from2->buf+intg2+frac2;
    stop=from2->buf+intg2+frac1;
    buf2=from1->buf+intg1+frac1;
    stop2=from2->buf+(intg2 > intg1 ? intg2-intg1 : 0);
  }
  while (buf1 > stop)
    *--buf0=*--buf1;

  /* part 2 - min(frac) ... min(intg) */
  carry=0;
  while (buf1 > stop2)
  {
    ADD(*--buf0, *--buf1, *--buf2, carry);
  }

  /* part 3 - min(intg) ... max(intg) */
  buf1= intg1 > intg2 ? ((stop=from1->buf)+intg1-intg2) :
                        ((stop=from2->buf)+intg2-intg1) ;
  while (buf1 > stop)
  {
    ADD(*--buf0, *--buf1, 0, carry);
  }

  if (unlikely(carry))
    *--buf0=1;
  DBUG_ASSERT(buf0 == to->buf || buf0 == to->buf+1);

  return error;
}

/* to=from1-from2.
   if to==0, return -1/0/+1 - the result of the comparison */
static int do_sub(decimal_t *from1, decimal_t *from2, decimal_t *to)
{
  int intg1=ROUND_UP(from1->intg), intg2=ROUND_UP(from2->intg),
      frac1=ROUND_UP(from1->frac), frac2=ROUND_UP(from2->frac);
  int frac0=max(frac1, frac2), error;
  dec1 *buf1, *buf2, *buf0, *stop1, *stop2, *start1, *start2, carry=0;

  /* let carry:=1 if from2 > from1 */
  start1=buf1=from1->buf; stop1=buf1+intg1;
  start2=buf2=from2->buf; stop2=buf2+intg2;
  if (unlikely(*buf1 == 0))
  {
    while (buf1 < stop1 && *buf1 == 0)
      buf1++;
    start1=buf1;
    intg1= (int) (stop1-buf1);
  }
  if (unlikely(*buf2 == 0))
  {
    while (buf2 < stop2 && *buf2 == 0)
      buf2++;
    start2=buf2;
    intg2= (int) (stop2-buf2);
  }
  if (intg2 > intg1)
    carry=1;
  else if (intg2 == intg1)
  {
    dec1 *end1= stop1 + (frac1 - 1);
    dec1 *end2= stop2 + (frac2 - 1);
    while (unlikely((buf1 <= end1) && (*end1 == 0)))
      end1--;
    while (unlikely((buf2 <= end2) && (*end2 == 0)))
      end2--;
    frac1= (int) (end1 - stop1) + 1;
    frac2= (int) (end2 - stop2) + 1;
    while (buf1 <=end1 && buf2 <= end2 && *buf1 == *buf2)
      buf1++, buf2++;
    if (buf1 <= end1)
    {
      if (buf2 <= end2)
        carry= *buf2 > *buf1;
      else
        carry= 0;
    }
    else
    {
      if (buf2 <= end2)
        carry=1;
      else /* short-circuit everything: from1 == from2 */
      {
        if (to == 0) /* decimal_cmp() */
          return 0;
        decimal_make_zero(to);
        return E_DEC_OK;
      }
    }
  }

  if (to == 0) /* decimal_cmp() */
    return carry == from1->sign ? 1 : -1;

  sanity(to);

  to->sign=from1->sign;

  /* ensure that always from1 > from2 (and intg1 >= intg2) */
  if (carry)
  {
    swap_variables(decimal_t *,from1,from1);
    swap_variables(dec1 *,start1, start2);
    swap_variables(int,intg1,intg2);
    swap_variables(int,frac1,frac2);
    to->sign= 1 - to->sign;
  }

  FIX_INTG_FRAC_ERROR(to->len, intg1, frac0, error);
  buf0=to->buf+intg1+frac0;

  to->frac=max(from1->frac, from2->frac);
  to->intg=intg1*DIG_PER_DEC1;
  if (unlikely(error))
  {
    set_if_smaller(to->frac, frac0*DIG_PER_DEC1);
    set_if_smaller(frac1, frac0);
    set_if_smaller(frac2, frac0);
    set_if_smaller(intg2, intg1);
  }
  carry=0;

  /* part 1 - max(frac) ... min (frac) */
  if (frac1 > frac2)
  {
    buf1=start1+intg1+frac1;
    stop1=start1+intg1+frac2;
    buf2=start2+intg2+frac2;
    while (frac0-- > frac1)
      *--buf0=0;
    while (buf1 > stop1)
      *--buf0=*--buf1;
  }
  else
  {
    buf1=start1+intg1+frac1;
    buf2=start2+intg2+frac2;
    stop2=start2+intg2+frac1;
    while (frac0-- > frac2)
      *--buf0=0;
    while (buf2 > stop2)
    {
      SUB(*--buf0, 0, *--buf2, carry);
    }
  }

  /* part 2 - min(frac) ... intg2 */
  while (buf2 > start2)
  {
    SUB(*--buf0, *--buf1, *--buf2, carry);
  }

  /* part 3 - intg2 ... intg1 */
  while (carry && buf1 > start1)
  {
    SUB(*--buf0, *--buf1, 0, carry);
  }

  while (buf1 > start1)
    *--buf0=*--buf1;

  while (buf0 > to->buf)
    *--buf0=0;

  return error;
}

int decimal_intg(decimal_t *from)
{
  int res;
  remove_leading_zeroes(from, &res);
  return res;
}

int decimal_add(decimal_t *from1, decimal_t *from2, decimal_t *to)
{
  if (likely(from1->sign == from2->sign))
    return do_add(from1, from2, to);
  return do_sub(from1, from2, to);
}

int decimal_sub(decimal_t *from1, decimal_t *from2, decimal_t *to)
{
  if (likely(from1->sign == from2->sign))
    return do_sub(from1, from2, to);
  return do_add(from1, from2, to);
}

int decimal_cmp(decimal_t *from1, decimal_t *from2)
{
  if (likely(from1->sign == from2->sign))
    return do_sub(from1, from2, 0);
  return from1->sign > from2->sign ? -1 : 1;
}

int decimal_is_zero(decimal_t *from)
{
  dec1 *buf1=from->buf,
       *end=buf1+ROUND_UP(from->intg)+ROUND_UP(from->frac);
  while (buf1 < end)
    if (*buf1++)
      return 0;
  return 1;
}

/*
  multiply two decimals

  SYNOPSIS
    decimal_mul()
      from1, from2 - factors
      to      - product

  RETURN VALUE
    E_DEC_OK/E_DEC_TRUNCATED/E_DEC_OVERFLOW;

  NOTES
    in this implementation, with sizeof(dec1)=4 we have DIG_PER_DEC1=9,
    and 63-digit number will take only 7 dec1 words (basically a 7-digit
    "base 999999999" number).  Thus there's no need in fast multiplication
    algorithms, 7-digit numbers can be multiplied with a naive O(n*n)
    method.

    XXX if this library is to be used with huge numbers of thousands of
    digits, fast multiplication must be implemented.
*/
int decimal_mul(decimal_t *from1, decimal_t *from2, decimal_t *to)
{
  int intg1=ROUND_UP(from1->intg), intg2=ROUND_UP(from2->intg),
      frac1=ROUND_UP(from1->frac), frac2=ROUND_UP(from2->frac),
      intg0=ROUND_UP(from1->intg+from2->intg),
      frac0=frac1+frac2, error, i, j, d_to_move;
  dec1 *buf1=from1->buf+intg1, *buf2=from2->buf+intg2, *buf0,
       *start2, *stop2, *stop1, *start0, carry;

  sanity(to);

  i=intg0;                                       /* save 'ideal' values */
  j=frac0;
  FIX_INTG_FRAC_ERROR(to->len, intg0, frac0, error);  /* bound size */
  to->sign=from1->sign != from2->sign;
  to->frac=from1->frac+from2->frac;              /* store size in digits */
  to->intg=intg0*DIG_PER_DEC1;

  if (unlikely(error))
  {
    set_if_smaller(to->frac, frac0*DIG_PER_DEC1);
    set_if_smaller(to->intg, intg0*DIG_PER_DEC1);
    if (unlikely(i > intg0))                     /* bounded integer-part */
    {
      i-=intg0;
      j=i >> 1;
      intg1-= j;
      intg2-=i-j;
      frac1=frac2=0; /* frac0 is already 0 here */
    }
    else                                         /* bounded fract part */
    {
      j-=frac0;
      i=j >> 1;
      if (frac1 <= frac2)
      {
        frac1-= i;
        frac2-=j-i;
      }
      else
      {
        frac2-= i;
        frac1-=j-i;
      }
    }
  }
  start0=to->buf+intg0+frac0-1;
  start2=buf2+frac2-1;
  stop1=buf1-intg1;
  stop2=buf2-intg2;

  bzero(to->buf, (intg0+frac0)*sizeof(dec1));

  for (buf1+=frac1-1; buf1 >= stop1; buf1--, start0--)
  {
    carry=0;
    for (buf0=start0, buf2=start2; buf2 >= stop2; buf2--, buf0--)
    {
      dec1 hi, lo;
      dec2 p= ((dec2)*buf1) * ((dec2)*buf2);
      hi=(dec1)(p/DIG_BASE);
      lo=(dec1)(p-((dec2)hi)*DIG_BASE);
      ADD2(*buf0, *buf0, lo, carry);
      carry+=hi;
    }
    if (carry)
    {
      if (buf0 < to->buf)
        return E_DEC_OVERFLOW;
      ADD2(*buf0, *buf0, 0, carry);
    }
    for (buf0--; carry; buf0--)
    {
      if (buf0 < to->buf)
        return E_DEC_OVERFLOW;
      ADD(*buf0, *buf0, 0, carry);
    }
  }

  /* Now we have to check for -0.000 case */
  if (to->sign)
  {
    dec1 *buf= to->buf;
    dec1 *end= to->buf + intg0 + frac0;
    DBUG_ASSERT(buf != end);
    for (;;)
    {
      if (*buf)
        break;
      if (++buf == end)
      {
        /* We got decimal zero */
        decimal_make_zero(to);
        break;
      }
    }
  }
  buf1= to->buf;
  d_to_move= intg0 + ROUND_UP(to->frac);
  while (!*buf1 && (to->intg > DIG_PER_DEC1))
  {
    buf1++;
    to->intg-= DIG_PER_DEC1;
    d_to_move--;
  }
  if (to->buf < buf1)
  {
    dec1 *cur_d= to->buf;
    for (; d_to_move--; cur_d++, buf1++)
      *cur_d= *buf1;
  }
  return error;
}

/*
  naive division algorithm (Knuth's Algorithm D in 4.3.1) -
  it's ok for short numbers
  also we're using alloca() to allocate a temporary buffer

  XXX if this library is to be used with huge numbers of thousands of
  digits, fast division must be implemented and alloca should be
  changed to malloc (or at least fallback to malloc if alloca() fails)
  but then, decimal_mul() should be rewritten too :(
*/
static int do_div_mod(decimal_t *from1, decimal_t *from2,
                       decimal_t *to, decimal_t *mod, int scale_incr)
{
  int frac1=ROUND_UP(from1->frac)*DIG_PER_DEC1, prec1=from1->intg+frac1,
      frac2=ROUND_UP(from2->frac)*DIG_PER_DEC1, prec2=from2->intg+frac2,
      UNINIT_VAR(error), i, intg0, frac0, len1, len2, dintg, div_mod=(!mod);
  dec1 *buf0, *buf1=from1->buf, *buf2=from2->buf, *tmp1,
       *start2, *stop2, *stop1, *stop0, norm2, carry, *start1, dcarry;
  dec2 norm_factor, x, guess, y;

  if (mod)
    to=mod;

  sanity(to);

  /* removing all the leading zeroes */
  i= ((prec2 - 1) % DIG_PER_DEC1) + 1;
  while (prec2 > 0 && *buf2 == 0)
  {
    prec2-= i;
    i= DIG_PER_DEC1;
    buf2++;
  }
  if (prec2 <= 0) /* short-circuit everything: from2 == 0 */
    return E_DEC_DIV_ZERO;
  for (i= (prec2 - 1) % DIG_PER_DEC1; *buf2 < powers10[i--]; prec2--) ;
  DBUG_ASSERT(prec2 > 0);

  i=((prec1-1) % DIG_PER_DEC1)+1;
  while (prec1 > 0 && *buf1 == 0)
  {
    prec1-=i;
    i=DIG_PER_DEC1;
    buf1++;
  }
  if (prec1 <= 0)
  { /* short-circuit everything: from1 == 0 */
    decimal_make_zero(to);
    return E_DEC_OK;
  }
  for (i=(prec1-1) % DIG_PER_DEC1; *buf1 < powers10[i--]; prec1--) ;
  DBUG_ASSERT(prec1 > 0);

  /* let's fix scale_incr, taking into account frac1,frac2 increase */
  if ((scale_incr-= frac1 - from1->frac + frac2 - from2->frac) < 0)
    scale_incr=0;

  dintg=(prec1-frac1)-(prec2-frac2)+(*buf1 >= *buf2);
  if (dintg < 0)
  {
    dintg/=DIG_PER_DEC1;
    intg0=0;
  }
  else
    intg0=ROUND_UP(dintg);
  if (mod)
  {
    /* we're calculating N1 % N2.
       The result will have
         frac=max(frac1, frac2), as for subtraction
         intg=intg2
    */
    to->sign=from1->sign;
    to->frac=max(from1->frac, from2->frac);
    frac0=0;
  }
  else
  {
    /*
      we're calculating N1/N2. N1 is in the buf1, has prec1 digits
      N2 is in the buf2, has prec2 digits. Scales are frac1 and
      frac2 accordingly.
      Thus, the result will have
         frac = ROUND_UP(frac1+frac2+scale_incr)
      and
         intg = (prec1-frac1) - (prec2-frac2) + 1
         prec = intg+frac
    */
    frac0=ROUND_UP(frac1+frac2+scale_incr);
    FIX_INTG_FRAC_ERROR(to->len, intg0, frac0, error);
    to->sign=from1->sign != from2->sign;
    to->intg=intg0*DIG_PER_DEC1;
    to->frac=frac0*DIG_PER_DEC1;
  }
  buf0=to->buf;
  stop0=buf0+intg0+frac0;
  if (likely(div_mod))
    while (dintg++ < 0)
      *buf0++=0;

  len1=(i=ROUND_UP(prec1))+ROUND_UP(2*frac2+scale_incr+1) + 1;
  set_if_bigger(len1, 3);
  if (!(tmp1=(dec1 *)my_alloca(len1*sizeof(dec1))))
    return E_DEC_OOM;
  memcpy(tmp1, buf1, i*sizeof(dec1));
  bzero(tmp1+i, (len1-i)*sizeof(dec1));

  start1=tmp1;
  stop1=start1+len1;
  start2=buf2;
  stop2=buf2+ROUND_UP(prec2)-1;

  /* removing end zeroes */
  while (*stop2 == 0 && stop2 >= start2)
    stop2--;
  len2= (int) (stop2++ - start2);

  /*
    calculating norm2 (normalized *start2) - we need *start2 to be large
    (at least > DIG_BASE/2), but unlike Knuth's Alg. D we don't want to
    normalize input numbers (as we don't make a copy of the divisor).
    Thus we normalize first dec1 of buf2 only, and we'll normalize *start1
    on the fly for the purpose of guesstimation only.
    It's also faster, as we're saving on normalization of buf2
  */
  norm_factor=DIG_BASE/(*start2+1);
  norm2=(dec1)(norm_factor*start2[0]);
  if (likely(len2>0))
    norm2+=(dec1)(norm_factor*start2[1]/DIG_BASE);

  if (*start1 < *start2)
    dcarry=*start1++;
  else
    dcarry=0;

  /* main loop */
  for (; buf0 < stop0; buf0++)
  {
    /* short-circuit, if possible */
    if (unlikely(dcarry == 0 && *start1 < *start2))
      guess=0;
    else
    {
      /* D3: make a guess */
      x=start1[0]+((dec2)dcarry)*DIG_BASE;
      y=start1[1];
      guess=(norm_factor*x+norm_factor*y/DIG_BASE)/norm2;
      if (unlikely(guess >= DIG_BASE))
        guess=DIG_BASE-1;
      if (likely(len2>0))
      {
        /* hmm, this is a suspicious trick - I removed normalization here */
        if (start2[1]*guess > (x-guess*start2[0])*DIG_BASE+y)
          guess--;
        if (unlikely(start2[1]*guess > (x-guess*start2[0])*DIG_BASE+y))
          guess--;
        DBUG_ASSERT(start2[1]*guess <= (x-guess*start2[0])*DIG_BASE+y);
      }

      /* D4: multiply and subtract */
      buf2=stop2;
      buf1=start1+len2;
      DBUG_ASSERT(buf1 < stop1);
      for (carry=0; buf2 > start2; buf1--)
      {
        dec1 hi, lo;
        x=guess * (*--buf2);
        hi=(dec1)(x/DIG_BASE);
        lo=(dec1)(x-((dec2)hi)*DIG_BASE);
        SUB2(*buf1, *buf1, lo, carry);
        carry+=hi;
      }
      carry= dcarry < carry;

      /* D5: check the remainder */
      if (unlikely(carry))
      {
        /* D6: correct the guess */
        guess--;
        buf2=stop2;
        buf1=start1+len2;
        for (carry=0; buf2 > start2; buf1--)
        {
          ADD(*buf1, *buf1, *--buf2, carry);
        }
      }
    }
    if (likely(div_mod))
      *buf0=(dec1)guess;
    dcarry= *start1;
    start1++;
  }
  if (mod)
  {
    /*
      now the result is in tmp1, it has
        intg=prec1-frac1
        frac=max(frac1, frac2)=to->frac
    */
    if (dcarry)
      *--start1=dcarry;
    buf0=to->buf;
    intg0=(int) (ROUND_UP(prec1-frac1)-(start1-tmp1));
    frac0=ROUND_UP(to->frac);
    error=E_DEC_OK;
    if (unlikely(frac0==0 && intg0==0))
    {
      decimal_make_zero(to);
      goto done;
    }
    if (intg0<=0)
    {
      if (unlikely(-intg0 >= to->len))
      {
        decimal_make_zero(to);
        error=E_DEC_TRUNCATED;
        goto done;
      }
      stop1=start1+frac0;
      frac0+=intg0;
      to->intg=0;
      while (intg0++ < 0)
        *buf0++=0;
    }
    else
    {
      if (unlikely(intg0 > to->len))
      {
        frac0=0;
        intg0=to->len;
        error=E_DEC_OVERFLOW;
        goto done;
      }
      DBUG_ASSERT(intg0 <= ROUND_UP(from2->intg));
      stop1=start1+frac0+intg0;
      to->intg=min(intg0*DIG_PER_DEC1, from2->intg);
    }
    if (unlikely(intg0+frac0 > to->len))
    {
      stop1-=frac0+intg0-to->len;
      frac0=to->len-intg0;
      to->frac=frac0*DIG_PER_DEC1;
        error=E_DEC_TRUNCATED;
    }
    DBUG_ASSERT(buf0 + (stop1 - start1) <= to->buf + to->len);
    while (start1 < stop1)
        *buf0++=*start1++;
  }
done:
  my_afree(tmp1);
  return error;
}

/*
  division of two decimals

  SYNOPSIS
    decimal_div()
      from1   - dividend
      from2   - divisor
      to      - quotient

  RETURN VALUE
    E_DEC_OK/E_DEC_TRUNCATED/E_DEC_OVERFLOW/E_DEC_DIV_ZERO;

  NOTES
    see do_div_mod()
*/

int
decimal_div(decimal_t *from1, decimal_t *from2, decimal_t *to, int scale_incr)
{
  return do_div_mod(from1, from2, to, 0, scale_incr);
}

/*
  modulus

  SYNOPSIS
    decimal_mod()
      from1   - dividend
      from2   - divisor
      to      - modulus

  RETURN VALUE
    E_DEC_OK/E_DEC_TRUNCATED/E_DEC_OVERFLOW/E_DEC_DIV_ZERO;

  NOTES
    see do_div_mod()

  DESCRIPTION
    the modulus R in    R = M mod N

   is defined as

     0 <= |R| < |M|
     sign R == sign M
     R = M - k*N, where k is integer

   thus, there's no requirement for M or N to be integers
*/

int decimal_mod(decimal_t *from1, decimal_t *from2, decimal_t *to)
{
  return do_div_mod(from1, from2, 0, to, 0);
}

#ifdef MAIN

int full= 0;
decimal_t a, b, c;
char buf1[100], buf2[100], buf3[100];

void dump_decimal(decimal_t *d)
{
  int i;
  printf("/* intg=%d, frac=%d, sign=%d, buf[]={", d->intg, d->frac, d->sign);
  for (i=0; i < ROUND_UP(d->frac)+ROUND_UP(d->intg)-1; i++)
    printf("%09d, ", d->buf[i]);
  printf("%09d} */ ", d->buf[i]);
}


void check_result_code(int actual, int want)
{
  if (actual != want)
  {
    printf("\n^^^^^^^^^^^^^ must return %d\n", want);
    exit(1);
  }
}


void print_decimal(decimal_t *d, const char *orig, int actual, int want)
{
  char s[100];
  int slen=sizeof(s);

  if (full) dump_decimal(d);
  decimal2string(d, s, &slen, 0, 0, 0);
  printf("'%s'", s);
  check_result_code(actual, want);
  if (orig && strcmp(orig, s))
  {
    printf("\n^^^^^^^^^^^^^ must've been '%s'\n", orig);
    exit(1);
  }
}

void test_d2s()
{
  char s[100];
  int slen, res;

  /***********************************/
  printf("==== decimal2string ====\n");
  a.buf[0]=12345; a.intg=5; a.frac=0; a.sign=0;
  slen=sizeof(s);
  res=decimal2string(&a, s, &slen, 0, 0, 0);
  dump_decimal(&a); printf("  -->  res=%d str='%s' len=%d\n", res, s, slen);

  a.buf[1]=987000000; a.frac=3;
  slen=sizeof(s);
  res=decimal2string(&a, s, &slen, 0, 0, 0);
  dump_decimal(&a); printf("  -->  res=%d str='%s' len=%d\n", res, s, slen);

  a.sign=1;
  slen=sizeof(s);
  res=decimal2string(&a, s, &slen, 0, 0, 0);
  dump_decimal(&a); printf("  -->  res=%d str='%s' len=%d\n", res, s, slen);

  slen=8;
  res=decimal2string(&a, s, &slen, 0, 0, 0);
  dump_decimal(&a); printf("  -->  res=%d str='%s' len=%d\n", res, s, slen);

  slen=5;
  res=decimal2string(&a, s, &slen, 0, 0, 0);
  dump_decimal(&a); printf("  -->  res=%d str='%s' len=%d\n", res, s, slen);

  a.buf[0]=987000000; a.frac=3; a.intg=0;
  slen=sizeof(s);
  res=decimal2string(&a, s, &slen, 0, 0, 0);
  dump_decimal(&a); printf("  -->  res=%d str='%s' len=%d\n", res, s, slen);
}

void test_s2d(const char *s, const char *orig, int ex)
{
  char s1[100], *end;
  int res;
  sprintf(s1, "'%s'", s);
  end= strend(s);
  printf("len=%2d %-30s => res=%d    ", a.len, s1,
         (res= string2decimal(s, &a, &end)));
  print_decimal(&a, orig, res, ex);
  printf("\n");
}

void test_d2f(const char *s, int ex)
{
  char s1[100], *end;
  double x;
  int res;

  sprintf(s1, "'%s'", s);
  end= strend(s);
  string2decimal(s, &a, &end);
  res=decimal2double(&a, &x);
  if (full) dump_decimal(&a);
  printf("%-40s => res=%d    %.*g\n", s1, res, a.intg+a.frac, x);
  check_result_code(res, ex);
}

void test_d2b2d(const char *str, int p, int s, const char *orig, int ex)
{
  char s1[100], buf[100], *end;
  int res, i, size=decimal_bin_size(p, s);

  sprintf(s1, "'%s'", str);
  end= strend(str);
  string2decimal(str, &a, &end);
  res=decimal2bin(&a, buf, p, s);
  printf("%-31s {%2d, %2d} => res=%d size=%-2d ", s1, p, s, res, size);
  if (full)
  {
    printf("0x");
    for (i=0; i < size; i++)
      printf("%02x", ((uchar *)buf)[i]);
  }
  res=bin2decimal(buf, &a, p, s);
  printf(" => res=%d ", res);
  print_decimal(&a, orig, res, ex);
  printf("\n");
}

void test_f2d(double from, int ex)
{
  int res;

  res=double2decimal(from, &a);
  printf("%-40.*f => res=%d    ", DBL_DIG-2, from, res);
  print_decimal(&a, 0, res, ex);
  printf("\n");
}

void test_ull2d(ulonglong from, const char *orig, int ex)
{
  char s[100];
  int res;

  res=ulonglong2decimal(from, &a);
  longlong10_to_str(from,s,10);
  printf("%-40s => res=%d    ", s, res);
  print_decimal(&a, orig, res, ex);
  printf("\n");
}

void test_ll2d(longlong from, const char *orig, int ex)
{
  char s[100];
  int res;

  res=longlong2decimal(from, &a);
  longlong10_to_str(from,s,-10);
  printf("%-40s => res=%d    ", s, res);
  print_decimal(&a, orig, res, ex);
  printf("\n");
}

void test_d2ull(const char *s, const char *orig, int ex)
{
  char s1[100], *end;
  ulonglong x;
  int res;

  end= strend(s);
  string2decimal(s, &a, &end);
  res=decimal2ulonglong(&a, &x);
  if (full) dump_decimal(&a);
  longlong10_to_str(x,s1,10);
  printf("%-40s => res=%d    %s\n", s, res, s1);
  check_result_code(res, ex);
  if (orig && strcmp(orig, s1))
  {
    printf("\n^^^^^^^^^^^^^ must've been '%s'\n", orig);
    exit(1);
  }
}

void test_d2ll(const char *s, const char *orig, int ex)
{
  char s1[100], *end;
  longlong x;
  int res;

  end= strend(s);
  string2decimal(s, &a, &end);
  res=decimal2longlong(&a, &x);
  if (full) dump_decimal(&a);
  longlong10_to_str(x,s1,-10);
  printf("%-40s => res=%d    %s\n", s, res, s1);
  check_result_code(res, ex);
  if (orig && strcmp(orig, s1))
  {
    printf("\n^^^^^^^^^^^^^ must've been '%s'\n", orig);
    exit(1);
  }
}

void test_da(const char *s1, const char *s2, const char *orig, int ex)
{
  char s[100], *end;
  int res;
  sprintf(s, "'%s' + '%s'", s1, s2);
  end= strend(s1);
  string2decimal(s1, &a, &end);
  end= strend(s2);
  string2decimal(s2, &b, &end);
  res=decimal_add(&a, &b, &c);
  printf("%-40s => res=%d    ", s, res);
  print_decimal(&c, orig, res, ex);
  printf("\n");
}

void test_ds(const char *s1, const char *s2, const char *orig, int ex)
{
  char s[100], *end;
  int res;
  sprintf(s, "'%s' - '%s'", s1, s2);
  end= strend(s1);
  string2decimal(s1, &a, &end);
  end= strend(s2);
  string2decimal(s2, &b, &end);
  res=decimal_sub(&a, &b, &c);
  printf("%-40s => res=%d    ", s, res);
  print_decimal(&c, orig, res, ex);
  printf("\n");
}

void test_dc(const char *s1, const char *s2, int orig)
{
  char s[100], *end;
  int res;
  sprintf(s, "'%s' <=> '%s'", s1, s2);
  end= strend(s1);
  string2decimal(s1, &a, &end);
  end= strend(s2);
  string2decimal(s2, &b, &end);
  res=decimal_cmp(&a, &b);
  printf("%-40s => res=%d\n", s, res);
  if (orig != res)
  {
    printf("\n^^^^^^^^^^^^^ must've been %d\n", orig);
    exit(1);
  }
}

void test_dm(const char *s1, const char *s2, const char *orig, int ex)
{
  char s[100], *end;
  int res;
  sprintf(s, "'%s' * '%s'", s1, s2);
  end= strend(s1);
  string2decimal(s1, &a, &end);
  end= strend(s2);
  string2decimal(s2, &b, &end);
  res=decimal_mul(&a, &b, &c);
  printf("%-40s => res=%d    ", s, res);
  print_decimal(&c, orig, res, ex);
  printf("\n");
}

void test_dv(const char *s1, const char *s2, const char *orig, int ex)
{
  char s[100], *end;
  int res;
  sprintf(s, "'%s' / '%s'", s1, s2);
  end= strend(s1);
  string2decimal(s1, &a, &end);
  end= strend(s2);
  string2decimal(s2, &b, &end);
  res=decimal_div(&a, &b, &c, 5);
  printf("%-40s => res=%d    ", s, res);
  check_result_code(res, ex);
  if (res == E_DEC_DIV_ZERO)
    printf("E_DEC_DIV_ZERO");
  else
    print_decimal(&c, orig, res, ex);
  printf("\n");
}

void test_md(const char *s1, const char *s2, const char *orig, int ex)
{
  char s[100], *end;
  int res;
  sprintf(s, "'%s' %% '%s'", s1, s2);
  end= strend(s1);
  string2decimal(s1, &a, &end);
  end= strend(s2);
  string2decimal(s2, &b, &end);
  res=decimal_mod(&a, &b, &c);
  printf("%-40s => res=%d    ", s, res);
  check_result_code(res, ex);
  if (res == E_DEC_DIV_ZERO)
    printf("E_DEC_DIV_ZERO");
  else
    print_decimal(&c, orig, res, ex);
  printf("\n");
}

const char *round_mode[]=
{"TRUNCATE", "HALF_EVEN", "HALF_UP", "CEILING", "FLOOR"};

void test_ro(const char *s1, int n, decimal_round_mode mode, const char *orig,
             int ex)
{
  char s[100], *end;
  int res;
  sprintf(s, "'%s', %d, %s", s1, n, round_mode[mode]);
  end= strend(s1);
  string2decimal(s1, &a, &end);
  res=decimal_round(&a, &b, n, mode);
  printf("%-40s => res=%d    ", s, res);
  print_decimal(&b, orig, res, ex);
  printf("\n");
}


void test_mx(int precision, int frac, const char *orig)
{
  char s[100];
  sprintf(s, "%d, %d", precision, frac);
  max_decimal(precision, frac, &a);
  printf("%-40s =>          ", s);
  print_decimal(&a, orig, 0, 0);
  printf("\n");
}


void test_pr(const char *s1, int prec, int dec, char filler, const char *orig,
             int ex)
{
  char s[100], *end;
  char s2[100];
  int slen= sizeof(s2);
  int res;

  sprintf(s, filler ? "'%s', %d, %d, '%c'" : "'%s', %d, %d, '\\0'",
          s1, prec, dec, filler);
  end= strend(s1);
  string2decimal(s1, &a, &end);
  res= decimal2string(&a, s2, &slen, prec, dec, filler);
  printf("%-40s => res=%d    '%s'", s, res, s2);
  check_result_code(res, ex);
  if (orig && strcmp(orig, s2))
  {
    printf("\n^^^^^^^^^^^^^ must've been '%s'\n", orig);
    exit(1);
  }
  printf("\n");
}


void test_sh(const char *s1, int shift, const char *orig, int ex)
{
  char s[100], *end;
  int res;
  sprintf(s, "'%s' %s %d", s1, ((shift < 0) ? ">>" : "<<"), abs(shift));
  end= strend(s1);
  string2decimal(s1, &a, &end);
  res= decimal_shift(&a, shift);
  printf("%-40s => res=%d    ", s, res);
  print_decimal(&a, orig, res, ex);
  printf("\n");
}


void test_fr(const char *s1, const char *orig)
{
  char s[100], *end;
  sprintf(s, "'%s'", s1);
  printf("%-40s =>          ", s);
  end= strend(s1);
  string2decimal(s1, &a, &end);
  a.frac= decimal_actual_fraction(&a);
  print_decimal(&a, orig, 0, 0);
  printf("\n");
}
#endif
