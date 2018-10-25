/******************************************************************************
 *
 * Copyright (C) 1997-2015 by Dimitri van Heesch.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License is hereby
 * granted. No representations are made about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 * See the GNU General Public License for more details.
 *
 * Documents produced by Doxygen are derivative works derived from the
 * input used in their production; they are not affected by this license.
 *
 */

#include "htmlentity.h"
#include "message.h"
#include "ftextstream.h"

//! Number of doxygen commands mapped as if it were HTML entities
static const int g_numberHtmlMappedCmds = 11;

//! @brief Structure defining all HTML4 entities, doxygen extensions and doxygen commands representing special symbols.
//! @details In case an entity does not exist a NULL is given for the entity. The first column contains the symbolic code
//!          for the entity, see also doxparser.h The second column contains the name of the entity (without the starting \& and
//!          ending ;)
static struct htmlEntityInfo
{
  DocSymbol::SymType symb;
  const char *item;
  const char *UTF8;
  const char *html;
  const char *xml;
  const char *docbook;
  const char *latex;
  const char *man;
  const char *rtf;
  const char *asciidoc;
  DocSymbol::PerlSymb perl;
} g_htmlEntities[] =
{
#undef SYM
// helper macro to force consistent entries for the symbol and item columns
#define SYM(s) DocSymbol::Sym_##s,"&"#s";"
  // HTML4 entities
  // symb+item     UTF-8           html          xml                     docbook          latex                     man       rtf            asciidoc    perl
  { SYM(nbsp),     "\xc2\xa0",     "&#160;",     "<nonbreakablespace/>", "&#160;",        "~",                      " ",      "\\~",         "{nbsp}",   { " ",          DocSymbol::Perl_char    }},
  { SYM(iexcl),    "\xc2\xa1",     "&iexcl;",    "<iexcl/>",             "&#161;",        "!`",                     NULL,     "\\'A1",       "(C)",      { NULL,         DocSymbol::Perl_unknown }},
  { SYM(cent),     "\xc2\xa2",     "&cent;",     "<cent/>",              "&#162;",        "\\textcent{}",           NULL,     "\\'A2",       "&#162;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(pound),    "\xc2\xa3",     "&pound;",    "<pound/>",             "&#163;",        "{$\\pounds$}",           NULL,     "\\'A3",       "&#163;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(curren),   "\xc2\xa4",     "&curren;",   "<curren/>",            "&#164;",        "\\textcurrency{}",       NULL,     "\\'A4",       "&#164;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(yen),      "\xc2\xa5",     "&yen;",      "<yen/>",               "&#165;",        "{$\\yen$}",              NULL,     "\\'A5",       "&#165;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(brvbar),   "\xc2\xa6",     "&brvbar;",   "<brvbar/>",            "&#166;",        "\\textbrokenbar{}",      NULL,     "\\'A6",       "{brvbar}", { NULL,         DocSymbol::Perl_unknown }},
  { SYM(sect),     "\xc2\xa7",     "&sect;",     "<sect/>",              "<simplesect/>", "{$\\S$}",                NULL,     "\\'A7",       "== ",      { "sect",       DocSymbol::Perl_symbol  }},
  { SYM(uml),      "\xc2\xa8",     "&uml;",      "<umlaut/>",            "&#168;",        "\\textasciidieresis{}",  " \\*(4", "\\'A8",       "&#168;",   { " ",          DocSymbol::Perl_umlaut  }},
  { SYM(copy),     "\xc2\xa9",     "&copy;",     "<copy/>",              "&#169;",        "\\copyright{}",          "(C)",    "\\'A9",       "(C)",      { "copyright",  DocSymbol::Perl_symbol  }},
  { SYM(ordf),     "\xc2\xaa",     "&ordf;",     "<ordf/>",              "&#170;",        "\\textordfeminine{}",    NULL,     "\\'AA",       "&#170;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(laquo),    "\xc2\xab",     "&laquo;",    "<laquo/>",             "&#171;",        "\\guillemotleft{}",      NULL,     "\\'AB",       "&#171;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(not),      "\xc2\xac",     "&not;",      "<not/>",               "&#172;",        "\\textlnot",             NULL,     "\\'AC",       "&#172;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(shy),      "\xc2\xad",     "&shy;",      "<shy/>",               "&#173;",        "{$\\-$}",                NULL,     "\\-",         "&#173;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(reg),      "\xc2\xae",     "&reg;",      "<registered/>",        "&#174;",        "\\textregistered{}",     "(R)",    "\\'AE",       "(R)",      { "registered", DocSymbol::Perl_symbol  }},
  { SYM(macr),     "\xc2\xaf",     "&macr;",     "<macr/>",              "&#175;",        "\\={}",                  NULL,     "\\'AF",       "&#175;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(deg),      "\xc2\xb0",     "&deg;",      "<deg/>",               "&#176;",        "\\textdegree{}",         NULL,     "\\'B0",       "{deg}",    { "deg",        DocSymbol::Perl_symbol  }},
  { SYM(plusmn),   "\xc2\xb1",     "&plusmn;",   "<plusmn/>",            "&#177;",        "{$\\pm$}",               NULL,     "\\'B1",       "&#177;",   { "+/-",        DocSymbol::Perl_string  }},
  { SYM(sup2),     "\xc2\xb2",     "&sup2;",     "<sup2/>",              "&#178;",        "\\texttwosuperior{}",    NULL,     "\\'B2",       "&#178;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(sup3),     "\xc2\xb3",     "&sup3;",     "<sup3/>",              "&#179;",        "\\textthreesuperior{}",  NULL,     "\\'B3",       "&#179;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(acute),    "\xc2\xb4",     "&acute;",    "<acute/>",             "&#180;",        "\\'{}",                  NULL,     "\\'B4",       "&#180;",   { " ",          DocSymbol::Perl_acute   }},
  { SYM(micro),    "\xc2\xb5",     "&micro;",    "<micro/>",             "&#181;",        "{$\\mu$}",               NULL,     "\\'B5",       "&#181;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(para),     "\xc2\xb6",     "&para;",     "<para/>",              "&#182;",        "{$\\P$}",                NULL,     "\\'B6",       "&#182;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(middot),   "\xc2\xb7",     "&middot;",   "<middot/>",            "&#183;",        "\\textperiodcentered{}", NULL,     "\\'B7",       "&#183;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(cedil),    "\xc2\xb8",     "&cedil;",    "<cedil/>",             "&#184;",        "\\c{}",                  " \\*,",  "\\'B8",       "&#184;",   { " ",          DocSymbol::Perl_cedilla }},
  { SYM(sup1),     "\xc2\xb9",     "&sup1;",     "<sup1/>",              "&#185;",        "\\textonesuperior{}",    NULL,     "\\'B9",       "&#185;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(ordm),     "\xc2\xba",     "&ordm;",     "<ordm/>",              "&#186;",        "\\textordmasculine{}",   NULL,     "\\'BA",       "&#186;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(raquo),    "\xc2\xbb",     "&raquo;",    "<raquo/>",             "&#187;",        "\\guillemotright{}",     NULL,     "\\'BB",       "&#187;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(frac14),   "\xc2\xbc",     "&frac14;",   "<frac14/>",            "&#188;",        "{$\\frac14$}",           "1/4",    "\\'BC",       "&#188;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(frac12),   "\xc2\xbd",     "&frac12;",   "<frac12/>",            "&#189;",        "{$\\frac12$}",           "1/2",    "\\'BD",       "&#189;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(frac34),   "\xc2\xbe",     "&frac34;",   "<frac34/>",            "&#190;",        "{$\\frac34$}",           "3/4",    "\\'BE",       "&#190;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(iquest),   "\xc2\xbf",     "&iquest;",   "<iquest/>",            "&#191;",        "?`",                     NULL,     "\\'BF",       "&#191;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(Agrave),   "\xc3\x80",     "&Agrave;",   "<Agrave/>",            "&#192;",        "\\`{A}",                 "A\\*:",  "\\'C0",       "&#192;",   { "A",          DocSymbol::Perl_grave   }},
  { SYM(Aacute),   "\xc3\x81",     "&Aacute;",   "<Aacute/>",            "&#193;",        "\\'{A}",                 "A\\*(`", "\\'C1",       "&#193;",   { "A",          DocSymbol::Perl_acute   }},
  { SYM(Acirc),    "\xc3\x82",     "&Acirc;",    "<Acirc/>",             "&#194;",        "\\^{A}",                 "A\\*^",  "\\'C2",       "&#194;",   { "A",          DocSymbol::Perl_circ    }},
  { SYM(Atilde),   "\xc3\x83",     "&Atilde;",   "<Atilde/>",            "&#195;",        "\\~{A}",                 "A\\*~",  "\\'C3",       "&#195;",   { "A",          DocSymbol::Perl_tilde   }},
  { SYM(Auml),     "\xc3\x84",     "&Auml;",     "<Aumlaut/>",           "&#196;",        "\\\"{A}",                "A\\*(4", "\\'C4",       "&#196;",   { "A",          DocSymbol::Perl_umlaut  }},
  { SYM(Aring),    "\xc3\x85",     "&Aring;",    "<Aring/>",             "&#197;",        "\\AA",                   "A\\*o",  "\\'C5",       "&#197;",   { "A",          DocSymbol::Perl_ring    }},
  { SYM(AElig),    "\xc3\x86",     "&AElig;",    "<AElig/>",             "&#198;",        "{\\AE}",                 NULL,     "\\'C6",       "&#198;",   { "AElig",      DocSymbol::Perl_symbol  }},
  { SYM(Ccedil),   "\xc3\x87",     "&Ccedil;",   "<Ccedil/>",            "&#199;",        "\\c{C}",                 "C\\*,",  "\\'C7",       "&#199;",   { "C",          DocSymbol::Perl_cedilla }},
  { SYM(Egrave),   "\xc3\x88",     "&Egrave;",   "<Egrave/>",            "&#200;",        "\\`{E}",                 "E\\*:",  "\\'C8",       "&#200;",   { "E",          DocSymbol::Perl_grave   }},
  { SYM(Eacute),   "\xc3\x89",     "&Eacute;",   "<Eacute/>",            "&#201;",        "\\'{E}",                 "E\\*(`", "\\'C9",       "&#201;",   { "E",          DocSymbol::Perl_acute   }},
  { SYM(Ecirc),    "\xc3\x8a",     "&Ecirc;",    "<Ecirc/>",             "&#202;",        "\\^{E}",                 "E\\*^",  "\\'CA",       "&#202;",   { "E",          DocSymbol::Perl_circ    }},
  { SYM(Euml),     "\xc3\x8b",     "&Euml;",     "<Eumlaut/>",           "&#203;",        "\\\"{E}",                "E\\*(4", "\\'CB",       "&#203;",   { "E",          DocSymbol::Perl_umlaut  }},
  { SYM(Igrave),   "\xc3\x8c",     "&Igrave;",   "<Igrave/>",            "&#204;",        "\\`{I}",                 "I\\*:",  "\\'CC",       "&#204;",   { "I",          DocSymbol::Perl_grave   }},
  { SYM(Iacute),   "\xc3\x8d",     "&Iacute;",   "<Iacute/>",            "&#205;",        "\\'{I}",                 "I\\*(`", "\\'CD",       "&#205;",   { "I",          DocSymbol::Perl_acute   }},
  { SYM(Icirc),    "\xc3\x8e",     "&Icirc;",    "<Icirc/>",             "&#206;",        "\\^{I}",                 "I\\*^",  "\\'CE",       "&#206;",   { "I",          DocSymbol::Perl_circ    }},
  { SYM(Iuml),     "\xc3\x8f",     "&Iuml;",     "<Iumlaut/>",           "&#207;",        "\\\"{I}",                "I\\*(4", "\\'CF",       "&#207;",   { "I",          DocSymbol::Perl_umlaut  }},
  { SYM(ETH),      "\xc3\x90",     "&ETH;",      "<ETH/>",               "&#208;",        "\\DH",                   NULL,     "\\'D0",       "&#208;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(Ntilde),   "\xc3\x91",     "&Ntilde;",   "<Ntilde/>",            "&#209;",        "\\~{N}",                 "N\\*~",  "\\'D1",       "&#209;",   { "N",          DocSymbol::Perl_tilde   }},
  { SYM(Ograve),   "\xc3\x92",     "&Ograve;",   "<Ograve/>",            "&#210;",        "\\`{O}",                 "O\\*:",  "\\'D2",       "&#210;",   { "O",          DocSymbol::Perl_grave   }},
  { SYM(Oacute),   "\xc3\x93",     "&Oacute;",   "<Oacute/>",            "&#211;",        "\\'{O}",                 "O\\*(`", "\\'D3",       "&#211;",   { "O",          DocSymbol::Perl_acute   }},
  { SYM(Ocirc),    "\xc3\x94",     "&Ocirc;",    "<Ocirc/>",             "&#212;",        "\\^{O}",                 "O\\*^",  "\\'D4",       "&#212;",   { "O",          DocSymbol::Perl_circ    }},
  { SYM(Otilde),   "\xc3\x95",     "&Otilde;",   "<Otilde/>",            "&#213;",        "\\~{O}",                 "O\\*~",  "\\'D5",       "&#213;",   { "O",          DocSymbol::Perl_tilde   }},
  { SYM(Ouml),     "\xc3\x96",     "&Ouml;",     "<Oumlaut/>",           "&#214;",        "\\\"{O}",                "O\\*(4", "\\'D6",       "&#214;",   { "O",          DocSymbol::Perl_umlaut  }},
  { SYM(times),    "\xc3\x97",     "&times;",    "<times/>",             "&#215;",        "{$\\times$}",            NULL,     "\\'D7",       "&#215;",   { "*",          DocSymbol::Perl_char    }},
  { SYM(Oslash),   "\xc3\x98",     "&Oslash;",   "<Oslash/>",            "&#216;",        "{\\O}",                  "O\x08/", "\\'D8",       "&#216;",   { "O",          DocSymbol::Perl_slash   }},
  { SYM(Ugrave),   "\xc3\x99",     "&Ugrave;",   "<Ugrave/>",            "&#217;",        "\\`{U}",                 "U\\*:",  "\\'D9",       "&#217;",   { "U",          DocSymbol::Perl_grave   }},
  { SYM(Uacute),   "\xc3\x9a",     "&Uacute;",   "<Uacute/>",            "&#218;",        "\\'{U}",                 "U\\*(`", "\\'DA",       "&#218;",   { "U",          DocSymbol::Perl_acute   }},
  { SYM(Ucirc),    "\xc3\x9b",     "&Ucirc;",    "<Ucirc/>",             "&#219;",        "\\^{U}",                 "U\\*^",  "\\'DB",       "&#219;",   { "U",          DocSymbol::Perl_circ    }},
  { SYM(Uuml),     "\xc3\x9c",     "&Uuml;",     "<Uumlaut/>",           "&#220;",        "\\\"{U}",                "U\\*(4", "\\'DC",       "&#220;",   { "U",          DocSymbol::Perl_umlaut  }},
  { SYM(Yacute),   "\xc3\x9d",     "&Yacute;",   "<Yacute/>",            "&#221;",        "\\'{Y}",                 "Y\\*(`", "\\'DD",       "&#221;",   { "Y",          DocSymbol::Perl_acute   }},
  { SYM(THORN),    "\xc3\x9e",     "&THORN;",    "<THORN/>",             "&#222;",        "\\TH",                   NULL,     "\\'DE",       "&#222;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(szlig),    "\xc3\x9f",     "&szlig;",    "<szlig/>",             "&#223;",        "{\\ss}",                 "s\\*:",  "\\'DF",       "&#223;",   { "szlig",      DocSymbol::Perl_symbol  }},
  { SYM(agrave),   "\xc3\xa0",     "&agrave;",   "<agrave/>",            "&#224;",        "\\`{a}",                 "a\\*:",  "\\'E0",       "&#224;",   { "a",          DocSymbol::Perl_grave   }},
  { SYM(aacute),   "\xc3\xa1",     "&aacute;",   "<aacute/>",            "&#225;",        "\\'{a}",                 "a\\*(`", "\\'E1",       "&#225;",   { "a",          DocSymbol::Perl_acute   }},
  { SYM(acirc),    "\xc3\xa2",     "&acirc;",    "<acirc/>",             "&#226;",        "\\^{a}",                 "a\\*^",  "\\'E2",       "&#226;",   { "a",          DocSymbol::Perl_circ    }},
  { SYM(atilde),   "\xc3\xa3",     "&atilde;",   "<atilde/>",            "&#227;",        "\\~{a}",                 "a\\*~",  "\\'E3",       "&#227;",   { "a",          DocSymbol::Perl_tilde   }},
  { SYM(auml),     "\xc3\xa4",     "&auml;",     "<aumlaut/>",           "&#228;",        "\\\"{a}",                "a\\*(4", "\\'E4",       "&#228;",   { "a",          DocSymbol::Perl_umlaut  }},
  { SYM(aring),    "\xc3\xa5",     "&aring;",    "<aring/>",             "&#229;",        "\\aa",                   "a\\*o",  "\\'E5",       "&#229;",   { "a",          DocSymbol::Perl_ring    }},
  { SYM(aelig),    "\xc3\xa6",     "&aelig;",    "<aelig/>",             "&#230;",        "{\\ae}",                 NULL,     "\\'E6",       "&#230;",   { "aelig",      DocSymbol::Perl_symbol  }},
  { SYM(ccedil),   "\xc3\xa7",     "&ccedil;",   "<ccedil/>",            "&#231;",        "\\c{c}",                 "c\\*,",  "\\'E7",       "&#231;",   { "c",          DocSymbol::Perl_cedilla }},
  { SYM(egrave),   "\xc3\xa8",     "&egrave;",   "<egrave/>",            "&#232;",        "\\`{e}",                 "e\\*:",  "\\'E8",       "&#232;",   { "e",          DocSymbol::Perl_grave   }},
  { SYM(eacute),   "\xc3\xa9",     "&eacute;",   "<eacute/>",            "&#233;",        "\\'{e}",                 "e\\*(`", "\\'E9",       "&#233;",   { "e",          DocSymbol::Perl_acute   }},
  { SYM(ecirc),    "\xc3\xaa",     "&ecirc;",    "<ecirc/>",             "&#234;",        "\\^{e}",                 "e\\*^",  "\\'EA",       "&#234;",   { "e",          DocSymbol::Perl_circ    }},
  { SYM(euml),     "\xc3\xab",     "&euml;",     "<eumlaut/>",           "&#235;",        "\\\"{e}",                "e\\*(4", "\\'EB",       "&#235;",   { "e",          DocSymbol::Perl_umlaut  }},
  { SYM(igrave),   "\xc3\xac",     "&igrave;",   "<igrave/>",            "&#236;",        "\\`{\\i}",               "i\\*:",  "\\'EC",       "&#236;",   { "i",          DocSymbol::Perl_grave   }},
  { SYM(iacute),   "\xc3\xad",     "&iacute;",   "<iacute/>",            "&#237;",        "\\'{\\i}",               "i\\*(`", "\\'ED",       "&#237;",   { "i",          DocSymbol::Perl_acute   }},
  { SYM(icirc),    "\xc3\xae",     "&icirc;",    "<icirc/>",             "&#238;",        "\\^{\\i}",               "i\\*^",  "\\'EE",       "&#238;",   { "i",          DocSymbol::Perl_circ    }},
  { SYM(iuml),     "\xc3\xaf",     "&iuml;",     "<iumlaut/>",           "&#239;",        "\\\"{\\i}",              "i\\*(4", "\\'EF",       "&#239;",   { "i",          DocSymbol::Perl_umlaut  }},
  { SYM(eth),      "\xc3\xb0",     "&eth;",      "<eth/>",               "&#240;",        "\\dh",                   NULL,     "\\'F0",       "&#240;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(ntilde),   "\xc3\xb1",     "&ntilde;",   "<ntilde/>",            "&#241;",        "\\~{n}",                 "n\\*~",  "\\'F1",       "&#241;",   { "n",          DocSymbol::Perl_tilde   }},
  { SYM(ograve),   "\xc3\xb2",     "&ograve;",   "<ograve/>",            "&#242;",        "\\`{o}",                 "o\\*:",  "\\'F2",       "&#242;",   { "o",          DocSymbol::Perl_grave   }},
  { SYM(oacute),   "\xc3\xb3",     "&oacute;",   "<oacute/>",            "&#243;",        "\\'{o}",                 "o\\*(`", "\\'F3",       "&#243;",   { "o",          DocSymbol::Perl_acute   }},
  { SYM(ocirc),    "\xc3\xb4",     "&ocirc;",    "<ocirc/>",             "&#244;",        "\\^{o}",                 "o\\*^",  "\\'F4",       "&#244;",   { "o",          DocSymbol::Perl_circ    }},
  { SYM(otilde),   "\xc3\xb5",     "&otilde;",   "<otilde/>",            "&#245;",        "\\~{o}",                 "o\\*~",  "\\'F5",       "&#245;",   { "o",          DocSymbol::Perl_tilde   }},
  { SYM(ouml),     "\xc3\xb6",     "&ouml;",     "<oumlaut/>",           "&#246;",        "\\\"{o}",                "o\\*(4", "\\'F6",       "&#246;",   { "o",          DocSymbol::Perl_umlaut  }},
  { SYM(divide),   "\xc3\xb7",     "&divide;",   "<divide/>",            "&#247;",        "{$\\div$}",              NULL,     "\\'F7",       "&#247;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(oslash),   "\xc3\xb8",     "&oslash;",   "<oslash/>",            "&#248;",        "{\\o}",                  "o\x08/", "\\'F8",       "&#248;",   { "o",          DocSymbol::Perl_slash   }},
  { SYM(ugrave),   "\xc3\xb9",     "&ugrave;",   "<ugrave/>",            "&#249;",        "\\`{u}",                 "u\\*:",  "\\'F9",       "&#249;",   { "u",          DocSymbol::Perl_grave   }},
  { SYM(uacute),   "\xc3\xba",     "&uacute;",   "<uacute/>",            "&#250;",        "\\'{u}",                 "u\\*(`", "\\'FA",       "&#250;",   { "u",          DocSymbol::Perl_acute   }},
  { SYM(ucirc),    "\xc3\xbb",     "&ucirc;",    "<ucirc/>",             "&#251;",        "\\^{u}",                 "u\\*^",  "\\'FB",       "&#251;",   { "u",          DocSymbol::Perl_circ    }},
  { SYM(uuml),     "\xc3\xbc",     "&uuml;",     "<uumlaut/>",           "&#252;",        "\\\"{u}",                "u\\*(4", "\\'FC",       "&#252;",   { "u",          DocSymbol::Perl_umlaut  }},
  { SYM(yacute),   "\xc3\xbd",     "&yacute;",   "<yacute/>",            "&#253;",        "\\'{y}",                 "y\\*(`", "\\'FD",       "&#253;",   { "y",          DocSymbol::Perl_acute   }},
  { SYM(thorn),    "\xc3\xbe",     "&thorn;",    "<thorn/>",             "&#254;",        "\\th",                   NULL,     "\\'FE",       "&#254;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(yuml),     "\xc3\xbf",     "&yuml;",     "<yumlaut/>",           "&#255;",        "\\\"{y}",                "y\\*(4", "\\'FF",       "&#255;",   { "y",          DocSymbol::Perl_umlaut  }},
  { SYM(fnof),     "\xc6\x92",     "&fnof;",     "<fnof/>",              "&#402;",        "\\textflorin",           NULL,     "\\'83",       "&#402;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(Alpha),    "\xce\x91",     "&Alpha;",    "<Alpha/>",             "&#913;",        "A",                      NULL,     "\\u0913?",    "&#913;",   { "A",          DocSymbol::Perl_char    }},
  { SYM(Beta),     "\xce\x92",     "&Beta;",     "<Beta/>",              "&#914;",        "B",                      NULL,     "\\u0914?",    "&#914;",   { "B",          DocSymbol::Perl_char    }},
  { SYM(Gamma),    "\xce\x93",     "&Gamma;",    "<Gamma/>",             "&#915;",        "{$\\Gamma$}",            NULL,     "\\u0915?",    "&#915;",   { "Gamma",      DocSymbol::Perl_symbol  }},
  { SYM(Delta),    "\xce\x94",     "&Delta;",    "<Delta/>",             "&#916;",        "{$\\Delta$}",            NULL,     "\\u0916?",    "&#916;",   { "Delta",      DocSymbol::Perl_symbol  }},
  { SYM(Epsilon),  "\xce\x95",     "&Epsilon;",  "<Epsilon/>",           "&#917;",        "E",                      NULL,     "\\u0917?",    "&#917;",   { "E",          DocSymbol::Perl_char    }},
  { SYM(Zeta),     "\xce\x96",     "&Zeta;",     "<Zeta/>",              "&#918;",        "Z",                      NULL,     "\\u0918?",    "&#918;",   { "Z",          DocSymbol::Perl_char    }},
  { SYM(Eta),      "\xce\x97",     "&Eta;",      "<Eta/>",               "&#919;",        "H",                      NULL,     "\\u0919?",    "&#919;",   { "H",          DocSymbol::Perl_char    }},
  { SYM(Theta),    "\xce\x98",     "&Theta;",    "<Theta/>",             "&#920;",        "{$\\Theta$}",            NULL,     "\\u0920?",    "&#920;",   { "Theta",      DocSymbol::Perl_symbol  }},
  { SYM(Iota),     "\xce\x99",     "&Iota;",     "<Iota/>",              "&#921;",        "I",                      NULL,     "\\u0921?",    "&#921;",   { "I",          DocSymbol::Perl_char    }},
  { SYM(Kappa),    "\xce\x9a",     "&Kappa;",    "<Kappa/>",             "&#922;",        "K",                      NULL,     "\\u0922?",    "&#922;",   { "K",          DocSymbol::Perl_char    }},
  { SYM(Lambda),   "\xce\x9b",     "&Lambda;",   "<Lambda/>",            "&#923;",        "{$\\Lambda$}",           NULL,     "\\u0923?",    "&#923;",   { "Lambda",     DocSymbol::Perl_symbol  }},
  { SYM(Mu),       "\xce\x9c",     "&Mu;",       "<Mu/>",                "&#924;",        "M",                      NULL,     "\\u0924?",    "&#924;",   { "M",          DocSymbol::Perl_char    }},
  { SYM(Nu),       "\xce\x9d",     "&Nu;",       "<Nu/>",                "&#925;",        "N",                      NULL,     "\\u0925?",    "&#925;",   { "N",          DocSymbol::Perl_char    }},
  { SYM(Xi),       "\xce\x9e",     "&Xi;",       "<Xi/>",                "&#926;",        "{$\\Xi$}",               NULL,     "\\u0926?",    "&#926;",   { "Xi",         DocSymbol::Perl_symbol  }},
  { SYM(Omicron),  "\xce\x9f",     "&Omicron;",  "<Omicron/>",           "&#927;",        "O",                      NULL,     "\\u0927?",    "&#927;",   { "O",          DocSymbol::Perl_char    }},
  { SYM(Pi),       "\xce\xa0",     "&Pi;",       "<Pi/>",                "&#928;",        "{$\\Pi$}",               NULL,     "\\u0928?",    "&#928;",   { "Pi",         DocSymbol::Perl_symbol  }},
  { SYM(Rho),      "\xce\xa1",     "&Rho;",      "<Rho/>",               "&#929;",        "P",                      NULL,     "\\u0929?",    "&#929;",   { "P",          DocSymbol::Perl_char    }},
  { SYM(Sigma),    "\xce\xa3",     "&Sigma;",    "<Sigma/>",             "&#931;",        "{$\\Sigma$}",            NULL,     "\\u0931?",    "&#931;",   { "Sigma",      DocSymbol::Perl_symbol  }},
  { SYM(Tau),      "\xce\xa4",     "&Tau;",      "<Tau/>",               "&#932;",        "T",                      NULL,     "\\u0932?",    "&#932;",   { "T",          DocSymbol::Perl_char    }},
  { SYM(Upsilon),  "\xce\xa5",     "&Upsilon;",  "<Upsilon/>",           "&#933;",        "{$\\Upsilon$}",          NULL,     "\\u0933?",    "&#933;",   { "Upsilon",    DocSymbol::Perl_symbol  }},
  { SYM(Phi),      "\xce\xa6",     "&Phi;",      "<Phi/>",               "&#934;",        "{$\\Phi$}",              NULL,     "\\u0934?",    "&#934;",   { "Phi",        DocSymbol::Perl_symbol  }},
  { SYM(Chi),      "\xce\xa7",     "&Chi;",      "<Chi/>",               "&#935;",        "X",                      NULL,     "\\u0935?",    "&#935;",   { "X",          DocSymbol::Perl_char    }},
  { SYM(Psi),      "\xce\xa8",     "&Psi;",      "<Psi/>",               "&#936;",        "{$\\Psi$}",              NULL,     "\\u0936?",    "&#936;",   { "Psi",        DocSymbol::Perl_symbol  }},
  { SYM(Omega),    "\xce\xa9",     "&Omega;",    "<Omega/>",             "&#937;",        "{$\\Omega$}",            NULL,     "\\u0937?",    "&#937;",   { "Omega",      DocSymbol::Perl_symbol  }},
  { SYM(alpha),    "\xce\xb1",     "&alpha;",    "<alpha/>",             "&#945;",        "{$\\alpha$}",            NULL,     "\\u0945?",    "&#945;",   { "alpha",      DocSymbol::Perl_symbol  }},
  { SYM(beta),     "\xce\xb2",     "&beta;",     "<beta/>",              "&#946;",        "{$\\beta$}",             NULL,     "\\u0946?",    "&#946;",   { "beta",       DocSymbol::Perl_symbol  }},
  { SYM(gamma),    "\xce\xb3",     "&gamma;",    "<gamma/>",             "&#947;",        "{$\\gamma$}",            NULL,     "\\u0947?",    "&#947;",   { "gamma",      DocSymbol::Perl_symbol  }},
  { SYM(delta),    "\xce\xb4",     "&delta;",    "<delta/>",             "&#948;",        "{$\\delta$}",            NULL,     "\\u0948?",    "&#948;",   { "delta",      DocSymbol::Perl_symbol  }},
  { SYM(epsilon),  "\xce\xb5",     "&epsilon;",  "<epsilon/>",           "&#949;",        "{$\\varepsilon$}",       NULL,     "\\u0949?",    "&#949;",   { "epsilon",    DocSymbol::Perl_symbol  }},
  { SYM(zeta),     "\xce\xb6",     "&zeta;",     "<zeta/>",              "&#950;",        "{$\\zeta$}",             NULL,     "\\u0950?",    "&#950;",   { "zeta",       DocSymbol::Perl_symbol  }},
  { SYM(eta),      "\xce\xb7",     "&eta;",      "<eta/>",               "&#951;",        "{$\\eta$}",              NULL,     "\\u0951?",    "&#951;",   { "eta",        DocSymbol::Perl_symbol  }},
  { SYM(theta),    "\xce\xb8",     "&theta;",    "<theta/>",             "&#952;",        "{$\\theta$}",            NULL,     "\\u0952?",    "&#952;",   { "theta",      DocSymbol::Perl_symbol  }},
  { SYM(iota),     "\xce\xb9",     "&iota;",     "<iota/>",              "&#953;",        "{$\\iota$}",             NULL,     "\\u0953?",    "&#953;",   { "iota",       DocSymbol::Perl_symbol  }},
  { SYM(kappa),    "\xce\xba",     "&kappa;",    "<kappa/>",             "&#954;",        "{$\\kappa$}",            NULL,     "\\u0954?",    "&#954;",   { "kappa",      DocSymbol::Perl_symbol  }},
  { SYM(lambda),   "\xce\xbb",     "&lambda;",   "<lambda/>",            "&#955;",        "{$\\lambda$}",           NULL,     "\\u0955?",    "&#955;",   { "lambda",     DocSymbol::Perl_symbol  }},
  { SYM(mu),       "\xce\xbc",     "&mu;",       "<mu/>",                "&#956;",        "{$\\mu$}",               NULL,     "\\u0956?",    "&#956;",   { "mu",         DocSymbol::Perl_symbol  }},
  { SYM(nu),       "\xce\xbd",     "&nu;",       "<nu/>",                "&#957;",        "{$\\nu$}",               NULL,     "\\u0957?",    "&#957;",   { "nu",         DocSymbol::Perl_symbol  }},
  { SYM(xi),       "\xce\xbe",     "&xi;",       "<xi/>",                "&#958;",        "{$\\xi$}",               NULL,     "\\u0958?",    "&#958;",   { "xi",         DocSymbol::Perl_symbol  }},
  { SYM(omicron),  "\xce\xbf",     "&omicron;",  "<omicron/>",           "&#959;",        "{$\\omicron$}",          NULL,     "\\u0959?",    "&#959;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(pi),       "\xcf\x80",     "&pi;",       "<pi/>",                "&#960;",        "{$\\pi$}",               NULL,     "\\u0960?",    "&#960;",   { "pi",         DocSymbol::Perl_symbol  }},
  { SYM(rho),      "\xcf\x81",     "&rho;",      "<rho/>",               "&#961;",        "{$\\rho$}",              NULL,     "\\u0961?",    "&#961;",   { "rho",        DocSymbol::Perl_symbol  }},
  { SYM(sigmaf),   "\xcf\x82",     "&sigmaf;",   "<sigmaf/>",            "&#962;",        "{$\\varsigma$}",         NULL,     "\\u0962?",    "&#962;",   { "sigma",      DocSymbol::Perl_symbol  }},
  { SYM(sigma),    "\xcf\x83",     "&sigma;",    "<sigma/>",             "&#963;",        "{$\\sigma$}",            NULL,     "\\u0963?",    "&#963;",   { "sigma",      DocSymbol::Perl_symbol  }},
  { SYM(tau),      "\xcf\x84",     "&tau;",      "<tau/>",               "&#964;",        "{$\\tau$}",              NULL,     "\\u0964?",    "&#964;",   { "tau",        DocSymbol::Perl_symbol  }},
  { SYM(upsilon),  "\xcf\x85",     "&upsilon;",  "<upsilon/>",           "&#965;",        "{$\\upsilon$}",          NULL,     "\\u0965?",    "&#965;",   { "upsilon",    DocSymbol::Perl_symbol  }},
  { SYM(phi),      "\xcf\x86",     "&phi;",      "<phi/>",               "&#966;",        "{$\\varphi$}",           NULL,     "\\u0966?",    "&#966;",   { "phi",        DocSymbol::Perl_symbol  }},
  { SYM(chi),      "\xcf\x87",     "&chi;",      "<chi/>",               "&#967;",        "{$\\chi$}",              NULL,     "\\u0967?",    "&#967;",   { "chi",        DocSymbol::Perl_symbol  }},
  { SYM(psi),      "\xcf\x88",     "&psi;",      "<psi/>",               "&#968;",        "{$\\psi$}",              NULL,     "\\u0968?",    "&#968;",   { "psi",        DocSymbol::Perl_symbol  }},
  { SYM(omega),    "\xcf\x89",     "&omega;",    "<omega/>",             "&#969;",        "{$\\omega$}",            NULL,     "\\u0969?",    "&#969;",   { "omega",      DocSymbol::Perl_symbol  }},
  { SYM(thetasym), "\xcf\x91",     "&thetasym;", "<thetasym/>",          "&#977;",        "{$\\vartheta$}",         NULL,     "\\u977?",     "&#977;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(upsih),    "\xcf\x92",     "&upsih;",    "<upsih/>",             "&#978;",        "{$\\Upsilon$}",          NULL,     "\\u978?",     "&#978;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(piv),      "\xcf\x96",     "&piv;",      "<piv/>",               "&#982;",        "{$\\varpi$}",            NULL,     "\\u982?",     "&#982;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(bull),     "\xe2\x80\xa2", "&bull;",     "<bull/>",              "&#8226;",       "\\textbullet{}",         NULL,     "\\'95",       "&#8226;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(hellip),   "\xe2\x80\xa6", "&hellip;",   "<hellip/>",            "&#8230;",       "{$\\dots$}",             NULL,     "\\'85",       "&#8230;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(prime),    "\xe2\x80\xb2", "&prime;",    "<prime/>",             "&#8242;",       "'",                      NULL,     "\\u8242?",    "&#8242;",  { "\\\'",       DocSymbol::Perl_string  }},
  { SYM(Prime),    "\xe2\x80\xb3", "&Prime;",    "<Prime/>",             "&#8243;",       "''",                     NULL,     "\\u8243?",    "&#8243;",  { "\"",         DocSymbol::Perl_char    }},
  { SYM(oline),    "\xe2\x80\xbe", "&oline;",    "<oline/>",             "&#8254;",       "{$\\overline{\\,}$}",    NULL,     "\\u8254?",    "&#8254;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(frasl),    "\xe2\x81\x84", "&frasl;",    "<frasl/>",             "&#8260;",       "/",                      NULL,     "\\u8260?",    "&#8260;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(weierp),   "\xe2\x84\x98", "&weierp;",   "<weierp/>",            "&#8472;",       "{$\\wp$}",               NULL,     "\\u8472?",    "&#8472;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(image),    "\xe2\x84\x91", "&image;",    "<imaginary/>",         "&#8465;",       "{$\\Im$}",               NULL,     "\\u8465?",    "&#8465;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(real),     "\xe2\x84\x9c", "&real;",     "<real/>",              "&#8476;",       "{$\\Re$}",               NULL,     "\\u8476?",    "&#8476;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(trade),    "\xe2\x84\xa2", "&trade;",    "<trademark/>",         "&#8482;",       "\\texttrademark{}",      "(TM)",   "\\'99",       "(TM)",     { "trademark",  DocSymbol::Perl_symbol  }},
  { SYM(alefsym),  "\xe2\x85\xb5", "&alefsym;",  "<alefsym/>",           "&#8501;",       "{$\\aleph$}",            NULL,     "\\u8501?",    "&#8501;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(larr),     "\xe2\x86\x90", "&larr;",     "<larr/>",              "&#8592;",       "{$\\leftarrow$}",        NULL,     "\\u8592?",    "&#8592;",  { "<-",         DocSymbol::Perl_string  }},
  { SYM(uarr),     "\xe2\x86\x91", "&uarr;",     "<uarr/>",              "&#8593;",       "{$\\uparrow$}",          NULL,     "\\u8593?",    "&#8593;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(rarr),     "\xe2\x86\x92", "&rarr;",     "<rarr/>",              "&#8594;",       "{$\\rightarrow$}",       NULL,     "\\u8594?",    "&#8594;",  { "->",         DocSymbol::Perl_string  }},
  { SYM(darr),     "\xe2\x86\x93", "&darr;",     "<darr/>",              "&#8595;",       "{$\\downarrow$}",        NULL,     "\\u8595?",    "&#8595;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(harr),     "\xe2\x86\x94", "&harr;",     "<harr/>",              "&#8596;",       "{$\\leftrightarrow$}",   NULL,     "\\u8596?",    "&#8596;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(crarr),    "\xe2\x86\xb5", "&crarr;",    "<crarr/>",             "&#8629;",       "{$\\hookleftarrow$}",    NULL,     "\\u8629?",    "&#8629;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(lArr),     "\xe2\x87\x90", "&lArr;",     "<lArr/>",              "&#8656;",       "{$\\Leftarrow$}",        NULL,     "\\u8656?",    "&#8656;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(uArr),     "\xe2\x87\x91", "&uArr;",     "<uArr/>",              "&#8657;",       "{$\\Uparrow$}",          NULL,     "\\u8657?",    "&#8657;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(rArr),     "\xe2\x87\x92", "&rArr;",     "<rArr/>",              "&#8658;",       "{$\\Rightarrow$}",       NULL,     "\\u8658?",    "&#8658;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(dArr),     "\xe2\x87\x93", "&dArr;",     "<dArr/>",              "&#8659;",       "{$\\Downarrow$}",        NULL,     "\\u8659?",    "&#8659;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(hArr),     "\xe2\x87\x94", "&hArr;",     "<hArr/>",              "&#8660;",       "{$\\Leftrightarrow$}",   NULL,     "\\u8660?",    "&#8660;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(forall),   "\xe2\x88\x80", "&forall;",   "<forall/>",            "&#8704;",       "{$\\forall$}",           NULL,     "\\u8704?",    "&#8704;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(part),     "\xe2\x88\x82", "&part;",     "<part/>",              "&#8706;",       "{$\\partial$}",          NULL,     "\\u8706?",    "&#8706;",  { "partial",    DocSymbol::Perl_symbol  }},
  { SYM(exist),    "\xe2\x88\x83", "&exist;",    "<exist/>",             "&#8707;",       "{$\\exists$}",           NULL,     "\\u8707?",    "&#8707;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(empty),    "\xe2\x88\x85", "&empty;",    "<empty/>",             "&#8709;",       "{$\\emptyset$}",         NULL,     "\\u8709?",    "&#8709;",  { "empty",      DocSymbol::Perl_symbol  }},
  { SYM(nabla),    "\xe2\x88\x87", "&nabla;",    "<nabla/>",             "&#8711;",       "{$\\nabla$}",            NULL,     "\\u8711?",    "&#8711;",  { "nabla",      DocSymbol::Perl_symbol  }},
  { SYM(isin),     "\xe2\x88\x88", "&isin;",     "<isin/>",              "&#8712;",       "{$\\in$}",               NULL,     "\\u8712?",    "&#8712;",  { "in",         DocSymbol::Perl_symbol  }},
  { SYM(notin),    "\xe2\x88\x89", "&notin;",    "<notin/>",             "&#8713;",       "{$\\notin$}",            NULL,     "\\u8713?",    "&#8713;",  { "notin",      DocSymbol::Perl_symbol  }},
  { SYM(ni),       "\xe2\x88\x8b", "&ni;",       "<ni/>",                "&#8715;",       "{$\\ni$}",               NULL,     "\\u8715?",    "&#8715;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(prod),     "\xe2\x88\x8f", "&prod;",     "<prod/>",              "&#8719;",       "{$\\prod$}",             NULL,     "\\u8719?",    "&#8719;",  { "prod",       DocSymbol::Perl_symbol  }},
  { SYM(sum),      "\xe2\x88\x91", "&sum;",      "<sum/>",               "&#8721;",       "{$\\sum$}",              NULL,     "\\u8721?",    "&#8721;",  { "sum",        DocSymbol::Perl_symbol  }},
  { SYM(minus),    "\xe2\x88\x92", "&minus;",    "<minus/>",             "&#8722;",       "-",                      NULL,     "\\u8722?",    "&#8722;",  { "-",          DocSymbol::Perl_char    }},
  { SYM(lowast),   "\xe2\x88\x97", "&lowast;",   "<lowast/>",            "&#8727;",       "{$\\ast$}",              NULL,     "\\u8727?",    "&#8727;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(radic),    "\xe2\x88\x9a", "&radic;",    "<radic/>",             "&#8730;",       "{$\\surd$}",             NULL,     "\\u8730?",    "&#8730;",  { "sqrt",       DocSymbol::Perl_symbol  }},
  { SYM(prop),     "\xe2\x88\x9d", "&prop;",     "<prop/>",              "&#8733;",       "{$\\propto$}",           NULL,     "\\u8733?",    "&#8733;",  { "propto",     DocSymbol::Perl_symbol  }},
  { SYM(infin),    "\xe2\x88\x9e", "&infin;",    "<infin/>",             "&#8734;",       "{$\\infty$}",            NULL,     "\\u8734?",    "&#8734;",  { "inf",        DocSymbol::Perl_symbol  }},
  { SYM(ang),      "\xe2\x88\xa0", "&ang;",      "<ang/>",               "&#8736;",       "{$\\angle$}",            NULL,     "\\u8736?",    "&#8736;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(and),      "\xe2\x88\xa7", "&and;",      "<and/>",               "&#8743;",       "{$\\wedge$}",            NULL,     "\\u8743?",    "&#8743;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(or),       "\xe2\x88\xa8", "&or;",       "<or/>",                "&#8744;",       "{$\\vee$}",              NULL,     "\\u8744?",    "&#8744;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(cap),      "\xe2\x88\xa9", "&cap;",      "<cap/>",               "&#8745;",       "{$\\cap$}",              NULL,     "\\u8745?",    "&#8745;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(cup),      "\xe2\x88\xaa", "&cup;",      "<cup/>",               "&#8746;",       "{$\\cup$}",              NULL,     "\\u8746?",    "&#8746;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(int),      "\xe2\x88\xab", "&int;",      "<int/>",               "&#8747;",       "{$\\int$}",              NULL,     "\\u8747?",    "&#8747;",  { "int",        DocSymbol::Perl_symbol  }},
  { SYM(there4),   "\xe2\x88\xb4", "&there4;",   "<there4/>",            "&#8756;",       "{$\\therefore$}",        NULL,     "\\u8756?",    "&#8756;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(sim),      "\xe2\x88\xbc", "&sim;",      "<sim/>",               "&#8764;",       "{$\\sim$}",              NULL,     "\\u8764?",    "&#8764;",  { "~",          DocSymbol::Perl_char    }},
  { SYM(cong),     "\xe2\x89\x85", "&cong;",     "<cong/>",              "&#8773;",       "{$\\cong$}",             NULL,     "\\u8773?",    "&#8773;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(asymp),    "\xe2\x89\x88", "&asymp;",    "<asymp/>",             "&#8776;",       "{$\\approx$}",           NULL,     "\\u8776?",    "&#8776;",  { "approx",     DocSymbol::Perl_symbol  }},
  { SYM(ne),       "\xe2\x89\xa0", "&ne;",       "<ne/>",                "&#8800;",       "{$\\ne$}",               NULL,     "\\u8800?",    "&#8800;",  { "!=",         DocSymbol::Perl_string  }},
  { SYM(equiv),    "\xe2\x89\xa1", "&equiv;",    "<equiv/>",             "&#8801;",       "{$\\equiv$}",            NULL,     "\\u8801?",    "&#8801;",  { "equiv",      DocSymbol::Perl_symbol  }},
  { SYM(le),       "\xe2\x89\xa4", "&le;",       "<le/>",                "&#8804;",       "{$\\le$}",               NULL,     "\\u8804?",    "&#8804;",  { "<=",         DocSymbol::Perl_string  }},
  { SYM(ge),       "\xe2\x89\xa5", "&ge;",       "<ge/>",                "&#8805;",       "{$\\ge$}",               NULL,     "\\u8805?",    "&#8805;",  { ">=",         DocSymbol::Perl_string  }},
  { SYM(sub),      "\xe2\x8a\x82", "&sub;",      "<sub/>",               "&#8834;",       "{$\\subset$}",           NULL,     "\\u8834?",    "&#8834;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(sup),      "\xe2\x8a\x83", "&sup;",      "<sup/>",               "&#8835;",       "{$\\supset$}",           NULL,     "\\u8835?",    "&#8835;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(nsub),     "\xe2\x8a\x84", "&nsub;",     "<nsub/>",              "&#8836;",       "{$\\not\\subset$}",      NULL,     "\\u8836?",    "&#8836;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(sube),     "\xe2\x8a\x86", "&sube;",     "<sube/>",              "&#8838;",       "{$\\subseteq$}",         NULL,     "\\u8838?",    "&#8838;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(supe),     "\xe2\x8a\x87", "&supe;",     "<supe/>",              "&#8839;",       "{$\\supseteq$}",         NULL,     "\\u8839?",    "&#8839;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(oplus),    "\xe2\x8a\x95", "&oplus;",    "<oplus/>",             "&#8853;",       "{$\\oplus$}",            NULL,     "\\u8853?",    "&#8853;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(otimes),   "\xe2\x8a\x97", "&otimes;",   "<otimes/>",            "&#8855;",       "{$\\otimes$}",           NULL,     "\\u8855?",    "&#8855;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(perp),     "\xe2\x8a\xa5", "&perp;",     "<perp/>",              "&#8869;",       "{$\\perp$}",             NULL,     "\\u8869?",    "&#8869;",  { "perp",       DocSymbol::Perl_symbol  }},
  { SYM(sdot),     "\xe2\x8b\x85", "&sdot;",     "<sdot/>",              "&#8901;",       "{$\\cdot$}",             NULL,     "\\u8901?",    "&#8901;",  { ".",          DocSymbol::Perl_char    }},
  { SYM(lceil),    "\xe2\x8c\x88", "&lceil;",    "<lceil/>",             "&#8968;",       "{$\\lceil$}",            NULL,     "\\u8968?",    "&#8968;",  { "lceil",      DocSymbol::Perl_symbol  }},
  { SYM(rceil),    "\xe2\x8c\x89", "&rceil;",    "<rceil/>",             "&#8969;",       "{$\\rceil$}",            NULL,     "\\u8969?",    "&#8969;",  { "rceil",      DocSymbol::Perl_symbol  }},
  { SYM(lfloor),   "\xe2\x8c\x8a", "&lfloor;",   "<lfloor/>",            "&#8970;",       "{$\\lfloor$}",           NULL,     "\\u8970?",    "&#8970;",  { "lfloor",     DocSymbol::Perl_symbol  }},
  { SYM(rfloor),   "\xe2\x8c\x8b", "&rfloor;",   "<rfloor/>",            "&#8971;",       "{$\\rfloor$}",           NULL,     "\\u8971?",    "&#8971;",  { "rfloor",     DocSymbol::Perl_symbol  }},
  { SYM(lang),     "\xe2\x8c\xa9", "&lang;",     "<lang/>",              "&#9001;",       "{$\\langle$}",           NULL,     "\\u9001?",    "&#9001;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(rang),     "\xe2\x8c\xaa", "&rang;",     "<rang/>",              "&#9002;",       "{$\\rangle$}",           NULL,     "\\u9002?",    "&#9002;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(loz),      "\xe2\x97\x8a", "&loz;",      "<loz/>",               "&#9674;",       "{$\\lozenge$}",          NULL,     "\\u9674?",    "&#9674;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(spades),   "\xe2\x99\xa0", "&spades;",   "<spades/>",            "&#9824;",       "{$\\spadesuit$}",        NULL,     "\\u9824?",    "&#9824;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(clubs),    "\xe2\x99\xa3", "&clubs;",    "<clubs/>",             "&#9827;",       "{$\\clubsuit$}",         NULL,     "\\u9827?",    "&#9827;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(hearts),   "\xe2\x99\xa5", "&hearts;",   "<hearts/>",            "&#9829;",       "{$\\heartsuit$}",        NULL,     "\\u9829?",    "&#9829;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(diams),    "\xe2\x99\xa6", "&diams;",    "<diams/>",             "&#9830;",       "{$\\diamondsuit$}",      NULL,     "\\u9830?",    "&#9830;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(quot),     "\"",           "&quot;",     "\"",                   "&quot;",        "\"",                     "\"",     "\"",          "\"",       { "\"",         DocSymbol::Perl_char    }},
  { SYM(amp),      "&",            "&amp;",      "&amp;",                "&amp;",         "\\&",                    "&",      "&",           "&",        { "&",          DocSymbol::Perl_char    }},
  { SYM(lt),       "<",            "&lt;",       "&lt;",                 "&lt;",          "<",                      "<",      "<",           "<",        { "<",          DocSymbol::Perl_char    }},
  { SYM(gt),       ">",            "&gt;",       "&gt;",                 "&gt;",          ">",                      ">",      ">",           ">",        { ">",          DocSymbol::Perl_char    }},
  { SYM(OElig),    "\xc5\x92",     "&OElig;",    "<OElig/>",             "&#338;",        "\\OE",                   NULL,     "\\'8C",       "&#338;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(oelig),    "\xc5\x93",     "&oelig;",    "<oelig/>",             "&#339;",        "\\oe",                   NULL,     "\\'9C",       "&#339;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(Scaron),   "\xc5\xa0",     "&Scaron;",   "<Scaron/>",            "&#352;",        "\\v{S}",                 NULL,     "\\'8A",       "&#352;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(scaron),   "\xc5\xa1",     "&scaron;",   "<scaron/>",            "&#353;",        "\\v{s}",                 NULL,     "\\'9A",       "&#353;",   { NULL,         DocSymbol::Perl_unknown }},
  { SYM(Yuml),     "\xc5\xb8",     "&Yuml;",     "<Yumlaut/>",           "&#376;",        "\\\"{Y}",                "Y\\*(4", "\\'9F",       "&#376;",   { "Y",          DocSymbol::Perl_umlaut  }},
  { SYM(circ),     "\xcb\x86",     "&circ;",     "<circ/>",              "&#710;",        "{$\\circ$}",             NULL,     "\\'88",       "&#710;",   { " ",          DocSymbol::Perl_circ    }},
  { SYM(tilde),    "\xcb\x9c",     "&tilde;",    "<tilde/>",             "&#732;",        "\\~{}",                  "~",      "\\'98",       "&#732;",   { " ",          DocSymbol::Perl_tilde   }},
  { SYM(ensp),     "\xe2\x80\x82", "&ensp;",     "<ensp/>",              "&#8194;",       "\\enskip{}",             NULL,     "{\\enspace}", "&#8194;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(emsp),     "\xe2\x80\x83", "&emsp;",     "<emsp/>",              "&#8195;",       "\\quad{}",               NULL,     "{\\emspace}", "&#8195;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(thinsp),   "\xe2\x80\x89", "&thinsp;",   "<thinsp/>",            "&#8201;",       "\\,",                    NULL,     "{\\qmspace}", "&#8201;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(zwnj),     "\xe2\x80\x8c", "&zwnj;",     "<zwnj/>",              "&#8204;",       "{}",                     NULL,     "\\zwnj",      "&#8204;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(zwj),      "\xe2\x80\x8d", "&zwj;",      "<zwj/>",               "&#8205;",       "",                       NULL,     "\\zwj",       "&#8205;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(lrm),      "\xe2\x80\x8e", "&lrm;",      "<lrm/>",               "&#8206;",       "",                       NULL,     "\\ltrmark",   "&#8206;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(rlm),      "\xe2\x80\x8f", "&rlm;",      "<rlm/>",               "&#8207;",       "",                       NULL,     "\\rtlmark",   "&#8207;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(ndash),    "\xe2\x80\x93", "&ndash;",    "<ndash/>",             "&#8211;",       "--",                     "--",     "\\'96",       "&#8211;",  { "-",          DocSymbol::Perl_char    }},
  { SYM(mdash),    "\xe2\x80\x94", "&mdash;",    "<mdash/>",             "&#8212;",       "---",                    "---",    "\\'97",       "--",       { "--",         DocSymbol::Perl_string  }},
  { SYM(lsquo),    "\xe2\x80\x98", "&lsquo;",    "<lsquo/>",             "&#8216;",       "`",                      "`",      "\\'91",       "{lsquo}",  { "\\\'",       DocSymbol::Perl_string  }},
  { SYM(rsquo),    "\xe2\x80\x99", "&rsquo;",    "<rsquo/>",             "&#8217;",       "'",                      "'",      "\\'92",       "{rsquo}",  { "\\\'",       DocSymbol::Perl_string  }},
  { SYM(sbquo),    "\xe2\x80\x9a", "&sbquo;",    "<sbquo/>",             "&#8218;",       "\\quotesinglbase{}",     NULL,     "\\'82",       "&#8218;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(ldquo),    "\xe2\x80\x9c", "&ldquo;",    "<ldquo/>",             "&#8220;",       "``",                     "``",     "\\'93",       "{ldquo}",  { "\"",         DocSymbol::Perl_char    }},
  { SYM(rdquo),    "\xe2\x80\x9d", "&rdquo;",    "<rdquo/>",             "&#8221;",       "''",                     "''",     "\\'94",       "{rdquo}",  { "\"",         DocSymbol::Perl_char    }},
  { SYM(bdquo),    "\xe2\x80\x9e", "&bdquo;",    "<bdquo/>",             "&#8222;",       "\\quotedblbase{}",       NULL,     "\\'84",       "&#8222;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(dagger),   "\xe2\x80\xa0", "&dagger;",   "<dagger/>",            "&#8224;",       "{$\\dagger$}",           NULL,     "\\'86",       "&#8224;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(Dagger),   "\xe2\x80\xa1", "&Dagger;",   "<Dagger/>",            "&#8225;",       "{$\\ddagger$}",          NULL,     "\\'87",       "&#8225;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(permil),   "\xe2\x80\xb0", "&permil;",   "<permil/>",            "&#8240;",       "{$\\permil{}$}",         NULL,     "\\'89",       "&#8240;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(lsaquo),   "\xe2\x80\xb9", "&lsaquo;",   "<lsaquo/>",            "&#8249;",       "\\guilsinglleft{}",      NULL,     "\\'8B",       "&#8249;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(rsaquo),   "\xe2\x80\xba", "&rsaquo;",   "<rsaquo/>",            "&#8250;",       "\\guilsinglright{}",     NULL,     "\\'9B",       "&#8250;",  { NULL,         DocSymbol::Perl_unknown }},
  { SYM(euro),     "\xe2\x82\xac", "&euro;",     "<euro/>",              "&#8364;",       "\\texteuro{}",           NULL,     "\\'80",       "&#8364;",  { NULL,         DocSymbol::Perl_unknown }},

  // doxygen extension to the HTML4 table of HTML entities
  { SYM(tm),       "\xe2\x84\xa2", "&trade;",    "<tm/>",                "&#8482;",       "\\texttrademark{}",      "(TM)",   "\\'99",       "(TM)",     { "trademark",  DocSymbol::Perl_symbol  }},
  { SYM(apos),     "'",            "'",          "'",                    "&apos;",        "\\textquotesingle{}",    "'",      "'",           "'",        { "\\\'",       DocSymbol::Perl_string  }},

  // doxygen commands represented as HTML entities
  { SYM(BSlash),   "\\",           "\\",         "\\",                   "\\",            "\\textbackslash{}",      "\\\\",   "\\\\",        "\\\\",     { "\\\\",       DocSymbol::Perl_string  }},
  { SYM(At),       "@",            "@",          "@",                    "@",             "@",                      "@",      "@",           "@",        { "@",          DocSymbol::Perl_char    }},
  { SYM(Less),     "<",            "&lt;",       "&lt;",                 "&lt;",          "<",                      "<",      "<",           "<",        { "<",          DocSymbol::Perl_char    }},
  { SYM(Greater),  ">",            "&gt;",       "&gt;",                 "&gt;",          ">",                      ">",      ">",           ">",        { ">",          DocSymbol::Perl_char    }},
  { SYM(Amp),      "&",            "&amp;",      "&amp;",                "&amp;",         "\\&",                    "&",      "&",           "&",        { "&",          DocSymbol::Perl_char    }},
  { SYM(Dollar),   "$",            "$",          "$",                    "$",             "\\$",                    "$",      "$",           "$",        { "$",          DocSymbol::Perl_char    }},
  { SYM(Hash),     "#;",           "#",          "#",                    "#",             "\\#",                    "#",      "#",           "\\#"       { "#",          DocSymbol::Perl_char    }},
  { SYM(DoubleColon), "::",        "::",         "::",                   "::",            "::",                     "::",     "::",          "{two-colons}", { "::",         DocSymbol::Perl_string  }},
  { SYM(Percent),  "%",            "%",          "%",                    "%",             "\\%",                    "%",      "%",           "%",        { "%",          DocSymbol::Perl_char    }},
  { SYM(Pipe),     "|",            "|",          "|",                    "|",             "$|$",                    "|",      "|",           "|",        { "|",          DocSymbol::Perl_char    }},
  { SYM(Quot),     "\"",           "\"",         "\"",                   "&quot;",        "\"",                     "\"",     "\"",          "\"",       { "\"",         DocSymbol::Perl_char    }},
  { SYM(Minus),    "-",            "-",          "-",                    "-",             "-\\/",                   "-",      "-",           "-",        { "-",          DocSymbol::Perl_char    }},
  { SYM(Plus),     "+",            "+",          "+",                    "+",             "+",                      "+",      "+",           "+",        { "+",          DocSymbol::Perl_char    }},
  { SYM(Dot),      ".",            ".",          ".",                    ".",             ".",                      ".",      ".",           "\\.",      { ".",          DocSymbol::Perl_char    }},
  { SYM(Colon),    ":",            ":",          ":",                    ":",             ":",                      ":",      ":",           ":",        { ":",          DocSymbol::Perl_char    }},
  { SYM(Equal),    "=",            "=",          "=",                    "=",             "=",                      "=",      "=",           "=",        { "=",          DocSymbol::Perl_char    }}
};

static const int g_numHtmlEntities = (int)(sizeof(g_htmlEntities)/ sizeof(*g_htmlEntities));

HtmlEntityMapper *HtmlEntityMapper::s_instance = 0;

HtmlEntityMapper::HtmlEntityMapper()
{
  m_name2sym = new QDict<int>(1009);
  m_name2sym->setAutoDelete(TRUE);
  for (int i = 0; i < g_numHtmlEntities; i++)
  {
    m_name2sym->insert(g_htmlEntities[i].item,new int(g_htmlEntities[i].symb));
  }
  validate();
}

HtmlEntityMapper::~HtmlEntityMapper()
{
  delete m_name2sym;
}

/** Returns the one and only instance of the HTML entity mapper */
HtmlEntityMapper *HtmlEntityMapper::instance()
{
  if (s_instance==0)
  {
    s_instance = new HtmlEntityMapper;
  }
  return s_instance;
}

/** Deletes the one and only instance of the HTML entity mapper */
void HtmlEntityMapper::deleteInstance()
{
  delete s_instance;
  s_instance=0;
}


/*! @brief Access routine to the UTF8 code of the HTML entity
 *
 * @param symb Code of the requested HTML entity
 * @param useInPrintf If TRUE the result will be escaped such that it can be
 *                    used in a printf string pattern
 * @return the UTF8 code of the HTML entity,
 *         in case the UTF code is unknown \c NULL is returned.
 */
const char *HtmlEntityMapper::utf8(DocSymbol::SymType symb,bool useInPrintf) const
{
  if (useInPrintf && symb==DocSymbol::Sym_Percent)
  {
    return "%%"; // escape for printf
  }
  else
  {
    return g_htmlEntities[symb].UTF8;
  }
}

/*! @brief Access routine to the html code of the HTML entity
 *
 * @param symb        Code of the requested HTML entity
 * @param useInPrintf If TRUE the result will be escaped such that it can be
 *                    used in a printf string pattern
 * @return the html representation of the HTML entity,
 *         in case the html code is unknown \c NULL is returned.
 */
const char *HtmlEntityMapper::html(DocSymbol::SymType symb,bool useInPrintf) const
{
  if (useInPrintf && symb==DocSymbol::Sym_Percent)
  {
    return "%%"; // escape for printf
  }
  else
  {
    return g_htmlEntities[symb].html;
  }
}

/*! @brief Access routine to the XML code of the HTML entity
 *
 * @param symb Code of the requested HTML entity
 * @return the XML code of the HTML entity,
 *         in case the XML code is unknown \c NULL is returned.
 */
const char *HtmlEntityMapper::xml(DocSymbol::SymType symb) const
{
  return g_htmlEntities[symb].xml;
}

/*! @brief Access routine to the docbook code of the HTML entity
 *
 * @param symb Code of the requested HTML entity
 * @return the docbook code of the HTML entity,
 *         in case the docbook code is unknown \c NULL is returned.
 */
const char *HtmlEntityMapper::docbook(DocSymbol::SymType symb) const
{
  return g_htmlEntities[symb].docbook;
}

/*! @brief Access routine to the asciidoctor code of the HTML entity
 *
 * @param symb Code of the requested HTML entity
 * @return the asciidoc code of the HTML entity,
 *         in case the asciidoc code is unknown \c NULL is returned.
 */
const char *HtmlEntityMapper::asciidoc(DocSymbol::SymType symb) const
{
  return g_htmlEntities[symb].asciidoc;
}

/*! @brief Access routine to the LaTeX code of the HTML entity
 *
 * @param symb Code of the requested HTML entity
 * @return the LaTeX code of the HTML entity,
 *         in case the LaTeX code is unknown \c NULL is returned.
 */
const char *HtmlEntityMapper::latex(DocSymbol::SymType symb) const
{
  return g_htmlEntities[symb].latex;
}

/*! @brief Access routine to the man code of the HTML entity
 *
 * @param symb Code of the requested HTML entity
 * @return the man of the HTML entity,
 *         in case the man code is unknown \c NULL is returned.
 */
const char *HtmlEntityMapper::man(DocSymbol::SymType symb) const
{
  return g_htmlEntities[symb].man;
}

/*! @brief Access routine to the RTF code of the HTML entity
 *
 * @param symb Code of the requested HTML entity
 * @return the RTF of the HTML entity,
 *         in case the RTF code is unknown \c NULL is returned.
 */
const char *HtmlEntityMapper::rtf(DocSymbol::SymType symb) const
{
  return g_htmlEntities[symb].rtf;
}

/*! @brief Access routine to the perl struct with the perl code of the HTML entity
 *
 * @param symb Code of the requested HTML entity
 * @return the pointer to perl struct with the perl code of the HTML entity,
 *         in case the perl code does not exists the NULL pointer is entered in the
 *         \c symb field and in the `DocSymbol::Perl_unknown` in the \c type field.
 */
const DocSymbol::PerlSymb *HtmlEntityMapper::perl(DocSymbol::SymType symb) const
{
  return &g_htmlEntities[symb].perl;
}

/*!
 * @brief Give code of the requested HTML entity name
 * @param symName HTML entity name without \c & and \c ;
 * @return the code for the requested HTML entity name,
 *         in case the requested HTML item does not exist `DocSymbol::Sym_unknown` is returned.
 */
DocSymbol::SymType HtmlEntityMapper::name2sym(const QCString &symName) const
{
  int *pSymb = m_name2sym->find(symName);
  return pSymb ? ((DocSymbol::SymType)*pSymb) : DocSymbol::Sym_Unknown;
}

void HtmlEntityMapper::writeXMLSchema(FTextStream &t)
{
  for (int i=0;i<g_numHtmlEntities - g_numberHtmlMappedCmds;i++)
  {
    QCString bareName = g_htmlEntities[i].xml;
    if (!bareName.isEmpty() && bareName.at(0)=='<' && bareName.right(2)=="/>")
    {
      bareName = bareName.mid(1,bareName.length()-3); // strip < and />
      t << "      <xsd:element name=\"" << bareName << "\" type=\"docEmptyType\" />\n";
    }
  }
}

/*! @brief Routine to check if the entries of the html_entities are numbered correctly
 *  @details in case of a mismatch a warning message is given.
 */
void HtmlEntityMapper::validate()
{
  for (int i = 0; i < g_numHtmlEntities; i++)
  {
    if (i != g_htmlEntities[i].symb)
    {
      warn_uncond("Internal inconsistency, htmlentries code %d (item=%s)\n",i,g_htmlEntities[i].item);
    }
  }
}
