#!/usr/bin/env python
#!/bin/env python

from __future__ import print_function
import sys
from sys import *
import lex
import re
import os

# Reserved words

tokens = (
    # Literals (identifier, integer constant, float constant, string constant, char const)
    'ID', 'TYPEID', 'ICONST', 'FCONST', 'SCONST', 'CCONST',

    # Operators (+,-,*,/,%,|,&,~,^,<<,>>, ||, &&, !, <, <=, >, >=, ==, !=)
    'PLUS', 'MINUS', 'TIMES', 'DIVIDE', 'MOD',
    'OR', 'AND', 'NOT', 'XOR', 'LSHIFT', 'RSHIFT',
    'LOR', 'LAND', 'LNOT',
    'LT', 'LE', 'GT', 'GE', 'EQ', 'NE', 'HREF', 'FINDEX', 'SUBSECTION', 'CHAPTER', 'SECTION','CAPTION','SINDEX','TRL','BEGIN{VERBATIM}','END{VERBATIM}','LSTINLINE','BEGIN{LSTLISTING}','END{LSTLISTING}','BEGIN{OUTPUTLISTING}','END{OUTPUTLISTING}','BEGIN{BASHLISTING}','END{BASHLISTING}','BEGIN{MAKELISING}','END{MAKELISTING}','BEGIN{TIKZPICTURE}','END{TIKZPICTURE}'

    # Assignment (=, *=, /=, %=, +=, -=, <<=, >>=, &=, ^=, |=)
    'EQUALS', 'TIMESEQUAL', 'DIVEQUAL', 'MODEQUAL', 'PLUSEQUAL', 'MINUSEQUAL',
    'LSHIFTEQUAL','RSHIFTEQUAL', 'ANDEQUAL', 'XOREQUAL', 'OREQUAL',

    # Increment/decrement (++,--)
    'PLUSPLUS', 'MINUSMINUS',

    # Structure dereference (->)
    'ARROW',

    # Conditional operator (?)
    'CONDOP',

    # Delimiters ( ) [ ] { } , . ; :
    'LPAREN', 'RPAREN',
    'LBRACKET', 'RBRACKET',
    'LBRACE', 'RBRACE',
    'COMMA', 'PERIOD', 'SEMI', 'COLON',

    # Ellipsis (...)
    'ELLIPSIS',

    'SPACE',

    'SLASH',

    'DOLLAR',
    'AT',
    'SHARP',
    'DOUBLEQUOTE'
    'QUOTE',
    'BACKQUOTE',
    'HAT'
    )

# Completely ignored characters
t_ignore           = '\t\x0c'


# Operators
t_SUBSECTION       = r'\\subsection\{'
t_CAPTION          = r'\\caption\{'
t_CHAPTER          = r'\\chapter\{'
t_SECTION          = r'\\section\{'
t_HREF             = r'\\href\{'
t_FINDEX           = r'\\findex\{'
t_SINDEX           = r'\\sindex\{'
t_TRL              = r'\\trl\{'
t_LSTINLINE        = r'\\lstinline\{'
t_LSTINLINEMOD     = r'\\lstinline\|'
t_BVERB            = r'\\begin\{verbatim\}'
t_EVERB            = r'\\end\{verbatim\}'
t_BLSTLISTING      = r'\\begin\{lstlisting\}'
t_ELSTLISTING      = r'\\end\{lstlisting\}'
t_BOUTPUTLISTING   = r'\\begin\{outputlisting\}'
t_EOUTPUTLISTING   = r'\\end\{outputlisting\}'
t_BBASHLISTING     = r'\\begin\{bashlisting\}'
t_EBASHLISTING     = r'\\end\{bashlisting\}'
t_BMAKELISTING     = r'\\begin\{makelisting\}'
t_EMAKELISTING     = r'\\end\{makelisting\}'
t_BTIKZPICTURE     = r'\\begin\{tikzpicture\}'
t_ETIKZPICTURE     = r'\\end\{tikzpicture\}'
t_PLUS             = r'\+'
t_MINUS            = r'-'
t_TIMES            = r'\*'
t_DIVIDE           = r'/'
t_MOD              = r'%'
t_OR               = r'\|'
t_AND              = r'&'
t_NOT              = r'~'
t_XOR              = r'\^'
t_LSHIFT           = r'<<'
t_RSHIFT           = r'>>'
t_LOR              = r'\|\|'
t_LAND             = r'&&'
t_LNOT             = r'!'
t_LT               = r'<'
t_GT               = r'>'
t_LE               = r'<='
t_GE               = r'>='
t_EQ               = r'=='
t_NE               = r'!='
t_SLASH            = r'\\'
t_DOLLAR           = r'\$'
t_AT               = r'@'
t_SHARP            = r'\#'
t_DOUBLEQUOTE      = r'"'
t_QUOTE            = r'\''
t_BACKQUOTE        = r'`'
t_HAT              = r'\^'

# Assignment operators
t_EQUALS           = r'='
t_TIMESEQUAL       = r'\*='
t_DIVEQUAL         = r'/='
t_MODEQUAL         = r'%='
t_PLUSEQUAL        = r'\+='
t_MINUSEQUAL       = r'-='
t_LSHIFTEQUAL      = r'<<='
t_RSHIFTEQUAL      = r'>>='
t_ANDEQUAL         = r'&='
t_OREQUAL          = r'\|='
t_XOREQUAL         = r'^='

# Increment/decrement
t_PLUSPLUS         = r'\+\+'
t_MINUSMINUS       = r'--'

# ->
t_ARROW            = r'->'

# ?
t_CONDOP           = r'\?'

# Delimiters
t_LPAREN           = r'\('
t_RPAREN           = r'\)'
t_LBRACKET         = r'\['
t_RBRACKET         = r'\]'
t_LBRACE           = r'\{'
t_RBRACE           = r'\}'
t_COMMA            = r','
t_PERIOD           = r'\.'
t_SEMI             = r';'
t_COLON            = r':'
t_ELLIPSIS         = r'\.\.\.'

t_SPACE            = r'\ +'
t_NEWLINE          = r'\n'

# Identifiers and reserved words
def t_ID(t):
    r'[A-Za-z_][\w_]*'
    return t

# Integer literal
t_ICONST = r'\d+([uU]|[lL]|[uU][lL]|[lL][uU])?'

# Floating literal
t_FCONST = r'((\d+)(\.\d+)(e(\+|-)?(\d+))? | (\d+)e(\+|-)?(\d+))([lL]|[fF])?'

# String literal
t_SCONST = r'\"([^\\\n]|(\\.))*?\"'

# Character constant 'c' or L'c'
t_CCONST = r'(L)?\'([^\\\n]|(\\.))*?\''

def t_error(t):
    print("Illegal character %s" % repr(t.value[0]))
    t.skip(1)
#     return t

lexer = lex.lex(optimize=1)
if __name__ == "__main__":

    #
    # use Use LOC as PETSC_DIR [for reading/writing relevant files]
    #
    try:
        PETSC_DIR = sys.argv[1]
        htmlmapfile = sys.argv[2]
    except:
        raise RuntimeError('Insufficient arguments. Use: '+ sys.argv[0] + 'PETSC_DIR htmlmap')

    # get the version string for this release
    try:
        fd = open(os.path.join(PETSC_DIR,'include','petscversion.h'))
    except:
        raise RuntimeError('Unable to open petscversion.h\n')

    buf=fd.read()
    fd.close()
    try:
        isrelease       = re.compile(' PETSC_VERSION_RELEASE[ ]*([0-9]*)').search(buf).group(1)
        majorversion    = re.compile(' PETSC_VERSION_MAJOR[ ]*([0-9]*)').search(buf).group(1)
        minorversion    = re.compile(' PETSC_VERSION_MINOR[ ]*([0-9]*)').search(buf).group(1)
    except:
        raise RuntimeError('Unable to read version information from petscversion.h')

    if isrelease == '0':
        version = 'dev'
    else:
        version=str(majorversion)+'.'+str(minorversion)

#
#  Read in mapping of names to manual pages
#
    reg = re.compile('man:\+([a-zA-Z_0-9]*)\+\+([a-zA-Z_0-9 .:]*)\+\+\+\+man\+([a-zA-Z_0-9#./:-]*)')
    try:
        fd = open(os.path.join(htmlmapfile))
    except:
        raise RuntimeError('Unable to open htmlmap-file: '+htmlmapfile+'\n')

    lines = fd.readlines()
    fd.close()
    n = len(lines)
    mappedstring = { }
    mappedlink   = { }
    for i in range(0,n):
        fl = reg.search(lines[i])
        if not fl:
           print('Bad line in '+htmlmapfile,lines[i])
        else:
            tofind = fl.group(1)
#   replace all _ in tofind with \_
#            m     = len(tofind)
#            tfind = tofind[0]
#	    for j in range(1,m):
#		if tofind[j] == '_':
#		    tfind = tfind+'\\'
#                tfind = tfind+tofind[j]
            mappedstring[tofind] = fl.group(2)
            mappedlink[tofind]   = fl.group(3)

#   Read in file that is to be mapped
    lines = sys.stdin.read()
    lex.input(lines)

    text     = ''
    bracket  = 0
    vbracket = 0
    lstinline_bracket = 0
    lstinlinemod_bracket = 0
    lstlisting_bracket = 0 
    outputlisting_bracket = 0
    bashlisting_bracket = 0
    makelisting_bracket = 0
    tikzpicture_bracket = 0;
    while 1:
        token = lex.token()       # Get a token
        if not token: break        # No more tokens
        if token.type == 'NEWLINE':
            print(text)
            text = ''
        else:
            value = token.value
            # various verbatim-style environments disable bracket count
            # Note that a closing bracket inside a \trl{} will break things
            #print value + " : " + str(bracket) + str(vbracket) + str(lstinline_bracket) + str(outputlisting_bracket) + str(bashlisting_bracket) + str(makelisting_bracket) + str(lstlisting_bracket) + str(tikzpicture_bracket)
            if value == '\\begin{verbatim}':
                vbracket = vbracket + 1;
            if value == '\\begin{bashlisting}':
                bashlisting_bracket = bashlisting_bracket + 1;
            if value == '\\begin{makelisting}':
                makelisting_bracket = makelisting_bracket + 1;
            if value == '\\begin{outputlisting}':
                outputlisting_bracket = outputlisting_bracket + 1;
            if value == '\\begin{lstlisting}':
                lstlisting_bracket = lstlisting_bracket + 1;
            if value == '\\begin{tikzpicture}':
                tikzpicture_bracket = tikzpicture_bracket + 1;
            # \href cannot be used in many places in Latex
            if value in ['\\href{','\\findex{','\\sindex{','\\subsection{','\\chapter{','\\section{','\\caption{','\\trl{'] and vbracket == 0 and lstlisting_bracket == 0 and outputlisting_bracket==0 and bashlisting_bracket==0 and makelisting_bracket==0 and tikzpicture_bracket==0:
        	bracket = bracket + 1;
            #We keep track of whether we are inside an inline listing
            elif value in ['\\lstinline{'] and vbracket == 0 and lstlisting_bracket == 0 and outputlisting_bracket==0 and bashlisting_bracket==0 and makelisting_bracket==0 and tikzpicture_bracket==0:
                lstinline_bracket = lstinline_bracket + 1
                if lstinline_bracket > 1 :
                    raise Exception('Nested \\lstinline detected')
            elif value in ['\\lstinline|'] and vbracket == 0 and lstlisting_bracket == 0 and outputlisting_bracket==0 and bashlisting_bracket==0 and makelisting_bracket==0 and tikzpicture_bracket==0:
                lstinlinemod_bracket = lstinlinemod_bracket + 1
                if lstinlinemod_bracket > 1 :
                    raise Exception('Nested \\lstinline (mod) detected')
            if bracket == 0 and vbracket == 0 and outputlisting_bracket == 0 and bashlisting_bracket == 0 and makelisting_bracket==0 and tikzpicture_bracket==0:
        	value = token.value
        	if value in mappedstring:
                    mvalue = mappedstring[value].replace('_','\\_')
                    if lstlisting_bracket > 0 :
                        # NOTE: The latex listings escapechar ($) is hard-coded here 
                        # and must match the latex source 
                        # (see src/docs/tex/manual/manualpreamble.tex)
                        value = '$\\href{'+'https://www.mcs.anl.gov/petsc/petsc-'+version+'/docs/'+mappedlink[value]+'}{'+mvalue+'}\\findex{'+value+'}$'
                    elif lstinline_bracket > 0 :
                        value = '}\\href{'+'https://www.mcs.anl.gov/petsc/petsc-'+version+'/docs/'+mappedlink[value]+'}{\\lstinline{'+mvalue+'}}\\findex{'+value+'}\\lstinline{' #end and start a new inside href
                    elif lstinlinemod_bracket > 0 :
                        value = '|\\href{'+'https://www.mcs.anl.gov/petsc/petsc-'+version+'/docs/'+mappedlink[value]+'}{\\lstinline|'+mvalue+'|}\\findex{'+value+'}\\lstinline|' #end and start a new inside href
                    else :
                        value = '\\href{'+'https://www.mcs.anl.gov/petsc/petsc-'+version+'/docs/'+mappedlink[value]+'}{'+mvalue+'}\\findex{'+value+'}'
            else:
        	value = token.value
            if token.value[0] == '}' and lstinline_bracket > 0 :
                if bracket > 0 or vbracket > 0 or lstlisting_bracket > 0 or outputlisting_bracket > 0 or bashlisting_bracket > 0 or makelisting_bracket > 0 or tikzpicture_bracket > 0:
                    raise Exception("Unexpected to have anything nested inside of lstinline")
                lstinline_bracket = lstinline_bracket-1
            elif token.value[0] == '|' and lstinlinemod_bracket > 0 :
                if bracket > 0 or vbracket > 0 or lstlisting_bracket > 0 or outputlisting_bracket > 0 or bashlisting_bracket > 0 or makelisting_bracket >0 or tikzpicture_bracket > 0:
                    raise Exception("Unexpected to have anything nested inside of lstinline (mod)")
                lstinlinemod_bracket = lstinlinemod_bracket-1
            elif token.value[0] == '}' and bracket and vbracket == 0 and lstlisting_bracket == 0 and outputlisting_bracket==0 and bashlisting_bracket==0 and makelisting_bracket==0 and tikzpicture_bracket == 0:
        	bracket = bracket - 1;
            elif value == '\\end{verbatim}' and vbracket:
                vbracket = vbracket - 1;
            elif value == '\\end{outputlisting}' and outputlisting_bracket:
                outputlisting_bracket = outputlisting_bracket - 1;
            elif value == '\\end{bashlisting}' and bashlisting_bracket:
                bashlisting_bracket = bashlisting_bracket - 1;
            elif value == '\\end{makelisting}' and makelisting_bracket:
                makelisting_bracket = makelisting_bracket - 1;
            elif value == '\\end{lstlisting}' and lstlisting_bracket:
                lstlisting_bracket = lstlisting_bracket - 1;
            elif value == '\\end{tikzpicture}' and tikzpicture_bracket:
                tikzpicture_bracket = tikzpicture_bracket - 1;

            text = text+value
