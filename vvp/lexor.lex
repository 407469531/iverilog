
%{
/*
 * Copyright (c) 2001 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#if !defined(WINNT)
#ident "$Id: lexor.lex,v 1.29.2.1 2002/03/02 00:27:10 steve Exp $"
#endif

# include  "parse_misc.h"
# include  "compile.h"
# include  "parse.h"
# include  <string.h>
# include  <assert.h>
%}

%%

  /* These are some special header keywords. */
^":vpi_module" { return K_vpi_module; }
^":vpi_time_precision" { return K_vpi_time_precision; }


  /* A label is any non-blank text that appears left justified. */
^[.$_a-zA-Z\\][.$_a-zA-Z\\0-9<>/]* {
      yylval.text = strdup(yytext);
      assert(yylval.text);
      return T_LABEL; }

  /* String tokens are parsed here. Return as the token value the
     contents of the string without the enclosing quotes. */
\"([^\"\\]|\\.)*\" {
      yytext[strlen(yytext)-1] = 0;
      yylval.text = strdup(yytext+1);
      assert(yylval.text);
      return T_STRING; }

[1-9][0-9]*"'b"[01xz]+ {
      yylval.vect.idx = strtoul(yytext, 0, 10);
      yylval.vect.text = (char*)malloc(yylval.vect.idx + 1);
      assert(yylval.vect.text);

      const char*bits = strchr(yytext, 'b');
      bits += 1;
      unsigned pad = 0;
      if (strlen(bits) < yylval.vect.idx)
	    pad = yylval.vect.idx - strlen(bits);

      memset(yylval.vect.text, '0', pad);
      for (unsigned idx = pad ;  idx < yylval.vect.idx ;  idx += 1)
	    yylval.vect.text[idx] = bits[idx-pad];

      yylval.vect.text[yylval.vect.idx] = 0;
      return T_VECTOR; }


  /* These are some keywords that are recognized. */
".arith/div"  { return K_ARITH_DIV; }
".arith/mod"  { return K_ARITH_MOD; }
".arith/mult" { return K_ARITH_MULT; }
".arith/sub"  { return K_ARITH_SUB; }
".arith/sum"  { return K_ARITH_SUM; }
".cmp/ge"   { return K_CMP_GE; }
".cmp/gt"   { return K_CMP_GT; }
".event"    { return K_EVENT; }
".event/or" { return K_EVENT_OR; }
".functor"  { return K_FUNCTOR; }
".net"      { return K_NET; }
".net/s"    { return K_NET_S; }
".resolv"   { return K_RESOLV; }
".scope"    { return K_SCOPE; }
".shift/l"  { return K_SHIFTL; }
".shift/r"  { return K_SHIFTR; }
".thread"   { return K_THREAD; }
".var"      { return K_VAR; }
".var/s"    { return K_VAR_S; }
".udp"         { return K_UDP; }
".udp/c"(omb)? { return K_UDP_C; }
".udp/s"(equ)? { return K_UDP_S; }
".mem" 	       { return K_MEM; }
".mem/p"(ort)? { return K_MEM_P; }
".mem/i"(nit)? { return K_MEM_I; }
".force"     { return K_FORCE; }

  /* instructions start with a % character. The compiler decides what
     kind of instruction this really is. The few exceptions (that have
     exceptional parameter requirements) are listed first. */

"%vpi_call" { return K_vpi_call; }
"%vpi_func" { return K_vpi_func; }
"%disable"  { return K_disable; }
"%fork"     { return K_fork; }

"%"[.$_/a-zA-Z0-9]+ {
      yylval.text = strdup(yytext);
      assert(yylval.text);
      return T_INSTR; }

[0-9][0-9]* {
      yylval.numb = strtol(yytext, 0, 0);
      return T_NUMBER; }

"0x"[0-9a-fA-F]+ {
      yylval.numb = strtol(yytext, 0, 0);
      return T_NUMBER; }



  /* Symbols are pretty much what is left. They are used to refer to
     labels so the rule must match a string that a label would match. */
[.$_a-zA-Z\\][.$_a-zA-Z\\0-9<>/]* {
      yylval.text = strdup(yytext);
      assert(yylval.text);
      return T_SYMBOL; }

  /* Symbols may include komma `,' in certain constructs */

[A-Z]"<"[.$_a-zA-Z0-9/,]*">" {
      yylval.text = strdup(yytext);
      assert(yylval.text);
      return T_SYMBOL; }


  /* Accept the common assembler style comments, treat them as white
     space. Of course, also skip white space. The semi-colon is
     special, though, in that it is also a statement terminator. */
";".* { return ';'; }
"#".* { ; }

[ \t\b\r] { ; }

\n { yyline += 1; }

. { return yytext[0]; }

%%

int yywrap()
{
      return -1;
}


/*
 * $Log: lexor.lex,v $
 * Revision 1.29.2.1  2002/03/02 00:27:10  steve
 *  cariage return fixes from main trunk.
 *
 * Revision 1.29  2002/01/03 04:19:02  steve
 *  Add structural modulus support down to vvp.
 *
 * Revision 1.28  2001/11/01 03:00:19  steve
 *  Add force/cassign/release/deassign support. (Stephan Boettcher)
 *
 * Revision 1.27  2001/10/16 02:47:37  steve
 *  Add arith/div object.
 *
 * Revision 1.26  2001/07/07 02:57:33  steve
 *  Add the .shift/r functor.
 *
 * Revision 1.25  2001/07/06 04:46:44  steve
 *  Add structural left shift (.shift/l)
 *
 * Revision 1.24  2001/06/30 23:03:17  steve
 *  support fast programming by only writing the bits
 *  that are listed in the input file.
 *
 * Revision 1.23  2001/06/18 03:10:34  steve
 *   1. Logic with more than 4 inputs
 *   2. Id and name mangling
 *   3. A memory leak in draw_net_in_scope()
 *   (Stephan Boettcher)
 *
 * Revision 1.22  2001/06/16 23:45:05  steve
 *  Add support for structural multiply in t-dll.
 *  Add code generators and vvp support for both
 *  structural and behavioral multiply.
 *
 * Revision 1.21  2001/06/15 04:07:58  steve
 *  Add .cmp statements for structural comparison.
 *
 * Revision 1.20  2001/06/07 03:09:03  steve
 *  Implement .arith/sub subtraction.
 *
 * Revision 1.19  2001/06/05 03:05:41  steve
 *  Add structural addition.
 *
 * Revision 1.18  2001/05/20 00:46:12  steve
 *  Add support for system function calls.
 *
 * Revision 1.17  2001/05/10 00:26:53  steve
 *  VVP support for memories in expressions,
 *  including general support for thread bit
 *  vectors as system task parameters.
 *  (Stephan Boettcher)
 *
 * Revision 1.16  2001/05/09 02:53:25  steve
 *  Implement the .resolv syntax.
 *
 * Revision 1.15  2001/05/01 01:09:39  steve
 *  Add support for memory objects. (Stephan Boettcher)
 *
 * Revision 1.14  2001/04/24 02:23:59  steve
 *  Support for UDP devices in VVP (Stephen Boettcher)
 *
 * Revision 1.13  2001/04/18 04:21:23  steve
 *  Put threads into scopes.
 *
 * Revision 1.12  2001/04/14 05:10:56  steve
 *  support the .event/or statement.
 *
 * Revision 1.11  2001/04/05 01:34:26  steve
 *  Add the .var/s and .net/s statements for VPI support.
 *
 * Revision 1.10  2001/04/04 04:33:08  steve
 *  Take vector form as parameters to vpi_call.
 *
 * Revision 1.9  2001/03/26 04:00:39  steve
 *  Add the .event statement and the %wait instruction.
 *
 * Revision 1.8  2001/03/25 19:36:45  steve
 *  Accept <> characters in labels and symbols.
 *
 * Revision 1.7  2001/03/25 00:35:35  steve
 *  Add the .net statement.
 *
 * Revision 1.6  2001/03/23 02:40:22  steve
 *  Add the :module header statement.
 *
 * Revision 1.5  2001/03/20 02:48:40  steve
 *  Copyright notices.
 *
 */
