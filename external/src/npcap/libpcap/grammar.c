/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.7"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         pcap_parse
#define yylex           pcap_lex
#define yyerror         pcap_error
#define yylval          pcap_lval
#define yychar          pcap_char
#define yydebug         pcap_debug
#define yynerrs         pcap_nerrs

/* Copy the first part of user declarations.  */
/* Line 371 of yacc.c  */
#line 47 "grammar.y"

/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>

#if __STDC__
struct mbuf;
struct rtentry;
#endif

#include <netinet/in.h>
#include <arpa/inet.h>
#endif /* _WIN32 */

#include <stdio.h>

#include "diag-control.h"

#include "pcap-int.h"

#include "gencode.h"
#include "grammar.h"
#include "scanner.h"

#ifdef HAVE_NET_PFVAR_H
#include <net/if.h>
#include <net/pfvar.h>
#include <net/if_pflog.h>
#endif
#include "llc.h"
#include "ieee80211.h"
#include <pcap/namedb.h>

#ifdef HAVE_OS_PROTO_H
#include "os-proto.h"
#endif

#ifdef YYBYACC
/*
 * Both Berkeley YACC and Bison define yydebug (under whatever name
 * it has) as a global, but Bison does so only if YYDEBUG is defined.
 * Berkeley YACC define it even if YYDEBUG isn't defined; declare it
 * here to suppress a warning.
 */
#if !defined(YYDEBUG)
extern int yydebug;
#endif

/*
 * In Berkeley YACC, yynerrs (under whatever name it has) is global,
 * even if it's building a reentrant parser.  In Bison, it's local
 * in reentrant parsers.
 *
 * Declare it to squelch a warning.
 */
extern int yynerrs;
#endif

#define QSET(q, p, d, a) (q).proto = (unsigned char)(p),\
			 (q).dir = (unsigned char)(d),\
			 (q).addr = (unsigned char)(a)

struct tok {
	int v;			/* value */
	const char *s;		/* string */
};

static const struct tok ieee80211_types[] = {
	{ IEEE80211_FC0_TYPE_DATA, "data" },
	{ IEEE80211_FC0_TYPE_MGT, "mgt" },
	{ IEEE80211_FC0_TYPE_MGT, "management" },
	{ IEEE80211_FC0_TYPE_CTL, "ctl" },
	{ IEEE80211_FC0_TYPE_CTL, "control" },
	{ 0, NULL }
};
static const struct tok ieee80211_mgt_subtypes[] = {
	{ IEEE80211_FC0_SUBTYPE_ASSOC_REQ, "assocreq" },
	{ IEEE80211_FC0_SUBTYPE_ASSOC_REQ, "assoc-req" },
	{ IEEE80211_FC0_SUBTYPE_ASSOC_RESP, "assocresp" },
	{ IEEE80211_FC0_SUBTYPE_ASSOC_RESP, "assoc-resp" },
	{ IEEE80211_FC0_SUBTYPE_REASSOC_REQ, "reassocreq" },
	{ IEEE80211_FC0_SUBTYPE_REASSOC_REQ, "reassoc-req" },
	{ IEEE80211_FC0_SUBTYPE_REASSOC_RESP, "reassocresp" },
	{ IEEE80211_FC0_SUBTYPE_REASSOC_RESP, "reassoc-resp" },
	{ IEEE80211_FC0_SUBTYPE_PROBE_REQ, "probereq" },
	{ IEEE80211_FC0_SUBTYPE_PROBE_REQ, "probe-req" },
	{ IEEE80211_FC0_SUBTYPE_PROBE_RESP, "proberesp" },
	{ IEEE80211_FC0_SUBTYPE_PROBE_RESP, "probe-resp" },
	{ IEEE80211_FC0_SUBTYPE_BEACON, "beacon" },
	{ IEEE80211_FC0_SUBTYPE_ATIM, "atim" },
	{ IEEE80211_FC0_SUBTYPE_DISASSOC, "disassoc" },
	{ IEEE80211_FC0_SUBTYPE_DISASSOC, "disassociation" },
	{ IEEE80211_FC0_SUBTYPE_AUTH, "auth" },
	{ IEEE80211_FC0_SUBTYPE_AUTH, "authentication" },
	{ IEEE80211_FC0_SUBTYPE_DEAUTH, "deauth" },
	{ IEEE80211_FC0_SUBTYPE_DEAUTH, "deauthentication" },
	{ 0, NULL }
};
static const struct tok ieee80211_ctl_subtypes[] = {
	{ IEEE80211_FC0_SUBTYPE_PS_POLL, "ps-poll" },
	{ IEEE80211_FC0_SUBTYPE_RTS, "rts" },
	{ IEEE80211_FC0_SUBTYPE_CTS, "cts" },
	{ IEEE80211_FC0_SUBTYPE_ACK, "ack" },
	{ IEEE80211_FC0_SUBTYPE_CF_END, "cf-end" },
	{ IEEE80211_FC0_SUBTYPE_CF_END_ACK, "cf-end-ack" },
	{ 0, NULL }
};
static const struct tok ieee80211_data_subtypes[] = {
	{ IEEE80211_FC0_SUBTYPE_DATA, "data" },
	{ IEEE80211_FC0_SUBTYPE_CF_ACK, "data-cf-ack" },
	{ IEEE80211_FC0_SUBTYPE_CF_POLL, "data-cf-poll" },
	{ IEEE80211_FC0_SUBTYPE_CF_ACPL, "data-cf-ack-poll" },
	{ IEEE80211_FC0_SUBTYPE_NODATA, "null" },
	{ IEEE80211_FC0_SUBTYPE_NODATA_CF_ACK, "cf-ack" },
	{ IEEE80211_FC0_SUBTYPE_NODATA_CF_POLL, "cf-poll"  },
	{ IEEE80211_FC0_SUBTYPE_NODATA_CF_ACPL, "cf-ack-poll" },
	{ IEEE80211_FC0_SUBTYPE_QOS|IEEE80211_FC0_SUBTYPE_DATA, "qos-data" },
	{ IEEE80211_FC0_SUBTYPE_QOS|IEEE80211_FC0_SUBTYPE_CF_ACK, "qos-data-cf-ack" },
	{ IEEE80211_FC0_SUBTYPE_QOS|IEEE80211_FC0_SUBTYPE_CF_POLL, "qos-data-cf-poll" },
	{ IEEE80211_FC0_SUBTYPE_QOS|IEEE80211_FC0_SUBTYPE_CF_ACPL, "qos-data-cf-ack-poll" },
	{ IEEE80211_FC0_SUBTYPE_QOS|IEEE80211_FC0_SUBTYPE_NODATA, "qos" },
	{ IEEE80211_FC0_SUBTYPE_QOS|IEEE80211_FC0_SUBTYPE_NODATA_CF_POLL, "qos-cf-poll" },
	{ IEEE80211_FC0_SUBTYPE_QOS|IEEE80211_FC0_SUBTYPE_NODATA_CF_ACPL, "qos-cf-ack-poll" },
	{ 0, NULL }
};
static const struct tok llc_s_subtypes[] = {
	{ LLC_RR, "rr" },
	{ LLC_RNR, "rnr" },
	{ LLC_REJ, "rej" },
	{ 0, NULL }
};
static const struct tok llc_u_subtypes[] = {
	{ LLC_UI, "ui" },
	{ LLC_UA, "ua" },
	{ LLC_DISC, "disc" },
	{ LLC_DM, "dm" },
	{ LLC_SABME, "sabme" },
	{ LLC_TEST, "test" },
	{ LLC_XID, "xid" },
	{ LLC_FRMR, "frmr" },
	{ 0, NULL }
};
struct type2tok {
	int type;
	const struct tok *tok;
};
static const struct type2tok ieee80211_type_subtypes[] = {
	{ IEEE80211_FC0_TYPE_MGT, ieee80211_mgt_subtypes },
	{ IEEE80211_FC0_TYPE_CTL, ieee80211_ctl_subtypes },
	{ IEEE80211_FC0_TYPE_DATA, ieee80211_data_subtypes },
	{ 0, NULL }
};

static int
str2tok(const char *str, const struct tok *toks)
{
	int i;

	for (i = 0; toks[i].s != NULL; i++) {
		if (pcap_strcasecmp(toks[i].s, str) == 0) {
			/*
			 * Just in case somebody is using this to
			 * generate values of -1/0xFFFFFFFF.
			 * That won't work, as it's indistinguishable
			 * from an error.
			 */
			if (toks[i].v == -1)
				abort();
			return (toks[i].v);
		}
	}
	return (-1);
}

static const struct qual qerr = { Q_UNDEF, Q_UNDEF, Q_UNDEF, Q_UNDEF };

static void
yyerror(void *yyscanner _U_, compiler_state_t *cstate, const char *msg)
{
	bpf_set_error(cstate, "can't parse filter expression: %s", msg);
}

#ifdef HAVE_NET_PFVAR_H
static int
pfreason_to_num(compiler_state_t *cstate, const char *reason)
{
	const char *reasons[] = PFRES_NAMES;
	int i;

	for (i = 0; reasons[i]; i++) {
		if (pcap_strcasecmp(reason, reasons[i]) == 0)
			return (i);
	}
	bpf_set_error(cstate, "unknown PF reason \"%s\"", reason);
	return (-1);
}

static int
pfaction_to_num(compiler_state_t *cstate, const char *action)
{
	if (pcap_strcasecmp(action, "pass") == 0 ||
	    pcap_strcasecmp(action, "accept") == 0)
		return (PF_PASS);
	else if (pcap_strcasecmp(action, "drop") == 0 ||
		pcap_strcasecmp(action, "block") == 0)
		return (PF_DROP);
#if HAVE_PF_NAT_THROUGH_PF_NORDR
	else if (pcap_strcasecmp(action, "rdr") == 0)
		return (PF_RDR);
	else if (pcap_strcasecmp(action, "nat") == 0)
		return (PF_NAT);
	else if (pcap_strcasecmp(action, "binat") == 0)
		return (PF_BINAT);
	else if (pcap_strcasecmp(action, "nordr") == 0)
		return (PF_NORDR);
#endif
	else {
		bpf_set_error(cstate, "unknown PF action \"%s\"", action);
		return (-1);
	}
}
#else /* !HAVE_NET_PFVAR_H */
static int
pfreason_to_num(compiler_state_t *cstate, const char *reason _U_)
{
	bpf_set_error(cstate, "libpcap was compiled on a machine without pf support");
	return (-1);
}

static int
pfaction_to_num(compiler_state_t *cstate, const char *action _U_)
{
	bpf_set_error(cstate, "libpcap was compiled on a machine without pf support");
	return (-1);
}
#endif /* HAVE_NET_PFVAR_H */

/*
 * For calls that might return an "an error occurred" value.
 */
#define CHECK_INT_VAL(val)	if (val == -1) YYABORT
#define CHECK_PTR_VAL(val)	if (val == NULL) YYABORT

DIAG_OFF_BISON_BYACC

/* Line 371 of yacc.c  */
#line 349 "grammar.c"

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "grammar.h".  */
#ifndef YY_PCAP_C_USERS_DELL_CMAKEBUILDS_F6D0C8EA_6AE4_7C3C_97B3_A829ECED2726_BUILD_X86_DEBUG_GRAMMAR_H_INCLUDED
# define YY_PCAP_C_USERS_DELL_CMAKEBUILDS_F6D0C8EA_6AE4_7C3C_97B3_A829ECED2726_BUILD_X86_DEBUG_GRAMMAR_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int pcap_debug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     DST = 258,
     SRC = 259,
     HOST = 260,
     GATEWAY = 261,
     NET = 262,
     NETMASK = 263,
     PORT = 264,
     PORTRANGE = 265,
     LESS = 266,
     GREATER = 267,
     PROTO = 268,
     PROTOCHAIN = 269,
     CBYTE = 270,
     ARP = 271,
     RARP = 272,
     IP = 273,
     SCTP = 274,
     TCP = 275,
     UDP = 276,
     ICMP = 277,
     IGMP = 278,
     IGRP = 279,
     PIM = 280,
     VRRP = 281,
     CARP = 282,
     ATALK = 283,
     AARP = 284,
     DECNET = 285,
     LAT = 286,
     SCA = 287,
     MOPRC = 288,
     MOPDL = 289,
     TK_BROADCAST = 290,
     TK_MULTICAST = 291,
     NUM = 292,
     INBOUND = 293,
     OUTBOUND = 294,
     IFINDEX = 295,
     PF_IFNAME = 296,
     PF_RSET = 297,
     PF_RNR = 298,
     PF_SRNR = 299,
     PF_REASON = 300,
     PF_ACTION = 301,
     TYPE = 302,
     SUBTYPE = 303,
     DIR = 304,
     ADDR1 = 305,
     ADDR2 = 306,
     ADDR3 = 307,
     ADDR4 = 308,
     RA = 309,
     TA = 310,
     LINK = 311,
     GEQ = 312,
     LEQ = 313,
     NEQ = 314,
     ID = 315,
     EID = 316,
     HID = 317,
     HID6 = 318,
     AID = 319,
     LSH = 320,
     RSH = 321,
     LEN = 322,
     IPV6 = 323,
     ICMPV6 = 324,
     AH = 325,
     ESP = 326,
     VLAN = 327,
     MPLS = 328,
     PPPOED = 329,
     PPPOES = 330,
     GENEVE = 331,
     ISO = 332,
     ESIS = 333,
     CLNP = 334,
     ISIS = 335,
     L1 = 336,
     L2 = 337,
     IIH = 338,
     LSP = 339,
     SNP = 340,
     CSNP = 341,
     PSNP = 342,
     STP = 343,
     IPX = 344,
     NETBEUI = 345,
     LANE = 346,
     LLC = 347,
     METAC = 348,
     BCC = 349,
     SC = 350,
     ILMIC = 351,
     OAMF4EC = 352,
     OAMF4SC = 353,
     OAM = 354,
     OAMF4 = 355,
     CONNECTMSG = 356,
     METACONNECT = 357,
     VPI = 358,
     VCI = 359,
     RADIO = 360,
     FISU = 361,
     LSSU = 362,
     MSU = 363,
     HFISU = 364,
     HLSSU = 365,
     HMSU = 366,
     SIO = 367,
     OPC = 368,
     DPC = 369,
     SLS = 370,
     HSIO = 371,
     HOPC = 372,
     HDPC = 373,
     HSLS = 374,
     LEX_ERROR = 375,
     AND = 376,
     OR = 377,
     UMINUS = 378
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 387 of yacc.c  */
#line 321 "grammar.y"

	int i;
	bpf_u_int32 h;
	char *s;
	struct stmt *stmt;
	struct arth *a;
	struct {
		struct qual q;
		int atmfieldtype;
		int mtp3fieldtype;
		struct block *b;
	} blk;
	struct block *rblk;


/* Line 387 of yacc.c  */
#line 531 "grammar.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int pcap_parse (void *YYPARSE_PARAM);
#else
int pcap_parse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int pcap_parse (void *yyscanner, compiler_state_t *cstate);
#else
int pcap_parse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_PCAP_C_USERS_DELL_CMAKEBUILDS_F6D0C8EA_6AE4_7C3C_97B3_A829ECED2726_BUILD_X86_DEBUG_GRAMMAR_H_INCLUDED  */

/* Copy the second part of user declarations.  */

/* Line 390 of yacc.c  */
#line 558 "grammar.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(N) (N)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   800

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  141
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  47
/* YYNRULES -- Number of rules.  */
#define YYNRULES  221
/* YYNRULES -- Number of states.  */
#define YYNSTATES  296

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   378

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   123,     2,     2,     2,   139,   125,     2,
     132,   131,   128,   126,     2,   127,     2,   129,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   138,     2,
     135,   134,   133,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   136,     2,   137,   140,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   124,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   130
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,     9,    11,    15,    19,    23,
      27,    29,    31,    33,    35,    39,    41,    45,    49,    51,
      55,    57,    59,    61,    64,    66,    68,    70,    74,    78,
      80,    82,    84,    87,    91,    94,    97,   100,   103,   106,
     109,   113,   115,   119,   123,   125,   127,   129,   132,   134,
     137,   139,   140,   142,   144,   148,   152,   156,   160,   162,
     164,   166,   168,   170,   172,   174,   176,   178,   180,   182,
     184,   186,   188,   190,   192,   194,   196,   198,   200,   202,
     204,   206,   208,   210,   212,   214,   216,   218,   220,   222,
     224,   226,   228,   230,   232,   234,   236,   238,   240,   242,
     244,   246,   248,   250,   252,   254,   256,   258,   260,   263,
     266,   269,   272,   277,   279,   281,   284,   287,   289,   292,
     294,   296,   299,   301,   304,   306,   308,   311,   313,   316,
     319,   322,   325,   328,   331,   336,   339,   342,   345,   347,
     349,   351,   353,   355,   357,   360,   363,   365,   367,   369,
     371,   373,   375,   377,   379,   381,   383,   385,   387,   389,
     394,   401,   405,   409,   413,   417,   421,   425,   429,   433,
     437,   441,   444,   448,   450,   452,   454,   456,   458,   460,
     462,   466,   468,   470,   472,   474,   476,   478,   480,   482,
     484,   486,   488,   490,   492,   494,   497,   500,   504,   506,
     508,   512,   514,   516,   518,   520,   522,   524,   526,   528,
     530,   532,   534,   536,   538,   540,   542,   545,   548,   552,
     554,   556
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     142,     0,    -1,   143,   144,    -1,   143,    -1,    -1,   153,
      -1,   144,   145,   153,    -1,   144,   145,   147,    -1,   144,
     146,   153,    -1,   144,   146,   147,    -1,   121,    -1,   122,
      -1,   148,    -1,   176,    -1,   150,   151,   131,    -1,    60,
      -1,    62,   129,    37,    -1,    62,     8,    62,    -1,    62,
      -1,    63,   129,    37,    -1,    63,    -1,    61,    -1,    64,
      -1,   149,   147,    -1,   123,    -1,   132,    -1,   148,    -1,
     152,   145,   147,    -1,   152,   146,   147,    -1,   176,    -1,
     151,    -1,   155,    -1,   149,   153,    -1,   156,   157,   158,
      -1,   156,   157,    -1,   156,   158,    -1,   156,    13,    -1,
     156,    14,    -1,   156,   159,    -1,   154,   147,    -1,   150,
     144,   131,    -1,   160,    -1,   173,   171,   173,    -1,   173,
     172,   173,    -1,   161,    -1,   177,    -1,   178,    -1,   179,
     180,    -1,   183,    -1,   184,   185,    -1,   160,    -1,    -1,
       4,    -1,     3,    -1,     4,   122,     3,    -1,     3,   122,
       4,    -1,     4,   121,     3,    -1,     3,   121,     4,    -1,
      50,    -1,    51,    -1,    52,    -1,    53,    -1,    54,    -1,
      55,    -1,     5,    -1,     7,    -1,     9,    -1,    10,    -1,
       6,    -1,    56,    -1,    18,    -1,    16,    -1,    17,    -1,
      19,    -1,    20,    -1,    21,    -1,    22,    -1,    23,    -1,
      24,    -1,    25,    -1,    26,    -1,    27,    -1,    28,    -1,
      29,    -1,    30,    -1,    31,    -1,    32,    -1,    34,    -1,
      33,    -1,    68,    -1,    69,    -1,    70,    -1,    71,    -1,
      77,    -1,    78,    -1,    80,    -1,    81,    -1,    82,    -1,
      83,    -1,    84,    -1,    85,    -1,    87,    -1,    86,    -1,
      79,    -1,    88,    -1,    89,    -1,    90,    -1,   105,    -1,
     156,    35,    -1,   156,    36,    -1,    11,    37,    -1,    12,
      37,    -1,    15,    37,   175,    37,    -1,    38,    -1,    39,
      -1,    40,    37,    -1,    72,   176,    -1,    72,    -1,    73,
     176,    -1,    73,    -1,    74,    -1,    75,   176,    -1,    75,
      -1,    76,   176,    -1,    76,    -1,   162,    -1,   156,   163,
      -1,   167,    -1,    41,    60,    -1,    42,    60,    -1,    43,
      37,    -1,    44,    37,    -1,    45,   169,    -1,    46,   170,
      -1,    47,   164,    48,   165,    -1,    47,   164,    -1,    48,
     166,    -1,    49,   168,    -1,    37,    -1,    60,    -1,    37,
      -1,    60,    -1,    60,    -1,    92,    -1,    92,    60,    -1,
      92,    43,    -1,    37,    -1,    60,    -1,    37,    -1,    60,
      -1,    60,    -1,   133,    -1,    57,    -1,   134,    -1,    58,
      -1,   135,    -1,    59,    -1,   176,    -1,   174,    -1,   160,
     136,   173,   137,    -1,   160,   136,   173,   138,    37,   137,
      -1,   173,   126,   173,    -1,   173,   127,   173,    -1,   173,
     128,   173,    -1,   173,   129,   173,    -1,   173,   139,   173,
      -1,   173,   125,   173,    -1,   173,   124,   173,    -1,   173,
     140,   173,    -1,   173,    65,   173,    -1,   173,    66,   173,
      -1,   127,   173,    -1,   150,   174,   131,    -1,    67,    -1,
     125,    -1,   124,    -1,   135,    -1,   133,    -1,   134,    -1,
      37,    -1,   150,   176,   131,    -1,    91,    -1,    93,    -1,
      94,    -1,    97,    -1,    98,    -1,    95,    -1,    96,    -1,
      99,    -1,   100,    -1,   101,    -1,   102,    -1,   103,    -1,
     104,    -1,   181,    -1,   171,    37,    -1,   172,    37,    -1,
     150,   182,   131,    -1,    37,    -1,   181,    -1,   182,   146,
     181,    -1,   106,    -1,   107,    -1,   108,    -1,   109,    -1,
     110,    -1,   111,    -1,   112,    -1,   113,    -1,   114,    -1,
     115,    -1,   116,    -1,   117,    -1,   118,    -1,   119,    -1,
     186,    -1,   171,    37,    -1,   172,    37,    -1,   150,   187,
     131,    -1,    37,    -1,   186,    -1,   187,   146,   186,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   395,   395,   399,   401,   403,   404,   405,   406,   407,
     409,   411,   413,   414,   416,   418,   419,   421,   423,   442,
     453,   464,   465,   466,   468,   470,   472,   473,   474,   476,
     478,   480,   481,   483,   484,   485,   486,   487,   495,   497,
     498,   499,   500,   502,   504,   505,   506,   507,   508,   509,
     512,   513,   516,   517,   518,   519,   520,   521,   522,   523,
     524,   525,   526,   527,   530,   531,   532,   533,   536,   538,
     539,   540,   541,   542,   543,   544,   545,   546,   547,   548,
     549,   550,   551,   552,   553,   554,   555,   556,   557,   558,
     559,   560,   561,   562,   563,   564,   565,   566,   567,   568,
     569,   570,   571,   572,   573,   574,   575,   576,   578,   579,
     580,   581,   582,   583,   584,   585,   586,   587,   588,   589,
     590,   591,   592,   593,   594,   595,   596,   597,   600,   601,
     602,   603,   604,   605,   608,   613,   616,   620,   623,   629,
     638,   644,   667,   684,   685,   709,   712,   713,   729,   730,
     733,   736,   737,   738,   740,   741,   742,   744,   745,   747,
     748,   749,   750,   751,   752,   753,   754,   755,   756,   757,
     758,   759,   760,   761,   763,   764,   765,   766,   767,   769,
     770,   772,   773,   774,   775,   776,   777,   778,   780,   781,
     782,   783,   786,   787,   789,   790,   791,   792,   794,   801,
     802,   805,   806,   807,   808,   809,   810,   813,   814,   815,
     816,   817,   818,   819,   820,   822,   823,   824,   825,   827,
     840,   841
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "DST", "SRC", "HOST", "GATEWAY", "NET",
  "NETMASK", "PORT", "PORTRANGE", "LESS", "GREATER", "PROTO", "PROTOCHAIN",
  "CBYTE", "ARP", "RARP", "IP", "SCTP", "TCP", "UDP", "ICMP", "IGMP",
  "IGRP", "PIM", "VRRP", "CARP", "ATALK", "AARP", "DECNET", "LAT", "SCA",
  "MOPRC", "MOPDL", "TK_BROADCAST", "TK_MULTICAST", "NUM", "INBOUND",
  "OUTBOUND", "IFINDEX", "PF_IFNAME", "PF_RSET", "PF_RNR", "PF_SRNR",
  "PF_REASON", "PF_ACTION", "TYPE", "SUBTYPE", "DIR", "ADDR1", "ADDR2",
  "ADDR3", "ADDR4", "RA", "TA", "LINK", "GEQ", "LEQ", "NEQ", "ID", "EID",
  "HID", "HID6", "AID", "LSH", "RSH", "LEN", "IPV6", "ICMPV6", "AH", "ESP",
  "VLAN", "MPLS", "PPPOED", "PPPOES", "GENEVE", "ISO", "ESIS", "CLNP",
  "ISIS", "L1", "L2", "IIH", "LSP", "SNP", "CSNP", "PSNP", "STP", "IPX",
  "NETBEUI", "LANE", "LLC", "METAC", "BCC", "SC", "ILMIC", "OAMF4EC",
  "OAMF4SC", "OAM", "OAMF4", "CONNECTMSG", "METACONNECT", "VPI", "VCI",
  "RADIO", "FISU", "LSSU", "MSU", "HFISU", "HLSSU", "HMSU", "SIO", "OPC",
  "DPC", "SLS", "HSIO", "HOPC", "HDPC", "HSLS", "LEX_ERROR", "AND", "OR",
  "'!'", "'|'", "'&'", "'+'", "'-'", "'*'", "'/'", "UMINUS", "')'", "'('",
  "'>'", "'='", "'<'", "'['", "']'", "':'", "'%'", "'^'", "$accept",
  "prog", "null", "expr", "and", "or", "id", "nid", "not", "paren", "pid",
  "qid", "term", "head", "rterm", "pqual", "dqual", "aqual", "ndaqual",
  "pname", "other", "pfvar", "p80211", "type", "subtype", "type_subtype",
  "pllc", "dir", "reason", "action", "relop", "irelop", "arth", "narth",
  "byteop", "pnum", "atmtype", "atmmultitype", "atmfield", "atmvalue",
  "atmfieldvalue", "atmlistvalue", "mtp2type", "mtp3field", "mtp3value",
  "mtp3fieldvalue", "mtp3listvalue", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,    33,   124,    38,    43,    45,    42,    47,
     378,    41,    40,    62,    61,    60,    91,    93,    58,    37,
      94
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   141,   142,   142,   143,   144,   144,   144,   144,   144,
     145,   146,   147,   147,   147,   148,   148,   148,   148,   148,
     148,   148,   148,   148,   149,   150,   151,   151,   151,   152,
     152,   153,   153,   154,   154,   154,   154,   154,   154,   155,
     155,   155,   155,   155,   155,   155,   155,   155,   155,   155,
     156,   156,   157,   157,   157,   157,   157,   157,   157,   157,
     157,   157,   157,   157,   158,   158,   158,   158,   159,   160,
     160,   160,   160,   160,   160,   160,   160,   160,   160,   160,
     160,   160,   160,   160,   160,   160,   160,   160,   160,   160,
     160,   160,   160,   160,   160,   160,   160,   160,   160,   160,
     160,   160,   160,   160,   160,   160,   160,   160,   161,   161,
     161,   161,   161,   161,   161,   161,   161,   161,   161,   161,
     161,   161,   161,   161,   161,   161,   161,   161,   162,   162,
     162,   162,   162,   162,   163,   163,   163,   163,   164,   164,
     165,   165,   166,   167,   167,   167,   168,   168,   169,   169,
     170,   171,   171,   171,   172,   172,   172,   173,   173,   174,
     174,   174,   174,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   175,   175,   175,   175,   175,   176,
     176,   177,   177,   177,   177,   177,   177,   177,   178,   178,
     178,   178,   179,   179,   180,   180,   180,   180,   181,   182,
     182,   183,   183,   183,   183,   183,   183,   184,   184,   184,
     184,   184,   184,   184,   184,   185,   185,   185,   185,   186,
     187,   187
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     0,     1,     3,     3,     3,     3,
       1,     1,     1,     1,     3,     1,     3,     3,     1,     3,
       1,     1,     1,     2,     1,     1,     1,     3,     3,     1,
       1,     1,     2,     3,     2,     2,     2,     2,     2,     2,
       3,     1,     3,     3,     1,     1,     1,     2,     1,     2,
       1,     0,     1,     1,     3,     3,     3,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     2,
       2,     2,     4,     1,     1,     2,     2,     1,     2,     1,
       1,     2,     1,     2,     1,     1,     2,     1,     2,     2,
       2,     2,     2,     2,     4,     2,     2,     2,     1,     1,
       1,     1,     1,     1,     2,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     4,
       6,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     3,     1,     1,     1,     1,     1,     1,     1,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     2,     3,     1,     1,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     3,     1,
       1,     3
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,     0,    51,     1,     0,     0,     0,    71,    72,    70,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    88,    87,   179,   113,   114,     0,
       0,     0,     0,     0,     0,     0,    69,   173,    89,    90,
      91,    92,   117,   119,   120,   122,   124,    93,    94,   103,
      95,    96,    97,    98,    99,   100,   102,   101,   104,   105,
     106,   181,   143,   182,   183,   186,   187,   184,   185,   188,
     189,   190,   191,   192,   193,   107,   201,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
      24,     0,    25,     2,    51,    51,     5,     0,    31,     0,
      50,    44,   125,   127,     0,   158,   157,    45,    46,     0,
      48,     0,   110,   111,     0,   115,   128,   129,   130,   131,
     148,   149,   132,   150,   133,     0,   116,   118,   121,   123,
     145,   144,     0,     0,   171,    10,    11,    51,    51,    32,
       0,   158,   157,    15,    21,    18,    20,    22,    39,    12,
       0,     0,    13,    53,    52,    64,    68,    65,    66,    67,
      36,    37,   108,   109,     0,     0,     0,    58,    59,    60,
      61,    62,    63,    34,    35,    38,   126,     0,   152,   154,
     156,     0,     0,     0,     0,     0,     0,     0,     0,   151,
     153,   155,     0,     0,     0,     0,   198,     0,     0,     0,
      47,   194,   219,     0,     0,     0,    49,   215,   175,   174,
     177,   178,   176,     0,     0,     0,     7,    51,    51,     6,
     157,     9,     8,    40,   172,   180,     0,     0,     0,    23,
      26,    30,     0,    29,     0,     0,     0,     0,   138,   139,
     135,   142,   136,   146,   147,   137,    33,     0,   169,   170,
     167,   166,   161,   162,   163,   164,   165,   168,    42,    43,
     199,     0,   195,   196,   220,     0,   216,   217,   112,   157,
      17,    16,    19,    14,     0,     0,    57,    55,    56,    54,
       0,   159,     0,   197,     0,   218,     0,    27,    28,   140,
     141,   134,     0,   200,   221,   160
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,   140,   137,   138,   229,   149,   150,   132,
     231,   232,    96,    97,    98,    99,   173,   174,   175,   133,
     101,   102,   176,   240,   291,   242,   103,   245,   122,   124,
     194,   195,   104,   105,   213,   106,   107,   108,   109,   200,
     201,   261,   110,   111,   206,   207,   265
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -217
static const yytype_int16 yypact[] =
{
    -217,    34,   223,  -217,    13,    18,    21,  -217,  -217,  -217,
    -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,
    -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,    41,
      24,    26,    51,    79,   -25,    66,  -217,  -217,  -217,  -217,
    -217,  -217,   -24,   -24,  -217,   -24,   -24,  -217,  -217,  -217,
    -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,
    -217,  -217,   -23,  -217,  -217,  -217,  -217,  -217,  -217,  -217,
    -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,
    -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,
    -217,   576,  -217,   -93,   459,   459,  -217,    19,  -217,   745,
       3,  -217,  -217,  -217,   558,  -217,  -217,  -217,  -217,    -5,
    -217,    39,  -217,  -217,   -14,  -217,  -217,  -217,  -217,  -217,
    -217,  -217,  -217,  -217,  -217,   -24,  -217,  -217,  -217,  -217,
    -217,  -217,   576,    -3,   -68,  -217,  -217,   341,   341,  -217,
    -100,    12,    22,  -217,  -217,    -7,    23,  -217,  -217,  -217,
      19,    19,  -217,   -31,    -4,  -217,  -217,  -217,  -217,  -217,
    -217,  -217,  -217,  -217,   -22,    78,   -18,  -217,  -217,  -217,
    -217,  -217,  -217,    60,  -217,  -217,  -217,   576,  -217,  -217,
    -217,   576,   576,   576,   576,   576,   576,   576,   576,  -217,
    -217,  -217,   576,   576,   576,   576,  -217,   125,   126,   127,
    -217,  -217,  -217,   132,   133,   144,  -217,  -217,  -217,  -217,
    -217,  -217,  -217,   145,    22,   602,  -217,   341,   341,  -217,
      10,  -217,  -217,  -217,  -217,  -217,   123,   149,   150,  -217,
    -217,    63,   -93,    22,   191,   192,   194,   195,  -217,  -217,
     151,  -217,  -217,  -217,  -217,  -217,  -217,   585,    64,    64,
     607,    49,   -66,   -66,   -68,   -68,   602,   602,   602,   602,
    -217,   -98,  -217,  -217,  -217,   -92,  -217,  -217,  -217,   -95,
    -217,  -217,  -217,  -217,    19,    19,  -217,  -217,  -217,  -217,
     -12,  -217,   163,  -217,   125,  -217,   132,  -217,  -217,  -217,
    -217,  -217,    65,  -217,  -217,  -217
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -217,  -217,  -217,   199,   -26,  -216,   -91,  -133,     7,    -2,
    -217,  -217,   -77,  -217,  -217,  -217,  -217,    32,  -217,     9,
    -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,
     -43,   -34,   -27,   -81,  -217,   -38,  -217,  -217,  -217,  -217,
    -195,  -217,  -217,  -217,  -217,  -180,  -217
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -42
static const yytype_int16 yytable[] =
{
      95,   226,   260,   -41,   126,   127,   148,   128,   129,    94,
     -13,   100,   120,    26,   141,   238,   275,   139,   230,   243,
     130,   135,   136,   264,   136,   289,   -29,   -29,   135,   136,
     136,   223,   196,   283,     3,   121,   225,   131,   239,   285,
     125,   125,   244,   125,   125,   284,   216,   221,   290,   286,
     112,   141,   178,   179,   180,   113,    26,   142,   114,   152,
     219,   222,   187,   188,   134,   155,   198,   157,   204,   158,
     159,   192,   193,   192,   193,   199,   202,   205,   115,   143,
     144,   145,   146,   147,   116,   230,   117,   214,   118,   293,
     234,   235,    95,    95,   142,   151,   178,   179,   180,   220,
     220,    94,    94,   100,   100,   215,   294,   197,    92,   203,
     208,   209,   152,   233,   181,   182,   119,   236,   237,   210,
     211,   212,   227,   125,   -41,   -41,   123,    92,   189,   190,
     191,   -13,   -13,   177,   -41,   218,   218,   141,   241,   177,
     139,   -13,    90,   224,   217,   217,   100,   100,   151,   125,
     247,    92,   228,   225,   248,   249,   250,   251,   252,   253,
     254,   255,   196,   262,   263,   256,   257,   258,   259,   202,
     266,    92,   189,   190,   191,   185,   186,   187,   188,   220,
     269,   267,   268,   287,   288,   270,   271,   272,   192,   193,
     185,   186,   187,   188,   273,   276,   277,   278,   279,   280,
     292,    93,   295,   192,   193,   246,   274,     0,     0,     0,
       0,     0,     0,     0,     0,   218,    95,     0,     0,     0,
       0,     0,     0,    -3,   217,   217,   100,   100,     0,     0,
       0,     0,     0,     0,     4,     5,   152,   152,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,     0,     0,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
       0,     0,   151,   151,     0,     0,     0,     0,     0,    36,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,     0,     0,     0,    90,     0,     0,     0,
      91,     0,     4,     5,     0,    92,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,     0,     0,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    36,     0,     0,
       0,   143,   144,   145,   146,   147,     0,     0,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,     0,     0,     0,    90,     0,     0,     0,    91,     0,
       4,     5,     0,    92,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,     0,     0,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    36,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,     0,
       0,     0,    90,     0,     0,     0,    91,     0,     0,     0,
       0,    92,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,     0,     0,    26,     0,   178,   179,   180,     0,     0,
       0,     0,     0,   181,   182,     0,     0,     0,     0,     0,
       0,     0,    36,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    37,    38,    39,    40,    41,     0,     0,
     181,   182,     0,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,   181,   182,     0,
       0,     0,   181,   182,     0,     0,     0,     0,     0,     0,
       0,    75,   183,   184,   185,   186,   187,   188,     0,     0,
       0,   189,   190,   191,     0,     0,     0,   192,   193,     0,
       0,     0,     0,    91,     0,     0,     0,     0,    92,   183,
     184,   185,   186,   187,   188,     0,     0,     0,     0,     0,
       0,     0,   281,   282,   192,   193,   183,   184,   185,   186,
     187,   188,   184,   185,   186,   187,   188,     0,     0,     0,
       0,   192,   193,     0,     0,     0,   192,   193,   153,   154,
     155,   156,   157,     0,   158,   159,     0,     0,   160,   161,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     162,   163,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   164,   165,   166,   167,   168,   169,   170,   171,
     172
};

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-217)))

#define yytable_value_is_error(Yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
       2,     8,   197,     0,    42,    43,    97,    45,    46,     2,
       0,     2,    37,    37,    95,    37,   232,    94,   151,    37,
      43,   121,   122,   203,   122,    37,   121,   122,   121,   122,
     122,   131,    37,   131,     0,    60,   131,    60,    60,   131,
      42,    43,    60,    45,    46,   261,   137,   138,    60,   265,
      37,   132,    57,    58,    59,    37,    37,    95,    37,    97,
     137,   138,   128,   129,    91,     5,   109,     7,   111,     9,
      10,   139,   140,   139,   140,   109,    37,   111,    37,    60,
      61,    62,    63,    64,    60,   218,    60,   125,    37,   284,
     121,   122,    94,    95,   132,    97,    57,    58,    59,   137,
     138,    94,    95,    94,    95,   132,   286,   109,   132,   111,
     124,   125,   150,   151,    65,    66,    37,   121,   122,   133,
     134,   135,   129,   125,   121,   122,    60,   132,   133,   134,
     135,   121,   122,   136,   131,   137,   138,   218,    60,   136,
     217,   131,   123,   131,   137,   138,   137,   138,   150,   151,
     177,   132,   129,   131,   181,   182,   183,   184,   185,   186,
     187,   188,    37,    37,    37,   192,   193,   194,   195,    37,
      37,   132,   133,   134,   135,   126,   127,   128,   129,   217,
     218,    37,    37,   274,   275,    62,    37,    37,   139,   140,
     126,   127,   128,   129,   131,     4,     4,     3,     3,    48,
      37,     2,   137,   139,   140,   173,   232,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   217,   218,    -1,    -1,    -1,
      -1,    -1,    -1,     0,   217,   218,   217,   218,    -1,    -1,
      -1,    -1,    -1,    -1,    11,    12,   274,   275,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,    -1,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      -1,    -1,   274,   275,    -1,    -1,    -1,    -1,    -1,    56,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,    -1,    -1,    -1,   123,    -1,    -1,    -1,
     127,    -1,    11,    12,    -1,   132,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    -1,    -1,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    56,    -1,    -1,
      -1,    60,    61,    62,    63,    64,    -1,    -1,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,    -1,    -1,    -1,   123,    -1,    -1,    -1,   127,    -1,
      11,    12,    -1,   132,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    -1,    -1,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    56,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,    -1,
      -1,    -1,   123,    -1,    -1,    -1,   127,    -1,    -1,    -1,
      -1,   132,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,    -1,    37,    -1,    57,    58,    59,    -1,    -1,
      -1,    -1,    -1,    65,    66,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    56,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    67,    68,    69,    70,    71,    -1,    -1,
      65,    66,    -1,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    65,    66,    -1,
      -1,    -1,    65,    66,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   105,   124,   125,   126,   127,   128,   129,    -1,    -1,
      -1,   133,   134,   135,    -1,    -1,    -1,   139,   140,    -1,
      -1,    -1,    -1,   127,    -1,    -1,    -1,    -1,   132,   124,
     125,   126,   127,   128,   129,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   137,   138,   139,   140,   124,   125,   126,   127,
     128,   129,   125,   126,   127,   128,   129,    -1,    -1,    -1,
      -1,   139,   140,    -1,    -1,    -1,   139,   140,     3,     4,
       5,     6,     7,    -1,     9,    10,    -1,    -1,    13,    14,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      35,    36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    47,    48,    49,    50,    51,    52,    53,    54,
      55
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   142,   143,     0,    11,    12,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    56,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     123,   127,   132,   144,   149,   150,   153,   154,   155,   156,
     160,   161,   162,   167,   173,   174,   176,   177,   178,   179,
     183,   184,    37,    37,    37,    37,    60,    60,    37,    37,
      37,    60,   169,    60,   170,   150,   176,   176,   176,   176,
      43,    60,   150,   160,   173,   121,   122,   145,   146,   153,
     144,   174,   176,    60,    61,    62,    63,    64,   147,   148,
     149,   150,   176,     3,     4,     5,     6,     7,     9,    10,
      13,    14,    35,    36,    47,    48,    49,    50,    51,    52,
      53,    54,    55,   157,   158,   159,   163,   136,    57,    58,
      59,    65,    66,   124,   125,   126,   127,   128,   129,   133,
     134,   135,   139,   140,   171,   172,    37,   150,   171,   172,
     180,   181,    37,   150,   171,   172,   185,   186,   124,   125,
     133,   134,   135,   175,   176,   173,   147,   149,   150,   153,
     176,   147,   153,   131,   131,   131,     8,   129,   129,   147,
     148,   151,   152,   176,   121,   122,   121,   122,    37,    60,
     164,    60,   166,    37,    60,   168,   158,   173,   173,   173,
     173,   173,   173,   173,   173,   173,   173,   173,   173,   173,
     181,   182,    37,    37,   186,   187,    37,    37,    37,   176,
      62,    37,    37,   131,   145,   146,     4,     4,     3,     3,
      48,   137,   138,   131,   146,   131,   146,   147,   147,    37,
      60,   165,    37,   181,   186,   137
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (yyscanner, cstate, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))

/* Error token number */
#define YYTERROR	1
#define YYERRCODE	256


/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */
#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, yyscanner)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, yyscanner, cstate); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void *yyscanner, compiler_state_t *cstate)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yyscanner, cstate)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    void *yyscanner;
    compiler_state_t *cstate;
#endif
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
  YYUSE (yyscanner);
  YYUSE (cstate);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
        break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void *yyscanner, compiler_state_t *cstate)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yyscanner, cstate)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    void *yyscanner;
    compiler_state_t *cstate;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yyscanner, cstate);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, void *yyscanner, compiler_state_t *cstate)
#else
static void
yy_reduce_print (yyvsp, yyrule, yyscanner, cstate)
    YYSTYPE *yyvsp;
    int yyrule;
    void *yyscanner;
    compiler_state_t *cstate;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , yyscanner, cstate);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, yyscanner, cstate); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, void *yyscanner, compiler_state_t *cstate)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yyscanner, cstate)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    void *yyscanner;
    compiler_state_t *cstate;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yyscanner);
  YYUSE (cstate);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}




/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *yyscanner, compiler_state_t *cstate)
#else
int
yyparse (yyscanner, cstate)
    void *yyscanner;
    compiler_state_t *cstate;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;


#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
static YYSTYPE yyval_default;
# define YY_INITIAL_VALUE(Value) = Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval YY_INITIAL_VALUE(yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
/* Line 1792 of yacc.c  */
#line 396 "grammar.y"
    {
	CHECK_INT_VAL(finish_parse(cstate, (yyvsp[(2) - (2)].blk).b));
}
    break;

  case 4:
/* Line 1792 of yacc.c  */
#line 401 "grammar.y"
    { (yyval.blk).q = qerr; }
    break;

  case 6:
/* Line 1792 of yacc.c  */
#line 404 "grammar.y"
    { gen_and((yyvsp[(1) - (3)].blk).b, (yyvsp[(3) - (3)].blk).b); (yyval.blk) = (yyvsp[(3) - (3)].blk); }
    break;

  case 7:
/* Line 1792 of yacc.c  */
#line 405 "grammar.y"
    { gen_and((yyvsp[(1) - (3)].blk).b, (yyvsp[(3) - (3)].blk).b); (yyval.blk) = (yyvsp[(3) - (3)].blk); }
    break;

  case 8:
/* Line 1792 of yacc.c  */
#line 406 "grammar.y"
    { gen_or((yyvsp[(1) - (3)].blk).b, (yyvsp[(3) - (3)].blk).b); (yyval.blk) = (yyvsp[(3) - (3)].blk); }
    break;

  case 9:
/* Line 1792 of yacc.c  */
#line 407 "grammar.y"
    { gen_or((yyvsp[(1) - (3)].blk).b, (yyvsp[(3) - (3)].blk).b); (yyval.blk) = (yyvsp[(3) - (3)].blk); }
    break;

  case 10:
/* Line 1792 of yacc.c  */
#line 409 "grammar.y"
    { (yyval.blk) = (yyvsp[(0) - (1)].blk); }
    break;

  case 11:
/* Line 1792 of yacc.c  */
#line 411 "grammar.y"
    { (yyval.blk) = (yyvsp[(0) - (1)].blk); }
    break;

  case 13:
/* Line 1792 of yacc.c  */
#line 414 "grammar.y"
    { CHECK_PTR_VAL(((yyval.blk).b = gen_ncode(cstate, NULL, (yyvsp[(1) - (1)].h),
						   (yyval.blk).q = (yyvsp[(0) - (1)].blk).q))); }
    break;

  case 14:
/* Line 1792 of yacc.c  */
#line 416 "grammar.y"
    { (yyval.blk) = (yyvsp[(2) - (3)].blk); }
    break;

  case 15:
/* Line 1792 of yacc.c  */
#line 418 "grammar.y"
    { CHECK_PTR_VAL((yyvsp[(1) - (1)].s)); CHECK_PTR_VAL(((yyval.blk).b = gen_scode(cstate, (yyvsp[(1) - (1)].s), (yyval.blk).q = (yyvsp[(0) - (1)].blk).q))); }
    break;

  case 16:
/* Line 1792 of yacc.c  */
#line 419 "grammar.y"
    { CHECK_PTR_VAL((yyvsp[(1) - (3)].s)); CHECK_PTR_VAL(((yyval.blk).b = gen_mcode(cstate, (yyvsp[(1) - (3)].s), NULL, (yyvsp[(3) - (3)].h),
				    (yyval.blk).q = (yyvsp[(0) - (3)].blk).q))); }
    break;

  case 17:
/* Line 1792 of yacc.c  */
#line 421 "grammar.y"
    { CHECK_PTR_VAL((yyvsp[(1) - (3)].s)); CHECK_PTR_VAL(((yyval.blk).b = gen_mcode(cstate, (yyvsp[(1) - (3)].s), (yyvsp[(3) - (3)].s), 0,
				    (yyval.blk).q = (yyvsp[(0) - (3)].blk).q))); }
    break;

  case 18:
/* Line 1792 of yacc.c  */
#line 423 "grammar.y"
    {
				  CHECK_PTR_VAL((yyvsp[(1) - (1)].s));
				  /* Decide how to parse HID based on proto */
				  (yyval.blk).q = (yyvsp[(0) - (1)].blk).q;
				  if ((yyval.blk).q.addr == Q_PORT) {
					bpf_set_error(cstate, "'port' modifier applied to ip host");
					YYABORT;
				  } else if ((yyval.blk).q.addr == Q_PORTRANGE) {
					bpf_set_error(cstate, "'portrange' modifier applied to ip host");
					YYABORT;
				  } else if ((yyval.blk).q.addr == Q_PROTO) {
					bpf_set_error(cstate, "'proto' modifier applied to ip host");
					YYABORT;
				  } else if ((yyval.blk).q.addr == Q_PROTOCHAIN) {
					bpf_set_error(cstate, "'protochain' modifier applied to ip host");
					YYABORT;
				  }
				  CHECK_PTR_VAL(((yyval.blk).b = gen_ncode(cstate, (yyvsp[(1) - (1)].s), 0, (yyval.blk).q)));
				}
    break;

  case 19:
/* Line 1792 of yacc.c  */
#line 442 "grammar.y"
    {
				  CHECK_PTR_VAL((yyvsp[(1) - (3)].s));
#ifdef INET6
				  CHECK_PTR_VAL(((yyval.blk).b = gen_mcode6(cstate, (yyvsp[(1) - (3)].s), NULL, (yyvsp[(3) - (3)].h),
				    (yyval.blk).q = (yyvsp[(0) - (3)].blk).q)));
#else
				  bpf_set_error(cstate, "'ip6addr/prefixlen' not supported "
					"in this configuration");
				  YYABORT;
#endif /*INET6*/
				}
    break;

  case 20:
/* Line 1792 of yacc.c  */
#line 453 "grammar.y"
    {
				  CHECK_PTR_VAL((yyvsp[(1) - (1)].s));
#ifdef INET6
				  CHECK_PTR_VAL(((yyval.blk).b = gen_mcode6(cstate, (yyvsp[(1) - (1)].s), 0, 128,
				    (yyval.blk).q = (yyvsp[(0) - (1)].blk).q)));
#else
				  bpf_set_error(cstate, "'ip6addr' not supported "
					"in this configuration");
				  YYABORT;
#endif /*INET6*/
				}
    break;

  case 21:
/* Line 1792 of yacc.c  */
#line 464 "grammar.y"
    { CHECK_PTR_VAL((yyvsp[(1) - (1)].s)); CHECK_PTR_VAL(((yyval.blk).b = gen_ecode(cstate, (yyvsp[(1) - (1)].s), (yyval.blk).q = (yyvsp[(0) - (1)].blk).q))); }
    break;

  case 22:
/* Line 1792 of yacc.c  */
#line 465 "grammar.y"
    { CHECK_PTR_VAL((yyvsp[(1) - (1)].s)); CHECK_PTR_VAL(((yyval.blk).b = gen_acode(cstate, (yyvsp[(1) - (1)].s), (yyval.blk).q = (yyvsp[(0) - (1)].blk).q))); }
    break;

  case 23:
/* Line 1792 of yacc.c  */
#line 466 "grammar.y"
    { gen_not((yyvsp[(2) - (2)].blk).b); (yyval.blk) = (yyvsp[(2) - (2)].blk); }
    break;

  case 24:
/* Line 1792 of yacc.c  */
#line 468 "grammar.y"
    { (yyval.blk) = (yyvsp[(0) - (1)].blk); }
    break;

  case 25:
/* Line 1792 of yacc.c  */
#line 470 "grammar.y"
    { (yyval.blk) = (yyvsp[(0) - (1)].blk); }
    break;

  case 27:
/* Line 1792 of yacc.c  */
#line 473 "grammar.y"
    { gen_and((yyvsp[(1) - (3)].blk).b, (yyvsp[(3) - (3)].blk).b); (yyval.blk) = (yyvsp[(3) - (3)].blk); }
    break;

  case 28:
/* Line 1792 of yacc.c  */
#line 474 "grammar.y"
    { gen_or((yyvsp[(1) - (3)].blk).b, (yyvsp[(3) - (3)].blk).b); (yyval.blk) = (yyvsp[(3) - (3)].blk); }
    break;

  case 29:
/* Line 1792 of yacc.c  */
#line 476 "grammar.y"
    { CHECK_PTR_VAL(((yyval.blk).b = gen_ncode(cstate, NULL, (yyvsp[(1) - (1)].h),
						   (yyval.blk).q = (yyvsp[(0) - (1)].blk).q))); }
    break;

  case 32:
/* Line 1792 of yacc.c  */
#line 481 "grammar.y"
    { gen_not((yyvsp[(2) - (2)].blk).b); (yyval.blk) = (yyvsp[(2) - (2)].blk); }
    break;

  case 33:
/* Line 1792 of yacc.c  */
#line 483 "grammar.y"
    { QSET((yyval.blk).q, (yyvsp[(1) - (3)].i), (yyvsp[(2) - (3)].i), (yyvsp[(3) - (3)].i)); }
    break;

  case 34:
/* Line 1792 of yacc.c  */
#line 484 "grammar.y"
    { QSET((yyval.blk).q, (yyvsp[(1) - (2)].i), (yyvsp[(2) - (2)].i), Q_DEFAULT); }
    break;

  case 35:
/* Line 1792 of yacc.c  */
#line 485 "grammar.y"
    { QSET((yyval.blk).q, (yyvsp[(1) - (2)].i), Q_DEFAULT, (yyvsp[(2) - (2)].i)); }
    break;

  case 36:
/* Line 1792 of yacc.c  */
#line 486 "grammar.y"
    { QSET((yyval.blk).q, (yyvsp[(1) - (2)].i), Q_DEFAULT, Q_PROTO); }
    break;

  case 37:
/* Line 1792 of yacc.c  */
#line 487 "grammar.y"
    {
#ifdef NO_PROTOCHAIN
				  bpf_set_error(cstate, "protochain not supported");
				  YYABORT;
#else
				  QSET((yyval.blk).q, (yyvsp[(1) - (2)].i), Q_DEFAULT, Q_PROTOCHAIN);
#endif
				}
    break;

  case 38:
/* Line 1792 of yacc.c  */
#line 495 "grammar.y"
    { QSET((yyval.blk).q, (yyvsp[(1) - (2)].i), Q_DEFAULT, (yyvsp[(2) - (2)].i)); }
    break;

  case 39:
/* Line 1792 of yacc.c  */
#line 497 "grammar.y"
    { (yyval.blk) = (yyvsp[(2) - (2)].blk); }
    break;

  case 40:
/* Line 1792 of yacc.c  */
#line 498 "grammar.y"
    { (yyval.blk).b = (yyvsp[(2) - (3)].blk).b; (yyval.blk).q = (yyvsp[(1) - (3)].blk).q; }
    break;

  case 41:
/* Line 1792 of yacc.c  */
#line 499 "grammar.y"
    { CHECK_PTR_VAL(((yyval.blk).b = gen_proto_abbrev(cstate, (yyvsp[(1) - (1)].i)))); (yyval.blk).q = qerr; }
    break;

  case 42:
/* Line 1792 of yacc.c  */
#line 500 "grammar.y"
    { CHECK_PTR_VAL(((yyval.blk).b = gen_relation(cstate, (yyvsp[(2) - (3)].i), (yyvsp[(1) - (3)].a), (yyvsp[(3) - (3)].a), 0)));
				  (yyval.blk).q = qerr; }
    break;

  case 43:
/* Line 1792 of yacc.c  */
#line 502 "grammar.y"
    { CHECK_PTR_VAL(((yyval.blk).b = gen_relation(cstate, (yyvsp[(2) - (3)].i), (yyvsp[(1) - (3)].a), (yyvsp[(3) - (3)].a), 1)));
				  (yyval.blk).q = qerr; }
    break;

  case 44:
/* Line 1792 of yacc.c  */
#line 504 "grammar.y"
    { (yyval.blk).b = (yyvsp[(1) - (1)].rblk); (yyval.blk).q = qerr; }
    break;

  case 45:
/* Line 1792 of yacc.c  */
#line 505 "grammar.y"
    { CHECK_PTR_VAL(((yyval.blk).b = gen_atmtype_abbrev(cstate, (yyvsp[(1) - (1)].i)))); (yyval.blk).q = qerr; }
    break;

  case 46:
/* Line 1792 of yacc.c  */
#line 506 "grammar.y"
    { CHECK_PTR_VAL(((yyval.blk).b = gen_atmmulti_abbrev(cstate, (yyvsp[(1) - (1)].i)))); (yyval.blk).q = qerr; }
    break;

  case 47:
/* Line 1792 of yacc.c  */
#line 507 "grammar.y"
    { (yyval.blk).b = (yyvsp[(2) - (2)].blk).b; (yyval.blk).q = qerr; }
    break;

  case 48:
/* Line 1792 of yacc.c  */
#line 508 "grammar.y"
    { CHECK_PTR_VAL(((yyval.blk).b = gen_mtp2type_abbrev(cstate, (yyvsp[(1) - (1)].i)))); (yyval.blk).q = qerr; }
    break;

  case 49:
/* Line 1792 of yacc.c  */
#line 509 "grammar.y"
    { (yyval.blk).b = (yyvsp[(2) - (2)].blk).b; (yyval.blk).q = qerr; }
    break;

  case 51:
/* Line 1792 of yacc.c  */
#line 513 "grammar.y"
    { (yyval.i) = Q_DEFAULT; }
    break;

  case 52:
/* Line 1792 of yacc.c  */
#line 516 "grammar.y"
    { (yyval.i) = Q_SRC; }
    break;

  case 53:
/* Line 1792 of yacc.c  */
#line 517 "grammar.y"
    { (yyval.i) = Q_DST; }
    break;

  case 54:
/* Line 1792 of yacc.c  */
#line 518 "grammar.y"
    { (yyval.i) = Q_OR; }
    break;

  case 55:
/* Line 1792 of yacc.c  */
#line 519 "grammar.y"
    { (yyval.i) = Q_OR; }
    break;

  case 56:
/* Line 1792 of yacc.c  */
#line 520 "grammar.y"
    { (yyval.i) = Q_AND; }
    break;

  case 57:
/* Line 1792 of yacc.c  */
#line 521 "grammar.y"
    { (yyval.i) = Q_AND; }
    break;

  case 58:
/* Line 1792 of yacc.c  */
#line 522 "grammar.y"
    { (yyval.i) = Q_ADDR1; }
    break;

  case 59:
/* Line 1792 of yacc.c  */
#line 523 "grammar.y"
    { (yyval.i) = Q_ADDR2; }
    break;

  case 60:
/* Line 1792 of yacc.c  */
#line 524 "grammar.y"
    { (yyval.i) = Q_ADDR3; }
    break;

  case 61:
/* Line 1792 of yacc.c  */
#line 525 "grammar.y"
    { (yyval.i) = Q_ADDR4; }
    break;

  case 62:
/* Line 1792 of yacc.c  */
#line 526 "grammar.y"
    { (yyval.i) = Q_RA; }
    break;

  case 63:
/* Line 1792 of yacc.c  */
#line 527 "grammar.y"
    { (yyval.i) = Q_TA; }
    break;

  case 64:
/* Line 1792 of yacc.c  */
#line 530 "grammar.y"
    { (yyval.i) = Q_HOST; }
    break;

  case 65:
/* Line 1792 of yacc.c  */
#line 531 "grammar.y"
    { (yyval.i) = Q_NET; }
    break;

  case 66:
/* Line 1792 of yacc.c  */
#line 532 "grammar.y"
    { (yyval.i) = Q_PORT; }
    break;

  case 67:
/* Line 1792 of yacc.c  */
#line 533 "grammar.y"
    { (yyval.i) = Q_PORTRANGE; }
    break;

  case 68:
/* Line 1792 of yacc.c  */
#line 536 "grammar.y"
    { (yyval.i) = Q_GATEWAY; }
    break;

  case 69:
/* Line 1792 of yacc.c  */
#line 538 "grammar.y"
    { (yyval.i) = Q_LINK; }
    break;

  case 70:
/* Line 1792 of yacc.c  */
#line 539 "grammar.y"
    { (yyval.i) = Q_IP; }
    break;

  case 71:
/* Line 1792 of yacc.c  */
#line 540 "grammar.y"
    { (yyval.i) = Q_ARP; }
    break;

  case 72:
/* Line 1792 of yacc.c  */
#line 541 "grammar.y"
    { (yyval.i) = Q_RARP; }
    break;

  case 73:
/* Line 1792 of yacc.c  */
#line 542 "grammar.y"
    { (yyval.i) = Q_SCTP; }
    break;

  case 74:
/* Line 1792 of yacc.c  */
#line 543 "grammar.y"
    { (yyval.i) = Q_TCP; }
    break;

  case 75:
/* Line 1792 of yacc.c  */
#line 544 "grammar.y"
    { (yyval.i) = Q_UDP; }
    break;

  case 76:
/* Line 1792 of yacc.c  */
#line 545 "grammar.y"
    { (yyval.i) = Q_ICMP; }
    break;

  case 77:
/* Line 1792 of yacc.c  */
#line 546 "grammar.y"
    { (yyval.i) = Q_IGMP; }
    break;

  case 78:
/* Line 1792 of yacc.c  */
#line 547 "grammar.y"
    { (yyval.i) = Q_IGRP; }
    break;

  case 79:
/* Line 1792 of yacc.c  */
#line 548 "grammar.y"
    { (yyval.i) = Q_PIM; }
    break;

  case 80:
/* Line 1792 of yacc.c  */
#line 549 "grammar.y"
    { (yyval.i) = Q_VRRP; }
    break;

  case 81:
/* Line 1792 of yacc.c  */
#line 550 "grammar.y"
    { (yyval.i) = Q_CARP; }
    break;

  case 82:
/* Line 1792 of yacc.c  */
#line 551 "grammar.y"
    { (yyval.i) = Q_ATALK; }
    break;

  case 83:
/* Line 1792 of yacc.c  */
#line 552 "grammar.y"
    { (yyval.i) = Q_AARP; }
    break;

  case 84:
/* Line 1792 of yacc.c  */
#line 553 "grammar.y"
    { (yyval.i) = Q_DECNET; }
    break;

  case 85:
/* Line 1792 of yacc.c  */
#line 554 "grammar.y"
    { (yyval.i) = Q_LAT; }
    break;

  case 86:
/* Line 1792 of yacc.c  */
#line 555 "grammar.y"
    { (yyval.i) = Q_SCA; }
    break;

  case 87:
/* Line 1792 of yacc.c  */
#line 556 "grammar.y"
    { (yyval.i) = Q_MOPDL; }
    break;

  case 88:
/* Line 1792 of yacc.c  */
#line 557 "grammar.y"
    { (yyval.i) = Q_MOPRC; }
    break;

  case 89:
/* Line 1792 of yacc.c  */
#line 558 "grammar.y"
    { (yyval.i) = Q_IPV6; }
    break;

  case 90:
/* Line 1792 of yacc.c  */
#line 559 "grammar.y"
    { (yyval.i) = Q_ICMPV6; }
    break;

  case 91:
/* Line 1792 of yacc.c  */
#line 560 "grammar.y"
    { (yyval.i) = Q_AH; }
    break;

  case 92:
/* Line 1792 of yacc.c  */
#line 561 "grammar.y"
    { (yyval.i) = Q_ESP; }
    break;

  case 93:
/* Line 1792 of yacc.c  */
#line 562 "grammar.y"
    { (yyval.i) = Q_ISO; }
    break;

  case 94:
/* Line 1792 of yacc.c  */
#line 563 "grammar.y"
    { (yyval.i) = Q_ESIS; }
    break;

  case 95:
/* Line 1792 of yacc.c  */
#line 564 "grammar.y"
    { (yyval.i) = Q_ISIS; }
    break;

  case 96:
/* Line 1792 of yacc.c  */
#line 565 "grammar.y"
    { (yyval.i) = Q_ISIS_L1; }
    break;

  case 97:
/* Line 1792 of yacc.c  */
#line 566 "grammar.y"
    { (yyval.i) = Q_ISIS_L2; }
    break;

  case 98:
/* Line 1792 of yacc.c  */
#line 567 "grammar.y"
    { (yyval.i) = Q_ISIS_IIH; }
    break;

  case 99:
/* Line 1792 of yacc.c  */
#line 568 "grammar.y"
    { (yyval.i) = Q_ISIS_LSP; }
    break;

  case 100:
/* Line 1792 of yacc.c  */
#line 569 "grammar.y"
    { (yyval.i) = Q_ISIS_SNP; }
    break;

  case 101:
/* Line 1792 of yacc.c  */
#line 570 "grammar.y"
    { (yyval.i) = Q_ISIS_PSNP; }
    break;

  case 102:
/* Line 1792 of yacc.c  */
#line 571 "grammar.y"
    { (yyval.i) = Q_ISIS_CSNP; }
    break;

  case 103:
/* Line 1792 of yacc.c  */
#line 572 "grammar.y"
    { (yyval.i) = Q_CLNP; }
    break;

  case 104:
/* Line 1792 of yacc.c  */
#line 573 "grammar.y"
    { (yyval.i) = Q_STP; }
    break;

  case 105:
/* Line 1792 of yacc.c  */
#line 574 "grammar.y"
    { (yyval.i) = Q_IPX; }
    break;

  case 106:
/* Line 1792 of yacc.c  */
#line 575 "grammar.y"
    { (yyval.i) = Q_NETBEUI; }
    break;

  case 107:
/* Line 1792 of yacc.c  */
#line 576 "grammar.y"
    { (yyval.i) = Q_RADIO; }
    break;

  case 108:
/* Line 1792 of yacc.c  */
#line 578 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_broadcast(cstate, (yyvsp[(1) - (2)].i)))); }
    break;

  case 109:
/* Line 1792 of yacc.c  */
#line 579 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_multicast(cstate, (yyvsp[(1) - (2)].i)))); }
    break;

  case 110:
/* Line 1792 of yacc.c  */
#line 580 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_less(cstate, (yyvsp[(2) - (2)].h)))); }
    break;

  case 111:
/* Line 1792 of yacc.c  */
#line 581 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_greater(cstate, (yyvsp[(2) - (2)].h)))); }
    break;

  case 112:
/* Line 1792 of yacc.c  */
#line 582 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_byteop(cstate, (yyvsp[(3) - (4)].i), (yyvsp[(2) - (4)].h), (yyvsp[(4) - (4)].h)))); }
    break;

  case 113:
/* Line 1792 of yacc.c  */
#line 583 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_inbound(cstate, 0))); }
    break;

  case 114:
/* Line 1792 of yacc.c  */
#line 584 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_inbound(cstate, 1))); }
    break;

  case 115:
/* Line 1792 of yacc.c  */
#line 585 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_ifindex(cstate, (yyvsp[(2) - (2)].h)))); }
    break;

  case 116:
/* Line 1792 of yacc.c  */
#line 586 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_vlan(cstate, (yyvsp[(2) - (2)].h), 1))); }
    break;

  case 117:
/* Line 1792 of yacc.c  */
#line 587 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_vlan(cstate, 0, 0))); }
    break;

  case 118:
/* Line 1792 of yacc.c  */
#line 588 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_mpls(cstate, (yyvsp[(2) - (2)].h), 1))); }
    break;

  case 119:
/* Line 1792 of yacc.c  */
#line 589 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_mpls(cstate, 0, 0))); }
    break;

  case 120:
/* Line 1792 of yacc.c  */
#line 590 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_pppoed(cstate))); }
    break;

  case 121:
/* Line 1792 of yacc.c  */
#line 591 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_pppoes(cstate, (yyvsp[(2) - (2)].h), 1))); }
    break;

  case 122:
/* Line 1792 of yacc.c  */
#line 592 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_pppoes(cstate, 0, 0))); }
    break;

  case 123:
/* Line 1792 of yacc.c  */
#line 593 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_geneve(cstate, (yyvsp[(2) - (2)].h), 1))); }
    break;

  case 124:
/* Line 1792 of yacc.c  */
#line 594 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_geneve(cstate, 0, 0))); }
    break;

  case 125:
/* Line 1792 of yacc.c  */
#line 595 "grammar.y"
    { (yyval.rblk) = (yyvsp[(1) - (1)].rblk); }
    break;

  case 126:
/* Line 1792 of yacc.c  */
#line 596 "grammar.y"
    { (yyval.rblk) = (yyvsp[(2) - (2)].rblk); }
    break;

  case 127:
/* Line 1792 of yacc.c  */
#line 597 "grammar.y"
    { (yyval.rblk) = (yyvsp[(1) - (1)].rblk); }
    break;

  case 128:
/* Line 1792 of yacc.c  */
#line 600 "grammar.y"
    { CHECK_PTR_VAL((yyvsp[(2) - (2)].s)); CHECK_PTR_VAL(((yyval.rblk) = gen_pf_ifname(cstate, (yyvsp[(2) - (2)].s)))); }
    break;

  case 129:
/* Line 1792 of yacc.c  */
#line 601 "grammar.y"
    { CHECK_PTR_VAL((yyvsp[(2) - (2)].s)); CHECK_PTR_VAL(((yyval.rblk) = gen_pf_ruleset(cstate, (yyvsp[(2) - (2)].s)))); }
    break;

  case 130:
/* Line 1792 of yacc.c  */
#line 602 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_pf_rnr(cstate, (yyvsp[(2) - (2)].h)))); }
    break;

  case 131:
/* Line 1792 of yacc.c  */
#line 603 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_pf_srnr(cstate, (yyvsp[(2) - (2)].h)))); }
    break;

  case 132:
/* Line 1792 of yacc.c  */
#line 604 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_pf_reason(cstate, (yyvsp[(2) - (2)].i)))); }
    break;

  case 133:
/* Line 1792 of yacc.c  */
#line 605 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_pf_action(cstate, (yyvsp[(2) - (2)].i)))); }
    break;

  case 134:
/* Line 1792 of yacc.c  */
#line 609 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_p80211_type(cstate, (yyvsp[(2) - (4)].i) | (yyvsp[(4) - (4)].i),
					IEEE80211_FC0_TYPE_MASK |
					IEEE80211_FC0_SUBTYPE_MASK)));
				}
    break;

  case 135:
/* Line 1792 of yacc.c  */
#line 613 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_p80211_type(cstate, (yyvsp[(2) - (2)].i),
					IEEE80211_FC0_TYPE_MASK)));
				}
    break;

  case 136:
/* Line 1792 of yacc.c  */
#line 616 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_p80211_type(cstate, (yyvsp[(2) - (2)].i),
					IEEE80211_FC0_TYPE_MASK |
					IEEE80211_FC0_SUBTYPE_MASK)));
				}
    break;

  case 137:
/* Line 1792 of yacc.c  */
#line 620 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_p80211_fcdir(cstate, (yyvsp[(2) - (2)].i)))); }
    break;

  case 138:
/* Line 1792 of yacc.c  */
#line 623 "grammar.y"
    { if (((yyvsp[(1) - (1)].h) & (~IEEE80211_FC0_TYPE_MASK)) != 0) {
					bpf_set_error(cstate, "invalid 802.11 type value 0x%02x", (yyvsp[(1) - (1)].h));
					YYABORT;
				  }
				  (yyval.i) = (int)(yyvsp[(1) - (1)].h);
				}
    break;

  case 139:
/* Line 1792 of yacc.c  */
#line 629 "grammar.y"
    { CHECK_PTR_VAL((yyvsp[(1) - (1)].s));
				  (yyval.i) = str2tok((yyvsp[(1) - (1)].s), ieee80211_types);
				  if ((yyval.i) == -1) {
					bpf_set_error(cstate, "unknown 802.11 type name \"%s\"", (yyvsp[(1) - (1)].s));
					YYABORT;
				  }
				}
    break;

  case 140:
/* Line 1792 of yacc.c  */
#line 638 "grammar.y"
    { if (((yyvsp[(1) - (1)].h) & (~IEEE80211_FC0_SUBTYPE_MASK)) != 0) {
					bpf_set_error(cstate, "invalid 802.11 subtype value 0x%02x", (yyvsp[(1) - (1)].h));
					YYABORT;
				  }
				  (yyval.i) = (int)(yyvsp[(1) - (1)].h);
				}
    break;

  case 141:
/* Line 1792 of yacc.c  */
#line 644 "grammar.y"
    { const struct tok *types = NULL;
				  int i;
				  CHECK_PTR_VAL((yyvsp[(1) - (1)].s));
				  for (i = 0;; i++) {
					if (ieee80211_type_subtypes[i].tok == NULL) {
						/* Ran out of types */
						bpf_set_error(cstate, "unknown 802.11 type");
						YYABORT;
					}
					if ((yyvsp[(-1) - (1)].i) == ieee80211_type_subtypes[i].type) {
						types = ieee80211_type_subtypes[i].tok;
						break;
					}
				  }

				  (yyval.i) = str2tok((yyvsp[(1) - (1)].s), types);
				  if ((yyval.i) == -1) {
					bpf_set_error(cstate, "unknown 802.11 subtype name \"%s\"", (yyvsp[(1) - (1)].s));
					YYABORT;
				  }
				}
    break;

  case 142:
/* Line 1792 of yacc.c  */
#line 667 "grammar.y"
    { int i;
				  CHECK_PTR_VAL((yyvsp[(1) - (1)].s));
				  for (i = 0;; i++) {
					if (ieee80211_type_subtypes[i].tok == NULL) {
						/* Ran out of types */
						bpf_set_error(cstate, "unknown 802.11 type name");
						YYABORT;
					}
					(yyval.i) = str2tok((yyvsp[(1) - (1)].s), ieee80211_type_subtypes[i].tok);
					if ((yyval.i) != -1) {
						(yyval.i) |= ieee80211_type_subtypes[i].type;
						break;
					}
				  }
				}
    break;

  case 143:
/* Line 1792 of yacc.c  */
#line 684 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_llc(cstate))); }
    break;

  case 144:
/* Line 1792 of yacc.c  */
#line 685 "grammar.y"
    { CHECK_PTR_VAL((yyvsp[(2) - (2)].s));
				  if (pcap_strcasecmp((yyvsp[(2) - (2)].s), "i") == 0) {
					CHECK_PTR_VAL(((yyval.rblk) = gen_llc_i(cstate)));
				  } else if (pcap_strcasecmp((yyvsp[(2) - (2)].s), "s") == 0) {
					CHECK_PTR_VAL(((yyval.rblk) = gen_llc_s(cstate)));
				  } else if (pcap_strcasecmp((yyvsp[(2) - (2)].s), "u") == 0) {
					CHECK_PTR_VAL(((yyval.rblk) = gen_llc_u(cstate)));
				  } else {
					int subtype;

					subtype = str2tok((yyvsp[(2) - (2)].s), llc_s_subtypes);
					if (subtype != -1) {
						CHECK_PTR_VAL(((yyval.rblk) = gen_llc_s_subtype(cstate, subtype)));
					} else {
						subtype = str2tok((yyvsp[(2) - (2)].s), llc_u_subtypes);
						if (subtype == -1) {
							bpf_set_error(cstate, "unknown LLC type name \"%s\"", (yyvsp[(2) - (2)].s));
							YYABORT;
						}
						CHECK_PTR_VAL(((yyval.rblk) = gen_llc_u_subtype(cstate, subtype)));
					}
				  }
				}
    break;

  case 145:
/* Line 1792 of yacc.c  */
#line 709 "grammar.y"
    { CHECK_PTR_VAL(((yyval.rblk) = gen_llc_s_subtype(cstate, LLC_RNR))); }
    break;

  case 146:
/* Line 1792 of yacc.c  */
#line 712 "grammar.y"
    { (yyval.i) = (int)(yyvsp[(1) - (1)].h); }
    break;

  case 147:
/* Line 1792 of yacc.c  */
#line 713 "grammar.y"
    { CHECK_PTR_VAL((yyvsp[(1) - (1)].s));
				  if (pcap_strcasecmp((yyvsp[(1) - (1)].s), "nods") == 0)
					(yyval.i) = IEEE80211_FC1_DIR_NODS;
				  else if (pcap_strcasecmp((yyvsp[(1) - (1)].s), "tods") == 0)
					(yyval.i) = IEEE80211_FC1_DIR_TODS;
				  else if (pcap_strcasecmp((yyvsp[(1) - (1)].s), "fromds") == 0)
					(yyval.i) = IEEE80211_FC1_DIR_FROMDS;
				  else if (pcap_strcasecmp((yyvsp[(1) - (1)].s), "dstods") == 0)
					(yyval.i) = IEEE80211_FC1_DIR_DSTODS;
				  else {
					bpf_set_error(cstate, "unknown 802.11 direction");
					YYABORT;
				  }
				}
    break;

  case 148:
/* Line 1792 of yacc.c  */
#line 729 "grammar.y"
    { (yyval.i) = (yyvsp[(1) - (1)].h); }
    break;

  case 149:
/* Line 1792 of yacc.c  */
#line 730 "grammar.y"
    { CHECK_PTR_VAL((yyvsp[(1) - (1)].s)); CHECK_INT_VAL(((yyval.i) = pfreason_to_num(cstate, (yyvsp[(1) - (1)].s)))); }
    break;

  case 150:
/* Line 1792 of yacc.c  */
#line 733 "grammar.y"
    { CHECK_PTR_VAL((yyvsp[(1) - (1)].s)); CHECK_INT_VAL(((yyval.i) = pfaction_to_num(cstate, (yyvsp[(1) - (1)].s)))); }
    break;

  case 151:
/* Line 1792 of yacc.c  */
#line 736 "grammar.y"
    { (yyval.i) = BPF_JGT; }
    break;

  case 152:
/* Line 1792 of yacc.c  */
#line 737 "grammar.y"
    { (yyval.i) = BPF_JGE; }
    break;

  case 153:
/* Line 1792 of yacc.c  */
#line 738 "grammar.y"
    { (yyval.i) = BPF_JEQ; }
    break;

  case 154:
/* Line 1792 of yacc.c  */
#line 740 "grammar.y"
    { (yyval.i) = BPF_JGT; }
    break;

  case 155:
/* Line 1792 of yacc.c  */
#line 741 "grammar.y"
    { (yyval.i) = BPF_JGE; }
    break;

  case 156:
/* Line 1792 of yacc.c  */
#line 742 "grammar.y"
    { (yyval.i) = BPF_JEQ; }
    break;

  case 157:
/* Line 1792 of yacc.c  */
#line 744 "grammar.y"
    { CHECK_PTR_VAL(((yyval.a) = gen_loadi(cstate, (yyvsp[(1) - (1)].h)))); }
    break;

  case 159:
/* Line 1792 of yacc.c  */
#line 747 "grammar.y"
    { CHECK_PTR_VAL(((yyval.a) = gen_load(cstate, (yyvsp[(1) - (4)].i), (yyvsp[(3) - (4)].a), 1))); }
    break;

  case 160:
/* Line 1792 of yacc.c  */
#line 748 "grammar.y"
    { CHECK_PTR_VAL(((yyval.a) = gen_load(cstate, (yyvsp[(1) - (6)].i), (yyvsp[(3) - (6)].a), (yyvsp[(5) - (6)].h)))); }
    break;

  case 161:
/* Line 1792 of yacc.c  */
#line 749 "grammar.y"
    { CHECK_PTR_VAL(((yyval.a) = gen_arth(cstate, BPF_ADD, (yyvsp[(1) - (3)].a), (yyvsp[(3) - (3)].a)))); }
    break;

  case 162:
/* Line 1792 of yacc.c  */
#line 750 "grammar.y"
    { CHECK_PTR_VAL(((yyval.a) = gen_arth(cstate, BPF_SUB, (yyvsp[(1) - (3)].a), (yyvsp[(3) - (3)].a)))); }
    break;

  case 163:
/* Line 1792 of yacc.c  */
#line 751 "grammar.y"
    { CHECK_PTR_VAL(((yyval.a) = gen_arth(cstate, BPF_MUL, (yyvsp[(1) - (3)].a), (yyvsp[(3) - (3)].a)))); }
    break;

  case 164:
/* Line 1792 of yacc.c  */
#line 752 "grammar.y"
    { CHECK_PTR_VAL(((yyval.a) = gen_arth(cstate, BPF_DIV, (yyvsp[(1) - (3)].a), (yyvsp[(3) - (3)].a)))); }
    break;

  case 165:
/* Line 1792 of yacc.c  */
#line 753 "grammar.y"
    { CHECK_PTR_VAL(((yyval.a) = gen_arth(cstate, BPF_MOD, (yyvsp[(1) - (3)].a), (yyvsp[(3) - (3)].a)))); }
    break;

  case 166:
/* Line 1792 of yacc.c  */
#line 754 "grammar.y"
    { CHECK_PTR_VAL(((yyval.a) = gen_arth(cstate, BPF_AND, (yyvsp[(1) - (3)].a), (yyvsp[(3) - (3)].a)))); }
    break;

  case 167:
/* Line 1792 of yacc.c  */
#line 755 "grammar.y"
    { CHECK_PTR_VAL(((yyval.a) = gen_arth(cstate, BPF_OR, (yyvsp[(1) - (3)].a), (yyvsp[(3) - (3)].a)))); }
    break;

  case 168:
/* Line 1792 of yacc.c  */
#line 756 "grammar.y"
    { CHECK_PTR_VAL(((yyval.a) = gen_arth(cstate, BPF_XOR, (yyvsp[(1) - (3)].a), (yyvsp[(3) - (3)].a)))); }
    break;

  case 169:
/* Line 1792 of yacc.c  */
#line 757 "grammar.y"
    { CHECK_PTR_VAL(((yyval.a) = gen_arth(cstate, BPF_LSH, (yyvsp[(1) - (3)].a), (yyvsp[(3) - (3)].a)))); }
    break;

  case 170:
/* Line 1792 of yacc.c  */
#line 758 "grammar.y"
    { CHECK_PTR_VAL(((yyval.a) = gen_arth(cstate, BPF_RSH, (yyvsp[(1) - (3)].a), (yyvsp[(3) - (3)].a)))); }
    break;

  case 171:
/* Line 1792 of yacc.c  */
#line 759 "grammar.y"
    { CHECK_PTR_VAL(((yyval.a) = gen_neg(cstate, (yyvsp[(2) - (2)].a)))); }
    break;

  case 172:
/* Line 1792 of yacc.c  */
#line 760 "grammar.y"
    { (yyval.a) = (yyvsp[(2) - (3)].a); }
    break;

  case 173:
/* Line 1792 of yacc.c  */
#line 761 "grammar.y"
    { CHECK_PTR_VAL(((yyval.a) = gen_loadlen(cstate))); }
    break;

  case 174:
/* Line 1792 of yacc.c  */
#line 763 "grammar.y"
    { (yyval.i) = '&'; }
    break;

  case 175:
/* Line 1792 of yacc.c  */
#line 764 "grammar.y"
    { (yyval.i) = '|'; }
    break;

  case 176:
/* Line 1792 of yacc.c  */
#line 765 "grammar.y"
    { (yyval.i) = '<'; }
    break;

  case 177:
/* Line 1792 of yacc.c  */
#line 766 "grammar.y"
    { (yyval.i) = '>'; }
    break;

  case 178:
/* Line 1792 of yacc.c  */
#line 767 "grammar.y"
    { (yyval.i) = '='; }
    break;

  case 180:
/* Line 1792 of yacc.c  */
#line 770 "grammar.y"
    { (yyval.h) = (yyvsp[(2) - (3)].h); }
    break;

  case 181:
/* Line 1792 of yacc.c  */
#line 772 "grammar.y"
    { (yyval.i) = A_LANE; }
    break;

  case 182:
/* Line 1792 of yacc.c  */
#line 773 "grammar.y"
    { (yyval.i) = A_METAC;	}
    break;

  case 183:
/* Line 1792 of yacc.c  */
#line 774 "grammar.y"
    { (yyval.i) = A_BCC; }
    break;

  case 184:
/* Line 1792 of yacc.c  */
#line 775 "grammar.y"
    { (yyval.i) = A_OAMF4EC; }
    break;

  case 185:
/* Line 1792 of yacc.c  */
#line 776 "grammar.y"
    { (yyval.i) = A_OAMF4SC; }
    break;

  case 186:
/* Line 1792 of yacc.c  */
#line 777 "grammar.y"
    { (yyval.i) = A_SC; }
    break;

  case 187:
/* Line 1792 of yacc.c  */
#line 778 "grammar.y"
    { (yyval.i) = A_ILMIC; }
    break;

  case 188:
/* Line 1792 of yacc.c  */
#line 780 "grammar.y"
    { (yyval.i) = A_OAM; }
    break;

  case 189:
/* Line 1792 of yacc.c  */
#line 781 "grammar.y"
    { (yyval.i) = A_OAMF4; }
    break;

  case 190:
/* Line 1792 of yacc.c  */
#line 782 "grammar.y"
    { (yyval.i) = A_CONNECTMSG; }
    break;

  case 191:
/* Line 1792 of yacc.c  */
#line 783 "grammar.y"
    { (yyval.i) = A_METACONNECT; }
    break;

  case 192:
/* Line 1792 of yacc.c  */
#line 786 "grammar.y"
    { (yyval.blk).atmfieldtype = A_VPI; }
    break;

  case 193:
/* Line 1792 of yacc.c  */
#line 787 "grammar.y"
    { (yyval.blk).atmfieldtype = A_VCI; }
    break;

  case 195:
/* Line 1792 of yacc.c  */
#line 790 "grammar.y"
    { CHECK_PTR_VAL(((yyval.blk).b = gen_atmfield_code(cstate, (yyvsp[(0) - (2)].blk).atmfieldtype, (yyvsp[(2) - (2)].h), (yyvsp[(1) - (2)].i), 0))); }
    break;

  case 196:
/* Line 1792 of yacc.c  */
#line 791 "grammar.y"
    { CHECK_PTR_VAL(((yyval.blk).b = gen_atmfield_code(cstate, (yyvsp[(0) - (2)].blk).atmfieldtype, (yyvsp[(2) - (2)].h), (yyvsp[(1) - (2)].i), 1))); }
    break;

  case 197:
/* Line 1792 of yacc.c  */
#line 792 "grammar.y"
    { (yyval.blk).b = (yyvsp[(2) - (3)].blk).b; (yyval.blk).q = qerr; }
    break;

  case 198:
/* Line 1792 of yacc.c  */
#line 794 "grammar.y"
    {
	(yyval.blk).atmfieldtype = (yyvsp[(0) - (1)].blk).atmfieldtype;
	if ((yyval.blk).atmfieldtype == A_VPI ||
	    (yyval.blk).atmfieldtype == A_VCI)
		CHECK_PTR_VAL(((yyval.blk).b = gen_atmfield_code(cstate, (yyval.blk).atmfieldtype, (yyvsp[(1) - (1)].h), BPF_JEQ, 0)));
	}
    break;

  case 200:
/* Line 1792 of yacc.c  */
#line 802 "grammar.y"
    { gen_or((yyvsp[(1) - (3)].blk).b, (yyvsp[(3) - (3)].blk).b); (yyval.blk) = (yyvsp[(3) - (3)].blk); }
    break;

  case 201:
/* Line 1792 of yacc.c  */
#line 805 "grammar.y"
    { (yyval.i) = M_FISU; }
    break;

  case 202:
/* Line 1792 of yacc.c  */
#line 806 "grammar.y"
    { (yyval.i) = M_LSSU; }
    break;

  case 203:
/* Line 1792 of yacc.c  */
#line 807 "grammar.y"
    { (yyval.i) = M_MSU; }
    break;

  case 204:
/* Line 1792 of yacc.c  */
#line 808 "grammar.y"
    { (yyval.i) = MH_FISU; }
    break;

  case 205:
/* Line 1792 of yacc.c  */
#line 809 "grammar.y"
    { (yyval.i) = MH_LSSU; }
    break;

  case 206:
/* Line 1792 of yacc.c  */
#line 810 "grammar.y"
    { (yyval.i) = MH_MSU; }
    break;

  case 207:
/* Line 1792 of yacc.c  */
#line 813 "grammar.y"
    { (yyval.blk).mtp3fieldtype = M_SIO; }
    break;

  case 208:
/* Line 1792 of yacc.c  */
#line 814 "grammar.y"
    { (yyval.blk).mtp3fieldtype = M_OPC; }
    break;

  case 209:
/* Line 1792 of yacc.c  */
#line 815 "grammar.y"
    { (yyval.blk).mtp3fieldtype = M_DPC; }
    break;

  case 210:
/* Line 1792 of yacc.c  */
#line 816 "grammar.y"
    { (yyval.blk).mtp3fieldtype = M_SLS; }
    break;

  case 211:
/* Line 1792 of yacc.c  */
#line 817 "grammar.y"
    { (yyval.blk).mtp3fieldtype = MH_SIO; }
    break;

  case 212:
/* Line 1792 of yacc.c  */
#line 818 "grammar.y"
    { (yyval.blk).mtp3fieldtype = MH_OPC; }
    break;

  case 213:
/* Line 1792 of yacc.c  */
#line 819 "grammar.y"
    { (yyval.blk).mtp3fieldtype = MH_DPC; }
    break;

  case 214:
/* Line 1792 of yacc.c  */
#line 820 "grammar.y"
    { (yyval.blk).mtp3fieldtype = MH_SLS; }
    break;

  case 216:
/* Line 1792 of yacc.c  */
#line 823 "grammar.y"
    { CHECK_PTR_VAL(((yyval.blk).b = gen_mtp3field_code(cstate, (yyvsp[(0) - (2)].blk).mtp3fieldtype, (yyvsp[(2) - (2)].h), (yyvsp[(1) - (2)].i), 0))); }
    break;

  case 217:
/* Line 1792 of yacc.c  */
#line 824 "grammar.y"
    { CHECK_PTR_VAL(((yyval.blk).b = gen_mtp3field_code(cstate, (yyvsp[(0) - (2)].blk).mtp3fieldtype, (yyvsp[(2) - (2)].h), (yyvsp[(1) - (2)].i), 1))); }
    break;

  case 218:
/* Line 1792 of yacc.c  */
#line 825 "grammar.y"
    { (yyval.blk).b = (yyvsp[(2) - (3)].blk).b; (yyval.blk).q = qerr; }
    break;

  case 219:
/* Line 1792 of yacc.c  */
#line 827 "grammar.y"
    {
	(yyval.blk).mtp3fieldtype = (yyvsp[(0) - (1)].blk).mtp3fieldtype;
	if ((yyval.blk).mtp3fieldtype == M_SIO ||
	    (yyval.blk).mtp3fieldtype == M_OPC ||
	    (yyval.blk).mtp3fieldtype == M_DPC ||
	    (yyval.blk).mtp3fieldtype == M_SLS ||
	    (yyval.blk).mtp3fieldtype == MH_SIO ||
	    (yyval.blk).mtp3fieldtype == MH_OPC ||
	    (yyval.blk).mtp3fieldtype == MH_DPC ||
	    (yyval.blk).mtp3fieldtype == MH_SLS)
		CHECK_PTR_VAL(((yyval.blk).b = gen_mtp3field_code(cstate, (yyval.blk).mtp3fieldtype, (yyvsp[(1) - (1)].h), BPF_JEQ, 0)));
	}
    break;

  case 221:
/* Line 1792 of yacc.c  */
#line 841 "grammar.y"
    { gen_or((yyvsp[(1) - (3)].blk).b, (yyvsp[(3) - (3)].blk).b); (yyval.blk) = (yyvsp[(3) - (3)].blk); }
    break;


/* Line 1792 of yacc.c  */
#line 3609 "grammar.c"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (yyscanner, cstate, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yyscanner, cstate, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, yyscanner, cstate);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yyscanner, cstate);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (yyscanner, cstate, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, yyscanner, cstate);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yyscanner, cstate);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


/* Line 2055 of yacc.c  */
#line 843 "grammar.y"

