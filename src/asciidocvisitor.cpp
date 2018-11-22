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
#include "definition.h"
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

#if 1
#define GREEN   "\x1b[32m"
#define RESET   "\x1b[0m"
#define AD_VIS_C AD_VIS_C1(m_t)
#define AD_VIS_C1(x) x << GREEN "AD_VIS" << __LINE__ << RESET;
#define AD_VIS_C2(y) AD_VIS_C2a(m_t,y)
#define AD_VIS_C2a(x,y) x << GREEN "AD_VIS" << __LINE__ << " " << y << RESET;
#else
#define AD_VIS_C
#define AD_VIS_C1(x)
#define AD_VIS_C2(y)
#define AD_VIS_C2a(x,y)
#endif

void visitADPreStart(FTextStream &t, const bool hasCaption, QCString name,  QCString width,  QCString height)
{
  t << endl
    << endl;
  if (hasCaption)
  {
    t << "." << convertToAsciidoc(name) << endl;
  }
  t << "image::" << name << "[Image";
  if (!width.isEmpty() || !height.isEmpty())
  {
      t << ',' << convertToAsciidoc(width) << ',' << convertToAsciidoc(height);
  }
  t << ",opts=inline]" << endl;
}

void visitADPostEnd(FTextStream &t, const bool hasCaption)
{
  t << endl;
}

static void visitCaption(AsciidocDocVisitor *parent, QList<DocNode> children)
{
  QListIterator<DocNode> cli(children);
  DocNode *n;
  for (cli.toFirst();(n=cli.current());++cli) n->accept(parent);
}

AsciidocDocVisitor::AsciidocDocVisitor(FTextStream &t,CodeOutputInterface &ci,
				       Definition *ctx)
  : DocVisitor(DocVisitor_Asciidoc), m_t(t), m_ci(ci), m_insidePre(FALSE), m_insideCode(FALSE), m_hide(FALSE)
{
AD_VIS_C
  if (ctx) m_langExt=ctx->getDefFileExtension();
}
AsciidocDocVisitor::~AsciidocDocVisitor()
{
AD_VIS_C
}

//--------------------------------------
// visitor functions for leaf nodes
//--------------------------------------

void AsciidocDocVisitor::visit(DocWord *w)
{
AD_VIS_C
  if (m_hide) return;
  filter(w->word());
}

void AsciidocDocVisitor::visit(DocLinkedWord *w)
{
AD_VIS_C
  if (m_hide) return;
  startLink(w->file(),w->anchor());
  filter(w->word());
  endLink();
}

void AsciidocDocVisitor::visit(DocWhiteSpace *w)
{
AD_VIS_C
  if (m_hide) return;
  if (m_insidePre)
  {
    m_t << w->chars();
  }
  else if (m_insideCode)
  {
    m_t << "{nbsp}";
  }
  else
  {
    m_t << " ";
  }
}

void AsciidocDocVisitor::visit(DocSymbol *s)
{
AD_VIS_C
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
AD_VIS_C
  if (m_hide) return;
  if (u->isEmail()) m_t << "mailto:";
  filter(u->url());
}

void AsciidocDocVisitor::visit(DocLineBreak *)
{
AD_VIS_C
  if (m_hide) return;
  if (m_insidePre) m_t << '\n';
  else m_t << " +" << endl;
}

void AsciidocDocVisitor::visit(DocHorRuler *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << "'''" << endl;
}

void AsciidocDocVisitor::visit(DocStyleChange *s)
{
AD_VIS_C
  if (m_hide) return;
  switch (s->style())
  {
    case DocStyleChange::Bold:
      if (s->enable()) m_t << "**";     else m_t << "**";
      break;
    case DocStyleChange::Italic:
      if (s->enable()) m_t << "__";     else m_t << "__";
      break;
    case DocStyleChange::Code:
      if (s->enable())
      {
	  m_t << "``";
	  m_insideCode=TRUE;
      }
      else
      {
	  m_t << "``";
	  m_insideCode=FALSE;
      }
      break;
    case DocStyleChange::Subscript:
      if (s->enable()) m_t << "~";      else m_t << "~";
      break;
    case DocStyleChange::Superscript:
      if (s->enable()) m_t << "^";      else m_t << "^";
      break;
    case DocStyleChange::Center:
      if (s->enable()) m_t << "|===" << endl;
      else m_t << "|===" << endl;
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
AD_VIS_C
  if (m_hide) return;
  SrcLangExt langExt = getLanguageFromFileName(m_langExt);
  switch(s->type())
  {
    case DocVerbatim::Code: // fall though
      m_t << endl << "[source," << langExt << ']' << endl;
      Doxygen::parserManager->getParser(m_langExt)
        ->parseCode(m_ci,s->context(),s->text(),langExt,
            s->isExample(),s->exampleFile());
      m_t << endl;
      break;
    case DocVerbatim::Verbatim:
      m_t << endl << "[source]" << endl;
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
AD_VIS_C
  if (m_hide) return;
  m_t << "<<" << anc->anchor() << stripPath(anc->file()) << "1" << ",";
}

void AsciidocDocVisitor::visit(DocInclude *inc)
{
AD_VIS_C
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
AD_VIS_C
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
AD_VIS_C
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
AD_VIS_C
  if (m_hide) return;
  m_t << "<indexterm><primary>";
  filter(ie->entry());
  m_t << "</primary></indexterm>" << endl;
}

void AsciidocDocVisitor::visit(DocSimpleSectSep *)
{
AD_VIS_C
  // m_t << "<simplesect/>";
}

void AsciidocDocVisitor::visit(DocCite *cite)
{
AD_VIS_C
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
AD_VIS_C
  if (m_hide) return;
  if (l->isEnumList())
  {
    m_t << ". ";
  }
  else
  {
    m_t << "* ";
  }
}

void AsciidocDocVisitor::visitPost(DocAutoList *l)
{
AD_VIS_C
  if (m_hide) return;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocAutoListItem *i)
{
AD_VIS_C
  if (m_hide) return;
  DocAutoList* l = dynamic_cast<DocAutoList*>(i->parent());
  if (l && l->isEnumList())
  {
    m_t << ". ";
  }
  else
  {
    m_t << "* ";
  }
}

void AsciidocDocVisitor::visitPost(DocAutoListItem *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocPara *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << endl;
}

void AsciidocDocVisitor::visitPost(DocPara *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << endl; // Needed
}

void AsciidocDocVisitor::visitPre(DocRoot *)
{
AD_VIS_C
}

void AsciidocDocVisitor::visitPost(DocRoot *)
{
AD_VIS_C
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocSimpleSect *s)
{
AD_VIS_C
  if (m_hide) return;
  switch(s->type())
  {
    case DocSimpleSect::See:
      if (m_insidePre) 
      {
	m_t << endl
	    << endl << "==== " << theTranslator->trSeeAlso() << endl;
      } 
      else 
      {
        m_t << endl
	    << endl <<"==== " << convertToAsciidoc(theTranslator->trSeeAlso()) << endl;
      }
      break;
    case DocSimpleSect::Return:
      if (m_insidePre) 
      {
	m_t << endl
	    << endl << "==== " << theTranslator->trReturns() << endl;
      } 
      else 
      {
        m_t << endl
	    << endl <<"==== " << convertToAsciidoc(theTranslator->trReturns()) << endl;
      }
      break;
    case DocSimpleSect::Author:
      if (m_insidePre) 
      {
        m_t << endl
	    << endl << "==== " << theTranslator->trAuthor(TRUE, TRUE) << endl;
      } 
      else 
      {
        m_t << endl
	    << endl <<"==== " << convertToAsciidoc(theTranslator->trAuthor(TRUE, TRUE)) << endl;
      }
      break;
    case DocSimpleSect::Authors:
      if (m_insidePre) 
      {
        m_t << endl
	    << endl << "==== " << theTranslator->trAuthor(TRUE, FALSE) << endl;
      } 
      else 
      {
        m_t << endl
	    << endl <<"==== " << convertToAsciidoc(theTranslator->trAuthor(TRUE, FALSE)) << endl;
      }
      break;
    case DocSimpleSect::Version:
      if (m_insidePre)
      {
        m_t << endl
	    << endl <<"==== " << theTranslator->trVersion() << endl;
      } 
      else 
      {
        m_t << endl
	    <<endl << "==== " << convertToAsciidoc(theTranslator->trVersion()) << endl;
      }
      break;
    case DocSimpleSect::Since:
      if (m_insidePre) 
      {
        m_t << endl
	    << endl <<"==== " << theTranslator->trSince() << endl;
      } 
      else 
      {
        m_t << endl
	    << endl <<"==== " << convertToAsciidoc(theTranslator->trSince()) << endl;
      }
      break;
    case DocSimpleSect::Date:
      if (m_insidePre) 
      {
        m_t << endl
	    << endl <<"==== " << theTranslator->trDate() << endl;
      } 
      else 
      {
        m_t << endl
	    << endl <<"==== " << convertToAsciidoc(theTranslator->trDate()) << endl;
      }
      break;
    case DocSimpleSect::Note:
      // if (m_insidePre) 
      m_t << endl
	  << endl <<"[NOTE]" << endl
	  << "====" << endl;
      break;
    case DocSimpleSect::Warning:
      m_t << endl
	  << endl <<"[WARNING]" << endl
	  << "====" << endl;
      break;
    case DocSimpleSect::Pre:
      if (m_insidePre) 
      {
        m_t << endl
	    << endl <<"==== " << theTranslator->trPrecondition() << endl;
      } 
      else 
      {
        m_t << endl
	    << endl <<"==== " << convertToAsciidoc(theTranslator->trPrecondition()) << endl;
      }
      break;
    case DocSimpleSect::Post:
      if (m_insidePre) 
      {
        m_t << endl
	    << endl <<"==== " << theTranslator->trPostcondition() << endl;
      } 
      else 
      {
        m_t << endl
	    << endl <<"==== " << convertToAsciidoc(theTranslator->trPostcondition()) << endl;
      }
      break;
    case DocSimpleSect::Copyright:
      if (m_insidePre) 
      {
        m_t << endl
	    << endl <<"==== " << theTranslator->trCopyright() << endl;
      } 
      else 
      {
        m_t << endl
	    << endl <<"==== " << convertToAsciidoc(theTranslator->trCopyright()) << endl;
      }
      break;
    case DocSimpleSect::Invar:
      if (m_insidePre) 
      {
        m_t << endl
	    << endl <<"==== " << theTranslator->trInvariant() << endl;
      } 
      else 
      {
        m_t << endl
	    << endl <<"==== " << convertToAsciidoc(theTranslator->trInvariant()) << endl;
      }
      break;
    case DocSimpleSect::Remark:
      if (m_insidePre) 
      {
        m_t << endl
	    << endl <<"==== " << theTranslator->trRemarks() << endl;
      } 
      else 
      {
        m_t << endl
	    << endl <<"==== " << convertToAsciidoc(theTranslator->trRemarks()) << endl;
      }
      break;
    case DocSimpleSect::Attention:
      m_t << endl
	  << endl <<"[CAUTION]" << endl
	  << "===" << endl;
      break;
    case DocSimpleSect::User:
    case DocSimpleSect::Rcs:
    case DocSimpleSect::Unknown:
      break;
  }
}

void AsciidocDocVisitor::visitPost(DocSimpleSect *s)
{
AD_VIS_C
  if (m_hide) return;
  switch(s->type())
  {
    case DocSimpleSect::Rcs:
    case DocSimpleSect::Unknown:
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
AD_VIS_C
  if (m_hide) return;
  if (t->hasTitle()) m_t << endl
			 << endl << "==== ";
}

void AsciidocDocVisitor::visitPost(DocTitle *t)
{
AD_VIS_C
  if (m_hide) return;
  if (t->hasTitle()) m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocSimpleList *)
{
AD_VIS_C
  if (m_hide) return;
    m_t << "PRE\n";
}

void AsciidocDocVisitor::visitPost(DocSimpleList *)
{
AD_VIS_C
  if (m_hide) return;
    m_t << "POST\n";
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocSimpleListItem *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << "*11 ";
}

void AsciidocDocVisitor::visitPost(DocSimpleListItem *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << "\n";
}

void AsciidocDocVisitor::visitPre(DocSection *s)
{
AD_VIS_C
  if (m_hide) return;
  m_t << endl
      << endl;
  m_t << "[[" <<  stripPath(s->file());
  if (!s->anchor().isEmpty()) m_t << "1" << s->anchor();
  m_t << "]]";
  m_t << endl << "=== ";
  filter(s->title());
  m_t << endl;
}

void AsciidocDocVisitor::visitPost(DocSection *)
{
AD_VIS_C
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocHtmlList *s)
{
AD_VIS_C
  if (m_hide) return;
  if (s->type()==DocHtmlList::Ordered)
    m_t << "<orderedlist>\n";
  else
    m_t << "<itemizedlist>\n";
}

void AsciidocDocVisitor::visitPost(DocHtmlList *s)
{
AD_VIS_C
  if (m_hide) return;
  m_t << "\n";
}

void AsciidocDocVisitor::visitPre(DocHtmlListItem *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << "*12 ";
}

void AsciidocDocVisitor::visitPost(DocHtmlListItem *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << "\n";
}

void AsciidocDocVisitor::visitPre(DocHtmlDescList *)
{
AD_VIS_C
  if (m_hide) return;
}

void AsciidocDocVisitor::visitPost(DocHtmlDescList *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocHtmlDescTitle *)
{
AD_VIS_C
  if (m_hide) return;
}

void AsciidocDocVisitor::visitPost(DocHtmlDescTitle *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << "::\n";
}

void AsciidocDocVisitor::visitPre(DocHtmlDescData *)
{
AD_VIS_C
  if (m_hide) return;
}

void AsciidocDocVisitor::visitPost(DocHtmlDescData *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << endl;
}

static int colCnt = 0;
static bool bodySet = FALSE; // it is possible to have tables without a header
void AsciidocDocVisitor::visitPre(DocHtmlTable *t)
{
AD_VIS_C
  bodySet = FALSE;
  if (m_hide) return;
  m_t << endl << "|===" << endl;
}

void AsciidocDocVisitor::visitPost(DocHtmlTable *)
{
AD_VIS_C
  if (m_hide) return;
m_t << "|===" << endl;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocHtmlRow *tr)
{
AD_VIS_C
  colCnt = 0;
  if (m_hide) return;

  if (tr->isHeading()) m_t << "| ";
  else if (!bodySet)
  {
    bodySet = TRUE;
    m_t << "| ";
  }

  // HtmlAttribListIterator li(tr->attribs());
  // HtmlAttrib *opt;
  // for (li.toFirst();(opt=li.current());++li)
  // {
  //   if (opt->name=="class")
  //   {
  //     // just skip it
  //   }
  //   else if (opt->name=="style")
  //   {
  //     // just skip it
  //   }
  //   else if (opt->name=="height")
  //   {
  //     // just skip it
  //   }
  //   else if (opt->name=="filter")
  //   {
  //     // just skip it
  //   }
  //   else
  //   {
  //     m_t << " " << opt->name << "='" << opt->value << "'";
  //   }
  // }
  // m_t << ">\n";
}

void AsciidocDocVisitor::visitPost(DocHtmlRow *tr)
{
AD_VIS_C
  if (m_hide) return;
  m_t << endl;
  if (tr->isHeading())
  {
    bodySet = TRUE;
    m_t << endl;
  }
}

void AsciidocDocVisitor::visitPre(DocHtmlCell *c)
{
AD_VIS_C
  colCnt++;
  if (m_hide) return;
  m_t << "|";

  // HtmlAttribListIterator li(c->attribs());
  // HtmlAttrib *opt;
  // for (li.toFirst();(opt=li.current());++li)
  // {
  //   if (opt->name=="colspan")
  //   {
  //     m_t << " namest='c" << colCnt << "'";
  //     int cols = opt->value.toInt();
  //     colCnt += (cols - 1);
  //     m_t << " nameend='c" << colCnt << "'";
  //   }
  //   else if (opt->name=="rowspan")
  //   {
  //     int extraRows = opt->value.toInt() - 1;
  //     m_t << " morerows='" << extraRows << "'";
  //   }
  //   else if (opt->name=="class")
  //   {
  //     if (opt->value == "markdownTableBodyRight")
  //     {
  //       m_t << " align='right'";
  //     }
  //     else if (opt->value == "markdownTableBodyLeftt")
  //     {
  //       m_t << " align='left'";
  //     }
  //     else if (opt->value == "markdownTableBodyCenter")
  //     {
  //       m_t << " align='center'";
  //     }
  //     else if (opt->value == "markdownTableHeadRight")
  //     {
  //       m_t << " align='right'";
  //     }
  //     else if (opt->value == "markdownTableHeadLeftt")
  //     {
  //       m_t << " align='left'";
  //     }
  //     else if (opt->value == "markdownTableHeadCenter")
  //     {
  //       m_t << " align='center'";
  //     }
  //   }
  //   else if (opt->name=="style")
  //   {
  //     // just skip it
  //   }
  //   else if (opt->name=="width")
  //   {
  //     // just skip it
  //   }
  //   else if (opt->name=="height")
  //   {
  //     // just skip it
  //   }
  //   else
  //   {
  //     m_t << " " << opt->name << "='" << opt->value << "'";
  //   }
  // }
  // m_t << ">";
}

void AsciidocDocVisitor::visitPost(DocHtmlCell *c)
{
AD_VIS_C
  if (m_hide) return;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocHtmlCaption *c)
{
AD_VIS_C
  if (m_hide) return;
   m_t << "<caption>";
}

void AsciidocDocVisitor::visitPost(DocHtmlCaption *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << "</caption>\n";
}

void AsciidocDocVisitor::visitPre(DocInternal *)
{
AD_VIS_C
  if (m_hide) return;
  // TODO: to be implemented
}

void AsciidocDocVisitor::visitPost(DocInternal *)
{
AD_VIS_C
  if (m_hide) return;
  // TODO: to be implemented
}

void AsciidocDocVisitor::visitPre(DocHRef *href)
{
AD_VIS_C
  if (m_hide) return;
  m_t << "link:++" << convertToAsciidoc(href->url());
}

void AsciidocDocVisitor::visitPost(DocHRef *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << "++";
}

void AsciidocDocVisitor::visitPre(DocHtmlHeader *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << endl
      << endl << "==== ";
}

void AsciidocDocVisitor::visitPost(DocHtmlHeader *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocImage *img)
{
AD_VIS_C
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
AD_VIS_C
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
AD_VIS_C
  if (m_hide) return;
  startDotFile(df->file(),df->width(),df->height(),df->hasCaption());
}

void AsciidocDocVisitor::visitPost(DocDotFile *df)
{
AD_VIS_C
  if (m_hide) return;
  endDotFile(df->hasCaption());
}

void AsciidocDocVisitor::visitPre(DocMscFile *df)
{
AD_VIS_C
  if (m_hide) return;
  startMscFile(df->file(),df->width(),df->height(),df->hasCaption());
}

void AsciidocDocVisitor::visitPost(DocMscFile *df)
{
AD_VIS_C
  if (m_hide) return;
  endMscFile(df->hasCaption());
}
void AsciidocDocVisitor::visitPre(DocDiaFile *df)
{
AD_VIS_C
  if (m_hide) return;
  startDiaFile(df->file(),df->width(),df->height(),df->hasCaption());
}

void AsciidocDocVisitor::visitPost(DocDiaFile *df)
{
AD_VIS_C
  if (m_hide) return;
  endDiaFile(df->hasCaption());
}

void AsciidocDocVisitor::visitPre(DocLink *lnk)
{
AD_VIS_C
  if (m_hide) return;
  startLink(lnk->file(),lnk->anchor());
}

void AsciidocDocVisitor::visitPost(DocLink *)
{
AD_VIS_C
  if (m_hide) return;
  endLink();
}

void AsciidocDocVisitor::visitPre(DocRef *ref)
{
AD_VIS_C
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
AD_VIS_C
  if (m_hide) return;
  if (!ref->file().isEmpty()) endLink();
}

void AsciidocDocVisitor::visitPre(DocSecRefItem *ref)
{
AD_VIS_C
  if (m_hide) return;
  //m_t << "<tocentry xml:idref=\"_" <<  stripPath(ref->file()) << "_1" << ref->anchor() << "\">";
  m_t << "<tocentry>";
}

void AsciidocDocVisitor::visitPost(DocSecRefItem *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << "</tocentry>" << endl;
}

void AsciidocDocVisitor::visitPre(DocSecRefList *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << "<toc>" << endl;
}

void AsciidocDocVisitor::visitPost(DocSecRefList *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << "</toc>" << endl;
}

void AsciidocDocVisitor::visitPre(DocParamSect *s)
{
AD_VIS_C
  if (m_hide) return;
  m_t << endl
      << endl << "==== ";
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
  m_t << "[horizontal]" << endl;
}

void AsciidocDocVisitor::visitPost(DocParamSect *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocParamList *pl)
{
AD_VIS_C
  if (m_hide) return;
  QListIterator<DocNode> li(pl->parameters());
  DocNode *param;
  if (!li.toFirst())
  {
    m_t << "|4" << endl;
  }
  else
  {
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

  DocParamSect::Type parentType = DocParamSect::Unknown;
  DocParamSect *sect = 0;
  if (pl->parent() && pl->parent()->kind()==DocNode::Kind_ParamSect)
  {
    parentType = ((DocParamSect*)pl->parent())->type();
    sect=(DocParamSect*)pl->parent();
  }

  if (sect)
  {
    if (sect->hasInOutSpecifier())
    {
      if (pl->direction()!=DocParamSect::Unspecified)
      {
	m_t << " [.small]#";
	if (pl->direction()==DocParamSect::In) m_t << "[in]";
	else if (pl->direction()==DocParamSect::Out) m_t << "[out]";
	else if (pl->direction()==DocParamSect::InOut) m_t << "[in,out]";
	m_t << "#";
      }
    }

    if (sect->hasTypeSpecifier())
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
  }

  m_t << "::" << endl;
}

void AsciidocDocVisitor::visitPost(DocParamList *)
{
AD_VIS_C
  if (m_hide) return;
}

void AsciidocDocVisitor::visitPre(DocXRefItem *x)
{
AD_VIS_C
  if (m_hide) return;
  if (x->title().isEmpty()) return;
  m_t << "[[";
  m_t << stripPath(x->file()) << "1" << x->anchor();
  m_t << ",";
  filter(x->title());
  m_t << "]]";
  m_t << " ";
}

void AsciidocDocVisitor::visitPost(DocXRefItem *x)
{
AD_VIS_C
  if (m_hide) return;
  if (x->title().isEmpty()) return;
  m_t << endl;
  m_t << endl;
}

void AsciidocDocVisitor::visitPre(DocInternalRef *ref)
{
AD_VIS_C
  if (m_hide) return;
  startLink(ref->file(),ref->anchor());
}

void AsciidocDocVisitor::visitPost(DocInternalRef *)
{
AD_VIS_C
  if (m_hide) return;
  endLink();
  m_t << " ";
}

void AsciidocDocVisitor::visitPre(DocCopy *)
{
AD_VIS_C
  if (m_hide) return;
  // TODO: to be implemented
}


void AsciidocDocVisitor::visitPost(DocCopy *)
{
AD_VIS_C
  if (m_hide) return;
  // TODO: to be implemented
}


void AsciidocDocVisitor::visitPre(DocText *)
{
AD_VIS_C
}


void AsciidocDocVisitor::visitPost(DocText *)
{
AD_VIS_C
  m_t << endl
      << endl;
}


void AsciidocDocVisitor::visitPre(DocHtmlBlockQuote *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << "[quote]" << endl;
  m_t << "____" << endl;
}

void AsciidocDocVisitor::visitPost(DocHtmlBlockQuote *)
{
AD_VIS_C
  if (m_hide) return;
  m_t << "____" << endl;
}

void AsciidocDocVisitor::visitPre(DocVhdlFlow *)
{
AD_VIS_C
  // TODO: to be implemented
}


void AsciidocDocVisitor::visitPost(DocVhdlFlow *)
{
AD_VIS_C
  // TODO: to be implemented
}

void AsciidocDocVisitor::visitPre(DocParBlock *)
{
AD_VIS_C
}

void AsciidocDocVisitor::visitPost(DocParBlock *)
{
AD_VIS_C
}


void AsciidocDocVisitor::filter(const char *str)
{
AD_VIS_C
  m_t << convertToAsciidoc(str);
}

void AsciidocDocVisitor::startLink(const QCString &file,const QCString &anchor)
{
AD_VIS_C
  m_t << "<<";
  if (!anchor.isEmpty())
  {
    m_t << anchor;
    if (file) m_t << stripPath(file) << "1";
  }
  else
  {
    m_t << stripPath(file);
  }

  m_t << ",";
}

void AsciidocDocVisitor::endLink()
{
AD_VIS_C
  m_t << ">>";
}

void AsciidocDocVisitor::pushEnabled()
{
AD_VIS_C
  m_enabled.push(new bool(m_hide));
}

void AsciidocDocVisitor::popEnabled()
{
AD_VIS_C
  bool *v=m_enabled.pop();
  ASSERT(v!=0);
  m_hide = *v;
  delete v;
}

void AsciidocDocVisitor::writeMscFile(const QCString &baseName, DocVerbatim *s)
{
AD_VIS_C
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
AD_VIS_C
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
AD_VIS_C
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
AD_VIS_C
  if (m_hide) return;
  visitADPostEnd(m_t, hasCaption);
  m_t << endl;
  m_t << endl;
}

void AsciidocDocVisitor::writeDiaFile(const QCString &baseName, DocVerbatim *s)
{
AD_VIS_C
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
AD_VIS_C
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
AD_VIS_C
  if (m_hide) return;
  visitADPostEnd(m_t, hasCaption);
  m_t << endl;
  m_t << endl;
}

void AsciidocDocVisitor::writeDotFile(const QCString &baseName, DocVerbatim *s)
{
AD_VIS_C
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
AD_VIS_C
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
AD_VIS_C
  if (m_hide) return;
  m_t << endl;
  visitADPostEnd(m_t, hasCaption);
  m_t << endl;
  m_t << endl;
}

