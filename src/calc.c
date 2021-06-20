/*

    EnergyMech, IRC bot software
    Copyright (c) 2020-2021 proton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/
#ifndef CALC_C
#define CALC_C
#include "config.h"

#ifdef TEST
#define MAKETABLES
char nick_buf[MAXHOSTLEN];
void *Calloc(int size)
{
	void	*tmp;

	if ((tmp = (void*)calloc(size,1)) == NULL)
		exit(1);
	return((void*)tmp);
}
#endif /* TEST */

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

/*
  sequential blocks of numbers and operations
  number, operation, bracket level, exponent,

	1.2 + 2.4 --> [ number 12, ADD, bracket 0, exponent -1 ] [ number 24, ADD, bracket 0, exponent -1 ]
*/
typedef struct CalcOp
{
	uint64_t	number;
	int8_t		operation;	/* + (minus not included! add negative numbers!) / * ! ^ %		*/
	int8_t		decimals;	/* decimals								*/
	int8_t		paralevel;	/* parenthesis level							*/

} CalcOp;

#define MAX_COP		32

#define OPER_ADD	1
#define OPER_DIV	2
#define OPER_MUL	3
#define OPER_POWER	4
#define OPER_MOD	5
#define OPER_ROOT	6

/*
    exponents and roots
    multiplication and division
    addition and subtraction
*/
int64_t decival[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000, 10000000000, 100000000000, 0 };

const char *calculate(const char *input, CalcOp *cop, char *prepin)
{
	char		*dst;
	uint64_t	num,r;
	int		x;
	int		cop_count,i;
	int		exp,sign,dp,op,para,hasnum;
	int8_t		maxdeci;

	/* remove spaces */
	dst = prepin;
	while(*input)
		if (*input != ' ')
			*dst++ = *input++;
		else
			input++;
	*dst = 0;
	input = prepin;

	printf("%s\n",prepin);
	cop_count = 0;
	maxdeci = 0;
	goto new_blank;

new_op_or_num:
	if (hasnum == 0)
		return("Parsing error: Missing number");
	if (cop_count >= MAX_COP)
		return("Parsing error: Too many operands");
	if (cop_count == 0 && op == 0)
		op = OPER_ADD;
#ifdef TEST
	printf("num %lu, op %i, sign %i, decimals %i, paralevel %i\n",num,op,sign,exp,para);
#endif /* TEST */
	if (exp > maxdeci)
		maxdeci = exp;
	cop[cop_count].number = (sign == 0) ? num : -num;
	cop[cop_count].operation = op;
	cop[cop_count].decimals = exp;
	cop[cop_count].paralevel = para;
	if (*input == 0)
		goto calculate_result;
	cop_count++;

new_blank:
	num = 0;
	hasnum = 0; /* need to keep track of numbers like "0" */
	sign = 0;
	exp = 0;
	dp = 0;
	op = 0;
	para = 0;

op_or_num:
	if (*input == '+')
	{
		op = OPER_ADD;
		input++;
		goto parse_num;
	}
	if (*input == '-')
	{
		op = OPER_ADD;
		sign = 1;
		input++;
		goto parse_num;
	}
	if (*input >= '0' && *input <= '9')
		goto parse_num;
	if (*input == 0)
		goto new_op_or_num;
	goto parsing_error;

parse_num:
	if (*input == '-' || *input == '+')
	{
		goto new_op_or_num;
	}
	if (*input == '.' && dp == 0)
	{
		dp = 1;
		input++;
		goto parse_num;
	}
	if (*input >= '0' && *input <= '9')
	{
		hasnum = 1;
		if (dp == 1)
			exp++;
		num = (num * 10) + (*input - '0');
		input++;
		goto parse_num;
	}
	if (*input == 0)
		goto new_op_or_num;
	goto parsing_error;

parsing_error:
	return("Parsing error");

calculate_result:
	x = 20;
	/* find most decimals and convert all other numbers */
	for(i=0;i<=cop_count;i++)
	{
		if (cop[i].decimals < maxdeci)
		{
			r = cop[i].number;
			exp = maxdeci - cop[i].decimals;
			while(exp-->0)
				r = r * 10;
			cop[i].number = r;
			cop[i].decimals = maxdeci;
		}
	}
	r = 0;
iterate:
	/* find highest paralevel */
	para = -1;
	for(i=0;i<=cop_count;i++)
	{
		if (cop[i].paralevel >= 0)
			printf("number %lli, operation %i, decimals %i, paralevel %i\n",cop[i].number,cop[i].operation,cop[i].decimals,cop[i].paralevel);
		if (cop[i].paralevel >= para)
			para = cop[i].paralevel;
	}
	if (para == -1)
		goto return_result;
	for(op=0;op<3;op++)
	{
		for(i=0;i<=cop_count;i++)
		{
			if (cop[i].paralevel == para && op == 2 && cop[i].operation == OPER_ADD)
			{
				r += cop[i].number;
				cop[i].paralevel = -1;
			}
		}
	}
	if (x--<0)
		return("Iteration error");
	goto iterate;
return_result:
	sprintf(prepin+200,"%%%s%illi",(maxdeci > 0) ? "0" : "",maxdeci+1);
	sprintf(prepin,prepin+200,r);
	if (maxdeci > 0)
	{
		dst = STREND(prepin);
		while(maxdeci >= 0 && dst > prepin)
		{
			dst[1] = dst[0];
			dst--;
			maxdeci--;
		}
		dst[1] = '.';
	}
	return(prepin);
}

void do_calc(COMMAND_ARGS)
{
	CalcOp	cop[MAX_COP];
	int	cp = 0;

	memset(&cop,0,sizeof(cop));

	/* start with a numeral, or '-' */
	if (*rest != '-' && (attrtab[(uchar)*rest] & NUM) == 0)
	{
		/* malformed */
		return;
	}
}

void mkbin(char *dst, int num, int bits)
{
	int	lim;
	char	*org;

	*(dst++) = 'b';
	*(dst++) = 'i';
	*(dst++) = 'n';
	*(dst++) = ' ';
	org = dst;
	lim = (bits == 8) ? 4 : 8;
	for(;bits;bits--)
	{
		if (((bits % lim) == 0) && org != dst)
			*(dst++) = '.';
		*(dst++) = ((num >> (bits-1)) & 1) ? '1' : '0';
	}
	*dst = 0;
}

int bas2int(const char *src, int base)
{
	int	n, v;
	char	ch;

	errno = EINVAL;
	n = 0;

	while(*src)
	{
		if (*src < '0')
			return(-1);

		switch(base)
		{
		case 16:
			ch = tolowertab[(uchar)*src];
			if (*src <= '9')
				v = *src - '0';
			else
			if (ch >= 'a' && ch <= 'f')
				v = ch - 'a' + 10;
			else
				return(-1);
			break;
		case 8:
			if (*src >= '8')
				return(-1);
			v = *src - '0';
			break;
		case 2:
			if (*src >= '2')
				return(-1);
			v = *src - '0';
		}
		src++;
		n = (n * base) + v;
	}
	errno = 0;
	return(n);
}

/*
 dh	decimal to hex
 db	decimal to binary
 d	decimal to hex, octal & binary
*/

#if !defined(TEST)

void do_convert(COMMAND_ARGS)
{
	char	output[200];
	char	*ops, *srcnum, *dst;
	int	num, todec, tochr, tooct, tohex, tobin;

	ops = chop(&rest);
	srcnum = chop(&rest);

	if (ops == NULL || srcnum == NULL)
		return;

	todec = tochr = tooct = tohex = tobin = 0;

	switch(ops[1])
	{
	case 0:
		todec = tochr = tooct = tohex = tobin = 1;
		break;
	case 'b':
		tobin = 1;
		break;
	case 'c':
		tochr = 1;
		break;
	case 'd':
		todec = 1;
		break;
	case 'h':
		tohex = 1;
		break;
	}

	errno = EINVAL;
	switch(*ops)
	{
	case 'c':
		num = srcnum[0];
		if (srcnum[1] != 0)
			return;
		errno = tochr = 0;
		break;
	case 'd':
		num = asc2int(srcnum);
		/*todec = 0;*/
		break;
	case 'h':
		if (*srcnum == '$')
			srcnum++;
		if (*srcnum == '0' && srcnum[1] == 'x')
			srcnum += 2;
		num = bas2int(srcnum,16);
		/*tohex = 1;*/
		break;
	case 'o':
		num = bas2int(srcnum,8);
		/* tooct = 0;*/
		break;
	}
	if (errno)
		return;

	*output = 0;
	dst = output;

	if (todec)
	{
		sprintf(dst,"dec %i", num);
	}

	if (tochr && (num & 0xffffff00) == 0)
	{
		dst = STREND(dst);
		if (dst != output)
			*(dst++) = ' ';

		if (num >= 32 && num <= 126)
			sprintf(dst,"chr '%c'",num);
		else
			sprintf(dst,"chr &#%02X;",num & 0xff);
	}

	if (tooct)
	{
		dst = STREND(dst);
		if (dst != output)
			*(dst++) = ' ';

		sprintf(dst,"oct 0%o",num,num);
	}

	if (tohex)
	{
		dst = STREND(dst);
		if (dst != output)
			*(dst++) = ' ';

		if (num <= 255 && num >= -128)
		{
			sprintf(dst,"hex 0x%02X",num & 0xff);
		}
		else
		if (num <= 65535 && num >= -32768)
		{
			sprintf(dst,"hex 0x%04X",num & 0xffff);
		}
		else
			sprintf(dst,"hex 0x%08X",num);
	}

	if (tobin)
	{
		dst = STREND(dst);
		if (dst != output)
			*(dst++) = ' ';

		if ((num & 0xffffff00) == 0 || (num & 0xffffff00) == 0xffffff00)
			mkbin(dst,num,8);
		else
		if ((num & 0xffff0000) == 0 || (num & 0xffff0000) == 0xffff0000)
			mkbin(dst,num,16);
		else
			mkbin(dst,num,32);
	}

	to_user_q(from,"Conversion: %s",output);
}

#else /* not TEST */

const char *test[] = {
	"1 - .2 + .03 - .004",
	"2.008 + 9 + .992",
/*	"999 - 555.66",
	"1+2-3+4-5+6-7+8-9",*/
	NULL
};

int main(int argc, char **argv, char **envp)
{
	char	prep[MSGLEN];
	CalcOp	cop[MAX_COP];
	char	*tmp;
	int	i;

	for(i=0;test[i];i++)
	{
		memset(&cop,0,sizeof(cop));
		printf("calc \"%s\" = %s\n",test[i],calculate(test[i],cop,prep));
	}
}

#endif /* TEST */
#endif /* ifndef CALC_C */
