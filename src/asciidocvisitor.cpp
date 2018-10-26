/******************************************************************************
 *
 * 
 *
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

#include <qfileinfo.h>

#include "asciidocvisitor.h"
#include "docparser.h"
#include "language.h"
#include "doxygen.h"
#include "outputgen.h"
#include "asciidocgen.h"
#include "dot.h"
#include "message.h"
#include "util.h"
#include "parserintf.h"
#include "filename.h"
#include "config.h"
#include "filedef.h"
#include "msc.h"
#include "dia.h"
#include "htmlentity.h"
#include "plantuml.h"

#if 0
#define DB_VIS_C DB_VIS_C1(m_t)
#define DB_VIS_C1(x) x << "<!-- DB_VIS_C " << __LINE__ << " -->\n";
#define DB_VIS_C2(y) DB_VIS_C2a(m_t,y)
#define DB_VIS_C2a(x,y) x << "<!-- DB_VIS_C " << __LINE__ << " " << y << " -->\n";
#else
#define DB_VIS_C
#define DB_VIS_C1(x)
#define DB_VIS_C2(y)
#define DB_VIS_C2a(x,y)
#endif
void visitADPreStart(FTextStream &t, const bool hasCaption, QCString name,  QCString width,  QCString height)
{
  QCString tmpStr;
  if (hasCaption)
  {
    t << "<figure>" << endl;
  }
  else
  {
    t << "<informalfigure>" << endl;
  }
  t << "<mediaobject>" << endl;
  t << "<imageobject>" << endl;
  t << "<imagedata";
  if (!width.isEmpty())
  {
    t << " width=\"" << convertToAsciidoc(width) << "\"";
  }
  else
  {
    t << " width=\"50%\"";
  }
  if (!height.isEmpty())
  {
    t << " depth=\"" << convertToAsciidoc(tmpStr) << "\"";
  }
  t << " align=\"center\" valign=\"middle\" scalefit=\"0\" fileref=\"" << name << "\">";
  t << "</imagedata>" << endl;
  t << "</imageobject>" << endl;
  if (hasCaption)
  {
    t << "        TITTEL1" << endl;
  }
}

void visitADPostEnd(FTextStream &t, const bool hasCaption)
{
  t << endl;
  if (hasCaption)
  {
    t << "        TITTEL2" << endl;
  }
  t << "</mediaobject>" << endl;
  if (hasCaption)
  {
    t << "</figure>" << endl;
  }
  else
  {
    t << "</informalfigure>" << endl;
  }
}

static void visitCaption(AsciidocDocVisitor *parent, QList<DocNode> children)
{
  QListIterator<DocNode> cli(children);
  DocNode *n;
  for (cli.toFirst();(n=cli.current());++cli) n->accept(parent);
}

AsciidocDocVisitor::AsciidocDocVisitor(FTextStream &t,CodeOutputInterface &ci)
  : DocVisitor(DocVisitor_Asciidoc), m_t(t), m_ci(ci), m_insidePre(FALSE), m_hide(FALSE)
{
DB_VIS_C
  // m_t << "<section>" << endl;
}
AsciidocDocVisitor::~AsciidocDocVisitor()
{
DB_VIS_C
  // m_t << "</section>" << endl;
}

//--------------------------------------
// visitor functions for leaf nodes
//--------------------------------------

void AsciidocDocVisitor::visit(DocWord *w)
{
DB_VIS_C
  if (m_hide) return;
  filter(w->word());
}

void AsciidocDocVisitor::visit(DocLinkedWord *w)
{
DB_VIS_C
  if (m_hide) return;
  startLink(w->file(),w->anchor());
  filter(w->word());
  endLink();
}

void AsciidocDocVisitor::visit(DocWhiteSpace *w)
{
DB_VIS_C
  if (m_hide) return;
  if (m_insidePre)
  {
    m_t << w->chars();
  }
  else
  {
    m_t << " ";
  }
}

void AsciidocDocVisitor::visit(DocSymbol *s)
{
DB_VIS_C
  if (m_hide) return;
  const char *res = HtmlEntityMapper::instance()->asciidoc(s->symbol());
  if (res)
  {
    m_t << res;
  }
  else
  {
    err("Asciidoc: non supported HTML-entity found: %s\n",HtmlEntityMapper::instance()->html(s->symbol(),TRUE));
  }
}

void AsciidocDocVisitor::visit(DocURL *u)
{
DB_VIS_C
  if (m_hide) return;
  if (u->isEmail()) m_t << "mailto:";
  filter(u->url());
}

void AsciidocDocVisitor::visit(DocLineBreak *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << endl << "&#160;&#xa;" << endl;
  // gives nicer results but gives problems as it is not allowed in <pare> and also problems with dblatex
  // m_t << endl << "<sbr/>" << endl;
}

void AsciidocDocVisitor::visit(DocHorRuler *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "'''" << endl;
}

void AsciidocDocVisitor::visit(DocStyleChange *s)
{
DB_VIS_C
  if (m_hide) return;
  switch (s->style())
  {
    case DocStyleChange::Bold:
      if (s->enable()) m_t << "**";      else m_t << "**";
      break;
    case DocStyleChange::Italic:
      if (s->enable()) m_t << "__";     else m_t << "__";
      break;
    case DocStyleChange::Code:
      if (s->enable()) m_t << "[source6]" << endl; else m_t << endl;
      break;
    case DocStyleChange::Subscript:
      if (s->enable()) m_t << "~";    else m_t << "~";
      break;
    case DocStyleChange::Superscript:
      if (s->enable()) m_t << "^";    else m_t << "^";
      break;
    case DocStyleChange::Center:
      if (s->enable()) m_t << "|===1" << endl;
      else m_t << "|===2" << endl;
      break;
    case DocStyleChange::Preformatted:
      if (s->enable())
      {
	  m_t << "[literal]" << endl;
        m_insidePre=TRUE;
      }
      else
      {
        m_t << endl;
        m_insidePre=FALSE;
      }
      break;
    case DocStyleChange::Small:
	if (s->enable()) m_t << "[.small]#";
	else m_t << '#';
	break;
    case DocStyleChange::Strike:
	if (s->enable()) m_t << "[.line-through]#";
	else m_t << "#";
	break;
    case DocStyleChange::Underline:
	if (s->enable()) m_t << "[.underline]#";
	else m_t << "#";
	break;
    case DocStyleChange::Div:  /* HTML only */ break;
    case DocStyleChange::Span: /* HTML only */ break;
  }
}

void AsciidocDocVisitor::visit(DocVerbatim *s)
{
DB_VIS_C
  if (m_hide) return;
  SrcLangExt langExt = getLanguageFromFileName(m_langExt);
  switch(s->type())
  {
    case DocVerbatim::Code: // fall though
	m_t << "[source7]" << endl;
      Doxygen::parserManager->getParser(m_langExt)
        ->parseCode(m_ci,s->context(),s->text(),langExt,
            s->isExample(),s->exampleFile());
      m_t << endl;
      break;
    case DocVerbatim::Verbatim:
      m_t << "[source8]" << endl;
      filter(s->text());
      m_t << endl;
      break;
    case DocVerbatim::HtmlOnly:    
      break;
    case DocVerbatim::RtfOnly:     
      break;
    case DocVerbatim::ManOnly:     
      break;
    case DocVerbatim::LatexOnly:   
      break;
    case DocVerbatim::XmlOnly:     
      break;
    case DocVerbatim::AsciidocOnly: 
      m_t << s->text();
      break;
    case DocVerbatim::Dot:
      {
        static int dotindex = 1;
        QCString baseName(4096);
        QCString name;
        QCString stext = s->text();
        name.sprintf("%s%d", "dot_inline_dotgraph_", dotindex);
        baseName.sprintf("%s%d",
            (Config_getString(ASCIIDOC_OUTPUT)+"/inline_dotgraph_").data(),
            dotindex++
            );
        QFile file(baseName+".dot");
        if (!file.open(IO_WriteOnly))
        {
          err("Could not open file %s.msc for writing\n",baseName.data());
        }
        file.writeBlock( stext, stext.length() );
        file.close();
        writeDotFile(baseName, s);
        m_t << endl;
      }
      break;
    case DocVerbatim::Msc:
      {
        static int mscindex = 1;
        QCString baseName(4096);
        QCString name;
        QCString stext = s->text();
        name.sprintf("%s%d", "msc_inline_mscgraph_", mscindex);
        baseName.sprintf("%s%d",
            (Config_getString(ASCIIDOC_OUTPUT)+"/inline_mscgraph_").data(),
            mscindex++
            );
        QFile file(baseName+".msc");
        if (!file.open(IO_WriteOnly))
        {
          err("Could not open file %s.msc for writing\n",baseName.data());
        }
        QCString text = "msc {";
        text+=stext;
        text+="}";
        file.writeBlock( text, text.length() );
        file.close();
        writeMscFile(baseName,s);
        m_t << endl;
      }
      break;
    case DocVerbatim::PlantUML:
      {
        static QCString asciidocOutput = Config_getString(ASCIIDOC_OUTPUT);
        QCString baseName = writePlantUMLSource(asciidocOutput,s->exampleFile(),s->text());
        QCString shortName = baseName;
        int i;
        if ((i=shortName.findRev('/'))!=-1)
        {
          shortName=shortName.right(shortName.length()-i-1);
        }
        writePlantUMLFile(baseName,s);
        m_t << endl;
      }
      break;
  }
}

void AsciidocDocVisitor::visit(DocAnchor *anc)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "<anchor xml:id=\"_" <<  stripPath(anc->file()) << "_1" << anc->anchor() << "\"/>";
}

void AsciidocDocVisitor::visit(DocInclude *inc)
{
DB_VIS_C
  if (m_hide) return;
  SrcLangExt langExt = getLanguageFromFileName(inc->extension());
  switch(inc->type())
  {
    case DocInclude::IncWithLines:
      {
	m_t << "[literal]" << endl;
        QFileInfo cfi( inc->file() );
        FileDef fd( cfi.dirPath().utf8(), cfi.fileName().utf8() );
        Doxygen::parserManager->getParser(inc->extension())
          ->parseCode(m_ci,inc->context(),
              inc->text(),
              langExt,
              inc->isExample(),
              inc->exampleFile(), &fd);
        m_t << endl;
      }
      break;
    case DocInclude::Include:
      m_t << "[literal]" << endl;
      Doxygen::parserManager->getParser(inc->extension())
        ->parseCode(m_ci,inc->context(),
            inc->text(),
            langExt,
            inc->isExample(),
            inc->exampleFile());
      m_t << endl;
      break;
    case DocInclude::DontInclude:
      break;
    case DocInclude::HtmlInclude:
      break;
    case DocInclude::LatexInclude:
      break;
    case DocInclude::VerbInclude:
      m_t << "[literal]";
      filter(inc->text());
      m_t << endl;
      break;
    case DocInclude::Snippet:
      m_t << "[literal]";
      Doxygen::parserManager->getParser(inc->extension())
        ->parseCode(m_ci,
            inc->context(),
            extractBlock(inc->text(),inc->blockId()),
            langExt,
            inc->isExample(),
            inc->exampleFile()
            );
      m_t << endl;
      break;
    case DocInclude::SnipWithLines:
      {
         QFileInfo cfi( inc->file() );
         FileDef fd( cfi.dirPath().utf8(), cfi.fileName().utf8() );
         m_t << "[literal]";
         Doxygen::parserManager->getParser(inc->extension())
                               ->parseCode(m_ci,
                                           inc->context(),
                                           extractBlock(inc->text(),inc->blockId()),
                                           langExt,
                                           inc->isExample(),
                                           inc->exampleFile(), 
                                           &fd,
                                           lineBlock(inc->text(),inc->blockId()),
                                           -1,    // endLine
                                           FALSE, // inlineFragment
                                           0,     // memberDef
                                           TRUE   // show line number
                                          );
         m_t << endl;
      }
      break;
    case DocInclude::SnippetDoc: 
    case DocInclude::IncludeDoc: 
      err("Internal inconsistency: found switch SnippetDoc / IncludeDoc in file: %s"
          "Please create a bug report\n",__FILE__);
      break;
  }
}

void AsciidocDocVisitor::visit(DocIncOperator *op)
{
DB_VIS_C
  if (op->isFirst())
  {
    if (!m_hide)
    {
      m_t << "<programlisting>";
    }
    pushEnabled();
    m_hide = TRUE;
  }
  SrcLangExt langExt = getLanguageFromFileName(m_langExt);
  if (op->type()!=DocIncOperator::Skip)
  {
    popEnabled();
    if (!m_hide)
    {
      Doxygen::parserManager->getParser(m_langExt)
        ->parseCode(m_ci,op->context(),
            op->text(),langExt,op->isExample(),
            op->exampleFile());
    }
    pushEnabled();
    m_hide=TRUE;
  }
  if (op->isLast())
  {
    popEnabled();
    if (!m_hide) m_t << "</programlisting>";
  }
  else
  {
    if (!m_hide) m_t << endl;
  }
}

void AsciidocDocVisitor::visit(DocFormula *f)
{
DB_VIS_C
  if (m_hide) return;

  if (f->isInline()) m_t  << "<inlinemediaobject>" << endl;
  else m_t << "<mediaobject>" << endl;
  m_t << "<imageobject>" << endl;
  m_t << "<imagedata ";
  m_t << "align=\"center\" valign=\"middle\" scalefit=\"0\" fileref=\"" << f->relPath() << f->name() << ".png\"/>" << endl;
  m_t << "</imageobject>" << endl;
  if (f->isInline()) m_t  << "</inlinemediaobject>" << endl;
  else m_t << "</mediaobject>" << endl;
}

void AsciidocDocVisitor::visit(DocIndexEntry *ie)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "<indexterm><primary>";
  filter(ie->entry());
  m_t << "</primary></indexterm>" << endl;
}

void AsciidocDocVisitor::visit(DocSimpleSectSep *)
{
DB_VIS_C
  // m_t << "<simplesect/>";
}

void AsciidocDocVisitor::visit(DocCite *cite)
{
DB_VIS_C
  if (m_hide) return;
  if (!cite->file().isEmpty()) startLink(cite->file(),cite->anchor());
  filter(cite->text());
  if (!cite->file().isEmpty()) endLink();
}

//--------------------------------------
// visitor functions for compound nodes
//--------------------------------------

void AsciidocDocVisitor::visitPre(DocAutoList *l)
{
DB_VIS_C
  if (m_hide) return;
  if (l->isEnumList())
  {
    m_t << "<orderedlist>\n";
  }
  else
  {
    m_t << "<itemizedlist>\n";
  }
}

void AsciidocDocVisitor::visitPost(DocAutoList *l)
{
DB_VIS_C
  if (m_hide) return;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocAutoListItem *i)
{
DB_VIS_C
  if (m_hide) return;
  DocAutoList* l = dynamic_cast<DocAutoList*>(l->parent());
  if (l && l->isEnumList())
  {
    m_t << ". ";
  }
  else
  {
    m_t << "*10 ";
  }
}

void AsciidocDocVisitor::visitPost(DocAutoListItem *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocPara *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << endl;
}

void AsciidocDocVisitor::visitPost(DocPara *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocRoot *)
{
DB_VIS_C
  //m_t << "<hr><h4><font color=\"red\">New parser:</font></h4>\n";
}

void AsciidocDocVisitor::visitPost(DocRoot *)
{
DB_VIS_C
  //m_t << "<hr><h4><font color=\"red\">Old parser:</font></h4>\n";
}

void AsciidocDocVisitor::visitPre(DocSimpleSect *s)
{
DB_VIS_C
  if (m_hide) return;
  switch(s->type())
  {
    case DocSimpleSect::See:
      if (m_insidePre) 
      {
        m_t << "==== 56" << theTranslator->trSeeAlso() << ":" << endl;
      } 
      else 
      {
        m_t << "==== 57" << convertToAsciidoc(theTranslator->trSeeAlso()) << ":" << endl;
      }
      break;
    case DocSimpleSect::Return:
      if (m_insidePre) 
      {
        m_t << "==== 58" << theTranslator->trReturns()<< ":" << endl;
      } 
      else 
      {
        m_t << "==== 59" << convertToAsciidoc(theTranslator->trReturns()) << ":" << endl;
      }
      break;
    case DocSimpleSect::Author:
      if (m_insidePre) 
      {
        m_t << "==== 60" << theTranslator->trAuthor(TRUE, TRUE) << ":" << endl;
      } 
      else 
      {
        m_t << "==== 61" << convertToAsciidoc(theTranslator->trAuthor(TRUE, TRUE)) << ":" << endl;
      }
      break;
    case DocSimpleSect::Authors:
      if (m_insidePre) 
      {
        m_t << "==== 62" << theTranslator->trAuthor(TRUE, FALSE) << ":" << endl;
      } 
      else 
      {
        m_t << "==== 63" << convertToAsciidoc(theTranslator->trAuthor(TRUE, FALSE)) << ":" << endl;
      }
      break;
    case DocSimpleSect::Version:
      if (m_insidePre) 
      {
        m_t << "==== 64" << theTranslator->trVersion() << ":" << endl;
      } 
      else 
      {
        m_t << "==== 65" << convertToAsciidoc(theTranslator->trVersion()) << ":" << endl;
      }
      break;
    case DocSimpleSect::Since:
      if (m_insidePre) 
      {
        m_t << "==== 66" << theTranslator->trSince() << ":" << endl;
      } 
      else 
      {
        m_t << "==== 67" << convertToAsciidoc(theTranslator->trSince()) << ":" << endl;
      }
      break;
    case DocSimpleSect::Date:
      if (m_insidePre) 
      {
        m_t << "==== 68" << theTranslator->trDate() << ":" << endl;
      } 
      else 
      {
        m_t << "==== 69" << convertToAsciidoc(theTranslator->trDate()) << ":" << endl;
      }
      break;
    case DocSimpleSect::Note:
      // if (m_insidePre) 
      m_t << "[NOTE]" << endl;
      m_t << "====" << endl;
      break;
    case DocSimpleSect::Warning:
      m_t << "[WARNING]" << endl;
      m_t << "====" << endl;
      break;
    case DocSimpleSect::Pre:
      if (m_insidePre) 
      {
        m_t << "==== 70" << theTranslator->trPrecondition() << ":" << endl;
      } 
      else 
      {
        m_t << "==== 71" << convertToAsciidoc(theTranslator->trPrecondition()) << ":" << endl;
      }
      break;
    case DocSimpleSect::Post:
      if (m_insidePre) 
      {
        m_t << "==== 72" << theTranslator->trPostcondition() << ":" << endl;
      } 
      else 
      {
        m_t << "==== 73" << convertToAsciidoc(theTranslator->trPostcondition()) << ":" << endl;
      }
      break;
    case DocSimpleSect::Copyright:
      if (m_insidePre) 
      {
        m_t << "==== 74" << theTranslator->trCopyright() << ":" << endl;
      } 
      else 
      {
        m_t << "==== 75" << convertToAsciidoc(theTranslator->trCopyright()) << ":" << endl;
      }
      break;
    case DocSimpleSect::Invar:
      if (m_insidePre) 
      {
        m_t << "==== 76" << theTranslator->trInvariant() << ":" << endl;
      } 
      else 
      {
        m_t << "==== 77" << convertToAsciidoc(theTranslator->trInvariant()) << ":" << endl;
      }
      break;
    case DocSimpleSect::Remark:
      if (m_insidePre) 
      {
        m_t << "==== 78" << theTranslator->trRemarks() << ":" << endl;
      } 
      else 
      {
        m_t << "==== 79" << convertToAsciidoc(theTranslator->trRemarks()) << ":" << endl;
      }
      break;
    case DocSimpleSect::Attention:
      m_t << "[CAUTION]" << endl;
      m_t << "===" << endl;
      break;
    case DocSimpleSect::User:
    case DocSimpleSect::Rcs:
    case DocSimpleSect::Unknown:
      break;
  }
}

void AsciidocDocVisitor::visitPost(DocSimpleSect *s)
{
DB_VIS_C
  if (m_hide) return;
  switch(s->type())
  {
    case DocSimpleSect::Rcs:
    case DocSimpleSect::Unknown:
      m_t << endl;
      break;
    case DocSimpleSect::User:
      m_t << endl;
      break;
    case DocSimpleSect::Note:
    case DocSimpleSect::Attention:
    case DocSimpleSect::Warning:
      m_t << "====" << endl;
      m_t << endl;
    default:
      m_t << endl;
      break;
  }
}

void AsciidocDocVisitor::visitPre(DocTitle *t)
{
DB_VIS_C
  if (m_hide) return;
  if (t->hasTitle()) m_t << "==== 82";
}

void AsciidocDocVisitor::visitPost(DocTitle *t)
{
DB_VIS_C
  if (m_hide) return;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocSimpleList *)
{
DB_VIS_C
  if (m_hide) return;
}

void AsciidocDocVisitor::visitPost(DocSimpleList *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocSimpleListItem *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "*11 ";
}

void AsciidocDocVisitor::visitPost(DocSimpleListItem *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "\n";
}

void AsciidocDocVisitor::visitPre(DocSection *s)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "<section xml:id=\"_" <<  stripPath(s->file());
  if (!s->anchor().isEmpty()) m_t << "_1" << s->anchor();
  m_t << "\">" << endl;
  m_t << "=== 80";
  filter(s->title());
  m_t << endl;
}

void AsciidocDocVisitor::visitPost(DocSection *)
{
DB_VIS_C
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocHtmlList *s)
{
DB_VIS_C
  if (m_hide) return;
  if (s->type()==DocHtmlList::Ordered)
    m_t << "<orderedlist>\n";
  else
    m_t << "<itemizedlist>\n";
}

void AsciidocDocVisitor::visitPost(DocHtmlList *s)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "\n";
}

void AsciidocDocVisitor::visitPre(DocHtmlListItem *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "*12 ";
}

void AsciidocDocVisitor::visitPost(DocHtmlListItem *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "\n";
}

void AsciidocDocVisitor::visitPre(DocHtmlDescList *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "<variablelist>\n";
}

void AsciidocDocVisitor::visitPost(DocHtmlDescList *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "\n";
}

void AsciidocDocVisitor::visitPre(DocHtmlDescTitle *)
{
DB_VIS_C
  if (m_hide) return;
}

void AsciidocDocVisitor::visitPost(DocHtmlDescTitle *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "::\n";
}

void AsciidocDocVisitor::visitPre(DocHtmlDescData *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "*13 ";
}

void AsciidocDocVisitor::visitPost(DocHtmlDescData *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "\n";
}

static int colCnt = 0;
static bool bodySet = FALSE; // it is possible to have tables without a header
void AsciidocDocVisitor::visitPre(DocHtmlTable *t)
{
DB_VIS_C
  bodySet = FALSE;
  if (m_hide) return;
  m_t << "<informaltable frame=\"all\">" << endl;
  m_t << "<tgroup cols=\"" << t->numColumns() << "\" align=\"left\" colsep=\"1\" rowsep=\"1\">" << endl;
  for (int i = 0; i <t->numColumns(); i++)
  {
    // do something with colwidth based of cell width specification (be aware of possible colspan in the header)?
    m_t << "<colspec colname='c" << i+1 << "'/>\n";
  }
}

void AsciidocDocVisitor::visitPost(DocHtmlTable *)
{
DB_VIS_C
  if (m_hide) return;
m_t << "|===3" << endl;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocHtmlRow *tr)
{
DB_VIS_C
  colCnt = 0;
  if (m_hide) return;

  if (tr->isHeading()) m_t << "<thead>\n";
  else if (!bodySet)
  {
    bodySet = TRUE;
    m_t << "<tbody>\n";
  }

  m_t << "<row ";

  HtmlAttribListIterator li(tr->attribs());
  HtmlAttrib *opt;
  for (li.toFirst();(opt=li.current());++li)
  {
    if (opt->name=="class")
    {
      // just skip it
    }
    else if (opt->name=="style")
    {
      // just skip it
    }
    else if (opt->name=="height")
    {
      // just skip it
    }
    else if (opt->name=="filter")
    {
      // just skip it
    }
    else
    {
      m_t << " " << opt->name << "='" << opt->value << "'";
    }
  }
  m_t << ">\n";
}

void AsciidocDocVisitor::visitPost(DocHtmlRow *tr)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "</row>\n";
  if (tr->isHeading())
  {
    bodySet = TRUE;
    m_t << "\n";
  }
}

void AsciidocDocVisitor::visitPre(DocHtmlCell *c)
{
DB_VIS_C
  colCnt++;
  if (m_hide) return;
  m_t << "<entry";

  HtmlAttribListIterator li(c->attribs());
  HtmlAttrib *opt;
  for (li.toFirst();(opt=li.current());++li)
  {
    if (opt->name=="colspan")
    {
      m_t << " namest='c" << colCnt << "'";
      int cols = opt->value.toInt();
      colCnt += (cols - 1);
      m_t << " nameend='c" << colCnt << "'";
    }
    else if (opt->name=="rowspan")
    {
      int extraRows = opt->value.toInt() - 1;
      m_t << " morerows='" << extraRows << "'";
    }
    else if (opt->name=="class")
    {
      if (opt->value == "markdownTableBodyRight")
      {
        m_t << " align='right'";
      }
      else if (opt->value == "markdownTableBodyLeftt")
      {
        m_t << " align='left'";
      }
      else if (opt->value == "markdownTableBodyCenter")
      {
        m_t << " align='center'";
      }
      else if (opt->value == "markdownTableHeadRight")
      {
        m_t << " align='right'";
      }
      else if (opt->value == "markdownTableHeadLeftt")
      {
        m_t << " align='left'";
      }
      else if (opt->value == "markdownTableHeadCenter")
      {
        m_t << " align='center'";
      }
    }
    else if (opt->name=="style")
    {
      // just skip it
    }
    else if (opt->name=="width")
    {
      // just skip it
    }
    else if (opt->name=="height")
    {
      // just skip it
    }
    else
    {
      m_t << " " << opt->name << "='" << opt->value << "'";
    }
  }
  m_t << ">";
}

void AsciidocDocVisitor::visitPost(DocHtmlCell *c)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "</entry>";
}

void AsciidocDocVisitor::visitPre(DocHtmlCaption *c)
{
DB_VIS_C
  if (m_hide) return;
   m_t << "<caption>";
}

void AsciidocDocVisitor::visitPost(DocHtmlCaption *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "</caption>\n";
}

void AsciidocDocVisitor::visitPre(DocInternal *)
{
DB_VIS_C
  if (m_hide) return;
  // TODO: to be implemented
}

void AsciidocDocVisitor::visitPost(DocInternal *)
{
DB_VIS_C
  if (m_hide) return;
  // TODO: to be implemented
}

void AsciidocDocVisitor::visitPre(DocHRef *href)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "link:++" << convertToAsciidoc(href->url());
}

void AsciidocDocVisitor::visitPost(DocHRef *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "++";
}

void AsciidocDocVisitor::visitPre(DocHtmlHeader *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "==== 81";
}

void AsciidocDocVisitor::visitPost(DocHtmlHeader *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocImage *img)
{
DB_VIS_C
  if (img->type()==DocImage::Asciidoc)
  {
    if (m_hide) return;
    m_t << endl;
    QCString baseName=img->name();
    int i;
    if ((i=baseName.findRev('/'))!=-1 || (i=baseName.findRev('\\'))!=-1)
    {
      baseName=baseName.right(baseName.length()-i-1);
    }
    visitADPreStart(m_t, img -> hasCaption(), img->relPath() + baseName, img -> width(), img -> height());
  }
  else
  {
    pushEnabled();
    m_hide=TRUE;
  }
}

void AsciidocDocVisitor::visitPost(DocImage *img)
{
DB_VIS_C
  if (img->type()==DocImage::Asciidoc)
  {
    if (m_hide) return;
    visitADPostEnd(m_t, img -> hasCaption());
    // copy the image to the output dir
    QCString baseName=img->name();
    int i;
    if ((i=baseName.findRev('/'))!=-1 || (i=baseName.findRev('\\'))!=-1)
    {
      baseName=baseName.right(baseName.length()-i-1);
    }
    QCString m_file;
    bool ambig;
    FileDef *fd=findFileDef(Doxygen::imageNameDict, baseName, ambig);
    if (fd) 
    {
      m_file=fd->absFilePath();
    }
    QFile inImage(m_file);
    QFile outImage(Config_getString(ASCIIDOC_OUTPUT)+"/"+baseName.data());
    if (inImage.open(IO_ReadOnly))
    {
      if (outImage.open(IO_WriteOnly))
      {
        char *buffer = new char[inImage.size()];
        inImage.readBlock(buffer,inImage.size());
        outImage.writeBlock(buffer,inImage.size());
        outImage.flush();
        delete[] buffer;
      }
    }
  } 
  else 
  {
    popEnabled();
  }
}

void AsciidocDocVisitor::visitPre(DocDotFile *df)
{
DB_VIS_C
  if (m_hide) return;
  startDotFile(df->file(),df->width(),df->height(),df->hasCaption());
}

void AsciidocDocVisitor::visitPost(DocDotFile *df)
{
DB_VIS_C
  if (m_hide) return;
  endDotFile(df->hasCaption());
}

void AsciidocDocVisitor::visitPre(DocMscFile *df)
{
DB_VIS_C
  if (m_hide) return;
  startMscFile(df->file(),df->width(),df->height(),df->hasCaption());
}

void AsciidocDocVisitor::visitPost(DocMscFile *df)
{
DB_VIS_C
  if (m_hide) return;
  endMscFile(df->hasCaption());
}
void AsciidocDocVisitor::visitPre(DocDiaFile *df)
{
DB_VIS_C
  if (m_hide) return;
  startDiaFile(df->file(),df->width(),df->height(),df->hasCaption());
}

void AsciidocDocVisitor::visitPost(DocDiaFile *df)
{
DB_VIS_C
  if (m_hide) return;
  endDiaFile(df->hasCaption());
}

void AsciidocDocVisitor::visitPre(DocLink *lnk)
{
DB_VIS_C
  if (m_hide) return;
  startLink(lnk->file(),lnk->anchor());
}

void AsciidocDocVisitor::visitPost(DocLink *)
{
DB_VIS_C
  if (m_hide) return;
  endLink();
}

void AsciidocDocVisitor::visitPre(DocRef *ref)
{
DB_VIS_C
  if (m_hide) return;
  if (ref->isSubPage())
  {
    startLink(0,ref->anchor());
  }
  else
  {
    if (!ref->file().isEmpty()) startLink(ref->file(),ref->anchor());
  }

  if (!ref->hasLinkText()) filter(ref->targetTitle());
}

void AsciidocDocVisitor::visitPost(DocRef *ref)
{
DB_VIS_C
  if (m_hide) return;
  if (!ref->file().isEmpty()) endLink();
}

void AsciidocDocVisitor::visitPre(DocSecRefItem *ref)
{
DB_VIS_C
  if (m_hide) return;
  //m_t << "<tocentry xml:idref=\"_" <<  stripPath(ref->file()) << "_1" << ref->anchor() << "\">";
  m_t << "<tocentry>";
}

void AsciidocDocVisitor::visitPost(DocSecRefItem *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "</tocentry>" << endl;
}

void AsciidocDocVisitor::visitPre(DocSecRefList *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "<toc>" << endl;
}

void AsciidocDocVisitor::visitPost(DocSecRefList *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "</toc>" << endl;
}

void AsciidocDocVisitor::visitPre(DocParamSect *s)
{
DB_VIS_C
  if (m_hide) return;
  m_t <<  endl;
  m_t << "==== 55";
  switch(s->type())
  {
    case DocParamSect::Param:         m_t << theTranslator->trParameters();         break;
    case DocParamSect::RetVal:        m_t << theTranslator->trReturnValues();       break;
    case DocParamSect::Exception:     m_t << theTranslator->trExceptions();         break;
    case DocParamSect::TemplateParam: m_t << theTranslator->trTemplateParameters(); break;
    default:
      ASSERT(0);
  }
  m_t << endl;
  m_t << endl;
  int ncols = 2;
  if (s->type() == DocParamSect::Param)
  {
    bool hasInOutSpecs = s->hasInOutSpecifier();
    bool hasTypeSpecs  = s->hasTypeSpecifier();
    if      (hasInOutSpecs && hasTypeSpecs) ncols += 2;
    else if (hasInOutSpecs || hasTypeSpecs) ncols += 1;
  }
  m_t << "[cols=\"";
  for (int i = 1; i <= ncols; i++)
  {
    if (i == ncols) m_t << 4;
    else m_t << 1;
    if (i < ncols) m_t << ',';
  }
  m_t << "\"]" << endl;
  m_t << "|===4" << endl;
}

void AsciidocDocVisitor::visitPost(DocParamSect *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "|===5" << endl;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocParamList *pl)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "|1 " << endl;

  DocParamSect::Type parentType = DocParamSect::Unknown;
  DocParamSect *sect = 0;
  if (pl->parent() && pl->parent()->kind()==DocNode::Kind_ParamSect)
  {
    parentType = ((DocParamSect*)pl->parent())->type();
    sect=(DocParamSect*)pl->parent();
  }

  if (sect && sect->hasInOutSpecifier())
  {
    m_t << "|2 ";
    if (pl->direction()!=DocParamSect::Unspecified)
    {
      if (pl->direction()==DocParamSect::In) m_t << "in";
      else if (pl->direction()==DocParamSect::Out) m_t << "out";
      else if (pl->direction()==DocParamSect::InOut) m_t << "in,out";
    }
    m_t << " ";
  }

  if (sect && sect->hasTypeSpecifier())
  {
    QListIterator<DocNode> li(pl->paramTypes());
    DocNode *type;
    bool first=TRUE;
    m_t << "|3 ";
    for (li.toFirst();(type=li.current());++li)
    {
      if (!first) m_t << " X|X "; else first=FALSE;
      if (type->kind()==DocNode::Kind_Word)
      {
        visit((DocWord*)type);
      }
      else if (type->kind()==DocNode::Kind_LinkedWord)
      {
        visit((DocLinkedWord*)type);
      }
    }
    m_t << endl;
  }

  QListIterator<DocNode> li(pl->parameters());
  DocNode *param;
  if (!li.toFirst())
  {
    m_t << "|4" << endl;
  }
  else
  {
    m_t << "|5 ";
    int cnt = 0;
    for (li.toFirst();(param=li.current());++li)
    {
      if (cnt)
      {
        m_t << ", ";
      }
      if (param->kind()==DocNode::Kind_Word)
      {
        visit((DocWord*)param);
      }
      else if (param->kind()==DocNode::Kind_LinkedWord)
      {
        visit((DocLinkedWord*)param);
      }
      cnt++;
    }
  }
  m_t << "XXXX" << endl;
}

void AsciidocDocVisitor::visitPost(DocParamList *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocXRefItem *x)
{
DB_VIS_C
  if (m_hide) return;
  if (x->title().isEmpty()) return;
  m_t << "15[[_";
  m_t << stripPath(x->file()) << "_1" << x->anchor();
  m_t << ",";
  filter(x->title());
  m_t << "]]";
  m_t << " ";
}

void AsciidocDocVisitor::visitPost(DocXRefItem *x)
{
DB_VIS_C
  if (m_hide) return;
  if (x->title().isEmpty()) return;
  m_t << endl;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocInternalRef *ref)
{
DB_VIS_C
  if (m_hide) return;
  startLink(ref->file(),ref->anchor());
}

void AsciidocDocVisitor::visitPost(DocInternalRef *)
{
DB_VIS_C
  if (m_hide) return;
  endLink();
  m_t << " ";
}

void AsciidocDocVisitor::visitPre(DocCopy *)
{
DB_VIS_C
  if (m_hide) return;
  // TODO: to be implemented
}


void AsciidocDocVisitor::visitPost(DocCopy *)
{
DB_VIS_C
  if (m_hide) return;
  // TODO: to be implemented
}


void AsciidocDocVisitor::visitPre(DocText *)
{
DB_VIS_C
  // TODO: to be implemented
}


void AsciidocDocVisitor::visitPost(DocText *)
{
DB_VIS_C
  // TODO: to be implemented
}


void AsciidocDocVisitor::visitPre(DocHtmlBlockQuote *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "[quote]" << endl;
  m_t << "____" << endl;
}

void AsciidocDocVisitor::visitPost(DocHtmlBlockQuote *)
{
DB_VIS_C
  if (m_hide) return;
  m_t << "____" << endl;
}

void AsciidocDocVisitor::visitPre(DocVhdlFlow *)
{
DB_VIS_C
  // TODO: to be implemented
}


void AsciidocDocVisitor::visitPost(DocVhdlFlow *)
{
DB_VIS_C
  // TODO: to be implemented
}

void AsciidocDocVisitor::visitPre(DocParBlock *)
{
DB_VIS_C
}

void AsciidocDocVisitor::visitPost(DocParBlock *)
{
DB_VIS_C
}


void AsciidocDocVisitor::filter(const char *str)
{
DB_VIS_C
  m_t << convertToAsciidoc(str);
}

void AsciidocDocVisitor::startLink(const QCString &file,const QCString &anchor)
{
DB_VIS_C
    m_t << "<<3_" << stripPath(file);
  if (!anchor.isEmpty())
  {
    if (file) m_t << "_1";
    m_t << anchor;
  }
  m_t << ",";
}

void AsciidocDocVisitor::endLink()
{
DB_VIS_C
  m_t << ">>";
}

void AsciidocDocVisitor::pushEnabled()
{
DB_VIS_C
  m_enabled.push(new bool(m_hide));
}

void AsciidocDocVisitor::popEnabled()
{
DB_VIS_C
  bool *v=m_enabled.pop();
  ASSERT(v!=0);
  m_hide = *v;
  delete v;
}

void AsciidocDocVisitor::writeMscFile(const QCString &baseName, DocVerbatim *s)
{
DB_VIS_C
  QCString shortName = baseName;
  int i;
  if ((i=shortName.findRev('/'))!=-1)
  {
    shortName=shortName.right(shortName.length()-i-1);
  }
  QCString outDir = Config_getString(ASCIIDOC_OUTPUT);
  writeMscGraphFromFile(baseName+".msc",outDir,shortName,MSC_BITMAP);
  visitADPreStart(m_t, s->hasCaption(), s->relPath() + shortName + ".png", s->width(),s->height());
  visitCaption(this, s->children());
  visitADPostEnd(m_t, s->hasCaption());
}

void AsciidocDocVisitor::writePlantUMLFile(const QCString &baseName, DocVerbatim *s)
{
DB_VIS_C
  QCString shortName = baseName;
  int i;
  if ((i=shortName.findRev('/'))!=-1)
  {
    shortName=shortName.right(shortName.length()-i-1);
  }
  QCString outDir = Config_getString(ASCIIDOC_OUTPUT);
  generatePlantUMLOutput(baseName,outDir,PUML_BITMAP);
  visitADPreStart(m_t, s->hasCaption(), s->relPath() + shortName + ".png", s->width(),s->height());
  visitCaption(this, s->children());
  visitADPostEnd(m_t, s->hasCaption());
}

void AsciidocDocVisitor::startMscFile(const QCString &fileName,
    const QCString &width,
    const QCString &height,
    bool hasCaption
    )
{
DB_VIS_C
  QCString baseName=fileName;
  int i;
  if ((i=baseName.findRev('/'))!=-1)
  {
    baseName=baseName.right(baseName.length()-i-1);
  }
  if ((i=baseName.find('.'))!=-1)
  {
    baseName=baseName.left(i);
  }
  baseName.prepend("msc_");
  QCString outDir = Config_getString(ASCIIDOC_OUTPUT);
  writeMscGraphFromFile(fileName,outDir,baseName,MSC_BITMAP);
  visitADPreStart(m_t, hasCaption, baseName + ".png",  width,  height);
}

void AsciidocDocVisitor::endMscFile(bool hasCaption)
{
DB_VIS_C
  if (m_hide) return;
  visitADPostEnd(m_t, hasCaption);
  m_t << endl;
  m_t << endl;
}

void AsciidocDocVisitor::writeDiaFile(const QCString &baseName, DocVerbatim *s)
{
DB_VIS_C
  QCString shortName = baseName;
  int i;
  if ((i=shortName.findRev('/'))!=-1)
  {
    shortName=shortName.right(shortName.length()-i-1);
  }
  QCString outDir = Config_getString(ASCIIDOC_OUTPUT);
  writeDiaGraphFromFile(baseName+".dia",outDir,shortName,DIA_BITMAP);
  visitADPreStart(m_t, s->hasCaption(), shortName, s->width(),s->height());
  visitCaption(this, s->children());
  visitADPostEnd(m_t, s->hasCaption());
}

void AsciidocDocVisitor::startDiaFile(const QCString &fileName,
    const QCString &width,
    const QCString &height,
    bool hasCaption
    )
{
DB_VIS_C
  QCString baseName=fileName;
  int i;
  if ((i=baseName.findRev('/'))!=-1)
  {
    baseName=baseName.right(baseName.length()-i-1);
  }
  if ((i=baseName.find('.'))!=-1)
  {
    baseName=baseName.left(i);
  }
  baseName.prepend("dia_");
  QCString outDir = Config_getString(ASCIIDOC_OUTPUT);
  writeDiaGraphFromFile(fileName,outDir,baseName,DIA_BITMAP);
  visitADPreStart(m_t, hasCaption, baseName + ".png",  width,  height);
}

void AsciidocDocVisitor::endDiaFile(bool hasCaption)
{
DB_VIS_C
  if (m_hide) return;
  visitADPostEnd(m_t, hasCaption);
  m_t << endl;
  m_t << endl;
}

void AsciidocDocVisitor::writeDotFile(const QCString &baseName, DocVerbatim *s)
{
DB_VIS_C
  QCString shortName = baseName;
  int i;
  if ((i=shortName.findRev('/'))!=-1)
  {
    shortName=shortName.right(shortName.length()-i-1);
  }
  QCString outDir = Config_getString(ASCIIDOC_OUTPUT);
  writeDotGraphFromFile(baseName+".dot",outDir,shortName,GOF_BITMAP);
  visitADPreStart(m_t, s->hasCaption(), s->relPath() + shortName + "." + getDotImageExtension(), s->width(),s->height());
  visitCaption(this, s->children());
  visitADPostEnd(m_t, s->hasCaption());
}

void AsciidocDocVisitor::startDotFile(const QCString &fileName,
    const QCString &width,
    const QCString &height,
    bool hasCaption
    )
{
DB_VIS_C
  QCString baseName=fileName;
  int i;
  if ((i=baseName.findRev('/'))!=-1)
  {
    baseName=baseName.right(baseName.length()-i-1);
  }
  if ((i=baseName.find('.'))!=-1)
  {
    baseName=baseName.left(i);
  }
  baseName.prepend("dot_");
  QCString outDir = Config_getString(ASCIIDOC_OUTPUT);
  QCString imgExt = getDotImageExtension();
  writeDotGraphFromFile(fileName,outDir,baseName,GOF_BITMAP);
  visitADPreStart(m_t, hasCaption, baseName + "." + imgExt,  width,  height);
}

void AsciidocDocVisitor::endDotFile(bool hasCaption)
{
DB_VIS_C
  if (m_hide) return;
  m_t << endl;
  visitADPostEnd(m_t, hasCaption);
  m_t << endl;
  m_t << endl;
}

