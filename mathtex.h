/******************************************************************************
 * mathTeX v2.0, Copyright(c) 2007-2023, John Forkosh Associates, Inc
 * Modified by mechabubba @ https://github.com/mechabubba/mathtex.
 *
 * This file is part of mathTeX, which is free software. You may redistribute
 * and/or modify it under the terms of the GNU General Public License, verison
 * 3 or later, as published by the Free Software Foundation.
 *
 * MathTeX is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY, not even the implied warranty of MERCHANTABILITY. See the GNU
 * General Public License for specific details.
 *
 * By using mathTeX, you warrant that you have read, understood and agreed
 * to these terms and conditions, and that you possess the legal right and
 * ability to enter into this agreement and to use mathTeX in accordance with
 * it.
 *
 * Your mathtex distribution should contain the file COPYING, an ascii text
 * copy of the GNU General Public License, version 3. If not, point your
 * browser to  http://www.gnu.org/licenses/ or write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330,  Boston, MA 02111-1307 USA.
 *
 *****************************************************************************/

/* -------------------------------------------------------------------------
Program ID
-------------------------------------------------------------------------- */
#define VERSION "2.0"              /* mathTeX version number */
#define REVISIONDATE "18 Oct 2023" /* date of most recent revision */
#define COPYRIGHTDATE "2007-2023"  /* copyright date */

/* -------------------------------------------------------------------------
Standard header files
-------------------------------------------------------------------------- */
/* ---
 * standard headers
 * ---------------- */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for getopts
#define _GNU_SOURCE
#include <string.h>
char *strcasestr(); /* non-standard extension */
#include <ctype.h>
#include <time.h>
extern char **environ; /* for \environment directive */
#include "md5.h"

/* -------------------------------------------------------------------------
Information adjustable by -D switches on compile line
-------------------------------------------------------------------------- */
/* ---
 * executable paths (e.g., path/latex including filename of executable image)
 * ----------------------------------------------------------------------- */
/* --- determine which switches have been explicitly specified --- */
#if defined(LATEX)
    #define ISLATEXSWITCH 1 /* have -DLATEX=\"path/latex\" */
#else
    #define ISLATEXSWITCH 0                    /* no -DLATEX switch */
    #define LATEX "/usr/share/texmf/bin/latex" /* default path to latex */
#endif
#if defined(PDFLATEX)
    #define ISPDFLATEXSWITCH 1 /* have -PDFDLATEX=\"path/latex\" */
#else
    #define ISPDFLATEXSWITCH 0                       /* no -PDFDLATEX switch */
    #define PDFLATEX "/usr/share/texmf/bin/pdflatex" /* default pdflatex path*/
#endif
#if defined(DVIPNG)
    #define ISDVIPNGSWITCH 1 /* have -DDVIPNG=\"path/dvipng\" */
#else
    #define ISDVIPNGSWITCH 0                     /* no -DDVIPNG switch */
    #define DVIPNG "/usr/share/texmf/bin/dvipng" /* default path to dvipng */
#endif
#if defined(DVIPS)
    #define ISDVIPSSWITCH 1 /* have -DDVIPS=\"path/dvips\" */
#else
    #define ISDVIPSSWITCH 0                    /* no -DDVIPS switch */
    #define DVIPS "/usr/share/texmf/bin/dvips" /* default path to dvips */
#endif
#if defined(PS2EPSI)
    #define ISPS2EPSISWITCH 1 /* have -DPS2EPSI=\"path/ps2epsi\" */
#else
    #define ISPS2EPSISWITCH 0          /* no -DPS2EPSI switch */
    #define PS2EPSI "/usr/bin/ps2epsi" /* default path to ps2epsi */
#endif
#if defined(CONVERT)
    #define ISCONVERTSWITCH 1 /* have -DCONVERT=\"path/convert\" */
#else
    #define ISCONVERTSWITCH 0          /* no -DCONVERT switch */
    #define CONVERT "/usr/bin/convert" /* default path to convert */
#endif
#if defined(TIMELIMIT)
    #define ISTIMELIMITSWITCH 1 /* have -DTIMELIMIT=\"path/timelimit\" */
#else
    #define ISTIMELIMITSWITCH 0                  /* no -DTIMELIMIT switch */
    #define TIMELIMIT "/usr/local/bin/timelimit" /* default path to timelimit*/
#endif
#if !defined(WAIT)      /* -DWAIT=\"50\" for half-second */
    #define WAIT "\000" /* default to no wait */
#endif
#if !defined(NOWAIT)      /* comma-separated list of referer's*/
    #define NOWAIT "\000" /* default to everyone waits */
#endif

/* --- paths, as specified by -D switches, else from whichpath() --- */
static char latexpath[256] = LATEX, pdflatexpath[256] = PDFLATEX, dvipngpath[256] = DVIPNG, dvipspath[256] = DVIPS, ps2epsipath[256] = PS2EPSI,
            convertpath[256] = CONVERT, timelimitpath[256] = TIMELIMIT;

/* --- source of path info: 0=default, 1=switch, 2=which, 3=locate --- */
static int islatexpath = ISLATEXSWITCH, ispdflatexpath = ISPDFLATEXSWITCH, isdvipngpath = ISDVIPNGSWITCH, isdvipspath = ISDVIPSSWITCH,
           isps2epsipath = ISPS2EPSISWITCH, isconvertpath = ISCONVERTSWITCH, istimelimitpath = ISTIMELIMITSWITCH;

/* ---
 * home path pwd of running executable image
 * ------------------------------------------- */
static char homepath[256] = "\000"; /* path to executable image */

/* ---
 * cache path -DCACHE=\"path/\" specifies directory
 * ------------------------------------------------ */
#if !defined(CACHE)
    #define CACHE "./cache/" /* relative to mathtex */
#endif
static int iscaching = 1;           /* true if caching images */
static char cachepath[256] = CACHE; /* path to cached image files */

/* ---
 * working directory for temp files -DWORK=\"path/\"
 * ------------------------------------------------- */
#if !defined(WORK)
    #define WORK "./cache/" /* relative to mathtex */
#endif
static char workpath[256] = WORK; /* path to temp file working dir */

/* ---
 * latex method info specifying latex,pdflatex
 * ------------------------------------------- */
#if ISLATEXSWITCH == 0 && ISPDFLATEXSWITCH == 1
    #define LATEXMETHOD 2
#elif !defined(LATEXMETHOD)
    #define LATEXMETHOD 1
#endif
static int latexmethod = LATEXMETHOD; /* 1=latex, 2=pdflatex */
static int ispicture = 0;             /* true for picture environment */

/* ---
 * image method info specifying dvipng or dvips/convert
 * use dvipng if -DDVIPNG supplied (or -DDVIPNGMETHOD specified),
 * else use dvips/convert if -DDVIPS supplied (or -DDVIPSMETHOD specified)
 * ----------------------------------------------------------------------- */
#if defined(DVIPNGMETHOD) || ISDVIPNGSWITCH == 1
    #define IMAGEMETHOD 1
#elif defined(DVIPSMETHOD) || (ISDVIPSSWITCH == 1 && ISDVIPNGSWITCH == 0)
    #define IMAGEMETHOD 2
#elif !defined(IMAGEMETHOD)
    #define IMAGEMETHOD 1
#endif
static int imagemethod = IMAGEMETHOD; /* 1=dvipng, 2=dvips/convert */

/* ---
 * image type info specifying gif, png
 * ----------------------------------- */
#if defined(GIF)
    #define IMAGETYPE 1
#endif
#if defined(PNG)
    #define IMAGETYPE 2
#endif
#if !defined(IMAGETYPE)
    #define IMAGETYPE 2 // default image type.
#endif
static int imagetype = IMAGETYPE;                       /* 1=gif, 2=png */
static char *extensions[] = {NULL, "gif", "png", NULL}; /* image type file .extensions */

/* ---
 * \[ \displaystyle \]  or  $ \textstyle $  or  \parstyle
 * ------------------------------------------------------ */
#if defined(DISPLAYSTYLE)
    #define MATHMODE 0
#endif
#if defined(TEXTSTYLE)
    #define MATHMODE 1
#endif
#if defined(PARSTYLE)
    #define MATHMODE 2
#endif
#if !defined(MATHMODE)
    #define MATHMODE 0
#endif
static int mathmode = MATHMODE; /* 0=display 1=text 2=paragraph */

/* ---
 * font size info 1=\tiny ... 10=\Huge
 * ----------------------------------- */
#if !defined(FONTSIZE)
    #define FONTSIZE 4
#endif
static int fontsize = FONTSIZE;  /* 0 = tiny ... 9 = Huge */
static char *sizedirectives[] = {/* fontsize directives */
                                 "\\tiny",  "\\scriptsize", "\\footnotesize", "\\small", "\\normalsize", "\\large", "\\Large",
                                 "\\LARGE", "\\huge",       "\\Huge",         NULL};

/* ---
 * dpi/density info for dvipng/convert
 * ----------------------------------- */
#if !defined(DPI)
    #define DPI "120"
#endif
static char density[256] = DPI; /*-D/-density arg for dvipng/convert*/

/* ---
 * default -gamma for convert is 0.5, or --gamma for dvipng is 2.5
 * --------------------------------------------------------------- */
#define DVIPNGGAMMA "2.5"  /* default gamma for dvipng */
#define CONVERTGAMMA "0.5" /* default gamma for convert */
#if !defined(GAMMA)
    #define ISGAMMA 0              /* no -DGAMMA=\"gamma\" switch */
    #if IMAGEMETHOD == 1           /* for dvipng... */
        #define GAMMA DVIPNGGAMMA  /* ...default gamma is 2.5 */
    #elif IMAGEMETHOD == 2         /* for convert... */
        #define GAMMA CONVERTGAMMA /* ...default gamma is 0.5 */
    #else                          /* otherwise... */
        #define GAMMA "1.0"        /* ...default gamma is 1.0 */
    #endif
#else
    #define ISGAMMA 1 /* -DGAMMA=\"gamma\" supplied */
#endif
static char gamma[256] = GAMMA; /* -gamma arg for convert() */

/* ---
 * latex -halt-on-error or quiet
 * ----------------------------- */
#if defined(QUIET)
    #define ISQUIET 99999 /* reply q */
#elif defined(NOQUIET)
    #define ISQUIET 0 /* reply x (-halt-on-error) */
#elif defined(NQUIET)
    #define ISQUIET NQUIET /* reply NQUIET <Enter>'s then x */
#else
    #define ISQUIET 3 /* default reply 3 <Enter>'s then x*/
#endif
static int isquiet = ISQUIET; /* >99=quiet, 0=-halt-on-error */

/* ---
 * emit depth below baseline (for vertical centering)
 * -------------------------------------------------- */
#if defined(DEPTH)
    #define ISDEPTH 1
#else
    #define ISDEPTH 0
#endif
static int isdepth = ISDEPTH; /* true to emit depth */

/* ---
 * timelimit -tWARNTIME -TKILLTIME
 * ------------------------------- */
#if !defined(KILLTIME)                  /* no -DKILLTIME given... */
    #define NOKILLTIMESWITCH            /* ...remember that fact below */
    #if ISTIMELIMITSWITCH == 0          /* standalone timelimit disabled */
        #if defined(WARNTIME)           /* have WARNTIME but not KILLTIME */
            #define KILLTIME (WARNTIME) /* so use WARNTIME for KILLTIME */
        #else                           /* neither WARNTIME nor KILLTIME */
            #define KILLTIME (10)       /* default for built-in timelimit()*/
        //#define KILLTIME (0)        /* disable until debugged */
        #endif
    #else                    /* using standalone timelimit */
        #define KILLTIME (1) /* always default -T1 killtime */
    #endif
#endif
#if !defined(WARNTIME)                  /*no -DWARNTIME given for timelimit*/
    #if ISTIMELIMITSWITCH == 0          /* and no -DTIMELIMIT path either */
        #define WARNTIME (-1)           /* so standalone timelimit disabled*/
    #else                               /*have path to standalone timelimit*/
        #if !defined(NOKILLTIMESWITCH)  /* have KILLTIME but not WARNTIME */
            #define WARNTIME (KILLTIME) /* so use KILLTIME for WARNTIME */
            #undef KILLTIME             /* but not for standalone killtime */
            #define KILLTIME (1)        /* default -T1 killtime instead */
        #else                           /* neither KILLTIME nor WARNTIME */
            #define WARNTIME (10)       /* so default -t10 warntime */
        #endif
    #endif
#endif
static int warntime = WARNTIME, killtime = KILLTIME; /* -twarn -Tkill values */

/* ---
 * compile (or not) built-in timelimit() code
 * ------------------------------------------ */
#if ISTIMELIMITSWITCH == 0 && KILLTIME > 0 /* no -DTIMELIMIT, but have KILLTIME */
    #define ISCOMPILETIMELIMIT 1           /* so we need built-in code */
#else
    #define ISCOMPILETIMELIMIT 0 /* else we don't need built-in code*/
#endif
static int iscompiletimelimit = ISCOMPILETIMELIMIT; /* 1=use timelimit() */
#if ISCOMPILETIMELIMIT                              /*header files, etc for timelimit()*/
    /* --- header files for timelimit() --- */
    #include <signal.h>
    #include <sys/signal.h>
    #include <sys/wait.h>
    #include <sysexits.h>
    #include <unistd.h>
    //#define EX_OSERR 71 /* system error (e.g., can't fork) */
    #define HAVE_SIGACTION
/* --- global variables for timelimit() --- */
volatile int fdone, falarm, fsig, sigcaught;
#endif

/* ---
 * additional latex \usepackage{}'s
 * -------------------------------- */
static int npackages = 0;     /* number of additional packages */
static char packages[9][128]; /* additional package names */
static char packargs[9][128]; /* optional arg for package */

/* ---
 * default uses locatepath() if whichpath() fails
 * ---------------------------------------------- */
#if !defined(NOWHICH)
    #define ISWHICH 1
#else
    #define ISWHICH 0
#endif
#if !defined(NOLOCATE)
    #define ISLOCATE 1
#else
    #define ISLOCATE 0
#endif

/* ---
 * debugging and error reporting
 * ----------------------------- */
#if !defined(MSGLEVEL)
    #define MSGLEVEL 1
#endif
#if !defined(MAXMSGLEVEL)
    #define MAXMSGLEVEL 999999
#endif
static int msglevel = MSGLEVEL; /* message level for verbose/debug */
static FILE *msgfp = NULL;      /* output in command-line mode */
char *strwrap();                /* help format debugging messages */

#define log(lvl, fp, ...)         \
    if (msglevel >= (lvl)) {      \
        fprintf(fp, __VA_ARGS__); \
        fflush(fp);               \
    }
#define log_info(lvl, ...) log(lvl, stdout, __VA_ARGS__)
#define log_error(...) log(0, stderr, __VA_ARGS__)

static int msgnumber = 0; /* embeddedimages() in query mode */
#define MAXEMBEDDED 16    /* 1...#embedded images available */
#define TESTMESSAGE 1     /* msg# for mathTeX test message */
#define UNKNOWNERROR 2    /* msg# for non-specific error */
#define CACHEFAILED 3     /* msg# if mkdir cache failed */
#define MKDIRFAILED 4     /* msg# if mkdir tempdir failed */
#define CHDIRFAILED 5     /* msg# if chdir tempdir failed */
#define FOPENFAILED 6     /* msg# if fopen(latex.tex) failed */
#define SYLTXFAILED 7     /* msg# if system(latex) failed */
#define LATEXFAILED 8     /* msg# if latex failed */
#define SYPNGFAILED 9     /* msg# if system(dvipng) failed */
#define DVIPNGFAILED 10   /* msg# if dvipng failed */
#define SYPSFAILED 11     /* msg# if system(dvips) failed */
#define DVIPSFAILED 12    /* msg# if dvips failed */
#define SYCVTFAILED 13    /* msg# if system(convert) failed */
#define CONVERTFAILED 14  /* msg# if convert failed */
#define EMITFAILED 15     /* msg# if emitcache() failed */
#define REMOVEWORKFAILED 16
static char *embeddedtext[] = {/* text of embedded image messages */
                               NULL,
                               "mathTeX test message; program running okay. :)",                                           // 1: mathTeX "okay" test
                               "mathTeX failed for some reason; probably due to bad paths, permissions, or installation.", // 2: general error message
                               "Can't mkdir cache directory; check permissions.",                                          // 3: can't create cache dir
                               "Can't mkdir tempnam/work directory; check permissions.",                                   // 4: can't create work dir
                               "Can't cd tempnam/work directory; check permissions.",                                      // 5
                               "Can't fopen(\"latex.tex\") file; check permissions.",                                      // 6
                               "Can't run latex program; check -DLATEX=\"path\", etc.",                                    // 7
                               "latex ran, but failed; check your input expression.",                                      // 8
                               "Can't run dvipng program; check -DDVIPNG=\"path\", etc.",                                  // 9
                               "dvipng ran, but failed for whatever reason.",                                              // 10
                               "Can't run dvips program; check -DDVIPS=\"path\", etc.",                                    // 11
                               "dvips ran, but failed for whatever reason.",                                               // 12
                               "Can't run convert program; check -DCONVERT=\"path\", etc.",                                // 13
                               "convert ran, but failed for whatever reason.",                                             // 14
                               "Can't emit cached image; check permissions.",                                              // 15
                               "Can't rm -r tempnam/work directory (or some content within it); check permissions.",       // 16
                               NULL};

/* ---
 * output file (from shell mode)
 * ----------------------------- */
static char outfile[256] = "\000"; /* output file, or empty for default*/

/* ---
 * temporary work directory
 * ------------------------ */
static char tempdir[256] = "\000"; /* temporary work directory */

/* ---
 * internal buffer sizes
 * --------------------- */
#if !defined(MAXEXPRSZ)
    #define MAXEXPRSZ (32767) /*max #bytes in input tex expression*/
#endif
#if !defined(MAXGIFSZ)
    #define MAXGIFSZ (131072) /* max #bytes in output GIF image */
#endif

/* ---
 * misc.
 * ----- */
#if !defined(WRITE_STDOUT)
    #define WRITE_STDOUT 0 /* print result to stdout */
#endif
static int write_stdout = WRITE_STDOUT;
#if !defined(TMP_CACHE)
    #define TMP_CACHE 0 /* whether to cache result (defaults cache to /tmp/) */
#endif
static int tmp_cache = TMP_CACHE;
#if !defined(KEEP_WORK)
    #define KEEP_WORK 0
#endif
static int keep_work = KEEP_WORK;

/* ---
 * latex wrapper document template (default, isdepth=0, without depth)
 * ------------------------------------------------------------------- */
static char latexdefaultwrapper[MAXEXPRSZ + 16384] =
    "\\documentclass[10pt]{article}\n" /*[fleqn] omitted*/
    "\\usepackage{amsmath}\n"
    "\\usepackage{amsfonts}\n"
    "\\usepackage{amssymb}\n"
//"\\usepackage{bm}\n" /* bold math */
#if defined(USEPACKAGE) /* cc -DUSEPACKAGE=\"filename\" */
    #include USEPACKAGE /* filename with \usepackage{}'s */
#endif                  /* or anything for the preamble */
    "%%usepackage%%\n"
#if 0
    "\\def\\stackboxes#1{\\vbox{\\def\\\\{\\egroup\\hbox\\bgroup}"
    "\\hbox\\bgroup#1\\egroup}}\n"
    "\\def\\fparbox#1{\\fbox{\\stackboxes{#1}}}\n"
#endif
    "%%%\\pagestyle{empty}\n"
    "%%pagestyle%%\n"
    "%%previewenviron%%\n"
    "\\begin{document}\n"
    //"\\setlength{\\mathindent}{0pt}\n" /* only with [fleqn] option */
    "\\setlength{\\parindent}{0pt}\n"
#if 0
	"%%%\\renewcommand{\\input}[1]"	/* don't let users \input{} */
	"{\\mbox{[[$\\backslash$input\\{#1\\} illegal]]}}\n"
#endif
#if defined(NEWCOMMAND) /* cc -DNEWCOMMAND=\"filename\" */
    #include NEWCOMMAND /* filename with \newcommand{}'s */
#endif                  /* or anything for the document */
    //"\\newcommand{\\amsatop}[2]{{\\genfrac{}{}{0pt}{1}{#1}{#2}}}\n"
    //"\\newcommand{\\twolines}[2]{{\\amsatop{\\mbox{#1}}{\\mbox{#2}}}}\n"
    //"\\newcommand{\\fs}{{\\eval{fs}}}\n" /* \eval{} test */
    "%%fontsize%%\n"
    "%%setlength%%\n"
    "%%beginmath%% "
    "%%expression%% \n" /* \n in case expression contains %*/
    " %%endmath%%\n"
    "\\end{document}\n";

/* ---
 * latex wrapper document template (optional, isdepth=1, with depth)
 * see http://www.mactextoolbox.sourceforge.net/articles/baseline.html for discussion of this procedure
 * ------------------------------------------------------------------- */
static char latexdepthwrapper[MAXEXPRSZ + 16384] =
    "\\documentclass[10pt]{article}\n" /*[fleqn] omitted*/
    "\\usepackage{amsmath}\n"
    "\\usepackage{amsfonts}\n"
    "\\usepackage{amssymb}\n"
    "%%%\\usepackage{calc}\n"
#if defined(USEPACKAGE) /* cc -DUSEPACKAGE=\"filename\" */
    #include USEPACKAGE /* filename with \usepackage{}'s */
#endif                  /* or anything for the preamble */
    "%%usepackage%%\n"
#if defined(NEWCOMMAND) /* cc -DNEWCOMMAND=\"filename\" */
    #include NEWCOMMAND /* filename with \newcommand{}'s */
#endif                  /* or anything for the document */
    "\\newcommand{\\amsatop}[2]{{\\genfrac{}{}{0pt}{1}{#1}{#2}}}\n"
    "\\newcommand{\\twolines}[2]{{\\amsatop{\\mbox{#1}}{\\mbox{#2}}}}\n"
    "\\newcommand{\\fs}{{\\eval{fs}}}\n" /* \eval{} test */
    "%%pagestyle%%\n"
    "%%previewenviron%%\n"
    "\\newsavebox{\\mybox}\n"
    "\n"
    "\\newlength{\\mywidth}\n"
    "\\newlength{\\myheight}\n"
    "\\newlength{\\mydepth}\n"
    "\n"
    //"\\setlength{\\mathindent}{0pt}\n" /* only with [fleqn] option */
    "\\setlength{\\parindent}{0pt}\n"
    "%%fontsize%%\n"
    "%%setlength%%\n"
    "\n"
    "\\begin{lrbox}{\\mybox}\n"
    "%%beginmath%% "
    "%%expression%% \n" /* \n in case expression contains %*/
    " %%endmath%%\n"
    "\\end{lrbox}\n"
    "\n"
    "\\settowidth {\\mywidth}  {\\usebox{\\mybox}}\n"
    "\\settoheight{\\myheight} {\\usebox{\\mybox}}\n"
    "\\settodepth {\\mydepth}  {\\usebox{\\mybox}}\n"
    "\n"
    "\\newwrite\\foo\n"
    "\\immediate\\openout\\foo=\\jobname.info\n"
    "    \\immediate\\write\\foo{depth = \\the\\mydepth}\n"
    "    \\immediate\\write\\foo{height = \\the\\myheight}\n"
    "    \\addtolength{\\myheight} {\\mydepth}\n"
    "    \\immediate\\write\\foo{totalheight = \\the\\myheight}\n"
    "    \\immediate\\write\\foo{width = \\the\\mywidth}\n"
    "\\closeout\\foo\n"
    "\n"
    "\\begin{document}\n"
    "\\usebox{\\mybox}\n"
    "\\end{document}\n";

/* ---
 * latex wrapper used
 * ------------------ */
static char *latexwrapper = (ISDEPTH ? latexdepthwrapper : latexdefaultwrapper); /* with or without depth */

/* ---
 * elements from \jobname.info to be prepended to graphics file
 * ------------------------------------------------------------ */
#define MAXIMAGEINFO 32 /* max 32 image info elements */
struct imageinfo_struct {
    char *identifier; /* identifier in \jobname.info */
    char *format;     /* format to write in graphics file*/
    double value;     /* value of identifier */
    char units[32];   /* units of value, e.g., "pt" */
    int algorithm;    /* value conversion before writing */
};                    /* --- end-of-store_struct --- */
static struct imageinfo_struct imageinfo[MAXIMAGEINFO] = {
    {"depth", "Vertical-Align:%dpx\n", -9999., "", 1}, /* below baseline */
    {NULL, NULL, -9999., "", -9999}                    /* end-of-imageinfo */
};                                                     /* --- end-of-imageinfo[] --- */

/* -------------------------------------------------------------------------
Unix or Windows header files
-------------------------------------------------------------------------- */
/* ---
 * compiling under Windows if -DWINDOWS explicitly supplied or...
 * -------------------------------------------------------------- */
#if !defined(WINDOWS)                                                            /* -DWINDOWS not explicitly given by user */
    #if defined(_WINDOWS) || defined(_WIN32) || defined(WIN32) || defined(DJGPP) /* try to recognize windows compilers */ \
        || defined(_USRDLL)                                                      /* must be WINDOWS if compiling for DLL */
        #define WINDOWS                                                          /* signal windows */
    #endif
#endif

/* ---
 * unix headers
 * ------------ */
#if !defined(WINDOWS) /* if not compiling under Windows... */
    #include <dirent.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif

/* ---
 * windows-specific header info
 * ---------------------------- */
#if defined(WINDOWS)   /* Windows opens stdout in char mode, and */
    #include <fcntl.h> /* precedes every 0x0A with spurious 0x0D.*/
    #include <io.h>    /* So emitcache() issues a Win _setmode() */
    /* call to put stdout in binary mode. */
    #if defined(_O_BINARY) && !defined(O_BINARY) /* only have _O_BINARY */
        #define O_BINARY _O_BINARY               /* make O_BINARY available, etc... */
        #define setmode _setmode
        #define fileno _fileno
    #endif
    #if defined(_O_BINARY) || defined(O_BINARY) /* setmode() now available */
        #define HAVE_SETMODE                    /* so we'll use setmode() */
    #endif
#endif
static int iswindows = /* 1 if running under Windows, or 0 if not */
#ifdef WINDOWS
    1; /* 1 when program running under Windows */
#else
    0; /* 0 when not running under Windows */
#endif

/* -------------------------------------------------------------------------
application macros
-------------------------------------------------------------------------- */
/* --- check if a string is empty --- */
#define isempty(s) ((s) == NULL ? 1 : (*(s) == '\000' ? 1 : 0))

/* --- last char of a string --- */
#define plastchar(s) (isempty(s) ? NULL : ((s) + (strlen(s) - 1)))
#define lastchar(s) (isempty(s) ? '\000' : *(plastchar(s)))

/* --- check for thischar in accept --- */
#define isthischar(thischar, accept) ((thischar) != '\000' && !isempty(accept) && strchr((accept), (thischar)) != (char *)NULL)

/* --- skip/find whitespace --- */
#define WHITESPACE " \t\n\r\f\v" /* skipped whitespace chars */
#define skipwhite(thisstr) \
    if ((thisstr) != NULL) thisstr += strspn(thisstr, WHITESPACE)
#define findwhite(thisstr)                        \
    while (!isempty(thisstr)) {                   \
        if (isthischar(*(thisstr), WHITESPACE)) { \
            break;                                \
        } else {                                  \
            (thisstr)++;                          \
        }                                         \
    }
// thisstr += strcspn(thisstr, WHITESPACE);

/* --- skip \command (i.e., find char past last char of \command) --- */
#define skipcommand(thisstr)             \
    while (!isempty(thisstr))            \
        if (!isalpha(*(thisstr))) break; \
        else                             \
            (thisstr)++

/* --- strncpy() n bytes and make sure it's null-terminated --- */
#define strninit(target, source, n)             \
    if ((target) != NULL && (n) >= 0) {         \
        char *thissource = (source);            \
        (target)[0] = '\000';                   \
        if ((n) > 0 && thissource != NULL) {    \
            strncpy((target), thissource, (n)); \
            (target)[(n)] = '\000';             \
        }                                       \
    }

/* --- strip leading and trailing whitespace --- */
#define trimwhite(thisstr)                                                                     \
    if ((thisstr) != NULL) {                                                                   \
        int thislen = strlen(thisstr);                                                         \
        while (--thislen >= 0) {                                                               \
            if (isthischar((thisstr)[thislen], WHITESPACE)) {                                  \
                (thisstr)[thislen] = '\000';                                                   \
            } else {                                                                           \
                break;                                                                         \
            }                                                                                  \
        }                                                                                      \
        if ((thislen = strspn((thisstr), WHITESPACE)) > 0) { strsqueeze((thisstr), thislen); } \
    } else /* user supplies ';' */

/* --- strcpy(s, s + n) using memmove() (also works for negative n) --- */
#define strsqueeze(s, n)                                     \
    if ((n) != 0) {                                          \
        if (!isempty((s))) {                                 \
            int thislen3 = strlen(s);                        \
            if ((n) >= thislen3) {                           \
                *(s) = '\000';                               \
            } else {                                         \
                memmove((s), (s) + (n), 1 + thislen3 - (n)); \
            }                                                \
        }                                                    \
    } else

/* --- strsqueeze(s, t) with two pointers --- */
#define strsqueezep(s, t)                                          \
    if (!isempty((s)) && !isempty((t))) {                          \
        int sqlen = strlen((s)) - strlen((t));                     \
        if (sqlen > 0 && sqlen <= 999) { strsqueeze((s), sqlen); } \
    } else

/* --- min and max of args --- */
#define max2(x, y) ((x) > (y) ? (x) : (y)) /* larger of 2 arguments */
#define min2(x, y) ((x) < (y) ? (x) : (y)) /* smaller of 2 arguments */

/* -------------------------------------------------------------------------
other application global data
-------------------------------------------------------------------------- */
/* --- getdirective() global data --- */
static int argformat = 0;           /* 111... if arg not {}-enclosed */
static int optionalpos = 0;         /* # {args} before optional [args] */
static int noptional = 0;           /* # optional [args] found */
static char optionalargs[8][512] = {/* buffer for optional args */
                                    "\000", "\000", "\000", "\000", "\000", "\000", "\000", "\000"};

/* -------------------------------------------------------------------------
store for evalterm() [n.b., these are stripped-down funcs from nutshell]
-------------------------------------------------------------------------- */
#define MAXSTORE 100 /* max 100 identifiers */
struct store_struct {
    char *identifier; /* identifier */
    int *value;       /* address of corresponding value */
};
static struct store_struct mathtexstore[MAXSTORE] = {
    {"fontsize", &fontsize},
    {"fs", &fontsize}, /* font size */
    //{ "mytestvar", &mytestvar },
    {NULL, NULL} /* end-of-store */
};
