# mathtex
Don't use this thing, it sucks. I wrote it because I wanted a binary that rendered LaTeX strings and I didn't really know how LaTeX worked and there was this standalone C file made by [this guy](http://www.forkosh.com/) with a permissable license so I figured "hey, two kill birds with one stone, fuck around and find out with C and get latex images" and here we are. Just write your own in a language that can handle doing certain things (like string manipulation and command-line execution) better.

For context, I built this to be able to render latex images in my [Discord bot.](https://github.com/mechabubba/bear) Its still functional, but every once and a while it breaks. It is what it is...

<details>
<summary>Original README</summary>

<pre>10/18/23: This program has been updated quite a bit. No longer a cgi app, now a standalone command line app.
The README below is pretty outdated. New one to be written at a later date; in the meantime, see the header of the mathtex.c file.

 --------------------------------------------------------------------------
 December 6, 2014                                              Version 1.05

                  m a t h T e X   R e a d m e   F i l e

 Copyright(c) 2007-2014, John Forkosh Associates, Inc. All rights reserved.
 --------------------------------------------------------------------------

                            by: John Forkosh
                  john@forkosh.com     www.forkosh.com

          This file is part of mathTeX, which is free software.
          You may redistribute and/or modify it under the terms
          of the GNU General Public License, version 3 or later,
          as published by the Free Software Foundation. See
                  http://www.gnu.org/licenses/gpl.html

          mathTeX is discussed and illustrated online by
          the mathTeX manual at its homepage
                   http://www.forkosh.com/mathtex.html
          Or you can follow the Installation instructions in
          Section II below to immediately install mathTeX on
          your own server.


I.   INTRODUCTION
------------------------------------------------------------------------
  MathTeX, licensed under the GPL, is a cgi program that lets you
  easily embed LaTeX math in your own html pages, blogs, wikis, etc.
  It parses a LaTeX math expression and immediately emits the
  corresponding gif (or png) image, rather than the usual TeX dvi.
  So just place an html <​img> tag in your document wherever you want
  to see the corresponding LaTeX expression.  For example,
        <​img src="/cgi-bin/mathtex.cgi?f(x)=\int_{-\infty}^xe^{-t^2}dt"
        alt="" border=0 align="middle">
  immediately displays the corresponding gif image wherever you put
  that <​img> tag.

  There's no inherent need to repeatedly write the cumbersome <​img> tag
  illustrated above.  For example, if you're using phpBB3, just click
  Postings from the Administrator Control Panel, and add the Custom BBCode
        [tex]{TEXT}[/tex]
  with the HTML replacement
        <​img src="/cgi-bin/mathtex.cgi?{TEXT}" align="middle">
  Then users can just type
        [tex] f(x)=\int_{-\infty}^xe^{-t^2}dt [/tex]
  in their posts to see a gif image of the enclosed expression.

  MathTeX uses the latex and dvipng programs, along with all necessary
  fonts, etc, from your TeX distribution.  If dvipng is not available,
  you can compile mathTeX to use dvips from your TeX distribution, and
  convert from the ImageMagick package, instead.  Links to online sources
  for all these dependencies are on  http://www.forkosh.com/mathtex.html
  and several are listed below.


II.  INSTALLATION
------------------------------------------------------------------------
  Note: The current release of mathTeX only runs under Unix-like
  operating systems.  To compile and install mathTeX on your own
  Unix server...
       +---
       | Install mathTeX's dependencies and download mathTeX
       +----------------------------------------------------
       * First, make sure you have a recent LaTeX distribution
              http://www.latex-project.org/ftp.html
         installed on your server.  Ask your ISP or sysadmin
         if you have any installation problems or questions.
         Or see  http://www.forkosh.com/mimetex.html  if you
         can't install latex.
              Besides latex, mathTeX uses dvipng, which recent
         LaTeX distributions typically include.  If you can't
         install dvipng, see http://www.forkosh.com/mathtex.html
         for instructions to use dvips and convert instead of dvipng.
       * Then, download  http://www.forkosh.dreamhost.com/mathtex.zip
         and  unzip mathtex.zip  in any convenient working
         directory.  Your working directory should now contain
          mathtex.zip          your downloaded gnu zipped mathTeX distribution
          mathtex/README       this file (see mathtex.html for demo/tutorial)
          mathtex/COPYING      GPL license, under which you may use mathTeX
          mathtex/mathtex.c    mathTeX source program and all functions
          mathtex/mathtex.html mathTeX users manual
       +---
       | Compile and Install mathTeX
       +----------------------------
       * To compile an executable that emits
         default gif images
              cc mathtex.c -DLATEX=\"$(which latex)\"  \
              -DDVIPNG=\"$(which dvipng)\"  \
              -o mathtex.cgi
         For default png images, add the -DPNG switch.  Additional
         command-line switches that you may find useful are
         discussed at  http://www.forkosh.com/mathtex.html
       * Finally,
              mv mathtex.cgi  to your server's cgi-bin/ directory,
              chmod its permissions as necessary (typically 755),
              making sure mathtex.cgi can rw files in cgi-bin/,
         and you're all done.
       +---
       | Test installed image
       +---------------------
       * To quickly test your installed mathtex.cgi, type
         a url into your browser's locator window something like
              http://www.yourdomain.com/cgi-bin/mathtex.cgi?x^2+y^2
         which should display the same image that you see at
              http://www.forkosh.com/cgi-bin/mathtex.cgi?x^2+y^2
         If you see the same image from your own domain link,
         then you've completed a successful mathTeX installation.
       * Optionally, to install a copy of the mathTeX manual
         on your server,
              mv mathtex.html  to your server's htdocs/ directory.
         And, if the relative path from htdocs to cgi-bin
         isn't ../cgi-bin, then edit mathtex.html and change
         the few dozen occurrences as necessary.  Now,
              http://www.yourdomain.com/mathtex.html
         should display your own copy of the mathTeX manual.

  Any problems with the above?
        Read the more detailed instructions on mathTeX's homepage
              http://www.forkosh.com/mathtex.html


III. REVISION HISTORY
------------------------------------------------------------------------
  See  http://www.forkosh.com/mathtexchangelog.html  for a detailed
  discussion of mathTeX revisions.
      o 11 Oct 2007 -- mathTeX version 1.00 released.
      o 12 Oct 2007 -- optional \usepackage[arg]{package} argument
        now recognized correctly (initial release neglected to
        handle optional [arg] following \usepackage).
      o 12 Oct 2007 -- html &#nnn; now translated during preprocessing,
        e.g., &#091 or &#091; becomes [ (left square bracket) before
        it's submitted to latex.
      o 12 Oct 2007 -- special mathTeX directives like \time
        are now checked for proper command termination, i.e., non-alpha
        character.  (In particular, LaTeX \times had been incorrectly
        interpreted as mathTeX \time followed by an s.)
      o 12 Oct 2007 -- url "unescape" translation, i.e.,
        %20-to-blank, etc, repeated (done twice) for
        <form> input.  (I'm not sure why this is necessary,
        and can't reproduce the problem myself, but am acting
        on seemingly reliable reports.)
      o 20 Oct 2007 -- removed leading and trailing pairs of $$...$$'s
        from input expressions, interpreting $...$ as \textstyle and
        $$...$$ as \displaystyle (and $$$...$$$ as \parstyle).
        Also removed leading and trailing \[...\], interpreting it
        as \displaystyle.  (Note: \displaystyle is mathTeX's default,
        so $$...$$'s or \[...\] are unnecessary.  But some people submit
        expressions containing them, so they're now interpreted.)
      o 16 Feb 2008 -- more robust test to display the correct
        error message when a required dependency isn't installed.
        (Occasionally, the "ran but failed" message was emitted
        when a dependency was actually "not installed".)
      o 16 Feb 2008 --  -DDENYREFERER=\"string\"  or
        -DDENYREFERER=\"string1,string2,etc\"  compile switch added.
        If compiled with it, mathTeX won't render images for
        HTTP_REFERER's containing  string (or string1 or string2, etc)
        as a substring of their url's.
      o 17 Feb 2008 -- updated (slightly) documentation
      o 18 Feb 2008 -- mathTeX version 1.01 released.
      o 05 Mar 2009 -- \environment directive added to display
        all http environment variables.
      o 06 Mar 2009 -- mathTeX version 1.02 released.
      o 15 Nov 2011 -- mathTeX version 1.05 released.
      o 26 Oct 2014 -- most recent change


IV.  CONCLUDING REMARKS
------------------------------------------------------------------------
  I hope you find mathTeX useful.  If so, a contribution to your
  country's TeX Users Group, or to the GNU project, is suggested,
  especially if you're a company that's currently profitable.
========================= END-OF-FILE README ===========================
</pre>
</details>
