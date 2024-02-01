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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for getopts
#define _GNU_SOURCE
#include <string.h>
char *strcasestr(); /* non-standard extension */
#include <ctype.h>
#include <time.h>
extern char **environ; /* for \environment directive */

#include <regex.h>

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
static int fontsize = FONTSIZE; /* 0 = tiny ... 9 = Huge */
static char *sizedirectives[] = {"\\tiny",  "\\scriptsize", "\\footnotesize", "\\small", "\\normalsize", "\\large", "\\Large",
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

/**
 * preamble detection
 */
static char dclassargs[2][256];
static char *dclassoptions = "10pt"; // directive options (optional, no pun intended)
static char *dclass = "article";     // directive class

/**
 * error detection
 */
#define LATEXERRORCOUNT 5     // the amount of errors to detect.
#define LATEXSTACKLENGTH 1024 // temp until something better comes along
static char **errors;
static int errorcount;

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

static char *about = "mathTeX v" VERSION ", Copyright(c) " COPYRIGHTDATE
                     ", John Forkosh Associates, Inc                                           \n"
                     "Modified by mechabubba @ https://github.com/mechabubba/mathtex.          \n";
static char *usage =
    "\n"
    "Usage: mathtex [options] [expression]                                    \n"
    "\n"
    "  -c [cache]         the image cache folder. defaults to `./cache/`.     \n"
    "                     set this to \"none\" to deliberately disable caching\n"
    "                     of the rendered image                               \n"
    "  -d [dpi]           set dpi of render (default: 120)                    \n"
    "  -f [input_file]    file to read latex expression in from               \n"
    "  -h                 prints this                                         \n"
    "  -m [log_verbosity] verbosity (\"message level\") of logs               \n"
    "  -o [output_file]   file to write output image from                     \n"
    "  -s                 writes output image to stdout (use `-m 0`!)         \n"
    "  -t                 overrides cache to store images in /tmp/mathtex     \n"
    "                     (shorthand for `-c /tmp/mathtex`)                   \n"
    "  -w                 keeps work directory. exists for debug reasons      \n"
    "\n"
    "Example: `mathtex -o equation1 \"f(x,y)=x^2+y^2\"`                       \n";
static char *license =
    "\n"
    "This program is free software licensed under the terms of the GNU General\n"
    "Public License, and comes with absolutely no warranty whatsoever. Please \n"
    "see https://github.com/mechabubba/mathtex/blob/master/COPYING for        \n"
    "complete details.\n";

/** Logs data of a level to a file pointer. */
#define log(lvl, fp, ...)         \
    if (msglevel >= (lvl)) {      \
        fprintf(fp, __VA_ARGS__); \
        fflush(fp);               \
    }
#define log_info(lvl, ...) log(lvl, stdout, __VA_ARGS__) /** Logs informational stuff to stdout. */
#define log_error(...) log(0, stderr, __VA_ARGS__)       /** Logs errors to stderr. */

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

/** Embedded messages for errors. */
static char *embeddedtext[] = {NULL,
                               "mathTeX test message; program running okay. :)\n",                                           // 1: mathTeX "okay" test
                               "mathTeX failed for some reason; probably due to bad paths, permissions, or installation.\n", // 2: general error message
                               "Can't mkdir cache directory; check permissions.\n",                                          // 3: can't create cache dir
                               "Can't mkdir tempnam/work directory; check permissions.\n",                                   // 4: can't create work dir
                               "Can't cd tempnam/work directory; check permissions.\n",                                      // 5
                               "Can't fopen(\"latex.tex\") file; check permissions.\n",                                      // 6
                               "Can't run latex program; check -DLATEX=\"path\", etc.\n",                                    // 7
                               "latex ran, but failed; check your input expression.\n",                                      // 8
                               "Can't run dvipng program; check -DDVIPNG=\"path\", etc.\n",                                  // 9
                               "dvipng ran, but failed for whatever reason.\n",                                              // 10
                               "Can't run dvips program; check -DDVIPS=\"path\", etc.\n",                                    // 11
                               "dvips ran, but failed for whatever reason.\n",                                               // 12
                               "Can't run convert program; check -DCONVERT=\"path\", etc.\n",                                // 13
                               "convert ran, but failed for whatever reason.\n",                                             // 14
                               "Can't emit cached image; check permissions.\n",                                              // 15
                               "Can't rm -r tempnam/work directory (or some content within it); check permissions.\n",       // 16
                               NULL};

static char outfile[256] = "\000"; /* output file, or empty for default*/
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
 * latex wrapper document template (default, isdepth=0, without depth)
 * ------------------------------------------------------------------- */
static char latexdefaultwrapper[MAXEXPRSZ + 16384] =
    "\\documentclass[%%dclassoptions%%]{%%dclass%%}\n" /*[fleqn] omitted*/
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
    //"%%%\\pagestyle{empty}\n"
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
    "%%beginmath%%\n"
    "%%expression%%\n" /* \n in case expression contains %*/
    "%%endmath%%\n"
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
    "%%beginmath%%\n" 
    "%%expression%%\n"
    "%%endmath%%\n"
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

/** Checks if a string is empty. */
#define isempty(s) ((s) == NULL ? 1 : (*(s) == '\000' ? 1 : 0))

/** Gets the last character of a string. */
#define plastchar(s) (isempty(s) ? NULL : ((s) + (strlen(s) - 1)))
#define lastchar(s) (isempty(s) ? '\000' : *(plastchar(s)))

/** Checks for `thischar` in `accept`. */
#define isthischar(thischar, accept) ((thischar) != '\000' && !isempty(accept) && strchr((accept), (thischar)) != (char *)NULL)

#define WHITESPACE " \t\n\r\f\v" /** Default skipped whitespace chars. */

/** Skips whitespace. */
#define skipwhite(thisstr) \
    if ((thisstr) != NULL) thisstr += strspn(thisstr, WHITESPACE)

/** Finds whitespace. */
#define findwhite(thisstr)                        \
    while (!isempty(thisstr)) {                   \
        if (isthischar(*(thisstr), WHITESPACE)) { \
            break;                                \
        } else {                                  \
            (thisstr)++;                          \
        }                                         \
    }
// thisstr += strcspn(thisstr, WHITESPACE);

/** Skips \\command's (eg. find char past last char of \\command) */
#define skipcommand(thisstr)             \
    while (!isempty(thisstr))            \
        if (!isalpha(*(thisstr))) break; \
        else                             \
            (thisstr)++

/** strncpy()'s n bytes and makes sure it's null terminated. */
#define strninit(target, source, n)             \
    if ((target) != NULL && (n) >= 0) {         \
        char *thissource = (source);            \
        (target)[0] = '\000';                   \
        if ((n) > 0 && thissource != NULL) {    \
            strncpy((target), thissource, (n)); \
            (target)[(n)] = '\000';             \
        }                                       \
    }

/** Strips leading and trailing whitespace. */
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

/** strcpy(s, s + n) using memmove() (also works for negative n). */
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

/** strsqueeze(s, t) with two pointers. */
#define strsqueezep(s, t)                                          \
    if (!isempty((s)) && !isempty((t))) {                          \
        int sqlen = strlen((s)) - strlen((t));                     \
        if (sqlen > 0 && sqlen <= 999) { strsqueeze((s), sqlen); } \
    } else

#define max2(x, y) ((x) > (y) ? (x) : (y)) /** Max of 2 args. */
#define min2(x, y) ((x) < (y) ? (x) : (y)) /** Min of 2 args. */

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

// Function prototypes.
/**
 * Driver for mathtex.c. Emits, usually to stdout, a .png or .gif image of a LaTeX math expression entered;
 *   - ...from a command-line argument
 *   - ...from a file (via the `-f` argument)
 *
 * @param argc[in] The number of string arguments.
 * @param argv[in] A char* list of arguments.
 */
int main(int argc, char *argv[]);

/**
 * Creates the image of the latex math expression.
 *
 * @param expression[in] The expression to evaluate.
 * @param filename[in] What to name the file. This is typically set to the md5 of the expression.
 * @return imagetype if successful, 0 if an error occured.
 */
int mathtex(char *expression, char *filename);

/**
 * Tries to set accurate paths for latex, pdflatex, timelimit, dvipng, dvips, and convert.
 * @todo What the fuck does this function do???
 *
 * @param method[in] 10 * ltxmethod + imgmethod, where;
 *   - ltxmethod = 1 for latex, 2 for pdflatex, 0 for both
 *   - imgmethod = 1 for dvipng, 2 for dvips/convert, 0 for both
 * @return 1
 */
int setpaths(int method);

int checkerrors(char *filename, char **errors);

/**
 * Checks a "... 2>filename.err" file for a "not found" message, indicating the corresponding program path is incorrect.
 *
 * @todo This is unnecessary when the exit code returns 127 for this case.
 * @param filename[in] Pointer to string containing filename (an .err extension is tacked on, just pass the filename) to be checked for "not found" message.
 * @return 1 if filename.err contains "not found", 0 otherwise, or if an error occured.
 */
// int isnotfound(char *filename);

/**
 * Removes/replaces illegal \\commands from expression.
 *
 * Implementation notes;
 * Because some \\renewcommand's appear to occasionally cause latex to go into a loop after encountering syntax errors, I'm now using validate() to disable
 * \\input, etc.
 *
 * For example, the following short document...
 * \code{.tex}
 * \documentclass[10pt]{article}
 * \begin{document}
 * \renewcommand{\input}[1]{dummy renewcommand}
 * %%\renewcommand{\sqrt}[1]{dummy renewcommand}
 * %%\renewcommand{\beta}{dummy renewcommand}
 * \[ \left( \begin{array}{cccc} 1 & 0 & 0 &0
 * \\ 0 & 1  %%% \end{array}\right)
 * \]
 * \end{document}
 * \endcode
 * ...reports a  "! LaTeX Error:"  and then goes into a loop if you reply q.  But if you comment out the \renewcommand{\input} and uncomment either or both of
 * the other \renewcommand's, then latex runs to completion (with the same syntax error, of course, but without hanging).
 *
 * @param expression[in,out] Null-terminated char* containing expression to validate.
 * @return Number of illegal \\commands found (hopefully 0).
 */
int validate(char *expression);

/**
 * Returns a string containing `path/name.extension`
 *
 * @param path[in] Null-terminated char* containing empty string, path, or `NULL` to use cachepath if caching enabled.
 * @param name[in] Null-terminated char* containing filename without extension.
 * @param extension[in] Null-terminated char* containing extension or `NULL`.
 * @return A string containing `path/name.extension`, or `NULL` if an error occured.
 */
char *makepath(char *path, char *name, char *extension);

/**
 * Checks whether or not filename exists.
 *
 * @param filename[in] Null-terminated char* containing filename to check for.
 * @return 1 if it exists, 0 if not.
 */
int isfexists(char *filename);

/**
 * Checks whether or not a directory exists.
 *
 * @param dirname[in] Null-terminated char* containing dirname to check for.
 * @return 1 if it exists, 0 if not.
 */
int isdexists(char *dirname);

/**
 * Determines the path to a program with `popen("which 'program'")`.
 *
 * @param program[in] Null-terminated char* containing program whose path is desired.
 * @param nlocate[in,out] Address of int containing `NULL` to ignore, or (addr of int containing) 0 to *not* use locate if which fails. If non-zero, use locate
 * if which fails, and return number of locate lines (if locate succeeds).
 * @return path to program, or NULL if an error occured.
 */
char *whichpath(char *program, int *nlocate);

/**
 * Determines the path to a program with `popen("locate 'program'")`.
 *
 * @param program[in] Null-terminated char* containing program whose path is desired.
 * @param nlocate[out] Address to int returning the number of lines locate or grep found, or `NULL` to ignore.
 * @return path to program, or NULL if an error occured.
 */
char *locatepath(char *program, int *nlocate);

/**
 * Determines pwd, modified by nsub directories "up" from it.
 *
 * Implementation notes;
 *   - A forward slash is always the last character.
 *
 * @param nsub[in] int containing the number of subdirectories "up" to which path is wanted, 0 for pwd.
 * @return Path to pwd, or `NULL` if an error occured.
 */
char *presentwd(int nsub);

/**
 * `rm -r path`
 *
 * Implementation notes;
 *   - Based on Program 4.7, pages 108-111, in Advanced Programming in the UNIX Environment, W. Richard Stevens, Addison-Wesley 1992, ISBN 0-201-56317-7.
 *
 * @param path[in] Null-terminated char* containing path to be removed, relative to cwd.
 * @return 0 on success, -1 on error.
 */
int rrmdir(char *path);

/**
 * Reads a file into *buffer.
 *
 * @param cachefile[in] Null-terminated char* containing full path to file to be read.
 * @param buffer[out] Unsigned char* to store contents of file in.
 * @return Number of bytes read, 0 if an error occured.
 */
int readcachefile(char *cachefile, unsigned char *buffer);

/**
 * 16-bit CRC of string s.
 *
 * @param s[in] The string to CRC.
 * @return 16-bit CRC of s.
 */
int crc16(char *s);

/**
 * Returns null-terminated char* containing MD5 hash of input string.
 *
 * MD5 library from Christophe Devine, at the time available from http://www.cr0.net:8040/code/crypto/md5/.
 *
 * @param instr[in] The string to calculate the MD5 of.
 * @return Null-terminated 32-character MD5 hash of input string.
 */
char *md5str(char *instr);

/**
 * Replaces 3-character encoded URI entities (%xx) in URL with the single character represented in hex.
 *
 * @param url[in] Null-terminated char* with embedded %xx sequences to be converted.
 * @return Length of url string after replacements.
 */
int unescape_url(char *url);

/**
 * Helper function of `unescape_url`; returns the single character represented by a byte encoded hexadecimally passed as a 2-character sequence in what.
 *
 * Implementation notes;
 *   - These two functions were taken from util.c in ftp://ftp.ncsa.uiuc.edu/Web/httpd/Unix/ncsa_httpd/cgi/ncsa-default.tar.Z
 *   - Added ^M, ^F, etc. and +'s to blank translation on 01-Oct-06
 *
 * @param what[in] char* whose first 2 characters are interpreted as ASCII representations of a byte encoded hexadecimally.
 * @return The single char corresponding to the characters passed in what.
 */
char x2c(char *what);

/**
 * Issues a system(command) call, but throttles command after killtime seconds if it hasn't already completed.
 *
 * Implementation notes;
 *   - The timelimit() code is adapted from http://devel.ringlet.net/sysutils/timelimit/. Compile with `-DTIMELIMIT=\"$(which timelimit)\"` to use an installed
 * copy of timelimit rather than this built-in code.
 *   - If symbol `ISCOMPILETIMELIMIT` is false, a stub function that just issues system(command) is compiled instead.
 *
 * @param command[in] Null-terminated char* containing command to be executed.
 * @param killtime[in] int containing maximum seconds to allow command to run.
 * @return Return status from command, or -1 for any error.
 */
int timelimit(char *command, int killtime);

/** Built-in limit functionality below. */
#if ISCOMPILETIMELIMIT
/** Signal handlers. */
void sigchld(int sig);
void sigalrm(int sig);
void sighandler(int sig);

int setsignal(int sig, void (*handler)(int));
#endif

/**
 * Locates the first \\directive{arg1}{...}{argN} in string, returns arg1, ..., argN in args[], and removes \\directive and its args from string.
 *
 * Implementation notes;
 *   - If optional [arg]'s are found, they're stored in the global optionalargs[] buffer, and the noptional counter is bumped.
 *   - Set global argformat's decimal digits for each arg, e.g., 1357... means 1 for 1st arg, 3 for 2nd, 5 for 3rd, etc.
 *     - 0 for an arg is the default format (i.e., argformat = 0), and means it's formatted as a LaTeX {arg} or [arg].
 *     - 1 for an arg means arg terminated by first non-alpha char.
 *     - 2 means arg terminated by '{' (e.g., as for /def).
 *     - 8 means arg terminated by first whitespace char.
 *
 * @param string[in,out] Null-terminated char* from which the first occurrence of \\directive will be interpreted and removed.
 * @param directive[in] Null-terminated char* containing the \\directive to be interpreted in string.
 * @param iscase[in] int containing 1 if match of \\directive in string should be case-sensitive, or 0 if match is case-insensitive.
 * @param isvalid[in] int containing validity check option; 0 = no checks, 1 = must be numeric.
 * @param nargs[in] int containing (maximum) number of {args} following \directive, or 0 if none.
 * @param args[out] void* interpreted as char* if nargs = 1 to return the one and only arg, or interpreted as char** if nargs > 1 to array of returned arg
 * strings.
 * @return Pointer to the first char after removed \\directive, or `NULL` if \\directive not found or any error.
 */
char *getdirective(char *string, char *directive, int iscase, int isvalid, int nargs, void *args);

/**
 * Preprocessor for mathTeX input. This function;
 *   1. Removes leading/trailing $'s from $$expression$$
 *   2. Translates HTML entities (&html and &#nnn) and other special chars to their equivalent LaTeX counterparts.
 * Should only be called once (after `unescape_url()`).
 *
 * Implementation notes;
 * - The ten special symbols ($, &, %, #, _, {, }, ~, ^, \\)  are reserved for use in LaTeX commands. The corresponding directives (\$, \&, \%, \#, \_, \{, \})
 * display the first seven, respectively, and \backslash displays \. It's not clear to me whether or not mathprep() should substitute the displayed symbols,
 * e.g., whether &#36; better translates to \$ or to $. Right now, it's the latter.
 *
 * @param expression[in,out] Null-terminated char* containing mathTeX/LaTeX expression to be processed.
 * @return char* to input expression, or `NULL` for any parsing error.
 */
char *mathprep(char *expression);

/**
 * Find first substr in string, but wherever substr contains a whitespace char (in white), string may contain any number (including 0) of whitespace chars. If
 * white contains I or i, then match is case-insensitive (and I, i *not* considered whitespace).
 *
 * Implementation notes;
 *   - Wherever a single whitespace char appears in substr, the corresponding position in string may contain any number (including 0) of whitespace chars, e.g.,
 * string="abc   def" and string="abcdef" both match substr="c d" at offset 2 of string.
 *   - If substr="c  d" (two spaces between c and d), then string must have at least one space, so now "abcdef" doesn't match. In general, the minimum number of
 * spaces in string is the number of spaces in substr minus 1 (so 1 space in substr permits 0 spaces in string).
 *   - Embedded spaces are counted in sublen, e.g., string="c   d" (three spaces) matches substr="c d" with sublen=5 returned. But string="ab   c   d" will also
 * match substr="  c d" returning sublen=5 and a ptr to the "c". That is, the mandatory preceding space is *not* counted as part of the match. But all the
 * embedded space is counted. (An inconsistent bug/feature is that mandatory terminating space is counted.)
 *   - Moreover, string="c   d" matches substr="  c d", i.e., the very beginning of a string is assumed to be preceded by "virtual blanks".
 *
 * @param string[in] Null-terminated char* in which first occurrence of substr will be found.
 * @param substr[in] Null-terminated char* containing substring to be searched for.
 * @param white[in] Null-terminated char* containing whitespace chars. If `NULL` or empty, then by default is set to " \t\n\r\f\v" (see #define WHITESPACE). If
 * white contains I or i, then match is case-insensitive (and I, i *not* considered whitespace).
 * @param sublen[out] int* representing "length" of substr found in string (which may be longer or shorter than substr itself).
 * @return char* to first char of substr in string, or `NULL` if not found or for any error.
 */
char *strwstr(char *string, char *substr, char *white, int *sublen);

/**
 * Changes the first nreplace occurrences of 'from' to 'to' in string, or all occurrences if nreplace = 0.
 *
 * @param string[in,out] Null-terminated char* in which occurrence of 'from' will be replaced by 'to'.
 * @param from[in] Null-terminated char* to be replaced by 'to'.
 * @param to[in] Null-terminated char* that will replace 'from'.
 * @param iscase[in] int containing 1 if matches of 'from' in 'string' should be case-sensitive, or 0 if matches are case-insensitive.
 * @param nreplace[in] int containing (maximum) number of replacements, or 0 to replace all.
 * @return number of replacements performed, or 0 for no replacements, or -1 for any error.
 */
int strreplace(char *string, char *from, char *to, int iscase, int nreplace);

/**
 * Changes the nfirst leading chars of 'from' to 'to'. For example, to change `char x[99] = "12345678"` to `"123ABC5678"`, call `strchange(1, x + 3, "ABC")`.
 *
 * Implementation notes;
 * - If strlen(to) > nfirst, `from` must have memory past its `NULL` (i.e., we don't do a realloc).
 *
 * @param nfirst[in] int containing the number of leading chars of 'from' that will be replace by 'to'.
 * @param from[in,out] Null-terminated char* whose nfirst leading chars will be replaced by 'to'.
 * @param to[in] Null-terminated char* that will replace the nfirst leading chars of 'from'.
 * @return char* to first char of input 'from', or `NULL` for any error.
 */
char *strchange(int nfirst, char *from, char *to);

/**
 * Determine whether any substring of 'string' matches any of the comma-separated list of 'snippets', ignoring case if iscase = 0.
 *
 * @param string[in] Null-terminated char* that will be searched for any one of the specified snippets.
 * @param snippets[in] Null-terminated char* containing comma-separated list of snippets.
 * @param iscase[in] int containing 0 for case-insensitive comparisons, or 1 for case-sensitive.
 * @return 1 if any snippet is a substring of string, 0 if not.
 */
int isstrstr(char *string, char *snippets, int iscase);

/**
 * Removes/replaces any LaTeX math chars in s so that s can be rendered in paragraph mode.
 *
 * @param s[in] Null-terminated char* whose math chars are to be removed/replaced.
 * @return The string with any math characters replaced. The returned pointer addresses a static buffer, so don't call nomath() again until you're finished with
 * output from the preceding call.
 */
char *nomath(char *s);

/**
 * Inserts \\n's and spaces in (a copy of) s to wrap lines at linelen and indent them by tablen.
 *
 * Implementation notes;
 * - The returned copy of s has embedded \n's as necessary to wrap lines at linelen.  Any \n's in the input copy are removed first.  If (and only if) the input
 * s contains a terminating \\n then so does the returned copy.
 *
 * @param s[in] Null-terminated char* to be wrapped.
 * @param linelen[in] int containing maximum linelen between \\n's.
 * @param tablen[in] int containing number of spaces to indent lines. 0 = no indent. Positive means only indent first line and not others. Negative means indent
 * all lines except first.
 * @return The returned copy of s has embedded \\n's as necessary to wrap lines at linelen. Any \\n's in the input copy are removed first. If (and only if) the
 * input s contains a terminating \\n then so does the returned copy. The returned pointer addresses a static buffer, so don't call strwrap() again until you're
 * finished with output from the preceding call.
 */
char *strwrap(char *s, int linelen, int tablen);

/**
 * Finds the initial segment of s containing no chars in reject that are outside (), [] and {} parens. For example `strpspn("abc(---)def+++", "+-", segment)`
 * returns `segment = "abc(---)def"` and a pointer to the first '+' in s, because the '-'s are enclosed in () parenthesis.
 *
 * Implementation notes;
 *   - The return value is _not_ like strcspn()'s.
 *   - Improperly nested strings, i.e. "(...[...)...]", are not detected, but are considered "balanced" after the ']'.
 *   - If reject not found, segment returns the entire string s.
 *   - ...but if reject is empty, returns segment up to and including matching ')]}'.
 *   - Leading/trailing whitespace is trimmed from returned segment.
 *
 * @param s[in] Null-terminated char* whose initial segment is desired.
 * @param reject[in] Null-terminated char* containing the "reject chars". If reject contains a " or a ', then the " or ' isn't itself a reject char, but other
 * reject chars within quoted strings (or substrings of s) are spanned.
 * @param segment[out] Null-terminated char* comprising the initial segment of s that contains non-rejected chars outside relevant parenthesis ((), [], {});
 * i.e., all the chars up to but not including the returned pointer. (That's the entire string if no non-rejected chars are found.)
 * @return Pointer to first reject-char found in s outside parenthesis, or a pointer to the terminating '\000' of s if there are no reject chars in s outside
 * all () parenthesis. But if reject is empty, returns pointer to matching ')]}' outside all parens.
 */
char *strpspn(char *s, char *reject, char *segment);

/**
 * Finds matching/closing " or ' in quoted string that begins with " or ', and optionally changes escaped quotes to unescaped quotes.
 *
 * Implementation notes;
 *   - Note: the following not implemented yet -- If unescape is negative, its abs() is used, but outer quotes aren't included in q.
 *
 * @param s[in] Null-terminated char* that begins with " or '.
 * @param q[out] Null-terminated char* containing quoted token, with or without outer quotes, and with or without escaped inner quotes changed to unescaped
 * quotes, depending on isunescape.
 * @param isunescape[in] int containing 1 to change \" to " if s is "quoted" or change \' to ' if 'quoted', or containing 2 to change both \" and \' to
 * unescaped quotes. Other \sequences aren't changed. Note that \\" still emits \". isunescape = 0 makes no changes at all.
 * @return pointer to matching/closing " or ' (or to char after quote if isunescape < 0), or terminating '\000' if none found, or unchanged (same as s) if not
 * quoted string.
 */
char *strqspn(char *s, char *q, int isunescape);

/**
 * Determines if s is an integer.
 *
 * @param s[in] Null-terminated char* that's checked for a leading + or -, followed by digits.
 * @return 1 if s is numeric, 0 if it is not.
 */
int isnumeric(char *s);

/**
 * Evaluates a term.
 *
 * @param store[in,out] struct store_struct* containing environment in which term is to be evaluated.
 * @param term[in] Null-terminated char* with a term like "3" or "a" or "a+3" whose value is to be determined.
 * @return value of term, or NOVALUE for any error.
 */
int evalterm(struct store_struct *store, char *term);

/**
 * Finds identifier in store and returns corresponding value.
 *
 * @param store[in] struct store_struct* to store containing the desired identifier.
 * @param identifier[in] Null-terminated char* containing the identifier whose value is to be returned.
 * @return identifier's corresponding value, or 0 if identifier not found or if an error occured.
 */
int getstore(struct store_struct *store, char *identifier);
