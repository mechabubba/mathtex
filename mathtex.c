/******************************************************************************
 * mathTeX v2.0, Copyright(c) 2007-2023, John Forkosh Associates, Inc
 * Modified by mechabubba @ https://github.com/mechabubba/mathtex.
 *
 * MathTeX, licensed under the GPL, is a standalone binary that lets you easily
 * generate images from LaTeX. It takes in a LaTeX math expression and
 * immediately generates the corresponding .png image, rather than the usual
 * TeX dvi. This is cached for future use to avoid regeneration.
 *
 * For example, running `mathtex -st "\int_{-\infty}^xe^{-t^2}dt"` will
 * generate the resulting image, print the data to stdout, and cache the image
 * by md5 in the /tmp/mathtex/ directory.
 *
 * =[ BUILDING ]===============================================================
 * To compile mathTeX, run;
 * ```sh
 * cc mathtex.c -DLATEX=\"$(which latex)\" \
 *     -DDVIPNG=\"$(which dvipng)\" \
 *     -DDVIPS=\"$(which dvips)\" \
 *     -DCONVERT=\"$(which convert)\" \
 *     -o mathtex
 * ```
 *
 * Some program parameters adjustable by optional -D switches on mathTeX's
 * compile line are illustrated with default values...
 *     -DLATEX=\"/usr/share/texmf/bin/latex\"       path to LaTeX program
 *     -DPDFLATEX=\"/usr/share/texmf/bin/pdflatex\" path to pdflatex
 *     -DDVIPNG=\"/usr/share/texmf/bin/dvipng\"     path to dvipng
 *     -DDVIPS=\"/usr/share/texmf/bin/dvips\"       path to dvips
 *     -DPS2EPSI=\"/usr/bin/ps2epsi\"               path to ps2epsi
 *     -DCONVERT=\"/usr/bin/convert\"               path to convert
 *     -DCACHE=\"mathtex/\"                         relative path to mathTeX's cache dir
 *     -DTIMELIMIT=\"/usr/local/bin/timelimit\"     path to timelimit
 *     -WARNTIME=10                                 #secs latex can run using standalone timelimit
 *     -KILLTIME=10                                 #secs latex can run using built-in timelimit()
 *     -DGIF                                        emit gif images
 *     -DPNG                                        emit png images (default)
 *     -DDISPLAYSTYLE                               [ \displaystyle ]
 *     -DTEXTSTYLE                                  $ \textstyle $
 *     -DPARSTYLE                                   paragraph mode, supply your own "$ $" or "[ ]"
 *     -DFONTSIZE=5                                 0 = \tiny, ... 4 = \normalsize, ... 9 = \Huge
 *     -DUSEPACKAGE=\"filename\"                    file containing \usepackage's
 *     -DNEWCOMMAND=\"filename\"                    file containing \newcommand's
 *     -DDPI=\"120\"                                dvipng -D DPI  parameter (as "string")
 *     -DGAMMA=\"2.5\"                              dvipng --gamma GAMMA  param (as "string")
 *     -DNOQUIET                                    -halt-on-error (default reply q(uiet) to error)
 *     -DMAXINVALID=0                               max length expression from invalid referer
 * See mathtex.h for more information.
 *
 * Notes;
 * - MathTeX runs only under Unix-like operating systems.
 * - The timelimit() code is adapted from
 *   http://devel.ringlet.net/sysutils/timelimit/. Compile with `-DTIMELIMIT`
 *   to use an installed copy of that program rather than the built-in code.
 *
 * =[ REVISION HISTORY ]=======================================================
 * 10/11/07 J.Forkosh   Installation Version 1.00.
 * 02/17/08 J.Forkosh   Version 1.01
 * 03/06/09 J.Forkosh   Version 1.02
 * 08/14/09 J.Forkosh   Version 1.03
 * 04/06/11 J.Forkosh   Version 1.04
 * 11/15/11 J.Forkosh   Version 1.05
 * 10/26/14 J.Forkosh   Latest original code.
 * 10/18/23 mechabubba  Modified for new features
 *
 * =[ LICENSE ]================================================================
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

#include "mathtex.h"

/* ==========================================================================
 * Function:	main ( argc, argv )
 * Purpose:	driver for mathtex.c
 *		emits, usually to stdout, a gif or png image
 *		of a LaTeX math expression entered either as
 *		    (1)	html query string from a browser (most typical), or
 *		    (2)	a query string from an html <form method="get">
 *			whose <textarea name=TEXTAREANAME>
 *			(usually for demo), or
 *		    (3)	command-line arguments (usually just to test).
 *		If no input supplied, expression defaults to "f(x)=x^2",
 *		treated as test (input method 3).
 * --------------------------------------------------------------------------
 * Command-Line Arguments:
 *		When running mathTeX from the command-line, rather than
 *		from a browser (usually just for testing), syntax is
 *		     ./mathtex	[-m msglevel]	verbosity of debugging output
 *				[-c cachepath ]	name of cache directory
 *				[expression	expression, e.g., x^2+y^2,
 *		-m   0-99, controls verbosity level for debugging output
 *		     >=9 retains all directories and files created
 *		-c   cachepath specifies relative path to cache directory
 *    -n   override cache to save to /tmp/mathtex/
 *    -s   write result to stdout
 * --------------------------------------------------------------------------
 * Exits:	0=success (always exits 0, regardless of success/failure)
 * --------------------------------------------------------------------------
 * Notes:     o For an executable that emits gif images
 *		     cc mathtex.c -o mathtex.cgi
 *		or, alternatively, for an executable that emits png images
 *		     cc -DPNG mathtex.c -o mathtex.cgi
 *		See Notes at top of file for other compile-line -D options.
 *	      o	Move executable to your cgi-bin directory and either
 *		point your browser to it directly in the form
 *		     http://www.yourdomain.com/cgi-bin/mathtex.cgi?f(x)=x^2
 *		or put a tag in your html document of the form
 *		     <img src="/cgi-bin/mathtex.cgi?f(x)=x^2"
 *		       border=0 align=middle>
 *		where f(x)=x^2 (or any other expression) will be displayed
 *		either as a gif or png image (as per -DIMAGETYPE flag).
 * ======================================================================= */
int main(int argc, char *argv[]) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    /* --- expression to be emitted --- */
    static char exprbuffer[MAXEXPRSZ + 1] = "\000"; /* input TeX expression */
    char hashexpr[MAXEXPRSZ + 1] = "\000";          /* usually use md5 of original expr*/
    char *expression = exprbuffer;                  /* ptr to expression */
    int isquery = 0;                                /* true if input from QUERY_STRING */

    /* --- preprocess expression for special mathTeX directives, etc --- */
    char *mathprep();   /* preprocess expression */
    int unescape_url(); /* convert %20 to blank space, etc */
    int strreplace();   /* look for keywords in expression */
    int irep = 0;
    char *strchange();    /* edit expression */
    char *getdirective(); /*look for \density,\usepackage,etc*/
    char argstring[256];
    char *pdirective = NULL; /* ptr to char after \directive */
    int validate();          /* remove \input, etc */
    int crc16();
    int evalterm(); /* preprocess \eval{expression} */
    char *nomath(); /* remove math chars from string */
    int isnumeric();

    /* --- other initialization variables --- */
    char *whichpath();                            /* look for latex,dvipng,etc */
    int setpaths();                               /* set paths to latex,dvipng,etc */
    int isstrstr();                               /* find any snippet in string */
    int isdexists();                              /* check whether cache dir exists */
    int perm_all = (S_IRWXU | S_IRWXG | S_IRWXO); /* 777 permissions */
    int readcachefile(), nbytes = 0;              /*read expr for -f command-line arg*/
    int timelimit();                              /* just to check stub or built-in */
    int iscolorpackage = 0;                       /* true if \usepackage{color} found*/
    int iseepicpackage = 0;                       /* true if \usepackage{eepic} found*/
    int ispict2epackage = 0;                      /* true if \usepackage{pict2e}found*/
    int ispreviewpackage = 0;                     /* true if \usepackage{preview} */

    /* --- image rendering --- */
    int mathtex(); /* generate new image using LaTeX */

    /* --- image caching --- */
    char *makepath(); /* construct full path/filename.ext*/
    char *md5str();
    char *md5hash = NULL; /* md5 has of expression */
    char *presentwd();
    char *pwdpath = NULL; /* home pwd for relative file paths */

    char *about = "mathTeX v" VERSION ", Copyright(c) " COPYRIGHTDATE
                  ", John Forkosh Associates, Inc\n"
                  "Modified by mechabubba @ https://github.com/mechabubba/mathtex.          \n";
    char *usage =
        "\n"
        "Usage: mathtex [options] [expression]                                    \n"
        "\n"
        "  -c [cache]         the image cache folder. defaults to `./cache/`.     \n"
        "                     set this to \"none\" to deliberately disable caching\n"
        "                     of the rendered image                               \n"
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
    char *license =
        "\n"
        "This program is free software licensed under the terms of the GNU General\n"
        "Public License, and comes with absolutely no warranty whatsoever. Please \n"
        "see https://github.com/mechabubba/mathtex/blob/master/COPYING for        \n"
        "complete details.\n";

    char whichtemplate[512] = /* mathTeX which "adtemplate" */
        "\\begin{center}\n"
        "\\fbox{\\footnotesize %%whichpath%%}\\\\ \\vspace*{-.2in}"
        "%%beginmath%% %%expression%% %%endmath%%\n"
        "\\end{center}\n";

    /* -------------------------------------------------------------------------
    Initialization
    -------------------------------------------------------------------------- */
    /* --- set global variables --- */
    msgfp = stdout;                                                  /* for query mode output. hardcoded from `NULL` to `stdout` for now... */
    msgnumber = 0;                                                   /* no errors to report yet */
    if (imagetype < 1 || imagetype > 2) imagetype = 1;               /* keep in bounds */
    if (imagemethod < 1 || imagemethod > 2) imagemethod = 1;         /* keep in bounds */
    if ((pwdpath = presentwd(0)) != NULL) strcpy(homepath, pwdpath); /* pwd where exectuable resides. save it for mathtex() later */

    /* ---
     * process command-line args
     * ----------------------------------------------- */
    int c;
    int iserror = 0;
    if (argc <= 1) {
        fprintf(msgfp, "%s%s%s", about, usage, license);
        exit(0);
    } else {
        while ((c = getopt(argc, argv, ":c:f:hm:no:stw")) != -1) {
            switch (c) {
                case 'c': // cache w/ location
                    strcpy(cachepath, optarg);
                    if (strlen(cachepath) < 1 || strcmp(cachepath, "none") == 0) iscaching = 0; // disable caching if path is an empty string or keyword "none"
                    break;
                case 'f': // input file
                    nbytes = readcachefile(optarg, (unsigned char *)exprbuffer);
                    if (nbytes == 0) {
                        log_error("Unable to open file %s.", optarg);
                        iserror++;
                    }
                    exprbuffer[nbytes] = '\000'; // @TODO whats the point of this? earlier null delimiter?
                    break;
                case 'h': // help. trumps normal message verbosity
                    fprintf(stdout, "%s%s%s", about, usage, license);
                    fflush(stdout);
                    exit(0);
                    break;
                case 'm': // message verbosity
                    if (isnumeric(optarg)) {
                        msglevel = atoi(optarg);
                    } else {
                        fprintf(stderr, "Operand to option -%c must be an integer.\n", c);
                        iserror++;
                    }
                    break;
                case 'o':                    // output file
                    strcpy(outfile, optarg); // strcpy output path
                    trimwhite(outfile);
                    break;
                case 's': // write stdout
                    write_stdout = 1;
                    break;
                case 't': // tmp cache
                    tmp_cache = 1;
                    break;
                case 'w': // keep work dir
                    keep_work = 1;
                    break;
                case ':': // one of those without an operand
                    fprintf(stderr, "Option -%c requires an operand.\n", optopt);
                    iserror++;
                    break;
                case '?': // unknown
                    fprintf(stderr, "Unrecognized option: '-%c'\n", optopt);
                    iserror++;
                    break;
            }
        }
    }
    if (iserror) {
        log_info(1, usage);
        exit(2);
    }

    // get expression
    if (isempty(exprbuffer)) {
        if (optind > argc) {
            log_error("Expression not provided - nothing to render.\n");
            log_info(1, usage);
            goto end_of_job;
        }
        strcpy(exprbuffer, argv[optind]);
    }

    /* ---
     * pre-process expression
     * ---------------------- */
    // @TODO should we throw everything below this into mathprep???
    unescape_url(expression); // reencode url (latex can crash if not used)
    mathprep(expression);     // preprocess expression; convert &lt; to < and whatnot
    validate(expression);     // remove dangerous stuff like \input

    /* ---
     * check for embedded image \message directive (which supercedes everything)
     * ----------------------------------------------------------------------- */
    if (getdirective(expression, "\\message", 1, 0, 1, argstring) != NULL) { /* found \message directive */
        log_info(1, "%s%s\n", about, license);
        msgnumber = atoi(argstring); /* requested message number */
        if (msgnumber > MAXEMBEDDED) {
            log_error("Invalid message number provided.");
        } else {
            log_error("%s\n", embeddedtext[msgnumber]);
        }
        goto end_of_job;
    } /*nothing to do after emitting image*/

    /* ---
     * check for \switches directive (which supercedes everything else)
     * ---------------------------------------------------------------- */
    if (strreplace(expression, "\\switches", "", 0, 0) >= 1) { /* remove \switches */
        char *pathsource[] = {"default", "switch", "which", "locate"};
        *expression = '\000';                         /* reset expression */
        setpaths(0);                                  /* show _all_ set paths */
        strcat(expression, "\\parstyle");             /* set paragraph mode */
        strcat(expression, "\\small\\tt");            /* set font,size */
        strcat(expression, "\\fparbox{");             /* emit -Dswitches in framed box */
        strcat(expression, "Program image...\\\\\n"); /* image */
        sprintf(expression + strlen(expression), "%s\\\\", argv[0]);
        strcat(expression, "Paths...\\\\\n");                                                                                                    /* paths */
        sprintf(expression + strlen(expression), "-DLATEX=$\\backslash$\"%s$\\backslash$\" \\ (%s)\\\\ \n", latexpath, pathsource[islatexpath]); // latex path
        sprintf(expression + strlen(expression), "-DPDFLATEX=$\\backslash$\"%s$\\backslash$\" \\ (%s)\\\\ \n", pdflatexpath,
                pathsource[ispdflatexpath]); // pdflatex path
        sprintf(expression + strlen(expression), "-DDVIPNG=$\\backslash$\"%s$\\backslash$\" \\ (%s)\\\\ \n", dvipngpath,
                pathsource[isdvipngpath]);                                                                                                       // dvipng path
        sprintf(expression + strlen(expression), "-DDVIPS=$\\backslash$\"%s$\\backslash$\" \\ (%s)\\\\ \n", dvipspath, pathsource[isdvipspath]); // dvips path
        sprintf(expression + strlen(expression), "-DPS2EPSI=$\\backslash$\"%s$\\backslash$\" \\ (%s)\\\\ \n", ps2epsipath,
                pathsource[isps2epsipath]); // ps2epsi path
        sprintf(expression + strlen(expression), "-DCONVERT=$\\backslash$\"%s$\\backslash$\" \\ (%s)\\\\ \n", convertpath,
                pathsource[isconvertpath]); // convert path
        strcat(expression, "}");            /* end of \fparbox{} */
    } else {                                /* no \switches in expression */
        /* ---
         * check for \environment directive (which supercedes everything else)
         * ------------------------------------------------------------------- */
        if (strreplace(expression, "\\environment", "", 0, 0)                                                       /* remove \environment */
            >= 1) {                                                                                                 /* found \environment */
            int ienv = 0;                                                                                           /* environ[] index */
            /*iscaching = 0;*/                                                                                      /* don't cache \environment image */
            *expression = '\000';                                                                                   /* reset expression */
            setpaths(10 * latexmethod + imagemethod);                                                               /* set paths */
            strcat(expression, "\\parstyle");                                                                       /* set paragraph mode */
            strcat(expression, "\\scriptsize\\tt");                                                                 /* set font,size */
            strcat(expression, "\\noindent");                                                                       /* don't indent first line */
            strcat(expression, "\\begin{verbatim}");                                                                /* begin verbatim environment */
            for (ienv = 0;; ienv++) {                                                                               /* loop over environ[] strings */
                if (environ[ienv] == (char *)NULL) break;                                                           /* null terminates list */
                if (*(environ[ienv]) == '\000') break;                                                              /* double-check empty string */
                sprintf(expression + strlen(expression), "  %2d. %s \n", ienv + 1, strwrap(environ[ienv], 50, -6)); /* display environment string */
                /* "%2d.\\ \\ %s\\ \\\\ \n", ienv+1,nomath(environ[ienv])); */
            }
            strcat(expression, "\\end{verbatim}"); /* end verbatim environment */
        }
    }

    /* ---
     * save copy of expression before further preprocessing for MD5 hash
     * ------------------------------------------------------------------ */
    strcpy(hashexpr, expression); /* save unmodified expr for hash */

    /* ---
     * check for \which directive (supercedes everything not above)
     * ------------------------------------------------------------ */
    if (getdirective(expression, "\\which", 1, 0, 1, argstring) != NULL) { /* found \which directive */
        int ispermitted = 1;                                               /* true if a legitimate request */
        int nlocate = 1;                                                   /* use locate if which fails */
        char *path = NULL;                                                 /* whichpath() to argstring program*/
        char whichmsg[512];                                                /* displayed message */
        trimwhite(argstring);                                              /*remove leading/trailing whitespace*/
        if (isempty(argstring)) ispermitted = 0;                           /* arg is an empty string */
        else {                                                             /* have non-empty argstring */
            int arglen = strlen(argstring);                                /* #chars in argstring */
            if (strcspn(argstring, WHITESPACE) < arglen ||                 /* embedded whitespace */
                strcspn(argstring, "{}[]()<>") < arglen ||                 /* illegal char */
                strcspn(argstring, "|/\"\'\\") < arglen ||                 /* illegal char */
                strcspn(argstring, "`!@%&*+=^") < arglen                   /* illegal char */
            )
                ispermitted = 0; /*probably not a legitimate request*/
        }
        if (ispermitted) {                         /* legitimate request */
            path = whichpath(argstring, &nlocate); /* path to argstring program */
            sprintf(whichmsg, "%s(%s) = %s", (path == NULL || nlocate < 1 ? "which" : "locate"), argstring,
                    (path != NULL ? path : "not found")); /* display path or "not found" */
        } else {                                          /* display "not permitted" message */
            sprintf(whichmsg, "which(%s) = not permitted", argstring);
        }
        strreplace(whichtemplate, "%%whichpath%%", whichmsg, 0, 0); /*insert message*/
        // adtemplate = whichtemplate;                                 /* set which message */
        // adfrequency = 1;                                            /* force its display */
        // iscaching = 0;                                              /* don't cache it */
        // if (path != NULL)                                           /* change \ to $\backslash$ */
        //     strreplace(path, "\\", "$\\backslash$", 0, 0);          /* make \ displayable */
        // strcpy(expression, "\\parstyle\\small\\tt ");               /* re-init expression */
        // strcat(expression, "\\fparbox{");                           /* emit path in framed box */
        // sprintf(
        //     expression + strlen(expression),
        //     "which(%s) = %s", argstring, (path != NULL ? path : "not found")
        //);                       /* display path or "not found" */
        // strcat(expression, "}"); /* end of \fparbox{} */
    }

    /* ---
     * check for picture environment, i.e., \begin{picture}, but don't remove it
     * ----------------------------------------------------------------------- */
    if (strstr(expression, "picture") != NULL) ispicture = 1;                /* signal picture environment */
    if (strreplace(expression, "\\nopicture", "", 0, 0) >= 1) ispicture = 0; /* remove \nopicture; user wants to handle picture */

    if (ispicture) {                               /* set picture environment defaults*/
        imagemethod = 2;                           /* must use convert, not dvipng */
        mathmode = 2;                              /* must be in paragraph mode */
        isdepth = 0;                               /* reset default in case it's true */
        if (!ISGAMMA) strcpy(gamma, CONVERTGAMMA); /* default convert gamma */
    }

    /* ---
     * check for embedded directives, remove them and set corresponding values
     * ----------------------------------------------------------------------- */
    /* --- first (re)set default modes required for certain environments --- */
    if (strstr(expression, "gather") != NULL) mathmode = 2;   /* gather environment used, need paragraph style for gather */
    if (strstr(expression, "eqnarray") != NULL) mathmode = 2; /* eqnarray environment used, need paragraph style for eqnarray */

    /* --- check for explicit displaystyle/textstyle/parstyle directives --- */
    if (strreplace(expression, "\\displaystyle", "", 0, 0) >= 1) mathmode = 0; /*remove \displaystyle, set displaystyle flag */
    if (strreplace(expression, "\\textstyle", "", 0, 0) >= 1) mathmode = 1;    /* remove \textstyle, set textstyle flag */
    if (strreplace(expression, "\\parstyle", "", 0, 0) >= 1) mathmode = 2;     /* remove \parstyle, set parstyle flag */
    if (strreplace(expression, "\\parmode", "", 0, 0) >= 1) mathmode = 2;      /* \parmode same as \parstyle */

    /* --- check for quiet/halt directives (\quiet or \halt) --- */
    if (strreplace(expression, "\\quiet", "", 0, 0) >= 1) isquiet = 64;     /* remove occurrences of \quiet, set isquiet flag */
    if (strreplace(expression, "\\noquiet", "", 0, 0) >= 1) isquiet = 0;    /* remove \noquiet, unset quiet flag */
    if (getdirective(expression, "\\nquiet", 1, 0, 1, argstring) != NULL) { /* \nquiet{#Enters}, interpret arg as isquiet value */
        isquiet = atoi(argstring);
    }

    /* --- check for program paths --- */
    if (getdirective(expression, "\\convertpath", 1, 0, 1, argstring) != NULL) { /* convert path. \convertpath{/path/to/convert} */
        strcpy(convertpath, argstring);                                          /*interpret arg as /path/to/convert*/
        if (strstr(convertpath, "convert") == NULL) {                            /* just path without name */
            if (lastchar(convertpath) != '/') strcat(convertpath, "/");          /* not even a / at end of path, so add a / */
            strcat(convertpath, "convert");
        } /* add name of convert program */
        isconvertpath = 1;
    } /* treat like -DCONVERT switch */

    /* --- check for fontsize directives (\tiny ... \Huge) --- */
    if (1 || mathmode != 2) {                                                              /*latex sets font size in \parstyle*/
        for (irep = 0; !isempty(sizedirectives[irep]); irep++) {                           /*1=\tiny...10=\Huge*/
            if (strstr(expression, sizedirectives[irep]) != NULL) {                        /*found \size*/
                if (mathmode != 2) strreplace(expression, sizedirectives[irep], "", 1, 0); /* not in paragraph mode, so remove \size */
                fontsize = irep;
            } /* found \size so set fontsize */
        }
    }

    /* --- check for \depth or \nodepth directive --- */
    if (strreplace(expression, "\\depth", "", 0, 0) >= 1) { /* \depth requested and found depth */
        if (1) {                                            /* guard */
                                                            /* ---
                                                             * note: curl_init() stops at the first whitespace char in $url argument,
                                                             * so php functions using \depth replace blanks with tildes
                                                             * ----------------------------------------------------------------------
                                                             */
            /*int ntilde = 0;*/                             /* # ~ chars replaced */
            /*ntilde =*/strreplace(expression, "~", " ", 0, 0);
        }
        isdepth = 1; /* so reset flag */
        latexwrapper = latexdepthwrapper;
    }                                                         /* and wrapper */
    if (strreplace(expression, "\\nodepth", "", 0, 0) >= 1) { /* \nodepth requested and found \nodepth */
        isdepth = 0;                                          /* so reset flag */
        latexwrapper = latexdefaultwrapper;
    } /* and wrapper */

    /* --- check for explicit usepackage directives in expression --- */
    while (npackages < 9) {                                                                         /* no more than 9 extra packages */
        if (getdirective(expression, "\\usepackage", 1, 0, -1, packages[npackages]) == NULL) break; /* no more \usepackage directives */
        else {                                                                                      /* found another \usepackage */
            /* --- check for optional \usepackage [args] --- */
            *(packargs[npackages]) = '\000'; /* init for no optional args */
            if (noptional > 0) {             /* but we found an optional arg */
                strninit(packargs[npackages], optionalargs[0], 127);
            } /*copy the first*/
            /* --- check for particular packages --- */
            if (strstr(packages[npackages], "color") != NULL) iscolorpackage = 1;     /* set color package flag*/
            if (strstr(packages[npackages], "eepic") != NULL) iseepicpackage = 1;     /* set eepic package flag */
            if (strstr(packages[npackages], "pict2e") != NULL) ispict2epackage = 1;   /* set pict2e package flag */
            if (strstr(packages[npackages], "preview") != NULL) ispreviewpackage = 1; /* set preview package flag */
            npackages++;
        } /* bump package count */
    }

    strreplace(expression, "\\version", "", 0, 0); // remove \version

    /* --- check for image type directives (\gif or \png) --- */
    if (strreplace(expression, "\\png", "", 0, 0) >= 1) imagetype = 2; /* remove occurrences of \png and set png imagetype */
    if (strreplace(expression, "\\gif", "", 0, 0) >= 1) imagetype = 1; /* remove occurrences of \gif and set gif imagetype */

    /* --- check for latex method directives (\latex or \pdflatex) --- */
    if (strreplace(expression, "\\latex", "", 1, 0) >= 1) latexmethod = 1;    /* remove occurrences of \latex and set latex method */
    if (strreplace(expression, "\\pdflatex", "", 0, 0) >= 1) latexmethod = 2; /* remove occurrences of \pdflatex and set pdflatex */

    /* --- check for image method directives (\dvips or \dvipng) --- */
    if (strreplace(expression, "\\dvipng", "", 0, 0) >= 1) { /*remove occurrences of \dvipng*/
        imagemethod = 1;                                     /* set dvipng imagemethod */
        if (!ISGAMMA) strcpy(gamma, DVIPNGGAMMA);
    }                                                       /* default dvipng gamma */
    if (strreplace(expression, "\\dvips", "", 0, 0) >= 1) { /* remove occurrences of -dvips*/
        imagemethod = 2;                                    /* set dvips/convert imagemethod */
        if (!ISGAMMA) strcpy(gamma, CONVERTGAMMA);
    } /* default convert gamma */

    /* --- check for convert/dvipng command's -density/-D parameter --- */
    if (getdirective(expression, "\\density", 1, 1, 1, density) == NULL)
        getdirective(expression, "\\dpi", 1, 1, 1, density); /* no \density directive, so try \dpi instead */

    /* --- check for convert command's \gammacorrection parameter --- */
    getdirective(expression, "\\gammacorrection", 1, 1, 1, gamma); /*look for \gamma*/

    /* --- check for \cache or \nocache directive --- */
    if (strreplace(expression, "\\cache", "", 0, 0) >= 1) iscaching = 1;   /* remove \cache and cache this image */
    if (strreplace(expression, "\\nocache", "", 0, 0) >= 1) iscaching = 0; /* remove \nocache and DON'T cache this image */

    /* ---check for \eval{}'s in (1)submitted expressions, (2)\newcommand's--- */
    for (irep = 1; irep <= 2; irep++) {                                                      /* 1=expression, 2=latexwrapper */
        char *thisrep = (irep == 1 ? expression : latexwrapper);                             /* choose 1 or 2 */
        while ((pdirective = getdirective(thisrep, "\\eval", 1, 0, 1, argstring)) != NULL) { /* found \eval{} directive */
            int ival = (isempty(argstring) ? 0 : evalterm(mathtexstore, argstring));
            char aval[256];            /* buffer to convert ival to alpha */
            sprintf(aval, "%d", ival); /* convert ival to alpha */
            strchange(0, pdirective, aval);
        } /* replace \eval{} with result */
    }

    /* --- see if we need any packages not already \usepackage'd by user --- */
    if (npackages < 9 && !iscolorpackage) {          /* have room for one more package. \usepackage{color} not specified */
        if (strstr(expression, "\\color") != NULL) { /* but \color is used */
            strcpy(packages[npackages], "color");    /* so \usepackage{color} is needed*/
            *(packargs[npackages]) = '\000';         /* no optional args */
            npackages++;
        } /* bump package count */
    }
    if (npackages < 9 && ispicture) {                 /* have room for one more package and using picture environment */
        if (latexmethod == 1) {                       /* using latex with eepic */
            if (!iseepicpackage) {                    /* \usepackage{eepic} not specified*/
                strcpy(packages[npackages], "eepic"); /* \usepackage{eepic} needed */
                *(packargs[npackages]) = '\000';      /* no optional args */
                npackages++;
            } /* bump package count */
        }
        if (latexmethod == 2 && npackages < 8) {       /* using pdflatex with pict2e and have room for two more packages */
            if (!ispict2epackage) {                    /*\usepackage{pict2e} not specified*/
                strcpy(packages[npackages], "pict2e"); /* \usepackage{pict2e} needed */
                *(packargs[npackages]) = '\000';       /* no optional args */
                npackages++;
            }                                                    /* bump package count */
            if (!ispreviewpackage) {                             /*\usepackage{preview} not specified*/
                strcpy(packages[npackages], "preview");          /*\usepackage{preview} needed*/
                strcpy(packargs[npackages], "active,tightpage"); /* set optional args */
                npackages++;
            } /* bump package count */
        }
    }

    /* ---
     * hash expression for name of cached image file
     * -------------------------- */
    trimwhite(expression); /*remove leading/trailing whitespace*/
    if (isempty(expression)) {
        log_error("Expression empty after preprocessing; not rendering.\n");
        goto end_of_job;
    }
    md5hash = md5str(hashexpr);

    /* ---
     * emit initial messages
     * --------------------- */
    log_info(1, "%s%s\n", about, license);
    log_info(5, "[main] running image: %s\n", argv[0]);
    log_info(5, "[main] home directory: %s\n", homepath);
    log_info(5, "[main] input expression: %s\n", hashexpr);
    log_info(10, "[main] %s timelimit info: warn/killtime=%d/%d, path=%s\n", (timelimit("", -99) == 992 ? "Built-in" : "Stub"), warntime, killtime,
             (istimelimitpath ? timelimitpath : "none"));

    /* -------------------------------------------------------------------------
    Emit cached image or render the expression
    -------------------------------------------------------------------------- */
    if (md5hash != NULL) { /* md5str() almost surely succeeded*/
        /* ---
         * check for cache directory, and create it if it doesn't already exist
         * -------------------------------------------------------------------- */
        if (isempty(outfile)) {                                         /* no explicit output file given */
            if (!isdexists(makepath(NULL, NULL, NULL))) {               /* and no cache directory */
                if (mkdir(makepath(NULL, NULL, NULL), perm_all) != 0) { /* tried to mkdir and failed, emit embedded error image*/
                    log_info(1, "[main] Error occurred whilst `mkdir %s`;\n", makepath(NULL, NULL, NULL));
                    log_error("%s\n", embeddedtext[CACHEFAILED]);
                    goto end_of_job;
                } /* quit if failed to mkdir cache */
            }
        }

        /* ---
         * now generate the new image and emit it
         * -------------------------------------- */
        /* --- set up name for temporary work directory --- */
        // strninit(tempdir, tmpnam(NULL), 255);   /* maximum name length is 255 */
        strninit(tempdir, md5hash, 255);                        /* maximum name length is 255 */
        log_info(5, "[main] working directory: %s\n", tempdir); /* additional message. working directory */

        if (mathtex(expression, md5hash) != imagetype) {
            // shits fucked. throw error message and abandon ship
            // isdepth = 0; /* no imageinfo in embedded images */
            if (msgnumber < 1) msgnumber = 2; /* if nothing specific, emit general error message */
            log_error(embeddedtext[msgnumber]);
            goto end_of_job;
        }

        /** ---
         * emit generated image to stdout
         * ------------------------------ */
        if (write_stdout) {
            FILE *img = fopen(makepath(NULL, md5hash, extensions[imagetype]), "rb");
            if (img == NULL) {
                log_error("Failed to open file to write stdout (did the file get created?)\n"); // @TODO necessary?
                exit(1);
            }
            fseek(img, 0, SEEK_END);
            int s = ftell(img);
            fseek(img, 0, SEEK_SET);
            // dumb shit ahead.
            // i don't know why `write(1, img, s)` doesn't write the whole file to stdout, but here we are.
            int c;
            while ((c = fgetc(img)) != EOF) putchar(c);
            fflush(stdout);
            fclose(img);
        }

        /* ---
         * remove images not being cached
         * ------------------------------ */
        if (!iscaching) {                                               /* don't want this image cached */
            if (isempty(outfile)) {                                     /* unless explicit outfile provided*/
                remove(makepath(NULL, md5hash, extensions[imagetype])); /* remove file */
            }
        }
    }

end_of_job:
    if (msgfp != NULL && msgfp != stdout) fclose(msgfp); /* have an open message file, so close it at eoj */
    exit(0);
}

/* ==========================================================================
 * Function:	mathtex ( expression, filename )
 * Purpose:	create image of latex math expression
 * --------------------------------------------------------------------------
 * Arguments:	expression (I)	pointer to null-terminated char string
 *				containing latex math expression
 *		filename (I)	pointer to null-terminated char string
 *				containing filename but not .extension
 *				of output file (should be the md5 hash
 *				of expression)
 * --------------------------------------------------------------------------
 * Returns:	( int )		imagetype if successful, 0=error
 * --------------------------------------------------------------------------
 * Notes:     o	created file will be filename.gif if imagetype=1
 *		or filename.png if imagetype=2.
 * ======================================================================= */
int mathtex(char *expression, char *filename) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    char errormsg[1024] = /* latex runs but can't make .dvi */
        "\\fbox{\\footnotesize $\\mbox{Latex failed, probably due to} "
        "\\atop \\mbox{an error in your expression.}$}";
    char usepackage[1024] = "\000"; /* additional \usepackage{}'s */
    char convertargs[1024] =        /* args/switches for convert */
        " -density %%dpi%% -gamma %%gamma%%"
        //" -border 0% -fuzz 2%"
        " -trim -transparent \"#FFFFFF\" ";
    char dvipngargs[1024] = /* args/switches for dvipng */
        " --%%imagetype%% -D %%dpi%% --gamma %%gamma%%"
        " -bg Transparent -T tight -v" /* -q for quiet, -v for verbose */
        " -o %%giffile%% ";            /* output filename supplied as -o */

    /* --- other variables --- */
    static int iserror = 0; /* true if procesing error message */
    int setpaths();         /* set paths for latex,dvipng,etc */
    char *makepath();       /* path/filename.ext */
    char latexfile[256];
    char giffile[256] = "\000";
    FILE *latexfp = NULL;                 /*latex wrapper file for expression*/
    char command[2048], subcommand[1024]; /*system(command) runs latex, etc*/
    char *beginmath[] = {" \\noindent $\\displaystyle ", " \\noindent $ ", " "};
    char *endmath[] = {" $ ", " $ ", " "};
    int perm_all = (S_IRWXU | S_IRWXG | S_IRWXO); /* 777 permissions */
    int dir_stat = 0;                             /* 1=mkdir okay, 2=chdir okay */
    int sys_stat = 0;                             /* system() return status */
    int isdexists();                              /* check if dir, .dvi file created */
    int isfexists();
    // int isnotfound();                             /* check .err file for "not found" */
    char *presentwd(); /* pwd for file paths */
    char *pwdpath = NULL;
    int isworkpath = 0; /* true if cd'ed to working dir */
    int strreplace();   /* replace template directives */
    int ipackage = 0;   /* packages[] index 0...npackages-1*/
    int rrmdir();       /* rm -r */
    int gifpathlen = 0; /* ../ or ../../ prefix of giffile */
    int status = 0;     /* imagetype or 0=error */
    int timelimit();    /* and using built-in timelimit() */

    /* -------------------------------------------------------------------------
    Make temporary work directory and cd to ~workpath/tempdir/
    -------------------------------------------------------------------------- */
    msgnumber = 0;                      /* no error to report yet */
    if (!isempty(workpath)) {           /*have a working dir for temp files*/
        if (isdexists(workpath)) {      /* if working directory exists */
            if (chdir(workpath) == 0) { /* cd to working directory */
                isworkpath = 1;         /* signal cd to workpath succeeded */
            }
        }
    }
    if (!isdexists(tempdir)) {               /* if temp directory doesn't exist */
        if (mkdir(tempdir, perm_all) != 0) { /* tried to make temp dir, but mkdir failed */
            msgnumber = MKDIRFAILED;         /* set corresponding message number and quit */
            goto end_of_job;
        }
    }
    dir_stat++;                  /* signal mkdir successful */
    if (chdir(tempdir) != 0) {   /* tried to cd to temp dir, but failed */
        msgnumber = CHDIRFAILED; /* set corresponding message number and quit */
        goto end_of_job;
    }
    dir_stat++; /* signal chdir successful */

    /* -------------------------------------------------------------------------
    set up latex directives for user-specified additional \usepackage{}'s
    -------------------------------------------------------------------------- */
    if (!iserror) {                                                /* don't \usepackage for error */
        if (npackages > 0) {                                       /* have additional packages */
            for (ipackage = 0; ipackage < npackages; ipackage++) { /*make \usepackage{}*/
                strcat(usepackage, "\\usepackage");                /* start with a directive */
                if (!isempty(packargs[ipackage])) {                /* have an optional arg */
                    strcat(usepackage, "[");                       /* begin optional argument */
                    strcat(usepackage, packargs[ipackage]);        /* add optional arg */
                    strcat(usepackage, "]");
                }                                       /* finish optional arg */
                strcat(usepackage, "{");                /* begin package name argument */
                strcat(usepackage, packages[ipackage]); /* add package name */
                strcat(usepackage, "}\n");
            } /* finish constructing directive */
        }
    }

    /* -------------------------------------------------------------------------
    Replace "keywords" in latex template with expression and other directives
    -------------------------------------------------------------------------- */
    /* --- usually replace %%pagestyle%% with \pagestyle{empty} --- */
    if (!ispicture || latexmethod == 1) { /* not \begin{picture} environment or using latex/dvips/ps2epsi */
        strreplace(latexwrapper, "%%pagestyle%%", "\\pagestyle{empty}", 1, 0);
    }
    /* --- replace %%previewenviron%% if a picture and using pfdlatex --- */
    if (ispicture && latexmethod == 2) { /* have \begin{picture} environment and using pdflatex/convert */
        strreplace(latexwrapper, "%%previewenviron%%", "\\PreviewEnvironment{picture}", 1, 0);
    }
    /* --- replace %%beginmath%%...%%endmath%% with \[...\] or with $...$ --- */
    if (mathmode < 0 || mathmode > 2) mathmode = 0; /* mathmode validity check */
    strreplace(latexwrapper, "%%beginmath%%", beginmath[mathmode], 1, 0);
    strreplace(latexwrapper, "%%endmath%%", endmath[mathmode], 1, 0);
    /* --- replace %%fontsize%% in template with \tiny...\Huge --- */
    strreplace(latexwrapper, "%%fontsize%%", sizedirectives[fontsize], 1, 0);
    /* --- replace %%setlength%% in template for pictures when necessary --- */
    if (ispicture && strstr(expression, "\\unitlength") == NULL) {                           /* have \begin{picture} environment, but no \unitlength */
        strreplace(latexwrapper, "%%setlength%%", "\\setlength{\\unitlength}{1.0in}", 1, 0); /* so default it to 1 inch */
    }
    /* --- replace %%usepackage%% in template with extra \usepackage{}'s --- */
    strreplace(latexwrapper, "%%usepackage%%", usepackage, 1, 0);
    /* --- replace %%expression%% in template with expression --- */
    strreplace(latexwrapper, "%%expression%%", expression, 1, 0);

    /* -------------------------------------------------------------------------
    Create latex document wrapper file containing expression
    -------------------------------------------------------------------------- */
    strcpy(latexfile, makepath("", "latex", ".tex")); /* latex filename latex.tex */
    latexfp = fopen(latexfile, "w");                  /* open latex file for write */
    if (latexfp == NULL) {                            /* couldn't open latex file */
        msgnumber = FOPENFAILED;                      /* set corresponding message number*/
        goto end_of_job;
    }                                     /* and quit */
    fprintf(latexfp, "%s", latexwrapper); /* write file */
    fclose(latexfp);                      /* close file after writing it */

    /* -------------------------------------------------------------------------
    Set paths to programs we'll need to run
    -------------------------------------------------------------------------- */
    setpaths(10 * latexmethod + imagemethod);

    /* -------------------------------------------------------------------------
    Execute the latex file
    -------------------------------------------------------------------------- */
    /* --- initialize system(command); to execute the latex file --- */
    *command = '\000'; /* init command as empty string */

    /* --- run latex under timelimit if explicitly given -DTIMELIMIT switch --- */
    if (istimelimitpath && warntime > 0 &&
        !iscompiletimelimit) {                              /* given explict -DTIMELIMIT path, and positive warntime, and not using builtin timelimit()... */
        if (killtime < 1) killtime = 1;                     /* don't make trouble for timelimit*/
        strcat(command, makepath("", timelimitpath, NULL)); /* timelimit program */
        if (isempty(command))                               /* no path to timelimit */
            warntime = -1;                                  /* reset flag to signal no timelimit*/
        else {                                              /* have path to timelimit program */
            sprintf(command + strlen(command), " -t%d -T%d ", warntime, killtime); /* add timelimit args after path */
        }
    }

    /* --- path to latex executable image followed by args --- */
    if (latexmethod != 2) {                                   /* not explicitly using pdflatex */
        strcpy(subcommand, makepath("", latexpath, NULL));    /* running latex program */
    } else {                                                  /* explicitly using pdflatex */
        strcpy(subcommand, makepath("", pdflatexpath, NULL)); /* running pdflatex */
    }
    if (isempty(subcommand)) {   /* no program path to latex */
        msgnumber = SYLTXFAILED; /* set corresponding error message */
        goto end_of_job;
    }                                                      /* signal failure and emit error */
    strcat(command, subcommand);                           /* add latex path (after timelimit)*/
    strcat(command, " ");                                  /* add a blank before latex args */
    strcat(command, latexfile);                            /* run on latexfile we just wrote */
    if (isquiet > 0) {                                     /* to continue after latex error */
        if (isquiet > 99) {                                /* explicit q requested */
            system("echo \"q\" > reply.txt");              /* reply  q  to latex error prompt */
        } else {                                           /* reply <Enter>'s followed by x */
            int nquiet = isquiet;                          /* this many <Enter>'s before x */
            FILE *freply = fopen("reply.txt", "w");        /* open reply.txt for write */
            if (freply != NULL) {                          /* opened successfully */
                while (--nquiet >= 0) fputs("\n", freply); /* write \n's to reply.txt nquiet times */
                fputs("x", freply);                        /* finally followed by an x */
                fclose(freply);
            }
        } /* close reply.txt */
        strcat(command, " < reply.txt");
    } else {                             /*by redirecting stdin to reply.txt*/
        strcat(command, " < /dev/null"); /* or redirect stdin to /dev/null */
    }
    strcat(command, " >latex.out 2>latex.err");                     /* redirect stdout and stderr */
    log_info(5, "[mathtex] latex command executed: %s\n", command); /* show latex command executed*/

    /* --- execute the latex file --- */
    sys_stat = timelimit(command, killtime); /* throttle the latex command */
    log_info(10, "[mathtex] system() return status: %d\n", sys_stat);
    if (latexmethod != 2) {
        if (!isfexists(makepath("", "latex", ".dvi"))) sys_stat = -1; /* ran latex, but no latex dvi. signal that latex failed */
    }
    if (latexmethod == 2) {
        if (!isfexists(makepath("", "latex", ".pdf"))) sys_stat = -1; /* ran pdflatex, but no pdflatex pdf. signal that pdflatex failed */
    }
    if (sys_stat == -1) {                         /* system() or pdf/latex failed */
        if (!iserror) {                           /* don't recurse if errormsg fails */
            iserror = 1;                          /* set error flag */
            isdepth = ispicture = 0;              /* reset depth, picture mode */
            status = mathtex(errormsg, filename); /* recurse just once for error msg*/
            goto end_of_job;
        } else {                                                     /* ignore 2nd try to recurse */
            msgnumber = sys_stat == 127 ? SYLTXFAILED : LATEXFAILED; /* latex failed for whatever reason */
            goto end_of_job;
        } /* and quit */
    }

    /* -------------------------------------------------------------------------
    Extract image info from latex.info (if available)
    -------------------------------------------------------------------------- */
    if (isdepth) {                                                   /* image info requested */
        if (isfexists(makepath("", "latex", ".info"))) {             /* and have latex.info */
            FILE *info = fopen(makepath("", "latex", ".info"), "r"); /* open it for read */
            char infoline[256];                                      /* read a line from latex.info */
            if (info != NULL) {                                      /* open succeeded */
                while (fgets(infoline, 255, info) != NULL) {         /* read until eof or error */
                    int i = 0;                                       /* imageinfo[] index */
                    char *delim = NULL;                              /* find '=' in infoline */
                    trimwhite(infoline);                             /* remove leading/trailing whitespace */
                    for (i = 0; imageinfo[i].format != NULL; i++) {  /* all imageinfo[] fields */
                        if (strstr(infoline, imageinfo[i].identifier) == infoline) {
                            imageinfo[i].value = (-9999.);                      /* init value to signal error */
                            *(imageinfo[i].units) = '\000';                     /* init units to signal error */
                            if ((delim = strchr(infoline, '=')) == NULL) break; /* no '=' delim */
                            memmove(infoline, delim + 1, strlen(delim));        /* get value after '=' */
                            trimwhite(infoline);                                /* remove leading/trailing whitespace*/
                            imageinfo[i].value = strtod(infoline, &delim);      /* convert to integer */
                            if (!isempty(delim)) {                              /* units, e.g., "pt", after value */
                                memmove(infoline, delim, strlen(delim) + 1);    /* get units field */
                                trimwhite(infoline);                            /* remove leading/trailing whitespace */
                                strninit(imageinfo[i].units, infoline, 16);
                            }      /* copy units */
                            break; /* don't check further identifiers */
                        }
                    }
                }
                fclose(info); /* close info file */
            }
        }
    }

    /* -------------------------------------------------------------------------
    Construct the output path/filename.[gif,png] for the image file
    -------------------------------------------------------------------------- */
    if (isempty(outfile) || !isthischar(*outfile, "/\\")) { /* using default cache directory, or juts given a relative path */
        *giffile = '\000';                                  /* start with empty string */
        if (isworkpath) {                                   /* we've cd'ed to a working dir */
            if (!isempty(homepath))                         /* have a homepath */
                strcpy(giffile, homepath);                  /* so just use it */
            else {                                          /* home path not available */
                int nsub = (isworkpath ? 2 : 1);            /* up two dirs for workdir, else 1 */
                if (iserror) nsub++;                        /* and another if in error subdir */
                if ((pwdpath = presentwd(nsub))             /* get path nsub dirs up from pwd */
                    != NULL)                                /* got it */
                    strcpy(giffile, pwdpath);
            } /* use it as giffile prefix */
        }
        if (isempty(giffile) && !tmp_cache) {       /* haven't constructed giffile */
            if (iserror) strcat(giffile, "../");    /*up to temp if in error subdir*/
            strcat(giffile, "../");                 /* up to home or working dir */
            if (isworkpath) strcat(giffile, "../"); /* temp under working dir */
        }
        gifpathlen = strlen(giffile); /* #chars in ../ or ../../ prefix */
    }

    if (isempty(outfile)) {
        strcat(giffile, makepath(NULL, filename, extensions[imagetype])); /* using default output filename */
    } else {
        strcat(giffile, makepath("", outfile, NULL)); /* have an explicit output file */
    }
    log_info(5, "[mathtex] output image file: %s\n", giffile + gifpathlen); /* show output filename (?) */

    /* -------------------------------------------------------------------------
    Run dvipng for .dvi-to-gif/png
    -------------------------------------------------------------------------- */
    if (imagemethod == 1) { /*dvipng method requested (default)*/
        /* ---
         * First replace "keywords" in dvipngargs template with actual values
         *------------------------------------------------------------------- */
        /* --- replace %%imagetype%% in dvipng arg template with gif or png --- */
        strreplace(dvipngargs, "%%imagetype%%", extensions[imagetype], 1, 0);
        /* --- replace %%dpi%% in dvipng arg template with actual dpi --- */
        strreplace(dvipngargs, "%%dpi%%", density, 1, 0);
        /* --- replace %%gamma%% in dvipng arg template with actual gamma --- */
        strreplace(dvipngargs, "%%gamma%%", gamma, 1, 0);
        /* --- replace %%giffile%% in dvipng arg template with actual giffile --- */
        strreplace(dvipngargs, "%%giffile%%", giffile, 1, 0);
        /* ---
         * And run dvipng to convert .dvi file directly to .gif/.png
         *---------------------------------------------------------- */
        strcpy(command, makepath("", dvipngpath, NULL)); /* running dvipng program */
        if (isempty(command)) {                          /* no program path to dvipng */
            msgnumber = SYPNGFAILED;                     /* set corresponding error message */
            goto end_of_job;
        }                                                                 /* signal failure and emit error */
        strcat(command, dvipngargs);                                      /* add dvipng switches */
        strcat(command, makepath("", "latex", ".dvi"));                   /* run dvipng on latex.dvi */
        strcat(command, " >dvipng.out 2>dvipng.err");                     /* redirect stdout, stderr */
        log_info(10, "[mathtex] dvipng command executed: %s\n", command); /* dvipng command executed */
        sys_stat = system(command);                                       /* execute the dvipng command */
        if (sys_stat == -1 || !isfexists(giffile)) {                      /* system(dvipng) failed or dvipng failed to create giffile*/
            msgnumber = sys_stat == 127 ? SYPNGFAILED : DVIPNGFAILED;     /* dvipng failed for whatever reason */
            goto end_of_job;
        } /* and quit */
    }

    /* -------------------------------------------------------------------------
    Run dvips for .dvi-to-postscript and convert for postscript-to-gif/png
    -------------------------------------------------------------------------- */
    if (imagemethod == 2) { /* dvips/convert method requested */
        /* ---
         * First run dvips to convert .dvi file to .ps postscript
         *------------------------------------------------------- */
        if (latexmethod != 2) {                             /* only if not using pdflatex */
            strcpy(command, makepath("", dvipspath, NULL)); /* running dvips program */
            if (isempty(command)) {                         /* no program path to dvips */
                msgnumber = SYPSFAILED;                     /* set corresponding error message */
                goto end_of_job;
            }                                                    /* signal failure and emit error */
            if (!ispicture) strcat(command, " -E");              /*add -E switch if not picture*/
            strcat(command, " ");                                /* add a blank */
            strcat(command, makepath("", "latex", ".dvi"));      /* run dvips on latex.dvi */
            strcat(command, " -o ");                             /* to produce output file in */
            if (!ispicture) {                                    /* when not a picture (usually) */
                strcat(command, makepath("", "dvips", ".ps"));   /*dvips.ps postscript file*/
            } else {                                             /*intermediate temp file for ps2epsi*/
                strcat(command, makepath("", "dvitemp", ".ps")); /*temp postscript file*/
            }
            strcat(command, " >dvips.out 2>dvips.err");                      /* redirect stdout, stderr */
            log_info(10, "[mathtex] dvips command executed: %s\n", command); /* dvips command executed */
            sys_stat = system(command);                                      /* execute system(dvips) */

            /* --- run ps2epsi if dvips ran without -E (for \begin{picture}) --- */
            if (sys_stat != -1 && ispicture) {                    /* system(dvips) succeeded and we ran dvips without -E */
                strcpy(command, makepath("", ps2epsipath, NULL)); /* running ps2epsi */
                if (isempty(command)) {                           /* no program path to ps2epsi */
                    msgnumber = SYPSFAILED;                       /* set corresponding error message */
                    goto end_of_job;
                }                                                                  /* signal failure and emit error */
                strcat(command, " ");                                              /* add a blank */
                strcat(command, makepath("", "dvitemp", ".ps"));                   /*temp postscript file*/
                strcat(command, " ");                                              /* add a blank */
                strcat(command, makepath("", "dvips", ".ps"));                     /*dvips.ps postscript file*/
                strcat(command, " >ps2epsi.out 2>ps2epsi.err");                    /*redirect stdout,stderr*/
                log_info(10, "[mathtex] ps2epsi command executed: %s\n", command); /* command executed */
                sys_stat = system(command);                                        /* execute system(ps2epsi) */
            }
            if (sys_stat == -1 || !isfexists(makepath("", "dvips", ".ps"))) { /* system(dvips) failed; dvips didn't create .ps*/
                msgnumber = sys_stat == 127 ? SYPSFAILED : DVIPSFAILED;       /* dvips failed for whatever reason */
                goto end_of_job;
            } /* and quit */
        }

        /* ---
         * Then replace "keywords" in convertargs template with actual values
         *------------------------------------------------------------------- */
        /* --- replace %%dpi%% in convert arg template with actual density --- */
        strreplace(convertargs, "%%dpi%%", density, 1, 0);
        /* --- replace %%gamma%% in convert arg template with actual gamma --- */
        strreplace(convertargs, "%%gamma%%", gamma, 1, 0);

        /* ---
         * And run convert to convert .ps file to .gif/.png
         *-------------------------------------------------- */
        strcpy(command, makepath("", convertpath, NULL)); /*running convert program*/
        if (isempty(command)) {                           /* no program path to convert */
            msgnumber = SYCVTFAILED;                      /* set corresponding error message */
            goto end_of_job;
        }                                                  /* signal failure and emit error */
        strcat(command, convertargs);                      /* add convert switches */
        if (latexmethod != 2) {                            /* we ran latex and dvips */
            strcat(command, makepath("", "dvips", ".ps")); /* convert from postscript */
        }
        if (latexmethod == 2) {                             /* we ran pdflatex */
            strcat(command, makepath("", "latex", ".pdf")); /* convert from pdf */
        }
        strcat(command, " ");                                              /* field separator */
        strcat(command, giffile);                                          /* followed by ../cache/filename */
        strcat(command, " >convert.out 2>convert.err");                    /*redirect stdout, stderr*/
        log_info(10, "[mathtex] convert command executed: %s\n", command); /*convert command executed*/
        sys_stat = system(command);                                        /* execute system(convert) command */
        if (sys_stat == -1 || !isfexists(giffile)) {                       /* system(convert) failed or convert didn't create giffile*/
            msgnumber = sys_stat == 127 ? SYCVTFAILED : CONVERTFAILED;     /* convert failed for whatever reason */
            goto end_of_job;
        } /* and quit */
    }
    status = imagetype; /* signal success */

/* -------------------------------------------------------------------------
Back to caller
-------------------------------------------------------------------------- */
end_of_job:
    if (dir_stat >= 2) chdir(".."); /* return up to working dir or home */
    if (dir_stat >= 1) {
        int status = rrmdir(tempdir /* filename */); /* rm -r [temp working dir] */
        if (status == -1) {
            log_info(99, "[main] rrmdir returned %d;\n", status);
            log_error("%s\n", embeddedtext[REMOVEWORKFAILED]);
        }
    }
    if (isworkpath) chdir((!isempty(homepath) ? homepath : "..")); /* return from working dir to home, best guess at home */
    iserror = 0;                                                   /* always reset error flag */
    return status;
}

/* ==========================================================================
 * Function:	setpaths ( method )
 * Purpose:	try to set accurate paths for
 *		latex,pdflatex,timelimit,dvipng,dvips,convert
 * --------------------------------------------------------------------------
 * Arguments:	method (I)	10*ltxmethod + imgmethod where
 *				ltxmethod =
 *				   1 for latex, 2 for pdflatex,
 *				   0 for both
 *				imgmethod =
 *				   1 for dvipng, 2 for dvips/convert,
 *				   0 for both
 * --------------------------------------------------------------------------
 * Returns:	( int )		1
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
int setpaths(int method) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    char *programpath = NULL, *whichpath(); /* look for latex,dvipng,etc */
    int nlocate = 0;                        /*if which fails, #locate|grep hits*/
    int ltxmethod = method / 10;            /* divide by 10 */
    int imgmethod = method % 10;            /* remainder after division by 10 */
    static int islatexwhich = 0;            /* set true after we try whichpath() for that program */
    static int ispdflatexwhich = 0;
    static int isdvipngwhich = 0;
    static int isdvipswhich = 0;
    static int isps2epsiwhich = 0;
    static int isconvertwhich = 0;
    static int istimelimitwhich = 0;

    /* ---
     * set paths, either from -DLATEX=\"path/latex\", etc, or call whichpath()
     * ----------------------------------------------------------------------- */
    /* --- path for latex program --- */
    if (ltxmethod == 1 || ltxmethod == 0) {                             /* not needed for pdflatex only */
        /*if ( !ISLATEXSWITCH && !islatexwhich )*/                      /* nb, use islatexpath instead */
        if (!islatexpath && !islatexwhich) {                            /* no -DLATEX=\"path/latex\" */
            islatexwhich = 1;                                           /* signal that which already tried */
            nlocate = ISLOCATE;                                         /* use locate if which fails??? */
            if ((programpath = whichpath("latex", &nlocate)) != NULL) { /* try to find latex path */
                islatexpath = (nlocate == 0 ? 2 : 3);                   /* set flag signalling which() */
                strninit(latexpath, programpath, 255);
            }
        } /* copy path from whichpath() */
    }

    /* --- path for pdflatex program --- */
    if (ltxmethod == 2 || ltxmethod == 0) {                                /* not needed for latex only */
        /*if ( !ISPDFLATEXSWITCH && !ispdflatexwhich )*/                   /* nb, use ispdflatexpath*/
        if (!ispdflatexpath && !ispdflatexwhich) {                         /* no -DPDFLATEX=\"pdflatex\"*/
            ispdflatexwhich = 1;                                           /* signal that which already tried */
            nlocate = ISLOCATE;                                            /* use locate if which fails??? */
            if ((programpath = whichpath("pdflatex", &nlocate)) != NULL) { /* try to find pdflatex */
                ispdflatexpath = (nlocate == 0 ? 2 : 3);                   /* set flag signalling which() */
                strninit(pdflatexpath, programpath, 255);
            }
        } /*copy path from whichpath()*/
    }

    /* --- path for timelimit program --- */
    if (0) {                            /* only use explicit -DTIMELIMIT */
        if (warntime > 0) {             /* have -DWARNTIME or -DTIMELIMIT */
            /*if ( !ISTIMELIMITSWITCH*/ /* nb, use istimelimitpath instead */
            if (!istimelimitpath        /*no -DTIMELIMIT=\"path/timelimit\"*/
                && !istimelimitwhich) {
                istimelimitwhich = 1;                                /* signal that which already tried */
                nlocate = ISLOCATE;                                  /* use locate if which fails??? */
                if ((programpath = whichpath("timelimit", &nlocate)) /* try to find path */
                    != NULL) {                                       /* succeeded */
                    istimelimitpath = (nlocate == 0 ? 2 : 3);        /* set flag signalling which() */
                    strninit(timelimitpath, programpath, 255);
                }
            } /* copy from whichpath()*/
        }
    }

    /* --- path for dvipng program --- */
    if (imgmethod != 2) {                                     /* not needed for dvips/convert */
        /*if ( !ISDVIPNGSWITCH && !isdvipngwhich )*/          /* nb, use isdvipngpath instead*/
        if (!isdvipngpath && !isdvipngwhich) {                /* no -DDVIPNG=\"path/dvipng\" */
            isdvipngwhich = 1;                                /* signal that which already tried */
            nlocate = ISLOCATE;                               /* use locate if which fails??? */
            if ((programpath = whichpath("dvipng", &nlocate)) /* try to find dvipng */
                != NULL) {                                    /* succeeded */
                isdvipngpath = (nlocate == 0 ? 2 : 3);        /* set flag signalling which() */
                strninit(dvipngpath, programpath, 255);
            }
        } /*so copy path from whichpath()*/
    }

    /* --- path for dvips program --- */
    if (imgmethod != 1) {                                    /* not needed for dvipng */
        /*if ( !ISDVIPSSWITCH && !isdvipswhich )*/           /* nb, use isdvipspath instead */
        if (!isdvipspath && !isdvipswhich) {                 /* no -DDVIPS=\"path/dvips\" */
            isdvipswhich = 1;                                /* signal that which already tried */
            nlocate = ISLOCATE;                              /* use locate if which fails??? */
            if ((programpath = whichpath("dvips", &nlocate)) /* try to find dvips path */
                != NULL) {                                   /* succeeded */
                isdvipspath = (nlocate == 0 ? 2 : 3);        /* set flag signalling which() */
                strninit(dvipspath, programpath, 255);
            }
        } /* so copy path from whichpath()*/
    }

    /* --- path for ps2epsi program --- */
    if ((ispicture || method == 0) && imgmethod != 1 && ltxmethod != 2) { /* not needed for pdflatex */
        /*if ( !ISPS2EPSISWITCH && !isps2epsiwhich )*/                    /*use isps2epsipath instead*/
        if (!isps2epsipath && !isps2epsiwhich) {                          /*no -DPS2EPSI=\"path/ps2epsi\"*/
            isps2epsiwhich = 1;                                           /* signal that which already tried */
            nlocate = ISLOCATE;                                           /* use locate if which fails??? */
            if ((programpath = whichpath("ps2epsi", &nlocate))            /*try to find ps2epsi path*/
                != NULL) {                                                /* succeeded */
                isps2epsipath = (nlocate == 0 ? 2 : 3);                   /* set flag signalling which() */
                strninit(ps2epsipath, programpath, 255);
            }
        } /*copy path from whichpath()*/
    }

    /* --- path for convert program --- */
    if (imgmethod != 1) {                                                 /* not needed for dvipng */
        /*if ( !ISCONVERTSWITCH && !isconvertwhich )*/                    /*use isconvertpath instead*/
        if (!isconvertpath && !isconvertwhich) {                          /*no -DCONVERT=\"path/convert\"*/
            isconvertwhich = 1;                                           /* signal that which already tried */
            nlocate = ISLOCATE;                                           /* use locate if which fails??? */
            if ((programpath = whichpath("convert", &nlocate)) != NULL) { /* try to find convert */
                isconvertpath = (nlocate == 0 ? 2 : 3);                   /* set flag signalling which() */
                strninit(convertpath, programpath, 255);
            }
        } /*copy path from whichpath()*/
    }

    /* --- adjust imagemethod to comply with available programs --- */
    if (imgmethod != imagemethod && imgmethod != 0) goto end_of_job; /* ignore recursive call. 0 is a call for both methods */
    if (imagemethod == 1) {                                          /* dvipng wanted */
        if (!isdvipngpath) {                                         /* but we have no real path to it */
            if (imgmethod == 1) setpaths(2);                         /* try to set dvips/convert paths */
            if (isdvipspath && isconvertpath) {                      /* and we do have dvips, convert */
                imagemethod = 2;                                     /* so flip default to use them */
                if (!ISGAMMA) strcpy(gamma, CONVERTGAMMA);
            }
        } /*default convert gamma*/
    }
    if (imagemethod == 2) {                   /* dvips, convert wanted */
        if (!isdvipspath || !isconvertpath) { /*but we have no real paths to them*/
            if (imgmethod == 2) setpaths(1);  /* try to set dvipng path */
            if (isdvipngpath) {               /* and we do have dvipng path */
                imagemethod = 1;              /* so flip default to use it */
                if (!ISGAMMA) strcpy(gamma, DVIPNGGAMMA);
            }
        } /* default dvipng gamma */
    }

/* --- back to caller --- */
end_of_job:
    return 1;
}

/* ==========================================================================
 * Function:	isnotfound ( filename )
 * Purpose:	check a  "... 2>filename.err" file for a "not found" message,
 *		indicating the corresponding program path is incorrect
 * --------------------------------------------------------------------------
 * Arguments:	filename (I)	pointer to null-terminated char string
 *				containing filename (an .err extension
 *				is tacked on, just pass the filename)
 *				to be checked for "not found" message
 * --------------------------------------------------------------------------
 * Returns:	( int )		1 if filename.err contains "not found",
 *				0 otherwise (or for any error)
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
// ! NOT CURRENTLY IN USE, MAY BE USEFUL LATER... KEEPING IT AROUND !
// int isnotfound(char *filename) {
//    /* -------------------------------------------------------------------------
//    Allocations and Declarations
//    -------------------------------------------------------------------------- */
//    int status = 0;       /* set 1 if "not found" found */
//    int isfexists();      /* check if filename exists */
//    char command[256];    /* grep program */
//    FILE *grepout = NULL; /* grep's stdout */
//    char grepline[256];   /* line from grep's stdout */
//    int strreplace();     /* make sure "not found" was found */
//    int nlines = 0;       /* #lines read */
//
//    /* -------------------------------------------------------------------------
//    grep filename.err for filename (i.e., the program name), then check each
//    such line for "not found", "no such", etc.
//    -------------------------------------------------------------------------- */
//    /* --- initialization --- */
//    if (isempty(filename)) goto end_of_job; /* no input filename supplied */
//    sprintf(command, "%s.err", filename);   /* look for filename.err */
//    if (!isfexists(command)) {              /* filename.err doesn't exist */
//        status = 1;                         /* assume this means "not found" */
//        goto end_of_job;
//    } /* and return that to caller */
//
//    /* --- grep filename.err for filename (i.e., for the program name) --- */
//    sprintf(command, "grep -i \"%s\" %s.err", filename, filename); /*construct cmd*/
//
//    /* --- use popen() to invoke grep --- */
//    grepout = popen(command, "r");        /* issue grep and capture stdout */
//    if (grepout == NULL) goto end_of_job; /* failed */
//
//    /* --- read the pipe one line at a time --- */
//    while (fgets(grepline, 255, grepout) != NULL) { /* not at eof yet */
//        if (strreplace(grepline, "not found", "", 0, 0) > 0) nlines++; /* check for "not found" + count it */
//        if (strreplace(grepline, "no such", "", 0, 0) > 0) nlines++; /* check for "no such" + count it */
//    }
//    if (nlines > 0) status = 1; /* "not found" found */
//
//    /* --- pclose() waits for command to terminate --- */
//    pclose(grepout); /* finish */
//
// end_of_job:          /* back to caller */
//    return status; /*1 if filename contains "not found"*/
//}

/* ==========================================================================
 * Function:	validate ( expression )
 * Purpose:	remove/replace illegal \commands from expression
 * --------------------------------------------------------------------------
 * Arguments:	expression (I/O) pointer to null-terminated char string
 *				containing latex expression to be validated.
 *				Upon return, invalid \commands have been
 *				removed or replaced
 * --------------------------------------------------------------------------
 * Returns:	( int )		#illegal \commands found (hopefully 0)
 * --------------------------------------------------------------------------
 * Notes:     o	Ignore this note and see the one below it instead...
 *		  This routine is currently "stubbed out",
 *		  i.e., the invalid[] list of invalid \commands is empty.
 *		  Instead, \renewcommand's are embedded in the LaTeX wrapper.
 *		  So the user's -DNEWCOMMAND=\"filename\" can contain
 *		  additional \renewcomand's for any desired validity checks.
 *	      o	Because some \renewcommand's appear to occasionally cause
 *		latex to go into a loop after encountering syntax errors,
 *		I'm now using validate() to disable \input, etc.
 *		For example, the following short document...
 *		  \documentclass[10pt]{article}
 *		  \begin{document}
 *		  \renewcommand{\input}[1]{dummy renewcommand}
 *		  %%\renewcommand{\sqrt}[1]{dummy renewcommand}
 *		  %%\renewcommand{\beta}{dummy renewcommand}
 *		  \[ \left( \begin{array}{cccc} 1 & 0 & 0 &0
 *		  \\ 0 & 1  %%% \end{array}\right)
 *		  \]
 *		  \end{document}
 *		reports a  "! LaTeX Error:"  and then goes into
 *		a loop if you reply q.  But if you comment out the
 *		\renewcommand{\input} and uncomment either or both
 *		of the other \renewcommand's, then latex runs to
 *		completion (with the same syntax error, of course,
 *		but without hanging).
 *
 * ======================================================================= */
int validate(char *expression) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    /* --- list of invalid \commands --- */
    static struct {
        int action;      /* 0=ignore, 1=apply, 2=abort */
        char *command;   /* invalid \command */
        int nargs;       /* #args, \command{arg1}...{nargs} */
        int optionalpos; /* #{args} before optional [arg] */
        int argformat;   /* 0=LaTeX {arg} or [arg], 1=arg */
        char *displaystring;
    } invalid[] = {  /* list of invalid commands */
#if defined(INVALID) /* cc -DINVALID=\"filename\" */
    #include INVALID /* filename with invalid \commands */
#endif               /* as illustrated below... */
                    // clang-format off
        /* actn "command"           #args pos fmt "replacement string" or NULL
         * ---- ------------------- ----- --- --- -------------------------- */
        {  1,   "\\newcommand",     2,    1,  0,  NULL},
        {  1,   "\\providecommand", 2,    1,  0,  NULL},
        {  1,   "\\renewcommand",   2,    1,  0,  NULL},
        {  1,   "\\input",          1,    -1, 0,  NULL},
        /* --plain TeX commands with non-{}-enclosed args we can't parse-- */
        {  1,   "\\def",            2,   -1,  20, NULL},
        {  1,   "\\edef",           2,   -1,  20, NULL},
        {  1,   "\\gdef",           2,   -1,  20, NULL},
        {  1,   "\\xdef",           2,   -1,  20, NULL},
        {  1,   "\\loop",           0,   -1,  0,  NULL},
        {  1,   "\\csname",         0,   -1,  0,  NULL},
        {  1,   "\\catcode",        0,   -1,  0,  NULL},
        {  1,   "\\output",         0,   -1,  0,  NULL},
        {  1,   "\\everycr",        0,   -1,  0,  NULL},
        {  1,   "\\everypar",       0,   -1,  0,  NULL},
        {  1,   "\\everymath",      0,   -1,  0,  NULL},
        {  1,   "\\everyhbox",      0,   -1,  0,  NULL},
        {  1,   "\\everyvbox",      0,   -1,  0,  NULL},
        {  1,   "\\everyjob",       0,   -1,  0,  NULL},
        {  1,   "\\openin",         0,   -1,  0,  NULL},
        {  1,   "\\read",           0,   -1,  0,  NULL},
        {  1,   "\\openout",        0,   -1,  0,  NULL},
        {  1,   "\\write",          0,   -1,  0,  NULL},
        /* --- other dangerous notation --- */
        {  1,   "^^",               0,   -1,  0,  NULL},
#if 0
	    /* --- test cases --- */
        {  1,  "\\input",           1,   -1,  0,  "{\\mbox{$\\backslash$input\\{#1\\}}}" },*/
	    {  1,  "\\input",           1,   -1,  0,  "{\\mbox{~$\\backslash$input\\{#1\\}~not~permitted~}}" },
	    {  1,  "\\newcommand",      2,    1,  0,  NULL},
	    /*"{\\mbox{~$\\backslash$newcommand\\{#1\\}[#0]\\{#2\\}~not~permitted~}}" },*/
#endif
        {  0,  NULL,                0,   -1,  0,  NULL}
                    // clang-format on
    };

    /* --- other variables --- */
    int ninvalid = 0; /* #invalid =commands found */
    int ivalid = 0;   /* invalid[ivalid] list index */
    char *getdirective();
    char *pcommand = NULL;                                                /* find and remove invalid command */
    static char args[10][512] = {"", "", "", "", "", "", "", "", "", ""}; /*\cmd{arg}'s*/
    char *pargs[11] = {                                                   /* ptrs to them */
                       args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], NULL};
    char display[2048]; /*displaystring with args*/
    char argstr[256];
    char optstr[1024];
    char *strchange(); /* place display where command was */
    int strreplace();  /* replace #1 with args[0], etc */
    char *nomath();    /* change \ to \\, { to \{, etc */
    int iarg = 0;      /* args[], optionalargs[] indexes */
    int iopt = 0;

    /* -------------------------------------------------------------------------
    Initialization
    -------------------------------------------------------------------------- */
    if (isempty(expression)) goto end_of_job; /* no input to validate, so quit */

    /* -------------------------------------------------------------------------
    Check each invalid command in list
    -------------------------------------------------------------------------- */
    for (ivalid = 0; !isempty(invalid[ivalid].command); ivalid++) { /*run list*/
        /* --- extract local copy of invalid command list elements --- */
        int action = invalid[ivalid].action;                 /* 0=ignore,1=apply,2=abort*/
        char *command = invalid[ivalid].command;             /* invalid \command */
        int nargs = invalid[ivalid].nargs;                   /*number of \command {arg}'s*/
        int myoptionalpos = invalid[ivalid].optionalpos;     /*#{args} before [arg]*/
        int myargformat = invalid[ivalid].argformat;         /* 0={arg},1=arg*/
        char *displaystring = invalid[ivalid].displaystring; /*display template*/

        /* --- arg format info --- */
        int nfmt = 0;
        // int isnegfmt = 0;                            /* {arg} format */
        int argfmt[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; /* argformat digits */

        /* --- args returned as (char *) if nargs=1 or (char **) if nargs>1 --- */
        void *argptr = (nargs < 2 ? (void *)args[0] : (void *)pargs); /*(char * or **)*/

        /* --- find and remove/replace all invalid command occurrences --- */
        if (action < 1) continue;    /* ignore this invalid \command */
        optionalpos = myoptionalpos; /* set global arg for getdirective */
        argformat = myargformat;     /* set global arg for getdirective */

        /* --- interpret argformat digits --- */
        if (argformat != 0) {      /* have argformat */
            int myfmt = argformat; /* local copy */
            if (myfmt < 0) {
                // isnegfmt = 1;
                myfmt = -myfmt;
            }                               /* check sign */
            while (myfmt > 0 && nfmt < 9) { /* have more format digits */
                argfmt[nfmt] = myfmt % 10;  /* store low-order decimal digit */
                myfmt /= 10;                /* and shift it out */
                nfmt++;
            } /* count another format digit */
        }

        /* --- remove/replace each occurrence of invalid \command --- */
        while ((pcommand = getdirective(expression, command, 1, 0, nargs, argptr)) != NULL) { /* found and removed an occurrence */
            ninvalid++;                                                                       /* count another invalid \command */
            if (noptional >= 8) noptional = 7;                                                /* don't overflow our buffers */

            /* --- construct optional [arg]...[arg] for display --- */
            *optstr = '\000';                                       /*init optional [arg]...[arg] string*/
            if (noptional > 0) {                                    /* have optional [args] */
                for (iopt = 0; iopt < noptional; iopt++) {          /* construct "[arg]...[arg]" */
                    if (!isempty(optionalargs[iopt])) {             /* have an optional arg */
                        strcat(optstr, "[");                        /* leading [ for optional arg */
                        strcat(optstr, nomath(optionalargs[iopt])); /*optional arg string*/
                        strcat(optstr, "]");
                    }
                }
            }

            /* --- construct error message display or use supplied template --- */
            if (isempty(displaystring)) {                                                /*replace \command by not~permitted*/
                strcpy(display, "\\mbox{~\\underline{");                                 /* underline error in \mbox{}*/
                strcat(display, nomath(command));                                        /* command without \ */
                for (iarg = 0; iarg < nargs; iarg++) {                                   /* add {args} and any [args] */
                    int ifmt = (nfmt <= iarg ? 0 : argfmt[nfmt - iarg - 1]);             /* arg format digit */
                    if (iarg == myoptionalpos && noptional > 0) strcat(display, optstr); /* [args] belong here, insert them before next {arg} */
                    if (isempty(args[iarg])) break;                                      /* no more args */
                    if (ifmt == 0) strcat(display, "\\{");                               /* insert leading \{ for arg */
                    strcat(display, nomath(args[iarg]));                                 /* arg */
                    if (ifmt == 0) strcat(display, "\\}");
                }
                strcat(display, "~not~permitted}~}");
            } else {                                                                                       /* replace \command by displaystring */
                strcpy(display, displaystring);                                                            /* local copy of display template */
                if (noptional < 1) strreplace(display, "[#0]", "", 0, 0);                                  /* no optional args, so remove tag from template */
                for (iarg = 0; iarg < nargs; iarg++) {                                                     /* replace #1 with args[0], etc */
                    if (iarg == myoptionalpos && noptional > 0) strreplace(display, "[#0]", optstr, 0, 0); /* [args] belong here, insert them before {arg}*/
                    if (isempty(args[iarg])) break;                                                        /* no more args */
                    else {                                                                                 /* have an arg */
                        sprintf(argstr, "#%d", iarg + 1);                                                  /* #1 in template displays args[0] */
                        strreplace(display, argstr, nomath(args[iarg]), 0, 0);
                    } /* replace */
                }
            }
            strchange(0, pcommand, display);                        /* place display where command was */
            for (iarg = 0; iarg < 10; iarg++) *args[iarg] = '\000'; /* reset args */
        }
    }

end_of_job:
    return ninvalid; /* back to caller with #invalid */
}

/* ==========================================================================
 * Function:	makepath ( path, name, extension )
 * Purpose:	return string containing path/name.extension
 * --------------------------------------------------------------------------
 * Arguments:	path (I)	pointer to null-terminated char string
 *				containing "" path or path/ or NULL to
 *				use cachepath if caching enabled
 *		name (I)	pointer to null-terminated char string
 *				containing filename but not .extension
 *				of output file or NULL
 *		extension (I)	pointer to null-terminated char string
 *				containing extension or NULL
 * --------------------------------------------------------------------------
 * Returns:	( char * )	"cachepath/filename.extension" or NULL=error
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
char *makepath(char *path, char *name, char *extension) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    static char namebuff[512]; /* buffer for constructed filename */
    char *filename = NULL;     /*ptr to filename returned to caller*/

    /* -------------------------------------------------------------------------
    construct filename
    -------------------------------------------------------------------------- */
    /* --- validity checks --- */
    // if (isempty(name)) goto end_of_job; /* no name supplied by caller */

    /* --- start with caller's path/ or default path to cache directory --- */
    *namebuff = '\000';            /* re-init namebuff */
    if (path == NULL) {            /* use default path to cache */
        if (!isempty(cachepath)) { /* have a cache path */
            if (!tmp_cache) {
                strcpy(namebuff, cachepath);
            } else {
                strcpy(namebuff, "/tmp/mathtex/");
            }
        }
    } else if (*path != '\000') { /* begin filename with path or use caller's supplied path if it's not an empty string */
        strcpy(namebuff, path);   /* begin filename with path */
    }
    if (!isempty(namebuff)) {                           /* have a leading path */
        if (!isthischar(lastchar(namebuff), "\\/")) {   /* no \ or / at end of path */
            strcat(namebuff, (iswindows ? "\\" : "/")); /* so add windows\ or unix/ */
        }
    }

    /* --- add name after path/ (name arg might just be a blank space) --- */
    if (!isempty(name)) {                         /* name supplied by caller */
        if (!isempty(namebuff)) {                 /* and if we already have a path */
            if (isthischar(*name, "\\/")) name++; /* skip leading \ or / in name */
        }
        strcat(namebuff, name);
    } /* name concatanated after path/ */

    /* --- add extension after path/name */
    if (!isempty(extension)) {                                       /* have a filename extension */
        if (!isthischar(lastchar(namebuff), ".")) {                  /* no . at end of name */
            if (!isthischar(*extension, ".")) strcat(namebuff, "."); /* and extension has no leading ., so we need to add our own */
            strcat(namebuff, extension);
        } else {                                                                          /* . already at end of name */
            strcat(namebuff, (!isthischar(*extension, ".") ? extension : extension + 1)); /* add extension without ., skip leading . */
        }
    }
    filename = namebuff; /* successful name back to caller */

    // end_of_job:
    return filename; /* back with name or NULL=error */
}

/* ==========================================================================
 * Function:	isfexists ( filename )
 * Purpose:	check whether or not filename exists
 * --------------------------------------------------------------------------
 * Arguments:	filename (I)	pointer to null-terminated char string
 *				containing filename to check for
 * --------------------------------------------------------------------------
 * Returns:	( int )		1 = filename exists, 0 = not
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
int isfexists(char *filename) {
    FILE *fp = (isempty(filename) ? NULL : fopen(filename, "r")); /* try to fopen*/
    int status = 0;                                               /* init for non-existant filename */
    if (fp != NULL) {                                             /* but filename does exist */
        status = 1;                                               /* so flip status */
        fclose(fp);
    }              /* and close the file */
    return status; /* tell caller if we found filename*/
}

/* ==========================================================================
 * Function:	isdexists ( dirname )
 * Purpose:	check whether or not directory exists
 * --------------------------------------------------------------------------
 * Arguments:	dirname (I)	pointer to null-terminated char string
 *				containing directory name to check for
 * --------------------------------------------------------------------------
 * Returns:	( int )		1 = directory exists, 0 = not
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
int isdexists(char *dirname) {
    int status = 0;                                /* init for non-existant dirname */
    if (!isempty(dirname)) {                       /* must have directory name */
        char directory[512];                       /* local copy of dirname */
        DIR *dp = NULL;                            /* dp=opendir() opens directory */
        strcpy(directory, dirname);                /* start with name given by caller */
        if (!isthischar(lastchar(directory), "/")) /* no / at end of directory */
            strcat(directory, "/");                /* so add one ourselves */
        if ((dp = opendir(directory)) != NULL) {   /* dirname exists */
            status = 1;                            /* so flip status */
            closedir(dp);
        } /* and close the directory */
    }
    return status; /* tell caller if we found dirname */
}

/* ==========================================================================
 * Function:	whichpath ( program, nlocate )
 * Purpose:	determines the path to program with popen("which 'program'")
 * --------------------------------------------------------------------------
 * Arguments:	program (I)	pointer to null-terminated char string
 *				containing program whose path is desired
 *		nlocate (I/0)	addr of int containing NULL to ignore,
 *				or (addr of int containing) 0 to *not*
 *				use locate if which fails.  If non-zero,
 *				use locate if which fails, and return
 *				number of locate lines (if locate succeeds)
 * --------------------------------------------------------------------------
 * Returns:	( char * )	path to program, or NULL for any error
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
char *whichpath(char *program, int *nlocate) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    static char pathbuff[256];                                      /* buffer for returned path */
    char command[256];                                              /* which program */
    FILE *whichout = NULL;                                          /* which's stdout */
    int nchars = 0;                                                 /* read whichout one char at a time*/
    int pathchar;                                                   /* fgetc(whichout) */
    char *path = NULL;                                              /* either pathbuff or NULL=error */
    char *locatepath();                                             /* try locate if which fails */
    int islocate = (nlocate == NULL ? 1 : (*nlocate != 0 ? 1 : 0)); /* 1=use locate*/

    /* -------------------------------------------------------------------------
    Issue which command and read its output
    -------------------------------------------------------------------------- */
    /* --- check if which() suppressed --- */
    if (!ISWHICH) return NULL; /*not running which, return failure*/

    /* --- first construct the command --- */
    if (isempty(program)) goto end_of_job; /* no input */
    sprintf(command, "which %s", program); /* construct command */

    /* --- use popen() to invoke which --- */
    whichout = popen(command, "r");        /* issue which and capture stdout */
    if (whichout == NULL) goto end_of_job; /* failed */

    /* --- read the pipe one char at a time --- */
    while ((pathchar = fgetc(whichout)) != EOF) { /* get chars until eof */
        pathbuff[nchars] = (char)pathchar;        /* store the char */
        if (++nchars >= 255) break;
    }                          /* don't overflow buffer */
    pathbuff[nchars] = '\000'; /* null-terminate path */
    trimwhite(pathbuff);       /*remove leading/trailing whitespace*/

    /* --- pclose() waits for command to terminate --- */
    pclose(whichout);    /* finish */
    if (nchars > 0) {    /* found path with which */
        path = pathbuff; /* give user successful path */
        if (islocate && nlocate != NULL) *nlocate = 0;
    } /* signal we used which */

end_of_job:
    if (path == NULL) {                                    /* which failed to find program */
        if (islocate) path = locatepath(program, nlocate); /* and we're using locate, so try locate instead */
    }
    return path; /* give caller path to command */
}

/* ==========================================================================
 * Function:	locatepath ( program, nlocate )
 * Purpose:	determines the path to program with popen("locate 'program'")
 * --------------------------------------------------------------------------
 * Arguments:	program (I)	pointer to null-terminated char string
 *				containing program whose path is desired
 *		nlocate (O)	addr to int returning #lines locate|grep
 *				found, or NULL to ignore
 * --------------------------------------------------------------------------
 * Returns:	( char * )	path to program, or NULL for any error
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
char *locatepath(char *program, int *nlocate) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    static char pathbuff[256]; /* buffer for returned path */
    char command[256];         /* locate program | grep /program$ */
    FILE *locateout = NULL;    /* locate's stdout */
    char pathline[256];        /* read locateout one line at a time*/
    int nlines = 0;            /* #lines read */
    int linelen = 0;           /* choose shortest path */
    int pathlen = 9999;
    char *path = NULL; /* either pathbuff or NULL=error */

    /* -------------------------------------------------------------------------
    Issue locate|grep command and read its output
    -------------------------------------------------------------------------- */
    /* --- first construct the command --- */
    if (isempty(program)) goto end_of_job; /* no input */

    /*if ( strlen(program) < 2 ) goto end_of_job; */ /* might run forever */
    sprintf(command, "locate -q -r \"/%s$\" | grep \"bin\"", program);

    /* --- use popen() to invoke locate|grep --- */
    locateout = popen(command, "r");        /* issue locate and capture stdout */
    if (locateout == NULL) goto end_of_job; /* failed */

    /* --- read the pipe one line at a time --- */
    while (fgets(pathline, 255, locateout) != NULL) { /* get next line until eof */
        trimwhite(pathline);                          /*remove leading/trailing whitespace*/
        if ((linelen = strlen(pathline)) > 0) {       /* ignore empty lines */
            if (linelen < pathlen) {                  /* new shortest path */
                strcpy(pathbuff, pathline);           /* store shortest for caller */
                pathlen = linelen;
            } /* and reset new shortest length */
            nlines++;
        }
    } /* count another non-empty line */

    /* --- pclose() waits for command to terminate --- */
    pclose(locateout);                                 /* finish */
    if (pathlen > 0 && pathlen < 256) path = pathbuff; /*give user successful path*/
    if (nlocate != NULL) *nlocate = nlines;            /* and number of locate|grep hits*/

end_of_job:
    return path; /* give caller path to command */
}

/* ==========================================================================
 * Function:	presentwd ( int nsub )
 * Purpose:	determines pwd, nsub directories "up"
 * --------------------------------------------------------------------------
 * Arguments:	nsub (I)	int containing #subdirectories "up" to
 *				which path is wanted. 0 for pwd
 * --------------------------------------------------------------------------
 * Returns:	( char * )	path to pwd, or NULL for any error
 * --------------------------------------------------------------------------
 * Notes:     o	A forward / is always the last character
 * ======================================================================= */
char *presentwd(int nsub) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    static char pathbuff[256]; /* buffer for returned path */
    FILE *pwd = NULL;          /* pwd's stdout */
    char *path = NULL;         /* either pathbuff or NULL=error */
    int wdmethod = 1;          /* 1=getcwd(), 2=popen("pwd","r") */

    /* -------------------------------------------------------------------------
    issue getcwd() call
    -------------------------------------------------------------------------- */
    if (wdmethod == 1) {                                             /* --- getcwd(pathbuff,255) --- */
        if ((path = getcwd(pathbuff, 255)) == NULL) goto end_of_job; /* fail if unable to get cwd */
        trimwhite(pathbuff);
    } /*remove leading/trailing whitesapce*/

    /* -------------------------------------------------------------------------
    issue pwd command and read its output
    -------------------------------------------------------------------------- */
    if (wdmethod == 2) { /* --- popen("pwd","r") --- */
        /* --- use popen() to invoke pwd --- */
        if ((pwd = popen("pwd", "r")) == NULL) goto end_of_job; /* fail if unable to issue pwd and capture stdout */
        *pathbuff = '\000';                                     /* empty string in case fgets fails */

        /* --- read the pipe one line at a time --- */
        while (fgets(pathbuff, 255, pwd) != NULL) { /* get next line while not EOF */
            trimwhite(pathbuff);                    /*remove leading/trailing whitespace*/
            if (strlen(pathbuff) > 0) break;
        } /* try again if empty line */

        /* --- pclose() waits for command to terminate --- */
        pclose(pwd);
    } /* finish */

    /* -------------------------------------------------------------------------
    strip leading directories
    -------------------------------------------------------------------------- */
    if (nsub > 0) {                               /* caller wants path above pwd */
        if (lastchar(pathbuff) == '/')            /* have terminating / */
            *plastchar(pathbuff) = '\000';        /* strip it */
        while (nsub > 0) {                        /* still higher path wanted */
            char *slash = strrchr(pathbuff, '/'); /* last / on path */
            if (slash == NULL) break;             /* can't go up any higher */
            *slash = '\000';                      /* truncate "/highestdir" from path */
            nsub--;
        } /* we're now one path up */
    }
    if (lastchar(pathbuff) != '/') strcat(pathbuff, "/"); /* need terminating, so add it */

    /* --- back to caller --- */
    path = pathbuff; /* back to caller. we have found a suitable path */

end_of_job:
    return path; /* give caller path */
}

/* ==========================================================================
 * Function:	rrmdir ( path )
 * Purpose:	rm -r path
 * --------------------------------------------------------------------------
 * Arguments:	path (I)	pointer to null-terminated char string
 *				containing path to be rm'ed,
 *				relative to cwd.
 * --------------------------------------------------------------------------
 * Returns:	( int )		0 = success, -1 = error
 * --------------------------------------------------------------------------
 * Notes:     o	Based on Program 4.7, pages 108-111, in
 *		Advanced Programming in the UNIX Environment,
 *		W. Richard Stevens, Addison-Wesley 1992, ISBN 0-201-56317-7
 * ======================================================================= */
int rrmdir(char *path) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    DIR *directory = NULL;       /* opendir() opens path directory */
    struct dirent *entry = NULL; /* readdir() gives directory entry */
    struct stat st_info;         /* lstat() gives info about entry */
    char nextpath[512];
    char *pnext = NULL; /* recurse path/filename in dir */
    int status = -1;    /* init in case of any error */

    /* -------------------------------------------------------------------------
    Check file type at path
    -------------------------------------------------------------------------- */
    if (path != NULL) {        /* have path argument */
        if (*path != '\000') { /* and it's not an empty string */
            if (strcmp(path, ".") != 0 && strcmp(path, "..") != 0)
                status = lstat(path, &st_info); /* and also not "." or ".." ... what kind of file is at path? */
        }
    }
    if (status < 0) goto end_of_job; /* no kind of file found at path */

    /* -------------------------------------------------------------------------
    Recurse level if file is a directory
    -------------------------------------------------------------------------- */
    if (S_ISDIR(st_info.st_mode)) { /* have a directory */
        /* ---
         * Initialization
         * -------------- */
        /* --- init path for each file in directory --- */
        strcpy(nextpath, path);                  /* start with path from caller */
        pnext = nextpath + strlen(path);         /* ptr to '\000' at end of path */
        if (*(pnext - 1) != '/') *pnext++ = '/'; /* no trailing / at end of path, so add one */
        *pnext = '\000';                         /* null-terminate nextpath */

        /* --- open the directory at caller's path (with trailing /) --- */
        directory = opendir(nextpath); /* open the directory */

        /* ---
         * Run through all files in directory
         * ---------------------------------- */
        if (directory != NULL) {                                                           /* directory successfully opened */
            while ((entry = readdir(directory)) != NULL) {                                 /* next entry in directory; NULL signals end-of-directory */
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) { /* filename not "." or ".." */
                    strcpy(pnext, entry->d_name);                                          /* add filename to path */
                    status = rrmdir(nextpath);                                             /* recurse */
                }
            }
            closedir(directory);                                             /* close directory after last file */
            log_info(99, "[rrmdir] trying to remove directory: %s\n", path); /* show directory removed*/
        }
    } else {
        log_info(99, "[rrmdir] trying to remove file: %s\n", path); /* file isn't a directory, so show file removed  */
    }

    /* -------------------------------------------------------------------------
    Remove file or directory (keep it for debugging if msglevel>=99)
    -------------------------------------------------------------------------- */
    if (msglevel < 999 && !keep_work) { status = remove(path); /* remove file or directory */ }

end_of_job:
    return status;
}

/* ==========================================================================
 * Function:	readcachefile ( cachefile, buffer )
 * Purpose:	read cachefile into buffer
 * --------------------------------------------------------------------------
 * Arguments:	cachefile (I)	pointer to null-terminated char string
 *				containing full path to file to be read
 *		buffer (O)	pointer to unsigned char string
 *				returning contents of cachefile
 * --------------------------------------------------------------------------
 * Returns:	( int )		#bytes read (0 signals error)
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
int readcachefile(char *cachefile, unsigned char *buffer) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    FILE *cacheptr;               /* cachefile */
    unsigned char cachebuff[512]; /* bytes from cachefile */
    int buflen = 256;             /* #bytes we try to read from file */
    int nread = 0;                /* #bytes actually read from file */
    int maxbytes = MAXGIFSZ;      /* max #bytes returned in buffer */
    int nbytes = 0;               /* total #bytes read */

    /* -------------------------------------------------------------------------
    initialization
    -------------------------------------------------------------------------- */
    /* --- check that files opened okay --- */
    if ((cacheptr = fopen(cachefile, "rb")) == NULL) { goto end_of_job; }

    /* --- check that output buffer provided --- */
    if (buffer == (unsigned char *)NULL) goto end_of_job; /* no buffer */

    /* -------------------------------------------------------------------------
    read bytes from cachefile
    -------------------------------------------------------------------------- */
    while (1) {
        /* --- read bytes from cachefile --- */
        nread = fread(cachebuff, sizeof(unsigned char), buflen, cacheptr); /* read */
        if (nbytes + nread > maxbytes) nread = maxbytes - nbytes;          /* block too big for buffer, so truncate it */
        if (nread < 1) break;                                              /* no bytes left in cachefile */

        /* --- store bytes in buffer --- */
        memcpy(buffer + nbytes, cachebuff, nread); /* copy current block to buffer */

        /* --- ready to read next block --- */
        nbytes += nread;               /* bump total #bytes emitted */
        if (nread < buflen) break;     /* no bytes left in cachefile */
        if (nbytes >= maxbytes) break; /* avoid buffer overflow */
    }

end_of_job:
    if (cacheptr != NULL) fclose(cacheptr); /* close file if opened */
    return nbytes;                          /* back with #bytes emitted */
}

/* ==========================================================================
 * Function:	crc16 ( s )
 * Purpose:	16-bit crc of string s
 * --------------------------------------------------------------------------
 * Arguments:	s (I)		pointer to null-terminated char string
 *				whose crc is desired
 * --------------------------------------------------------------------------
 * Returns:	( int )		16-bit crc of s
 * --------------------------------------------------------------------------
 * Notes:     o	From Numerical Recipes in C, 2nd ed, page 900.
 * ======================================================================= */
int crc16(char *s) {
    /* -------------------------------------------------------------------------
    Compute the crc
    -------------------------------------------------------------------------- */
    unsigned short crc = 0;                /* returned crc */
    int ibit;                              /* for(ibit) eight one-bit shifts */
    while (!isempty(s)) {                  /* while there are still more chars*/
        crc = (crc ^ (*s) << 8);           /* add next char */
        for (ibit = 0; ibit < 8; ibit++) { /* generator polynomial */
            if (crc & 0x8000) {
                crc <<= 1;
                crc = crc ^ 4129;
            } else {
                crc <<= 1;
            }
        }
        s++; /* next xhar */
    }
    return (int)crc; /* back to caller with crc */
}

/* ==========================================================================
 * Function:	md5str ( instr )
 * Purpose:	returns null-terminated character string containing
 *		md5 hash of instr (input string)
 * --------------------------------------------------------------------------
 * Arguments:	instr (I)	pointer to null-terminated char string
 *				containing input string whose md5 hash
 *				is desired
 * --------------------------------------------------------------------------
 * Returns:	( char * )	ptr to null-terminated 32-character
 *				md5 hash of instr
 * --------------------------------------------------------------------------
 * Notes:     o	Other md5 library functions are included below.
 *		They're all taken from Christophe Devine's code,
 *		which (as of 04-Aug-2004) is available from
 *		     http://www.cr0.net:8040/code/crypto/md5/
 *	      o	The P,F,S macros in the original code are replaced
 *		by four functions P1()...P4() to accommodate a problem
 *		with Compaq's vax/vms C compiler.
 * ======================================================================= */
char *md5str(char *instr) {
    static char outstr[64];
    unsigned char md5sum[16];
    md5_context ctx;
    int j;
    md5_starts(&ctx);
    md5_update(&ctx, (uint8 *)instr, strlen(instr));
    md5_finish(&ctx, md5sum);
    for (j = 0; j < 16; j++) sprintf(outstr + j * 2, "%02x", md5sum[j]);
    outstr[32] = '\000';
    return outstr;
}

/* ==========================================================================
 * Functions:	int  unescape_url ( char *url )
 *		char x2c ( char *what )
 * Purpose:	unescape_url replaces 3-character sequences %xx in url
 *		    with the single character represented by hex xx.
 *		x2c returns the single character represented by hex xx
 *		    passed as a 2-character sequence in what.
 * --------------------------------------------------------------------------
 * Arguments:	url (I)		char * containing null-terminated
 *				string with embedded %xx sequences
 *				to be converted.
 *		what (I)	char * whose first 2 characters are
 *				interpreted as ascii representations
 *				of hex digits.
 * --------------------------------------------------------------------------
 * Returns:	( int )		length of url string after replacements.
 *		( char )	x2c returns the single char
 *				corresponding to hex xx passed in what.
 * --------------------------------------------------------------------------
 * Notes:     o	These two functions were taken from util.c in
 *   ftp://ftp.ncsa.uiuc.edu/Web/httpd/Unix/ncsa_httpd/cgi/ncsa-default.tar.Z
 *	      o	Added ^M,^F,etc, and +'s to blank xlation 0n 01-Oct-06
 * ======================================================================= */
int unescape_url(char *url) {
    int x = 0;
    int y = 0;
    char x2c();
    static char *hex = "0123456789ABCDEFabcdef";
    int xlatectrl = 1;
    int xlateblank = 0 /* 1 */;

    /* ---
     * first xlate ctrl chars and +'s to blanks
     * ---------------------------------------- */
    if (xlatectrl || xlateblank) { /*xlate ctrl chars and +'s to blanks*/
        char *ctrlchars = (!xlateblank ? "\n\t\v\b\r\f\a\015" : (!xlatectrl ? "+" : "+\n\t\v\b\r\f\a\015"));
        int urllen = strlen(url); /* total length of url string */

        /* --- replace ctrlchars with blanks --- */
        while ((x = strcspn(url, ctrlchars)) < urllen) { url[x] = ' '; /* found a ctrlchar, replace with blank */ }

        /* --- get rid of leading/trailing ctrlchars (now whitespace) --- */
        trimwhite(url); /*remove leading/trailing whitespace*/
    }

    /* ---
     * now xlate %nn to corresponding char (original ncsa code)
     * -------------------------------------------------------- */
    for (x = y = 0; url[y]; ++x, ++y) {
        if ((url[x] = url[y]) == '%') {
            if (isthischar(url[y + 1], hex) && isthischar(url[y + 2], hex)) {
                url[x] = x2c(&url[y + 1]);
                y += 2;
            }
        }
    }
    url[x] = '\000';
    return x;
}

char x2c(char *what) {
    char digit;
    digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A') + 10 : (what[0] - '0'));
    digit *= 16;
    digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A') + 10 : (what[1] - '0'));
    return digit;
}

/* ==========================================================================
 * Function:	timelimit ( char *command, int killtime )
 * Purpose:	Issues a system(command) call, but throttles command
 *		after killtime seconds if it hasn't already completed.
 * --------------------------------------------------------------------------
 * Arguments:	command (I)	char * to null-terminated string
 *				containing system(command) to be executed
 *		killtime (I)	int containing maximum #seconds
 *				to allow command to run
 * --------------------------------------------------------------------------
 * Returns:	( int )		return status from command,
 *				or -1 for any error.
 * --------------------------------------------------------------------------
 * Notes:     o	The timelimit() code is adapted from
 *		   http://devel.ringlet.net/sysutils/timelimit/
 *		Compile with -DTIMELIMIT=\"$(which timelimit)\" to use an
 *		installed copy of timelimit rather than this built-in code.
 *	      o if symbol ISCOMPILETIMELIMIT is false, a stub function
 *		that just issues system(command) is compiled instead.
 * ======================================================================= */
#if !ISCOMPILETIMELIMIT
int timelimit(char *command, int killtime) {
    if (isempty(command))                    /* no command given */
        return (killtime == -99 ? 991 : -1); /* return -1 or stub identifier */
    return system(command);
} /* just issue system(command) */
#else

/* --- signal handlers --- */
void sigchld(int sig) { fdone = 1; }
void sigalrm(int sig) { falarm = 1; }
void sighandler(int sig) {
    sigcaught = sig;
    fsig = 1;
}

int timelimit(char *command, int killtime) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    pid_t pid = 0;
    int killsig = (int)(SIGKILL);
    int setsignal();
    int status = -1;

    /* -------------------------------------------------------------------------
    check args
    -------------------------------------------------------------------------- */
    if (isempty(command)) return (killtime == -99 ? 992 : -1); /* no command given, return -1 or built-in identifier */
    if (killtime < 1) return system(command);                  /* throttling disabled */
    if (killtime > 999) killtime = 999;                        /* default maximum to 999 seconds */

    /* -------------------------------------------------------------------------
    install signal handlers
    -------------------------------------------------------------------------- */
    fdone = falarm = fsig = sigcaught = 0;
    if (setsignal(SIGALRM, sigalrm) < 0) return -1;
    if (setsignal(SIGCHLD, sigchld) < 0) return -1;
    if (setsignal(SIGTERM, sighandler) < 0) return -1;
    if (setsignal(SIGHUP, sighandler) < 0) return -1;
    if (setsignal(SIGINT, sighandler) < 0) return -1;
    if (setsignal(SIGQUIT, sighandler) < 0) return -1;

    /* -------------------------------------------------------------------------
    fork off the child process
    -------------------------------------------------------------------------- */
    fflush(NULL);                      /* flush all buffers before fork */
    if ((pid = fork()) < 0) return -1; /* failed to fork */
    if (pid == 0) {                    /* child process... */
        status = system(command);      /* ...submits command */
        _exit(status);
    } /* and _exits without user cleanup */

    /* -------------------------------------------------------------------------
    parent process sleeps for allowed time
    -------------------------------------------------------------------------- */
    alarm(killtime);
    while (!(fdone || falarm || fsig)) pause();
    alarm(0);

    /* -------------------------------------------------------------------------
    send kill signal if child hasn't completed command
    -------------------------------------------------------------------------- */
    if (fsig) return -1;            /* some other signal stopped child */
    if (!fdone) kill(pid, killsig); /* not done, so kill it */

    /* -------------------------------------------------------------------------
    return status of child pid
    -------------------------------------------------------------------------- */
    if (waitpid(pid, &status, 0) == -1) return -1; /* can't get status */
    if (1) return status;                          /* return status to caller */
    #if 0 /* interpret status */
    if (WIFEXITED(status))
    	return WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
    	return WTERMSIG(status) + 128;
    else
    	return EX_OSERR;
    #endif
}

int setsignal(int sig, void (*handler)(int)) {
    #ifdef HAVE_SIGACTION
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    act.sa_flags = 0;
        #ifdef SA_NOCLDSTOP
    act.sa_flags |= SA_NOCLDSTOP;
        #endif
    if (sigaction(sig, &act, NULL) < 0) return -1;
    #else
    if (signal(sig, handler) == SIG_ERR) return -1;
    #endif
    return 0;
}
#endif /* ISCOMPILETIMELIMIT */

/* ==========================================================================
 * Function:	getdirective(string, directive, iscase, isvalid, nargs, args)
 * Purpose:	Locates the first \directive{arg1}...{nargs} in string,
 *		returns arg1...nargs in args[],
 *		and removes \directive and its args from string.
 * --------------------------------------------------------------------------
 * Arguments:	string (I/0)	char * to null-terminated string from which
 *				the first occurrence of \directive will be
 *				interpreted and removed
 *		directive (I)	char * to null-terminated string containing
 *				the \directive to be interpreted in string
 *		iscase (I)	int containing 1 if match of \directive
 *				in string should be case-sensitive,
 *				or 0 if match is case-insensitive.
 *		isvalid (I)	int containing validity check option:
 *				0=no checks, 1=must be numeric
 *		nargs (I)	int containing (maximum) number of
 *				{args} following \directive, or 0 if none.
 *		args (O)	void * interpreted as (char *) if nargs=1
 *				to return the one and only arg,
 *				or interpreted as (char **) if nargs>1
 *				to array of returned arg strings
 * --------------------------------------------------------------------------
 * Returns:	( char * )	ptr to first char after removed \directive, or
 *				NULL if \directive not found, or any error.
 * --------------------------------------------------------------------------
 * Notes:     o	If optional [arg]'s are found, they're stored in the global
 *		optionalargs[] buffer, and the noptional counter is bumped.
 *	      o	set global argformat's decimal digits for each arg,
 *		e.g., 1357... means 1 for 1st arg, 3 for 2nd, 5 for 3rd, etc.
 *		0 for an arg is the default format (i.e., argformat=0),
 *		and means it's formatted as a LaTeX {arg} or [arg].
 *		1 for an arg means arg terminated by first non-alpha char
 *		2 means arg terminated by {   (e.g., as for /def)
 *		8 means arg terminated by first whitespace char
 * ======================================================================= */
char *getdirective(char *string, char *directive, int iscase, int isvalid, int nargs, void *args) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    int iarg = -1;       /* init to signal error */
    char *pfirst = NULL; /* ptr to 1st char of directive */
    char *plast = NULL;  /* ptr past last char of last arg */
    char *plbrace = NULL;
    char *prbrace = NULL; /* ptr to left,right brace of arg */
    int fldlen = 0;       /* #chars between { and } delims */
    char argfld[512];     /* {arg} characters */
    int nfmt = 0;         /* {arg} format */
    // int isnegfmt = 0;
    int argfmt[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};         /* argformat digits */
    int gotargs = (args == NULL ? 0 : 1);                /* true if args array supplied */
    int isdalpha = 1;                                    /* true if directive ends with alpha*/
    char *strpspn(char *s, char *reject, char *segment); /*non-() not in rej*/

    /* -------------------------------------------------------------------------
    Find first \directive in string
    -------------------------------------------------------------------------- */
    noptional = 0; /* no optional [args] yet */
    for (iarg = 0; iarg < 8; iarg++) { *optionalargs[iarg] = '\000'; /* for each one, re-init optional [arg] buffer */ }
    if (argformat != 0) {      /* have argformat */
        int myfmt = argformat; /* local copy */
        if (myfmt < 0) {
            // isnegfmt = 1;
            myfmt = -myfmt;
        }                               /* check sign */
        while (myfmt > 0 && nfmt < 9) { /* have more format digits */
            argfmt[nfmt] = myfmt % 10;  /* store low-order decimal digit */
            myfmt /= 10;                /* and shift it out */
            nfmt++;
        } /* count another format digit */
    }
    if (isempty(directive)) goto end_of_job;                             /* no input \directive given */
    if (!isalpha((int)(directive[strlen(directive) - 1]))) isdalpha = 0; /*not alpha*/
    pfirst = string;                                                     /* start at beginning of string */
    while (1) {                                                          /* until we find \directive */
        if (!isempty(pfirst)) {                                          /* still have string from caller */
            pfirst = (iscase > 0 ? strstr(pfirst, directive)
                                 : strcasestr(pfirst, directive)); /* the ptr to 1st char of directive. case-sensistive match vs case-insensitive match */
        }
        if (isempty(pfirst)) { /* \directive not found in string */
            pfirst = NULL;     /* signal \directive not found */
            goto end_of_job;
        }                                                /* quit, signalling error to caller*/
        plast = pfirst + strlen(directive);              /*ptr to fist char past directive*/
        if (!isdalpha || !isalpha((int)(*plast))) break; /* found \directive */
        pfirst = plast;                                  /* keep looking */
        plast = NULL;                                    /* reset plast */
    }
    if (nargs < 0) {    /* optional [arg] may be present */
        nargs = -nargs; /* flip sign back to positive */
        // noptional = 1;                                 /* and set optional flag */
    }

    /* -------------------------------------------------------------------------
    Get arguments
    -------------------------------------------------------------------------- */
    iarg = 0;                                                        /* no args yet */
    if (nargs > 0)                                                   /* \directive has {args} */
        while (iarg < nargs + noptional) {                           /* get each arg */
            int karg = iarg - noptional;                             /* non-optional arg index */
            int kfmt = (nfmt <= karg ? 0 : argfmt[nfmt - karg - 1]); /* arg format digit */

            /* --- find left { and right } arg delimiters --- */
            plbrace = plast;             /*ptr to fist char past previous arg*/
            skipwhite(plbrace);          /* push it to first non-white char */
            if (isempty(plbrace)) break; /* reached end of string */

            /* --- check LaTeX for single-char arg or {arg} or optional [arg] --- */
            if (kfmt == 0) { /* interpret LaTeX {arg} format */
                if (!isthischar(*plbrace, (iarg == optionalpos + noptional ? "{[" : "{"))) {
                    /* --- single char argument --- */
                    plast = plbrace + 1;  /* first char after single-char arg*/
                    argfld[0] = *plbrace; /* arg field is this one char */
                    argfld[1] = '\000';
                }      /* null-terminate field */
                else { /* have {arg} or optional [arg] */
                    /* note: to use for validation, need robust {} match like strpspn() */
                    if ((prbrace = strchr(plbrace, (*plbrace == '{' ? '}' : ']'))) == NULL) { /* right '}' or ']' */
                        break;                                                                /*and no more args if no right brace*/
                    }
                    if (1) {                                    /* true to use strpspn() */
                        prbrace = strpspn(plbrace, NULL, NULL); /* push to matching } or ] */
                    }
                    plast = prbrace + 1; /* first char after right brace */

                    /* --- extract arg field between { and } delimiters --- */
                    fldlen = (int)(prbrace - plbrace) - 1;   /* #chars between { and } delims*/
                    if (fldlen >= 256) fldlen = 255;         /* don't overflow argfld[] buffer */
                    if (fldlen > 0) {                        /* have chars in field */
                        memcpy(argfld, plbrace + 1, fldlen); /*copy field chars to local buffer*/
                    }
                    argfld[fldlen] = '\000'; /* and null-terminate field */
                    trimwhite(argfld);       /* trim whitespace from argfld */
                }
            }

            /* --- check plain TeX for arg terminated by whitespace --- */
            if (kfmt != 0) {                 /* interpret plain TeX arg format */
                char *parg = NULL;           /* ptr into arg, used as per kfmt */
                plast = plbrace;             /* start at first char of arg */
                if (*plast == '\\') plast++; /* skip leading \command backslash */

                /* --- interpret arg according to its format --- */
                switch (kfmt) {
                    case 1:
                    default: skipcommand(plast); break; /* push ptr to non-alpha char */
                    case 2:
                        parg = strchr(plast, '{'); /* next arg always starts with { */
                        if (parg != NULL) plast = parg;
                        else
                            plast++; /* up to { or 1 char */
                        break;
                    case 8: findwhite(plast); break; /*ptr to whitespace after last char*/
                }

                /* --- extract arg field --- */
                fldlen = (int)(plast - plbrace);                 /* #chars between in field */
                if (fldlen >= 256) fldlen = 255;                 /* don't overflow argfld[] buffer */
                if (fldlen > 0) memcpy(argfld, plbrace, fldlen); /* have chars in field, copy field chars to local buffer*/
                argfld[fldlen] = '\000';                         /* and null-terminate field */
                if (1) { trimwhite(argfld); }                    /* trim whitespace from argfld */
            }
            if (isvalid != 0) {                                      /* argfld[] validity check desired */
                if (isvalid == 1) {                                  /* numeric check wanted */
                    int validlen = strspn(argfld, " +-.0123456789"); /*very simple check*/
                    argfld[validlen] = '\000';
                } /* truncate invalid chars */
            }

            /* --- store argument field in caller's array --- */
            if (kfmt == 0 && *plbrace == '[') { /*store [arg] as optionalarg instead*/
                if (noptional < 8) {            /* don't overflow our buffer */
                    strninit(optionalargs[noptional], argfld, 254);
                } /*copy to optionalarg*/
                noptional++;
            } else {           /*{args} returned in caller's array*/
                if (gotargs) { /*caller supplied address or array*/
                    if (nargs < 2) {
                        strcpy((char *)args, argfld);         /*just one arg, so it's an address. copy arg field there */
                    } else {                                  /* >1 arg, so it's a ptr array */
                        char *argptr = ((char **)args)[karg]; /* arg ptr in array of ptrs */
                        if (argptr != NULL) {                 /* array has iarg-th address */
                            strcpy(argptr, argfld);           /* so copy arg field there */
                        } else {
                            gotargs = 0;
                        }
                    }
                } /* no more addresses in array */
            }

            /* --- completed this arg --- */
            iarg++; /* bump arg count */
        }

/* -------------------------------------------------------------------------
Back to caller
-------------------------------------------------------------------------- */
end_of_job:
    if (1) argformat = 0;                  /* always/never reset global arg */
    if (1) optionalpos = 0;                /* always/never reset global arg */
    if (pfirst != NULL && plast != NULL) { /* have directive field delims */
        strsqueeze(pfirst, ((int)(plast - pfirst)));
    }              /* squeeze out directive */
    return pfirst; /* ptr to 1st char after directive */
}

/* ==========================================================================
 * Function:	mathprep ( expression )
 * Purpose:	preprocessor for mathTeX input, e.g.,
 *		(a) removes leading/trailing $'s from $$expression$$
 *		(b) xlates &html; special chars to equivalent latex
 *		(c) xlates &#nnn; special chars to equivalent latex
 *		Should only be called once (after unescape_url())
 * --------------------------------------------------------------------------
 * Arguments:	expression (I/O) char * to first char of null-terminated
 *				string containing mathTeX/LaTeX expression,
 *				and returning preprocessed string
 * --------------------------------------------------------------------------
 * Returns:	( char * )	ptr to input expression,
 *				or NULL for any parsing error.
 * --------------------------------------------------------------------------
 * Notes:     o	The ten special symbols  $ & % # _ { } ~ ^ \  are reserved
 *		for use in LaTeX commands.  The corresponding directives
 *		\$ \& \% \# \_ \{ \}  display the first seven, respectively,
 *		and \backslash displays \.  It's not clear to me whether
 *		or not mathprep() should substitute the displayed symbols,
 *		e.g., whether &#36; better xlates to \$ or to $.
 *		Right now, it's the latter.
 * ======================================================================= */
char *mathprep(char *expression) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    int isym = 0, inum = 0;                                      /* symbols[], numbers[] indexes */
    char *strchange();                                           /* change leading chars of string */
    char *strwstr();                                             /*use strwstr() instead of strstr()*/
    int strreplace();                                            /* substitute/from/to/ */
    int ndollars = 0;                                            /* #leading/trailing $$...$$'s */
    int explen = (isempty(expression) ? 0 : strlen(expression)); /*#input chars*/

    /* ---
     * html special/escape chars converted to latex equivalents
     * -------------------------------------------------------- */
    char *htmlsym = NULL; /* symbols[isym].html */
    static struct {
        char *html;
        char *termchar;
        char *latex;
    } symbols[] = {
    /* ---------------------------------------------------------
     * user-supplied newcommands (different than -DNEWCOMMAND)
     * --------------------------------------------------------- */
#ifdef NEWCOMMANDS /* -DNEWCOMMANDS=\"filename.h\" */
    #include NEWCOMMANDS
#endif
        // clang-format off
        /* -----------------------------------------
         html char     termchar  LaTeX equivalent...
         ------------------------------------------- */
        {"&quot",      ";",      "\""}, /* &quot; is first, &#034; */
        {"&amp",       ";",      "&"},
        {"&lt",        ";",      "<"},
        {"&gt",        ";",      ">"},
        {"&backslash", ";",      "\\"},
        {"&nbsp",      ";",      " " /*"~"*/},
        {"&iexcl",     ";",      "{\\mbox{!`}}"},
        {"&brvbar",    ";",      "|"},
        {"&plusmn",    ";",      "\\pm"},
        {"&sup2",      ";",      "{{}^2}"},
        {"&sup3",      ";",      "{{}^3}"},
        {"&micro",     ";",      "\\mu"},
        {"&sup1",      ";",      "{{}^1}"},
        {"&frac14",    ";",      "{\\frac14}"},
        {"&frac12",    ";",      "{\\frac12}"},
        {"&frac34",    ";",      "{\\frac34}"},
        {"&iquest",    ";",      "{\\mbox{?`}}"},
        {"&Acirc",     ";",      "{\\rm\\hat A}"},
        {"&Atilde",    ";",      "{\\rm\\tilde A}"},
        {"&Auml",      ";",      "{\\rm\\ddot A}"},
        {"&Aring",     ";",      "{\\overset{o}{\\rm A}}"},
        {"&atilde",    ";",      "{\\rm\\tilde a}"},
        {"&yuml",      ";",      "{\\rm\\ddot y}"}, /* &yuml; is last, &#255; */
        {"&#",         ";",      "{[\\&\\#nnn?]}"}, /* all other &#nnn's */
        {"< br >",     NULL,     " \000" /*"\\\\"*/},
        {"< br / >",   NULL,     " \000" /*"\\\\"*/},
        {"< dd >",     NULL,     " \000"},
        {"< / dd >",   NULL,     " \000"},
        {"< dl >",     NULL,     " \000"},
        {"< / dl >",   NULL,     " \000"},
        {"< p >",      NULL,     " \000"},
        {"< / p >",    NULL,     " \000"},
        /* ---------------------------------------
         garbage  termchar  LaTeX equivalent...
        --------------------------------------- */
        {"< tex >",    NULL,     "\000"},
        {"< / tex >",  NULL,     "\000"},
        {NULL,         NULL,     NULL}
        // clang-format on
    };

    /* ---
     * html &#nn chars converted to latex equivalents
     * ---------------------------------------------- */
    int htmlnum = 0; /* numbers[inum].html */
    static struct {
        int html;
        char *latex;
    } numbers[] = {
        // clang-format off
        /* ---------------------------------------
         html num  LaTeX equivalent...
         --------------------------------------- */
        {9,        " "},         /* horizontal tab */
        {10,       " "},         /* line feed */
        {13,       " "},         /* carriage return */
        {32,       " "},         /* space */
        {33,       "!"},         /* exclamation point */
        {34,       "\""},        /* &quot; */
        {35,       "#"},         /* hash mark */
        {36,       "$"},         /* dollar */
        {37,       "%"},         /* percent */
        {38,       "&"},         /* &amp; */
        {39,       "\'"},        /* apostrophe (single quote) */
        {40,       ")"},         /* left parenthesis */
        {41,       ")"},         /* right parenthesis */
        {42,       "*"},         /* asterisk */
        {43,       "+"},         /* plus */
        {44,       ","},         /* comma */
        {45,       "-"},         /* hyphen (minus) */
        {46,       "."},         /* period */
        {47,       "/"},         /* slash */
        {58,       ":"},         /* colon */
        {59,       ";"},         /* semicolon */
        {60,       "<"},         /* &lt; */
        {61,       "="},         /* = */
        {62,       ">"},         /* &gt; */
        {63,       "\?"},        /* question mark */
        {64,       "@"},         /* commercial at sign */
        {91,       "["},         /* left square bracket */
        {92,       "\\"},        /* backslash */
        {93,       "]"},         /* right square bracket */
        {94,       "^"},         /* caret */
        {95,       "_"},         /* underscore */
        {96,       "`"},         /* grave accent */
        {123,      "{"},         /* left curly brace */
        {124,      "|"},         /* vertical bar */
        {125,      "}"},         /* right curly brace */
        {126,      "~"},         /* tilde */
        {160,      "~"},         /* &nbsp; (use tilde for latex) */
        {166,      "|"},         /* &brvbar; (broken vertical bar) */
        {173,      "-"},         /* &shy; (soft hyphen) */
        {177,      "{\\pm}"},    /* &plusmn; (plus or minus) */
        {215,      "{\\times}"}, /* &times; (plus or minus) */
        {-999,     NULL}
        // clang-format on
    };

    /* -------------------------------------------------------------------------
    initialization
    -------------------------------------------------------------------------- */
    if (explen < 1) goto end_of_job; /* no input expression supplied */

    /* -------------------------------------------------------------------------
    remove leading/trailing $$...$$'s and set mathmode accordingly
    -------------------------------------------------------------------------- */
    /* --- count and remove leading/trailing $'s from $$expression$$ --- */
    while (explen > 2)                                               /* don't exhaust entire expression */
        if (expression[0] == '$' && expression[explen - 1] == '$') { /* have leading and trailing $ chars */
            explen -= 2;                                             /* remove leading and trailing $'s */
            strsqueeze(expression, 1);                               /* squeeze out leading $ */
            expression[explen] = '\000';                             /* and terminate at trailing $ */
            ndollars++;
        } /* count another dollar */
        else
            break; /* no more $...$ pairs */

    /* --- set mathmode for input $$expression$$ --- */
    if (ndollars > 0)                    /* have $$expression$$ input */
        switch (ndollars) {              /* set mathmode accordingly */
            case 1: mathmode = 1; break; /* $...$ is \textstyle */
            case 2: mathmode = 0; break; /* $$...$$ is \displaystyle */
            case 3: mathmode = 2; break; /* $$$...$$$ is \parstyle */
            default: break;
        } /* I have no idea what you want */

    /* --- check for input \[expression\] if no $$'s --- */
    if (ndollars < 1) {                                                                                  /* not an $$expression$$ */
        if (explen > 4) {                                                                                /* long enough to contain \[...\] */
            if (strncmp(expression, "\\[", 2) == 0 && strncmp(expression + explen - 2, "\\]", 2) == 0) { /* so check for leading \[ and trailing \] */
                explen -= 4;                                                                             /* remove leading/trailing \[...\] */
                strsqueeze(expression, 2);                                                               /* squeeze out leading \[ */
                expression[explen] = '\000';                                                             /* and terminate at trailing \] */
                mathmode = 0;
            } /* set \displaystyle */
        }
    }

    /* -------------------------------------------------------------------------
    run thru table, converting all occurrences of each html to latex equivalent
    -------------------------------------------------------------------------- */
    for (isym = 0; (htmlsym = symbols[isym].html) != NULL; isym++) {
        char *htmlterm = symbols[isym].termchar;         /* &symbol; terminator */
        char *latexsym = symbols[isym].latex;            /* latex replacement */
        char errorsym[256];                              /* error message replacement */
        int htmllen = strlen(htmlsym);                   /* length of html token */
        int wstrlen = htmllen;                           /* token length found by strwstr() */
        int latexlen = strlen(latexsym);                 /* length of latex replacement */
        int isstrwstr = 1;                               /* true to use strwstr() */
        int istag = (isthischar(*htmlsym, "<") ? 1 : 0); /* html <tag> starts with <*/
        int isamp = (isthischar(*htmlsym, "&") ? 1 : 0); /* html char starts with & */
        char wstrwhite[128] = "i";                       /* whitespace chars for strwstr() */
        char *expptr = expression;                       /* ptr within expression */
        char *tokptr = NULL;                             /* ptr to token found in expression*/

        /* ---
         * xlate every occurrence of current htmlsym command
         * ------------------------------------------------- */
        skipwhite(htmlsym);                                            /*skip any bogus leading whitespace*/
        htmllen = wstrlen = strlen(htmlsym);                           /*reset length of html token and...*/
        istag = (isthischar(*htmlsym, "<") ? 1 : 0);                   /* ...html <tag> starts with < */
        isamp = (isthischar(*htmlsym, "&") ? 1 : 0);                   /* ...html char starts with & */
        if (isamp) isstrwstr = 0;                                      /* don't use strwstr() for &char */
        if (istag) {                                                   /* use strwstr() for <tag> */
            isstrwstr = 1;                                             /* make sure flag is set true */
            if (!isempty(htmlterm)) strninit(wstrwhite, htmlterm, 64); /* got a term char string, interpret it as whitespace arg */
            htmlterm = NULL;
        } /* rather than as a terminater */
        while ((tokptr = (!isstrwstr ? strstr(expptr, htmlsym) : strwstr(expptr, htmlsym, wstrwhite, &wstrlen))) != NULL) {
            /* strtsr or strwstr. looks for another htmlsym. if null, we found another htmlsym */
            char termchar = *(tokptr + wstrlen);                      /* char terminating html sequence */
            char prevchar = (tokptr == expptr ? ' ' : *(tokptr - 1)); /* char preceding tok*/
            int toklen = wstrlen;                                     /* tot length of token+terminator */

            /* --- ignore match if leading char escaped (not really a match) --- */
            if (isthischar(prevchar, "\\")) { /* inline symbol escaped */
                expptr = tokptr + toklen;     /*just resume search after literal*/
                continue;
            } /* but don't replace it */

            /* --- ignore match if it's just a prefix of a longer expression --- */
            if (!istag) {                     /*<br>-type command can't be prefix*/
                if (isalpha((int)termchar)) { /*we just have prefix of longer sym*/
                    expptr = tokptr + toklen; /* just resume search after prefix */
                    continue;
                } /* but don't replace it */
            }

            /* --- check for &# prefix signalling &#nnn --- */
            if (strcmp(htmlsym, "&#") == 0) { /* replacing special &#nnn; chars */
                /* --- accumulate chars comprising number following &# --- */
                char anum[32];                          /* chars comprising number after &# */
                inum = 0;                               /* no chars accumulated yet */
                while (termchar != '\000') {            /* don't go past end-of-string */
                    if (!isdigit((int)termchar)) break; /* and don't go past digits */
                    if (inum > 10) break;               /* some syntax error in expression */
                    anum[inum] = termchar;              /* accumulate this digit */
                    inum++;
                    toklen++; /* bump field length, token length */
                    termchar = *(tokptr + toklen);
                }                    /* char terminating html sequence */
                anum[inum] = '\000'; /* null-terminate anum */

                /* --- look up &#nnn in number[] table --- */
                htmlnum = atoi(anum);                             /* convert anum[] to an integer */
                latexsym = errorsym;                              /* init latex replacement for error*/
                strninit(latexsym, symbols[isym].latex, 128);     /* init error message */
                strreplace(latexsym, "nnn", anum, 1, 1);          /* place actual &#num in message*/
                latexlen = strlen(latexsym);                      /* and length of latex replacement */
                for (inum = 0; numbers[inum].html >= 0; inum++) { /* run thru numbers[] */
                    if (htmlnum == numbers[inum].html) {          /* till we find a match */
                        latexsym = numbers[inum].latex;           /* latex replacement */
                        latexlen = strlen(latexsym);              /* length of latex replacement */
                        break;
                    } /* no need to look any further */
                }
            }

            /* --- check for optional ; terminator after &symbol --- */
            if (!istag) {                                                   /* html <tag> doesn't have term. */
                if (termchar != '\000') {                                   /* token not at end of expression */
                    if (!isempty(htmlterm)) {                               /* sequence may have terminator */
                        toklen += (isthischar(termchar, htmlterm) ? 1 : 0); /*add terminator*/
                    }
                }
            }

            /* --- replace html command with latex equivalent --- */
            strchange(toklen, tokptr, latexsym); /* replace html symbol with latex */
            expptr = tokptr + latexlen;          /* resume search after replacement */
        }
    }

    /* -------------------------------------------------------------------------
    back to caller with preprocessed expression
    -------------------------------------------------------------------------- */
    trimwhite(expression);                                             /*remove leading/trailing whitespace*/
    log_info(98, "[mathprep] processed expression: %s\n", expression); /*show preprocessed expression*/

end_of_job:
    return expression;
}

/* ==========================================================================
 * Function:	strwstr (char *string, char *substr, char *white, int *sublen)
 * Purpose:	Find first substr in string, but wherever substr contains
 *		a whitespace char (in white), string may contain any number
 *		(including 0) of whitespace chars. If white contains I or i,
 *		then match is case-insensitive (and I,i _not_ whitespace).
 * --------------------------------------------------------------------------
 * Arguments:	string (I)	char * to null-terminated string in which
 *				first occurrence of substr will be found
 *		substr (I)	char * to null-terminated string containing
 *				"template" that will be searched for
 *		white (I)	char * to null-terminated string containing
 *				whitespace chars.  If NULL or empty, then
 *				" \t\n\r\f\v" (see #define WHITESPACE) used.
 *				If white contains I or i, then match is
 *				case-insensitive (and I,i _not_ considered
 *				whitespace).
 *		sublen (O)	address of int returning "length" of substr
 *				found in string (which may be longer or
 *				shorter than substr itself).
 * --------------------------------------------------------------------------
 * Returns:	( char * )	ptr to first char of substr in string
 *				or NULL if not found or for any error.
 * --------------------------------------------------------------------------
 * Notes:     o	Wherever a single whitespace char appears in substr,
 *		the corresponding position in string may contain any
 *		number (including 0) of whitespace chars, e.g.,
 *		string="abc   def" and string="abcdef" both match
 *		substr="c d" at offset 2 of string.
 *	      o	If substr="c  d" (two spaces between c and d),
 *		then string must have at least one space, so now "abcdef"
 *		doesn't match.  In general, the minimum number of spaces
 *		in string is the number of spaces in substr minus 1
 *		(so 1 space in substr permits 0 spaces in string).
 *	      o	Embedded spaces are counted in sublen, e.g.,
 *		string="c   d" (three spaces) matches substr="c d"
 *		with sublen=5 returned.  But string="ab   c   d" will
 *		also match substr="  c d" returning sublen=5 and
 *		a ptr to the "c".  That is, the mandatory preceding
 *		space is _not_ counted as part of the match.
 *		But all the embedded space is counted.
 *		(An inconsistent bug/feature is that mandatory
 *		terminating space is counted.)
 *	      o	Moreover, string="c   d" matches substr="  c d", i.e.,
 *		the very beginning of a string is assumed to be preceded
 *		by "virtual blanks".
 * ======================================================================= */
char *strwstr(char *string, char *substr, char *white, int *sublen) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    char *psubstr = substr; /*ptr to current char in substr,str*/
    char *pstring = string;
    char *pfound = (char *)NULL;          /*ptr to found substr back to caller*/
    char *pwhite = NULL, whitespace[256]; /* callers white whithout i,I */
    int iscase = 1;                       /* case-insensitive if i,I in white*/
    int foundlen = 0;                     /* length of substr found in string*/
    int nstrwhite = 0;                    /* #leading white chars in str,sub */
    int nsubwhite = 0;
    int nminwhite = 0; /* #mandatory leading white in str */
    int nstrchars = 0; /* #non-white chars to be matched */
    int nsubchars = 0;
    int isncmp = 0; /*strncmp() or strncasecmp() result*/

    /* -------------------------------------------------------------------------
    Initialization
    -------------------------------------------------------------------------- */
    /* --- set up whitespace --- */
    strcpy(whitespace, WHITESPACE);                              /*default if no user input for white*/
    if (white != NULL)                                           /*user provided ptr to white string*/
        if (*white != '\000') {                                  /*and it's not just an empty string*/
            strcpy(whitespace, white);                           /* so use caller's white spaces */
            while ((pwhite = strchr(whitespace, 'i')) != NULL) { /* have an embedded i */
                iscase = 0;
                strsqueeze(pwhite, 1);
            }                                                    /*set flag and squeeze it out*/
            while ((pwhite = strchr(whitespace, 'I')) != NULL) { /* have an embedded I */
                iscase = 0;
                strsqueeze(pwhite, 1);
            }                                                          /*set flag and squeeze it out*/
            if (*whitespace == '\000') strcpy(whitespace, WHITESPACE); /* caller's white just had i,I */
        }                                                              /* so revert back to default */

    /* -------------------------------------------------------------------------
    Find first occurrence of substr in string
    -------------------------------------------------------------------------- */
    if (string != NULL)                      /* caller passed us a string ptr */
        while (*pstring != '\000') {         /* break when string exhausted */
            char *pstrptr = pstring;         /* (re)start at next char in string*/
            int leadingwhite = 0;            /* leading whitespace */
            psubstr = substr;                /* start at beginning of substr */
            foundlen = 0;                    /* reset length of found substr */
            if (substr != NULL) {            /* caller passed us a substr ptr */
                while (*psubstr != '\000') { /*see if pstring begins with substr*/
                    /* --- check for end of string before finding match --- */
                    if (*pstrptr == '\000') goto nextstrchar; /* end-of-string without a match, keep trying with next char */

                    /* --- actual amount of whitespace in string and substr --- */
                    nsubwhite = strspn(psubstr, whitespace); /* #leading white chars in sub */
                    nstrwhite = strspn(pstrptr, whitespace); /* #leading white chars in str */
                    nminwhite = max2(0, nsubwhite - 1);      /* #mandatory leading white in str */

                    /* --- check for mandatory leading whitespace in string --- */
                    if (pstrptr != string) {         /*not mandatory at start of string*/
                        if (nstrwhite < nminwhite) { /* too little leading white space */
                            goto nextstrchar;        /* keep trying with next char */
                        }
                    }

                    /* ---hold on to #whitespace chars in string preceding substr match--- */
                    if (pstrptr == pstring) {     /* whitespace at start of substr */
                        leadingwhite = nstrwhite; /* save it as leadingwhite */
                    }

                    /* --- check for optional whitespace --- */
                    if (psubstr != substr) {                                  /* always okay at start of substr */
                        if (nstrwhite > 0 && nsubwhite < 1) goto nextstrchar; /* too much leading white space, keep trying with next char */
                    }

                    /* --- skip any leading whitespace in substr and string --- */
                    psubstr += nsubwhite; /* push past leading sub whitespace*/
                    pstrptr += nstrwhite; /* push past leading str whitespace*/

                    /* --- now get non-whitespace chars that we have to match --- */
                    nsubchars = strcspn(psubstr, whitespace);    /* #non-white chars in sub */
                    nstrchars = strcspn(pstrptr, whitespace);    /* #non-white chars in str */
                    if (nstrchars < nsubchars) goto nextstrchar; /* too few chars for match, keep trying with next char */

                    /* --- see if next nsubchars are a match --- */
                    isncmp = (iscase ? strncmp(pstrptr, psubstr, nsubchars) : strncasecmp(pstrptr, psubstr, nsubchars)); /* case sensitivity vs insensitivity */
                    if (isncmp != 0) goto nextstrchar; /* no match so keep trying with next char */

                    /* --- push past matched chars --- */
                    psubstr += nsubchars;
                    pstrptr += nsubchars; /*nsubchars were matched*/
                }
            }
            pfound = pstring + leadingwhite;    /* found match starting at pstring */
            foundlen = (int)(pstrptr - pfound); /* consisting of this many chars */
            goto end_of_job;                    /* back to caller */

        /* ---failed to find substr, continue trying with next char in string--- */
        nextstrchar:   /* continue outer loop */
            pstring++; /* bump to next char in string */
        }

/* -------------------------------------------------------------------------
Back to caller with ptr to first occurrence of substr in string
-------------------------------------------------------------------------- */
end_of_job:
    log_info(99, "[strwstr] str=\"%.72s\" sub=\"%s\" found at offset %d\n", string, substr, (pfound == NULL ? (-1) : (int)(pfound - string)));
    if (sublen != NULL)     /*caller wants length of found substr*/
        *sublen = foundlen; /* give it to him along with ptr */
    return pfound;          /*ptr to first found substr, or NULL*/
}

/* ==========================================================================
 * Function:	strreplace ( string, from, to, iscase, nreplace )
 * Purpose:	Changes the first nreplace occurrences of 'from' to 'to'
 *		in string, or all occurrences if nreplace=0.
 * --------------------------------------------------------------------------
 * Arguments:	string (I/0)	char * to null-terminated string in which
 *				occurrence of 'from' will be replaced by 'to'
 *		from (I)	char * to null-terminated string
 *				to be replaced by 'to'
 *		to (I)		char * to null-terminated string that will
 *				replace 'from'
 *		iscase (I)	int containing 1 if matches of 'from'
 *				in 'string' should be case-sensitive,
 *				or 0 if matches are case-insensitive.
 *		nreplace (I)	int containing (maximum) number of
 *				replacements, or 0 to replace all.
 * --------------------------------------------------------------------------
 * Returns:	( int )		number of replacements performed,
 *				or 0 for no replacements or -1 for any error.
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
int strreplace(char *string, char *from, char *to, int iscase, int nreplace) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    int fromlen = (from == NULL ? 0 : strlen(from));             /* #chars to be replaced */
    int tolen = (to == NULL ? 0 : strlen(to));                   /* #chars in replacement string */
    int iscommand = (fromlen < 2 ? 0 : (*from == '\\' ? 1 : 0)); /* is from a \command ?*/
    char *pfrom = (char *)NULL;                                  /* ptr to 1st char of from in string*/
    char *pstring = string;                                      /* ptr past previously replaced from*/
    char *strchange();                                           /* change 'from' to 'to' */
    // int iscase = 1;                                              /* true for case-sensitive match */
    int nreps = 0; /* #replacements returned to caller*/

    /* -------------------------------------------------------------------------
    repace occurrences of 'from' in string to 'to'
    -------------------------------------------------------------------------- */
    if (string == (char *)NULL || (fromlen < 1 && nreplace <= 0)) { /* no input string. avoiding replacing an empty string forever */
        nreps = -1;                                                 /* so signal error */
    } else {                                                        /* args okay */
        while (nreplace < 1 || nreps < nreplace) {                  /* up to #replacements requested */
            if (fromlen > 0) {                                      /* have 'from' string */
                /* ptr to 1st char of from in string. case sensitive vs case-insensistive match */
                pfrom = (iscase > 0 ? strstr(pstring, from) : strcasestr(pstring, from));
            } else {
                pfrom = pstring; /*or empty from at start of string*/
            }
            if (pfrom == (char *)NULL) break;             /*no more from's, so back to caller*/
            if (iscommand) {                              /* ignore prefix of longer string */
                if (isalpha((int)(*(pfrom + fromlen)))) { /* just a longer string */
                    pstring = pfrom + fromlen;            /* pick up search after 'from' */
                    continue;
                } /* don't change anything */
            }
            if (iscase > 1) { ; }                                /* ignore \escaped matches */
            if (strchange(fromlen, pfrom, to) == (char *)NULL) { /* leading 'from' changed to 'to' */
                nreps = -1;
                break;
            }                              /* signal error to caller */
            nreps++;                       /* count another replacement */
            pstring = pfrom + tolen;       /* pick up search after 'to' */
            if (*pstring == '\000') break; /* but quit at end of string */
        }
    }
    return nreps; /* #replacements back to caller */
}

/* ==========================================================================
 * Function:	strchange ( nfirst, from, to )
 * Purpose:	Changes the nfirst leading chars of `from` to `to`.
 *		For example, to change char x[99]="12345678" to "123ABC5678"
 *		call strchange(1,x+3,"ABC")
 * --------------------------------------------------------------------------
 * Arguments:	nfirst (I)	int containing #leading chars of `from`
 *				that will be replace by `to`
 *		from (I/O)	char * to null-terminated string whose nfirst
 *				leading chars will be replaced by `to`
 *		to (I)		char * to null-terminated string that will
 *				replace the nfirst leading chars of `from`
 * --------------------------------------------------------------------------
 * Returns:	( char * )	ptr to first char of input `from`
 *				or NULL for any error.
 * --------------------------------------------------------------------------
 * Notes:     o	If strlen(to)>nfirst, from must have memory past its null
 *		(i.e., we don't do a realloc)
 * ======================================================================= */
char *strchange(int nfirst, char *from, char *to) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    int tolen = (to == NULL ? 0 : strlen(to)); /* #chars in replacement string */
    int nshift = abs(tolen - nfirst);          /*need to shift from left or right*/
    if (from == NULL) goto end_of_job;         /* error if no source string */

    /* -------------------------------------------------------------------------
    shift from left or right to accommodate replacement of its nfirst chars by to
    -------------------------------------------------------------------------- */
    if (tolen < nfirst) { /* shift left is easy */
        strsqueeze(from, nshift);
    }                                      /* squeeze out extra bytes */
    if (tolen > nfirst) {                  /* need more room at start of from */
        char *pfrom = from + strlen(from); /* ptr to null terminating from */
        for (; pfrom >= from; pfrom--) {   /* shift all chars including null */
            *(pfrom + nshift) = *pfrom;
        }
    } /* shift chars nshift places right */

    /* -------------------------------------------------------------------------
    from has exactly the right number of free leading chars, so just put to there
    -------------------------------------------------------------------------- */
    if (tolen != 0) {            /* make sure to not empty or null */
        memcpy(from, to, tolen); /* chars moved into place */
    }

end_of_job:
    return from; /* changed string back to caller */
}

/* ==========================================================================
 * Function:	isstrstr ( char *string, char *snippets, int iscase )
 * Purpose:	determine whether any substring of 'string'
 *		matches any of the comma-separated list of 'snippets',
 *		ignoring case if iscase=0.
 * --------------------------------------------------------------------------
 * Arguments:	string (I)	char * containing null-terminated
 *				string that will be searched for
 *				any one of the specified snippets
 *		snippets (I)	char * containing null-terminated,
 *				comma-separated list of snippets
 *				to be searched for in string
 *		iscase (I)	int containing 0 for case-insensitive
 *				comparisons, or 1 for case-sensitive
 * --------------------------------------------------------------------------
 * Returns:	( int )		1 if any snippet is a substring of
 *				string, 0 if not
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
int isstrstr(char *string, char *snippets, int iscase) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    int status = 0; /*1 if any snippet found in string*/
    char snip[256];
    char *snipptr = snippets; /* munge through each snippet */
    char delim = ',';
    char *delimptr = NULL;  /* separated by delim's */
    char *strstrptr = NULL; /* looking for a match in string */

    /* -------------------------------------------------------------------------
    initialization
    -------------------------------------------------------------------------- */
    /* --- arg check --- */
    if (isempty(string) || isempty(snippets)) { /*both string and snippets required*/
        goto end_of_job;                        /* quit if either missing */
    }

    /* -------------------------------------------------------------------------
    extract each snippet and see if it's a substring of string
    -------------------------------------------------------------------------- */
    while (snipptr != NULL) { /* while we still have snippets */
        /* --- extract next snippet --- */
        if ((delimptr = strchr(snipptr, delim)) == NULL) { /* next comma delim not found following last snippet*/
            strcpy(snip, snipptr);                         /* local copy of last snippet */
            snipptr = NULL;
        } else {                                         /* snippet ends just before delim */
            int sniplen = (int)(delimptr - snipptr) - 1; /* #chars in snippet */
            memcpy(snip, snipptr, sniplen);              /* local copy of snippet chars */
            snip[sniplen] = '\000';                      /* null-terminated snippet */
            snipptr = delimptr + 1;
        } /* next snippet starts after delim */

        /* --- check if snippet in string --- */
        strstrptr = (iscase ? strstr(string, snip) : strcasestr(string, snip));
        if (strstrptr != NULL) { /* found snippet in string */
            status = 1;          /* so reset return status */
            break;
        } /* no need to check any further */
    }

end_of_job:
    return status; /*1 if snippet found in list, else 0*/
}

/* ==========================================================================
 * Function:	nomath ( s )
 * Purpose:	Removes/replaces any LaTeX math chars in s
 *		so that s can be rendered in paragraph mode.
 * --------------------------------------------------------------------------
 * Arguments:	s (I)		char * to null-terminated string
 *				whose math chars are to be removed/replaced
 * --------------------------------------------------------------------------
 * Returns:	( char * )	ptr to "cleaned" copy of s
 *				or "" (empty string) for any error.
 * --------------------------------------------------------------------------
 * Notes:     o	The returned pointer addresses a static buffer,
 *		so don't call nomath() again until you're finished
 *		with output from the preceding call.
 * ======================================================================= */
char *nomath(char *s) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    static char sbuff[4096]; /* copy of s with no math chars */
    int strreplace();        /* replace _ with -, etc */

    /* -------------------------------------------------------------------------
    Make a clean copy of s
    -------------------------------------------------------------------------- */
    /* --- check input --- */
    *sbuff = '\000';                 /* initialize in case of error */
    if (isempty(s)) goto end_of_job; /* no input */

    /* --- start with copy of s --- */
    strninit(sbuff, s, 3000); /* leave room for replacements */

    /* --- make some replacements (*must* replace \ first) --- */
    strreplace(sbuff, "\\", "\\textbackslash ", 0, 0);          /* change all \'s to text */
    strreplace(sbuff, "_", "\\textunderscore ", 0, 0);          /* change all _'s to text */
    strreplace(sbuff, "<", "\\textlangle ", 0, 0);              /* change all <'s to text */
    strreplace(sbuff, ">", "\\textrangle ", 0, 0);              /* change all >'s to text */
    strreplace(sbuff, "$", "\\textdollar ", 0, 0);              /* change all $'s to text */
    strreplace(sbuff, "&", "\\&", 0, 0);                        /* change every & to \& */
    strreplace(sbuff, "%", "\\%", 0, 0);                        /* change every % to \% */
    strreplace(sbuff, "#", "\\#", 0, 0);                        /* change every # to \# */
    strreplace(sbuff, "~", "\\~", 0, 0);                        /* change every ~ to \~ */
    strreplace(sbuff, "{", "\\{", 0, 0);                        /* change every { to \{ */
    strreplace(sbuff, "}", "\\}", 0, 0);                        /* change every } to \} */
    strreplace(sbuff, "^", "\\ensuremath{\\widehat{~}}", 0, 0); /* change every ^ */

end_of_job:
    return sbuff; /* back with clean copy of s */
}

/* ==========================================================================
 * Function:	strwrap ( s, linelen, tablen )
 * Purpose:	Inserts \n's and spaces in (a copy of) s to wrap lines
 *		at linelen and indent them by tablen.
 * --------------------------------------------------------------------------
 * Arguments:	s (I)		char * to null-terminated string
 *				to be wrapped.
 *		linelen (I)	int containing maximum linelen
 *				between \n's.
 *		tablen (I)	int containing number of spaces to indent
 *				lines.  0=no indent.  Positive means
 *				only indent first line and not others.
 *				Negative means indent all lines except first.
 * --------------------------------------------------------------------------
 * Returns:	( char * )	ptr to "line-wrapped" copy of s
 *				or "" (empty string) for any error.
 * --------------------------------------------------------------------------
 * Notes:     o	The returned copy of s has embedded \n's as necessary
 *		to wrap lines at linelen.  Any \n's in the input copy
 *		are removed first.  If (and only if) the input s contains
 *		a terminating \n then so does the returned copy.
 *	      o	The returned pointer addresses a static buffer,
 *		so don't call strwrap() again until you're finished
 *		with output from the preceding call.
 * ======================================================================= */
char *strwrap(char *s, int linelen, int tablen) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    static char sbuff[4096];                          /* line-wrapped copy of s */
    char *sol = sbuff;                                /* ptr to start of current line*/
    char tab[32] = "                 ";               /* tab string */
    int strreplace();                                 /* remove \n's */
    char *strchange();                                /* add \n's and indent space */
    int finalnewline = (lastchar(s) == '\n' ? 1 : 0); /*newline at end of string?*/
    int istab = (tablen > 0 ? 1 : 0);                 /* init true to indent first line */
    int rhslen = 0;                                   /* remaining right hand side length*/
    int thislen = 0;                                  /* length of current line segment */
    int thistab = 0;                                  /* length of tab on current line */
    int wordlen = 0;                                  /* length to next whitespace char */

    /* -------------------------------------------------------------------------
    Make a clean copy of s
    -------------------------------------------------------------------------- */
    /* --- check input --- */
    *sbuff = '\000';                             /* initialize in case of error */
    if (isempty(s)) goto end_of_job;             /* no input */
    if (tablen < 0) tablen = -tablen;            /* set positive tablen */
    if (tablen >= linelen) tablen = linelen - 1; /* tab was longer than line */
    tab[min2(tablen, 16)] = '\000';              /* null-terminate tab string */
    tablen = strlen(tab);                        /* reset to actual tab length */
    /* --- start with copy of s --- */
    strninit(sbuff, s, 3000);           /* leave room for \n's and tabs */
    if (linelen < 1) goto end_of_job;   /* can't do anything */
    trimwhite(sbuff);                   /*remove leading/trailing whitespace*/
    strreplace(sbuff, "\n", " ", 0, 0); /* remove any original \n's */
    strreplace(sbuff, "\r", " ", 0, 0); /* remove any original \r's */
    strreplace(sbuff, "\t", " ", 0, 0); /* remove any original \t's */

    /* -------------------------------------------------------------------------
    Insert \n's and spaces as needed
    -------------------------------------------------------------------------- */
    while (1) { /* until end of line */
        /* --- init --- */
        trimwhite(sol);             /*remove leading/trailing whitespace*/
        thislen = thistab = 0;      /* no chars in current line yet */
        if (istab && tablen > 0) {  /* need to indent this line */
            strchange(0, sol, tab); /* insert indent at start of line */
            thistab = tablen;
        }                                    /* line starts with whitespace tab */
        if (sol == sbuff) istab = 1 - istab; /* flip tab flag after first line */
        sol += thistab;                      /* skip tab */
        rhslen = strlen(sol);                /* remaining right hand side chars */
        if (rhslen <= linelen) break;        /* no more \n's needed */
        log_info(99, "[strwrap] rhslen=%d, sol=\"\"%s\"\"\n", rhslen, sol);

        /* --- look for last whitespace preceding linelen --- */
        while (1) {                                            /* till we exceed linelen */
            wordlen = strcspn(sol + thislen, WHITESPACE);      /*ptr to next whitespace char*/
            if (thislen + thistab + wordlen >= linelen) break; /*next word won't fit*/
            thislen += (wordlen + 1);
        }                        /* ptr past next whitespace char */
        if (thislen < 1) break;  /* line will have one too-long word*/
        sol[thislen - 1] = '\n'; /* replace last space with newline */
        sol += thislen;          /* next line starts after newline */
    }

end_of_job:
    if (finalnewline) strcat(sbuff, "\n"); /* replace final newline */
    return sbuff;                          /* back with clean copy of s */
}

/* ==========================================================================
 * Function:	strpspn ( char *s, char *reject, char *segment )
 * Purpose:	finds the initial segment of s containing no chars
 *		in reject that are outside (), [] and {} parens, e.g.,
 *		   strpspn("abc(---)def+++","+-",segment) returns
 *		   segment="abc(---)def" and a pointer to the first '+' in s
 *		because the -'s are enclosed in () parens.
 * --------------------------------------------------------------------------
 * Arguments:	s (I)		(char *)pointer to null-terminated string
 *				whose initial segment is desired
 *		reject (I)	(char *)pointer to null-terminated string
 *				containing the "reject chars"
 *				If reject contains a " or a ', then the
 *				" or ' isn't itself a reject char,
 *				but other reject chars within quoted
 *				strings (or substrings of s) are spanned.
 *		segment (O)	(char *)pointer returning null-terminated
 *				string comprising the initial segment of s
 *				that contains non-rejected chars outside
 *				(),[],{} parens, i.e., all the chars up to
 *				but not including the returned pointer.
 *				(That's the entire string if no non-rejected
 *				chars are found.)
 * --------------------------------------------------------------------------
 * Returns:	( char * )	pointer to first reject-char found in s
 *				outside parens, or a pointer to the
 *				terminating '\000' of s if there are
 *				no reject chars in s outside all () parens.
 *				But if reject is empty, returns pointer
 *				to matching )]} outside all parens.
 * --------------------------------------------------------------------------
 * Notes:     o	the return value is _not_ like strcspn()'s
 *	      o	improperly nested (...[...)...] are not detected,
 *		but are considered "balanced" after the ]
 *	      o	if reject not found, segment returns the entire string s
 *	      o	but if reject is empty, returns segment up to and including
 *		matching )]}
 *	      o	leading/trailing whitespace is trimmed from returned segment
 * ======================================================================= */
char *strpspn(char *s, char *reject, char *segment) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    char *ps = s;                                    /* current pointer into s */
    char *strqspn(char *s, char *q, int isunescape); /*span quoted string*/
    char qreject[256] = "\000";
    char *pq = qreject; /* find "or" in reject */
    char *pr = reject;
    int isqspan = 0; /* true to span quoted strings */
    int depth = 0;   /* () paren nesting level */
    int seglen = 0;  /* segment length, max allowed */
    int maxseg = 2047;
    int isescaped = 0; /* signals escaped chars */
    int checkescapes = 1;

    /* -------------------------------------------------------------------------
    initialization
    -------------------------------------------------------------------------- */
    /* --- check arguments --- */
    if (isempty(s) /* || isempty(reject) */) goto end_of_job; /* no input string supplied */

    /* --- set up qreject w/o quotes --- */
    if (!isempty(reject)) {     /* have reject string from caller */
        while (*pr != '\000') { /* until end of reject string */
            if (!isthischar(*pr, "\"\'")) {
                *pq++ = *pr; /* not a " or ', copy actual reject char */
            } else {
                isqspan = 1; /* span rejects in quoted strings */
            }
            pr++;
        } /* next reject char from caller */
    }
    *pq = '\000'; /* null-terminate qreject */

    /* -------------------------------------------------------------------------
    find first char from s outside () parens (and outside ""'s) and in reject
    -------------------------------------------------------------------------- */
    while (*ps != '\000') {                      /* search till end of input string */
        int spanlen = 1;                         /*span 1 non-reject, non-quoted char*/
        if (!isescaped) {                        /* ignore escaped \(,\[,\{,\),\],\}*/
            if (isthischar(*ps, "([{")) depth++; /* push another paren */
            if (isthischar(*ps, ")]}")) depth--;
        }                                                               /* or pop another paren */
        if (depth < 1) {                                                /* we're outside all parens */
            if (isqspan) {                                              /* span rejects in quoted strings */
                if (isthischar(*ps, "\"\'")) {                          /* and we're at opening quote */
                    pq = strqspn(ps, NULL, 0);                          /* locate matching closing quote */
                    if (pq != ps) {                                     /* detected start of quoted string */
                        if (*pq == *ps) spanlen = ((int)(pq - ps)) + 1; /* and found closing quote */
                    }
                } /* span the entire quoted string */
            }
            if (isempty(qreject)) break; /* no reject so break immediately */
            if (isthischar(*ps, qreject)) break;
        }                                                           /* only break on a reject char */
        if (checkescapes) isescaped = (*ps == '\\' ? 1 : 0);        /* if checking escape sequences, reset iescaped signal */
        if (segment != NULL) {                                      /* caller gave us segment */
            int copylen = min2(spanlen, maxseg - seglen);           /* don't overflow segment */
            if (copylen > 0) memcpy(segment + seglen, ps, copylen); /* have room in segment buffer */
        }                                                           /* so copy non-reject chars */
        seglen += spanlen;
        ps += spanlen; /* bump to next char */
    }

end_of_job:
    if (segment != NULL) {                     /* caller gave us segment */
        if (isempty(qreject) && !isempty(s)) { /* no reject char */
            segment[min2(seglen, maxseg)] = *ps;
            seglen++;
        }                                       /*closing )]} to seg*/
        segment[min2(seglen, maxseg)] = '\000'; /* null-terminate the segment */
        trimwhite(segment);
    }          /* trim leading/trailing whitespace*/
    return ps; /* back to caller */
}

/* ==========================================================================
 * Function:	strqspn ( char *s, char *q, int isunescape )
 * Purpose:	finds matching/closing " or ' in quoted string
 *		that begins with " or ', and optionally changes
 *		escaped quotes to unescaped quotes.
 * --------------------------------------------------------------------------
 * Arguments:	s (I)		(char *)pointer to null-terminated string
 *				that begins with " or ',
 *		q (O)		(char *)pointer returning null-terminated
 *				quoted token, with or without outer quotes,
 *				and with or without escaped inner quotes
 *				changed to unescaped quotes, depending
 *				on isunescape.
 *		isunescape (I)	int containing 1 to change \" to " if s
 *				is "quoted" or change \' to ' if 'quoted',
 *				or containing 2 to change both \" and \'
 *				to unescaped quotes.  Other \sequences aren't
 *				changed.  Note that \\" emits \".
 *				isunescape=0 makes no changes at all.
 *				Note: the following not implemented yet --
 *				If unescape is negative, its abs() is used,
 *				but outer quotes aren't included in q.
 * --------------------------------------------------------------------------
 * Returns:	( char * )	pointer to matching/closing " or '
 *				(or to char after quote if isunescape<0),
 *				or terminating '\000' if none found,
 *				or unchanged (same as s) if not quoted string
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
char *strqspn(char *s, char *q, int isunescape) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    char *ps = s, *pq = q;                /* current pointer into s, q */
    char quote = '\000';                  /* " or ' quote character */
    int qcopy = (isunescape < 0 ? 0 : 1); /* true to copy outer quotes */
    int isescaped = 0;                    /* true to signal \escape sequence */
    int maxqlen = 2400;                   /* max length of returned q */

    /* -------------------------------------------------------------------------
    Initialization
    -------------------------------------------------------------------------- */
    /* --- check args --- */
    if (s == NULL) goto end_of_job;                    /* no string supplied */
    skipwhite(ps);                                     /* skip leading whitespace */
    if (*ps == '\000' || (!isthischar(*ps, "\"\'"))) { /* string exhausted, or not a " or ' quoted string */
        ps = s;
        goto end_of_job;
    }                                             /*signal error/not string to caller*/
    if (isunescape < 0) isunescape = -isunescape; /* flip positive */

    /* --- set quote character --- */
    quote = *ps;                           /* set quote character */
    if (qcopy && q != NULL) *pq++ = quote; /* and copy it to output token */

    /* -------------------------------------------------------------------------
    span characters between quotes
    -------------------------------------------------------------------------- */
    while (*(++ps) != '\000') { /* end of string always terminates */
        /* --- process escaped chars --- */
        if (isescaped) {                                                /* preceding char was \ */
            if (*ps != '\\') isescaped = 0;                             /* reset isescaped flag unless \\ */
            if (q != NULL) {                                            /* caller wants quoted token */
                if (isunescape == 0                                     /* don't unescape anything */
                    || (isunescape == 1 && *ps != quote)                /* escaped char not our quote */
                    || (isunescape == 2 && (!isthischar(*ps, "\"\'")))) /* not any quote */
                    if (--maxqlen > 0)                                  /* so if there's room in token */
                        *pq++ = '\\';                                   /*keep original \ in returned token*/
                if (!isescaped)                                         /* will have to check 2nd \ in \\ */
                    if (--maxqlen > 0)                                  /* if there's room in token */
                        *pq++ = *ps;
            } /* put escaped char in token */
            continue;
        } /* go on to next char in string */

        /* --- check if next char escaped --- */
        if (*ps == '\\') { /* found escape char */
            isescaped = 1;
            continue;
        } /*set flag and process escaped char*/

        /* --- check for unescaped closing quote --- */
        if (*ps == quote) {                        /* got an unescaped quote */
            if (qcopy && q != NULL) *pq++ = quote; /* copy it to output token */
            if (0 && !qcopy) ps++;                 /* return ptr to char after quote */
            goto end_of_job;
        } /* back to caller */

        /* --- process other chars --- */
        if (q != NULL) {                    /* caller want token returned */
            if (--maxqlen > 0) *pq++ = *ps; /* and there's still room in token, so put char in */
        }
    }
    // ps = NULL;
    // pq = q; /* error if no closing quote found */

end_of_job:
    if (q != NULL) *pq = '\000'; /* null-terminate returned token */
    return ps;                   /* return ptr to " or ', or NULL */
}

/* ==========================================================================
 * Function:	isnumeric ( s )
 * Purpose:	determine if s is an integer
 * --------------------------------------------------------------------------
 * Arguments:	s (I)		(char *)pointer to null-terminated string
 *				that's checked for a leading + or -
 *				followed by digits
 * --------------------------------------------------------------------------
 * Returns:	( int )		1 if s is numeric, 0 if it is not
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
int isnumeric(char *s) {
    /* -------------------------------------------------------------------------
    determine whether s is an integer
    -------------------------------------------------------------------------- */
    int status = 0;                                       /* return 0 if not numeric, 1 if is*/
    char *p = s;                                          /* pointer into s */
    if (isempty(s)) goto end_of_job;                      /* missing arg or empty string */
    skipwhite(p);                                         /*check for leading +or- after space*/
    if (*p == '+' || *p == '-') p++;                      /* skip leading + or - */
    for (; *p != '\000'; p++) {                           /* check rest of s for digits */
        if (isdigit(*p)) continue;                        /* still got uninterrupted digits */
        if (!isthischar(*p, WHITESPACE)) goto end_of_job; /* non-numeric char */
        skipwhite(p);                                     /* skip all subsequent whitespace */
        if (*p == '\000') break;                          /* trailing whitespace okay */
        goto end_of_job;                                  /* embedded whitespace non-numeric */
    }
    status = 1; /* numeric after checks succeeded */

end_of_job:
    return status; /*back to caller with 1=string, 0=no*/
}

/* ==========================================================================
 * Function:	evalterm ( STORE *store, char *term )
 * Purpose:	evaluates a term
 * --------------------------------------------------------------------------
 * Arguments:	store (I/O)	STORE * containing environment
 *				in which term is to be evaluated
 *		term (I)	char * containing null-terminated string
 *				with a term like "3" or "a" or "a+3"
 *				whose value is to be determined
 * --------------------------------------------------------------------------
 * Returns:	( int )		value of term,
 *				or NOVALUE for any error
 * --------------------------------------------------------------------------
 * Notes:     o	Also evaluates index?a:b:c:etc, returning a if index<=0,
 *		b if index=1, etc, and the last value if index is too large.
 *		Each a:b:c:etc can be another expression, including another
 *		(index?a:b:c:etc) which must be enclosed in parentheses.
 * ======================================================================= */
int evalterm(struct store_struct *store, char *term) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    int termval = 0;           /* term value returned to caller */
    char token[2048] = "\000"; /* copy term */
    char *delim = NULL;        /* delim '(' or '?' in token */
    // int evalwff();                /* recurse to evaluate terms */
    // int evalfunc();               /* evaluate function(arg1,arg2,...)*/
    char *strpspn();              /* span delims */
    int getstore();               /* lookup variables */
    int isnumeric();              /* numeric=constant, else variable */
    static int evaltermdepth = 0; /* recursion depth */
    int novalue = -89123456;      /* dummy (for now) error signal */

    /* -------------------------------------------------------------------------
    Initialization
    -------------------------------------------------------------------------- */
    if (++evaltermdepth > 99) goto end_of_job;           /*probably recursing forever*/
    if (store == NULL || isempty(term)) goto end_of_job; /*check for missing arg*/
    skipwhite(term);                                     /* skip any leading whitespace */

    /* -------------------------------------------------------------------------
    First look for conditional of the form term?term:term:...
    -------------------------------------------------------------------------- */
    /* ---left-hand part of conditional is chars preceding "?" outside ()'s--- */
    delim = strpspn(term, "?", token);                                          /* chars preceding ? outside () */
    if (*delim != '\000') {                                                     /* found conditional expression */
        int ncolons = 0;                                                        /* #colons we've found so far */
        if (*token != '\000') {                                                 /* evaluate "index" value on left */
            if ((termval = evalterm(store, token)) == novalue) goto end_of_job; /* evaluate left-hand term, return error if failed */
        }
        while (*delim != '\000') { /* still have chars in term */
            delim++;
            *token = '\000';                    /* initialize for next "value:" */
            if (*delim == '\000') break;        /* no more values */
            delim = strpspn(delim, ":", token); /* chars preceding : outside () */
            if (ncolons++ >= termval) break;    /* have corresponding term */
        }
        if (*token != '\000') termval = evalterm(store, token); /* have x:x:value:x:x on right, so evaluate it */
        goto end_of_job;                                        /* return result to caller */
    }

    /* -------------------------------------------------------------------------
    evaluate a+b recursively
    -------------------------------------------------------------------------- */
    /* --- left-hand part of term is chars preceding "/+-*%" outside ()'s --- */
    term = strpspn(term, "/+-*%", token); /* chars preceding /+-*% outside ()*/

    /* --- evaluate a+b, a-b, etc --- */
    if (*term != '\000') {                                                      /* found arithmetic operation */
        int leftval = 0, rightval = 0;                                          /* init leftval for unary +a or -a */
        if (*token != '\000') {                                                 /* or eval for binary a+b or a-b */
            if ((leftval = evalterm(store, token)) == novalue) goto end_of_job; /* evaluate left-hand term, return error if failed */
        }
        if ((rightval = evalterm(store, term + 1)) == novalue) goto end_of_job; /* evaluate right-hand term, return error if failed */
        switch (*term) {                                                        /* perform requested arithmetic */
            case '+': /* addition */ termval = leftval + rightval; break;
            case '-': /* subtraction */ termval = leftval - rightval; break;
            case '*': /* multiplication */ termval = leftval * rightval; break;
            case '/': /* integer division */
                if (rightval != 0) termval = leftval / rightval;
                break;
            case '%': /* left modulo right */
                if (rightval != 0) termval = leftval % rightval;
                break;
            default: /* internal error */ break;
        }
        goto end_of_job; /* return result to caller */
    }

    /* -------------------------------------------------------------------------
    check for parenthesized expression or term of the form function(arg1,arg2,...)
    -------------------------------------------------------------------------- */
    if ((delim = strchr(token, '(')) != NULL) { /* token contains a ( */
        /* --- strip trailing paren (if there hopefully is one) --- */
        int toklen = strlen(token);                             /* total #chars in token */
        if (token[toklen - 1] == ')') token[--toklen] = '\000'; /* found trailing ) at end of token, remove it */

        /* --- handle parenthesized subexpression --- */
        if (*token == '(') {      /* have parenthesized expression */
            strsqueeze(token, 1); /* so squeeze out leading ( */

            /* --- evaluate edited term --- */
            trimwhite(token); /* trim leading/trailing whitespace*/
            termval = evalterm(store, token);
        } else {
            /* --- handle function(arg1,arg2,...) --- */
            *delim = '\000'; /* separate function name and args */
            // termval = evalfunc(store, token, delim + 1); /* evaluate function */
        }
        goto end_of_job;
    } /* return result to caller */

    /* -------------------------------------------------------------------------
    evaluate constants directly, or recursively evaluate variables, etc
    -------------------------------------------------------------------------- */
    if (*token != '\000') { /* empty string */
        if (isnumeric(token)) {
            termval = atoi(token); /* have a constant, so convert ascii-to-int */
        } else {
            termval = getstore(store, token); /* variable or "stored proposition"*/
        }                                     /* look up token */
    }

/* -------------------------------------------------------------------------
back to caller with truth value of proposition
-------------------------------------------------------------------------- */
end_of_job:
    if (evaltermdepth > 0) evaltermdepth--; /* pop recursion depth */
    return termval;
}

/* ==========================================================================
 * Function:	getstore ( store, identifier )
 * Purpose:	finds identifier in store and returns corresponding value
 * --------------------------------------------------------------------------
 * Arguments:	store (I)	(STORE *)pointer to store containing
 *				the desired identifier
 *		identifier (I)	(char *)pointer to null-terminated string
 *				containing the identifier whose value
 *				is to be returned
 * --------------------------------------------------------------------------
 * Returns:	( int )		identifier's corresponding value,
 *				or 0 if identifier not found (or any error)
 * --------------------------------------------------------------------------
 * Notes:     o
 * ======================================================================= */
int getstore(struct store_struct *store, char *identifier) {
    /* -------------------------------------------------------------------------
    Allocations and Declarations
    -------------------------------------------------------------------------- */
    int value = 0;  /* store[istore].value for identifier */
    int istore = 0; /* store[] index containing identifier */
    char seek[512]; /* identifier arg, identifier in store */
    char hide[512];

    /* --- first check args --- */
    if (store == NULL || isempty(identifier)) goto end_of_job; /* missing arg */
    strninit(seek, identifier, 500);                           /* local copy of caller's identifier */
    trimwhite(seek);                                           /* remove leading/trailing whitespace */

    /* --- loop over store --- */
    for (istore = 0; istore < MAXSTORE; istore++) { /* until end of table */
        char *idstore = store[istore].identifier;   /* ptr to identifier in store */
        if (isempty(idstore)) break;                /* empty id signals eot, de-reference any default/error value */
        strninit(hide, idstore, 500);               /* local copy of store[] identifier */
        trimwhite(hide);                            /* remove leading/trailing whitespace */
        if (!strcmp(hide, seek)) break;             /* found match, dereference corresponding value */
    }
    if (store[istore].value != NULL)    /* address of int supplied */
        value = *(store[istore].value); /* return de-referenced int */

end_of_job:
    return value; /* store->values[istore] or NULL */
}
