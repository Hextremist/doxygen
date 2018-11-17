/******************************************************************************
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

#include <stdlib.h>

#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qintdict.h>
#include <qregexp.h>
#include "asciidocgen.h"
#include "doxygen.h"
#include "message.h"
#include "config.h"
#include "classlist.h"
#include "classdef.h"
#include "diagram.h"
#include "util.h"
#include "defargs.h"
#include "outputgen.h"
#include "dot.h"
#include "pagedef.h"
#include "filename.h"
#include "version.h"
#include "asciidocvisitor.h"
#include "docparser.h"
#include "language.h"
#include "parserintf.h"
#include "arguments.h"
#include "memberlist.h"
#include "groupdef.h"
#include "memberdef.h"
#include "namespacedef.h"
#include "membername.h"
#include "membergroup.h"
#include "dirdef.h"
#include "section.h"

#if 1
#define BLUE    "\x1b[34m"
#define RESET   "\x1b[0m"
#define AD_GEN_C AD_GEN_C1(t)
#define AD_GEN_C1(x) x << BLUE "AD_GEN_C " << __LINE__ << RESET;
#define AD_GEN_C2(y) AD_GEN_C2a(t,y)
#define AD_GEN_C2a(x,y) x << BLUE "AD_GEN_C " << __LINE__ << ":" << y << RESET;
#define AD_VIS_C AD_VIS_C1(m_t)
#define AD_VIS_C1(x) x << BLUE "AD_GEN_C " << __LINE__ << RESET;
#define AD_VIS_C2(y) AD_VIS_C2a(m_t,y)
#define AD_VIS_C2a(x,y) x << BLUE "AD_GEN_C " << __LINE__ << " " << y << RESET;
#else
#define AD_GEN_C
#define AD_GEN_C1(x)
#define AD_GEN_C2(y)
#define AD_GEN_C2a(x,y)
#endif
//------------------

class AsciidocSectionMapper : public QIntDict<char>
{
  public:
    AsciidocSectionMapper() : QIntDict<char>(47)
  {
    insert(MemberListType_pubTypes,"public-type");
    insert(MemberListType_pubMethods,"public-func");
    insert(MemberListType_pubAttribs,"public-attrib");
    insert(MemberListType_pubSlots,"public-slot");
    insert(MemberListType_signals,"signal");
    insert(MemberListType_dcopMethods,"dcop-func");
    insert(MemberListType_properties,"property");
    insert(MemberListType_events,"event");
    insert(MemberListType_pubStaticMethods,"public-static-func");
    insert(MemberListType_pubStaticAttribs,"public-static-attrib");
    insert(MemberListType_proTypes,"protected-type");
    insert(MemberListType_proMethods,"protected-func");
    insert(MemberListType_proAttribs,"protected-attrib");
    insert(MemberListType_proSlots,"protected-slot");
    insert(MemberListType_proStaticMethods,"protected-static-func");
    insert(MemberListType_proStaticAttribs,"protected-static-attrib");
    insert(MemberListType_pacTypes,"package-type");
    insert(MemberListType_pacMethods,"package-func");
    insert(MemberListType_pacAttribs,"package-attrib");
    insert(MemberListType_pacStaticMethods,"package-static-func");
    insert(MemberListType_pacStaticAttribs,"package-static-attrib");
    insert(MemberListType_priTypes,"private-type");
    insert(MemberListType_priMethods,"private-func");
    insert(MemberListType_priAttribs,"private-attrib");
    insert(MemberListType_priSlots,"private-slot");
    insert(MemberListType_priStaticMethods,"private-static-func");
    insert(MemberListType_priStaticAttribs,"private-static-attrib");
    insert(MemberListType_friends,"friend");
    insert(MemberListType_related,"related");
    insert(MemberListType_decDefineMembers,"define");
    insert(MemberListType_decProtoMembers,"prototype");
    insert(MemberListType_decTypedefMembers,"typedef");
    insert(MemberListType_decEnumMembers,"enum");
    insert(MemberListType_decFuncMembers,"func");
    insert(MemberListType_decVarMembers,"var");
  }
};

inline void writeAsciidocString(FTextStream &t,const char *s)
{
  t << convertToAsciidoc(s);
}

static void addIndexTerm(FTextStream &t, QCString prim, QCString sec = "")
{
  // No indexterm support in asciidoc yet
}

static void writeInclude(FTextStream &t, const char *const filename)
{
    t << "include::" << filename << ".adoc[leveloffset=+1]" << endl;
}

static void writeAsciidocLink(FTextStream &t,const char * /*extRef*/,const char *compoundId,
    const char *anchorId,const char * text,const char * /*tooltip*/)
{
  t << "<<" << stripPath(compoundId);
  if (anchorId) t << "_1" << anchorId;
  t << ",";
  writeAsciidocString(t,text);
  t << ">>";
}

class TextGeneratorAsciidocImpl : public TextGeneratorIntf
{
  public:
    TextGeneratorAsciidocImpl(FTextStream &t): m_t(t) {}
    void writeString(const char *s,bool /*keepSpaces*/) const
    {
      writeAsciidocString(m_t,s);
    }
    void writeBreak(int) const {}
    void writeLink(const char *extRef,const char *file,
        const char *anchor,const char *text
        ) const
    {
      writeAsciidocLink(m_t,extRef,file,anchor,text,0);
    }
  private:
    FTextStream &m_t;
};

AsciidocCodeGenerator::AsciidocCodeGenerator(FTextStream &t) : m_lineNumber(-1), m_col(0),
  m_insideCodeLine(FALSE), m_insideSpecialHL(FALSE)
{
  m_prettyCode=Config_getBool(ASCIIDOC_PROGRAMLISTING);
  setTextStream(t);
}

AsciidocCodeGenerator::AsciidocCodeGenerator() : m_lineNumber(-1), m_col(0),
  m_insideCodeLine(FALSE), m_insideSpecialHL(FALSE),  m_streamSet(FALSE)
{
  m_prettyCode=Config_getBool(ASCIIDOC_PROGRAMLISTING);
}

AsciidocCodeGenerator::~AsciidocCodeGenerator() {}

void AsciidocCodeGenerator::codify(const char *text)
{
  AD_VIS_C
  char c;
  while ((c=*text++))
  {
    switch(c)
    {
      case '\t':
        {
          static int tabSize = Config_getInt(TAB_SIZE);
          int spacesToNextTabStop = tabSize - (m_col%tabSize);
          m_col+=spacesToNextTabStop;
          while (spacesToNextTabStop--) m_t << ' ';
          break;
        }
      case '\007':  m_t << "^G"; m_col++; break; // bell
      case '\014':  m_t << "^L"; m_col++; break; // form feed
      default:   m_t << c; m_col++;       break;
    }
  }
}
void AsciidocCodeGenerator::writeCodeLink(const char *ref,const char *file,
    const char *anchor,const char *name, const char *tooltip)
{
  AD_VIS_C
  writeAsciidocLink(m_t,ref,file,anchor,name,tooltip);
  m_col+=strlen(name);
}
void AsciidocCodeGenerator::writeCodeLinkLine(const char *ref,const char *file,
    const char *anchor,const char *name,
    const char *tooltip)
{
  AD_VIS_C
  m_t << "<<" << stripExtensionGeneral(stripPath(file),".xml");
  m_t << "_1l";
  writeAsciidocString(m_t,name);
  m_t << ">>";
  m_col+=strlen(name);
}
void AsciidocCodeGenerator::writeTooltip(const char *, const DocLinkInfo &, const char *,
                  const char *, const SourceLinkInfo &, const SourceLinkInfo &
                 )
{
  AD_VIS_C
}
void AsciidocCodeGenerator::startCodeLine(bool)
{
  AD_VIS_C
  m_insideCodeLine=TRUE;
  m_col=0;
}
void AsciidocCodeGenerator::endCodeLine()
{
  AD_VIS_C
  m_t << endl;
  m_lineNumber = -1;
  m_refId.resize(0);
  m_external.resize(0);
  m_insideCodeLine=FALSE;
}
void AsciidocCodeGenerator::startFontClass(const char *colorClass)
{
  AD_VIS_C
  m_insideSpecialHL=TRUE;
}
void AsciidocCodeGenerator::endFontClass()
{
  AD_VIS_C
  m_insideSpecialHL=FALSE;
}
void AsciidocCodeGenerator::writeCodeAnchor(const char *)
{
  AD_VIS_C
}
void AsciidocCodeGenerator::writeLineNumber(const char *ref,const char *fileName,
    const char *anchor,int l)
{
  AD_VIS_C
  m_insideCodeLine = TRUE;
}
void AsciidocCodeGenerator::setCurrentDoc(Definition *,const char *,bool)
{
  AD_VIS_C
}
void AsciidocCodeGenerator::addWord(const char *,bool)
{
  AD_VIS_C
}
void AsciidocCodeGenerator::finish()
{
  AD_VIS_C
  if (m_insideCodeLine) endCodeLine();
}

void AsciidocCodeGenerator::startCodeFragment(SrcLangExt lang)
{
  AD_VIS_C
  m_t << endl
      << "[source]" << endl
      << "----" << endl;
}
void AsciidocCodeGenerator::endCodeFragment()
{
  AD_VIS_C
  m_t << "----" << endl
      << endl;
}


AsciidocGenerator::AsciidocGenerator() : OutputGenerator()
{
AD_GEN_C
  dir=Config_getString(ASCIIDOC_OUTPUT);
  //insideTabbing=FALSE;
  //firstDescItem=TRUE;
  //disableLinks=FALSE;
  //m_indent=0;
  //templateMemberItem = FALSE;
  m_prettyCode=Config_getBool(ASCIIDOC_PROGRAMLISTING);
  m_denseText = FALSE;
  m_inGroup = FALSE;
  m_inDetail = FALSE;
  m_levelListItem = 0;
  m_descTable = FALSE;
  m_inLevel = -1;
  m_firstMember = FALSE;
  m_pageref = FALSE;
  for (int i = 0 ; i < sizeof(m_inListItem) / sizeof(*m_inListItem) ; i++) m_inListItem[i] = FALSE;
  for (int i = 0 ; i < sizeof(m_inSimpleSect) / sizeof(*m_inSimpleSect) ; i++) m_inSimpleSect[i] = FALSE;
}

AsciidocGenerator::~AsciidocGenerator()
{
AD_GEN_C
}

void AsciidocGenerator::init()
{
  QCString dir=Config_getString(ASCIIDOC_OUTPUT);
  QDir d(dir);
  if (!d.exists() && !d.mkdir(dir))
  {
    err("Could not create output directory %s\n",dir.data());
    exit(1);
  }

  createSubDirs(d);
}

void AsciidocGenerator::startFile(const char *name,const char *,const char *)
{
AD_GEN_C
  t << "START FILE\n";
  QCString fileName=name;
  QCString pageName;
  QCString fileType="section";
  if (fileName == "refman")
  {
    fileName="index";
    fileType="book";
  }
  else if (fileName == "index")
  {
    fileName="mainpage";
    fileType="chapter";
  }
  pageName = fileName;
  relPath = relativePathToRoot(fileName);
  if (fileName.right(5)!=".adoc") fileName+=".adoc";
  startPlainFile(fileName);
  m_codeGen.setTextStream(t);
  m_codeGen.setRelativePath(relPath);
  m_codeGen.setSourceFileName(stripPath(fileName));

  if (!pageName.isEmpty()) t << "[[" <<  stripPath(pageName) << "]]" << endl;
}

void AsciidocGenerator::writeSearchInfo()
{
AD_GEN_C
}

void AsciidocGenerator::writeFooter(const char *)
{
AD_GEN_C
}

void AsciidocGenerator::endFile()
{
AD_GEN_C
  t << endl;
  endPlainFile();
}

void AsciidocGenerator::startIndexSection(IndexSections is)
{
AD_GEN_C2("IndexSections " << is)
  switch (is)
  {
    case isTitlePageStart:
      t << "= " << convertToAsciidoc(Config_getString(PROJECT_NAME)) << ": "
	<< convertToAsciidoc(Config_getString(PROJECT_BRIEF)) << endl
	<< ":toc: left" << endl
	<< ":experimental:" << endl
        << ":source-highlighter: coderay" << endl
        << endl
	<< "== Introduction" << endl;
      break;
    case isTitlePageAuthor:
      t << "[discrete]" << endl;
      t << "=== ";
      break;
    case isMainPage:
    case isModuleIndex:
    case isDirIndex:
    case isNamespaceIndex:
    case isClassHierarchyIndex:
    case isCompoundIndex:
    case isFileIndex:
    case isPageIndex:
    case isModuleDocumentation:
    case isDirDocumentation:
    case isNamespaceDocumentation:
    case isClassDocumentation:
    case isFileDocumentation:
    case isExampleDocumentation:
      t << "== ";
      break;
    case isPageDocumentation:
    case isPageDocumentation2:
    case isEndIndex:
      break;
  }
}

void AsciidocGenerator::endIndexSection(IndexSections is)
{
AD_GEN_C2("IndexSections " << is)
  static bool sourceBrowser = Config_getBool(SOURCE_BROWSER);
  switch (is)
  {
    case isTitlePageStart:
      break;
    case isTitlePageAuthor:
      break;
    case isMainPage:
      break;
    case isModuleIndex:
      //t << "</chapter>" << endl;
      break;
    case isDirIndex:
      writeInclude(t, "dirs");
      break;
    case isNamespaceIndex:
      writeInclude(t, "namespaces");
      break;
    case isClassHierarchyIndex:
      writeInclude(t, "hierarchy");
      t << endl;
      break;
    case isCompoundIndex:
      writeInclude(t, "annotated");
      t << endl;
      break;
    case isFileIndex:
      //writeInclude(t, files);
      break;
    case isPageIndex:
      //writeInclude(t, pages);
      break;
    case isModuleDocumentation:
      {
        GroupSDict::Iterator gli(*Doxygen::groupSDict);
        GroupDef *gd;
        bool found=FALSE;
        for (gli.toFirst();(gd=gli.current()) && !found;++gli)
        {
          if (!gd->isReference())
          {
	    writeInclude(t, gd->getOutputFileBase());
            found=TRUE;
          }
        }
        for (;(gd=gli.current());++gli)
        {
          if (!gd->isReference())
          {
	    writeInclude(t, gd->getOutputFileBase());
          }
        }
      }
      t << endl;
      break;
    case isDirDocumentation:
      {
        SDict<DirDef>::Iterator dli(*Doxygen::directories);
        DirDef *dd;
        bool found=FALSE;
        for (dli.toFirst();(dd=dli.current()) && !found;++dli)
        {
          if (dd->isLinkableInProject())
          {
	    writeInclude(t, dd->getOutputFileBase());
            found=TRUE;
          }
        }
        for (;(dd=dli.current());++dli)
        {
          if (dd->isLinkableInProject())
          {
	    writeInclude(t, dd->getOutputFileBase());
          }
        }
      }
      break;
    case isNamespaceDocumentation:
      {
        NamespaceSDict::Iterator nli(*Doxygen::namespaceSDict);
        NamespaceDef *nd;
        bool found=FALSE;
        for (nli.toFirst();(nd=nli.current()) && !found;++nli)
        {
          if (nd->isLinkableInProject())
          {
	    writeInclude(t, nd->getOutputFileBase());
            found=TRUE;
          }
        }
        while ((nd=nli.current()))
        {
          if (nd->isLinkableInProject())
          {
	    writeInclude(t, nd->getOutputFileBase());
          }
          ++nli;
        }
      }
      t << endl;
      break;
    case isClassDocumentation:
      {
        ClassSDict::Iterator cli(*Doxygen::classSDict);
        ClassDef *cd=0;
        bool found=FALSE;
        for (cli.toFirst();(cd=cli.current()) && !found;++cli)
        {
          if (cd->isLinkableInProject() && 
              cd->templateMaster()==0 &&
             !cd->isEmbeddedInOuterScope()
             )
          {
	    writeInclude(t, cd->getOutputFileBase());
            found=TRUE;
          }
        }
        for (;(cd=cli.current());++cli)
        {
          if (cd->isLinkableInProject() && 
              cd->templateMaster()==0 &&
             !cd->isEmbeddedInOuterScope()
             )
          {
	    writeInclude(t, cd->getOutputFileBase());
          } 
        }
      }
      t << endl;
      break;
    case isFileDocumentation:
      {
        bool isFirst=TRUE;
        FileNameListIterator fnli(*Doxygen::inputNameList); 
        FileName *fn;
        for (fnli.toFirst();(fn=fnli.current());++fnli)
        {
          FileNameIterator fni(*fn);
          FileDef *fd;
          for (;(fd=fni.current());++fni)
          {
            if (fd->isLinkableInProject())
            {
              if (isFirst)
              {
		writeInclude(t, fd->getOutputFileBase());
                if (sourceBrowser && m_prettyCode && fd->generateSourceFile())
                {
		  writeInclude(t, fd->getSourceFileBase());
                }
                isFirst=FALSE;
              }
              else
              {
		writeInclude(t, fd->getOutputFileBase());
                if (sourceBrowser && m_prettyCode && fd->generateSourceFile())
                {
		  writeInclude(t, fd->getSourceFileBase());
                }
              }
            }
	    else printf ("NOT LINKABLE\n");
          }
        }
      }
      t << endl;
      break;
    case isExampleDocumentation:
      {
        PageSDict::Iterator pdi(*Doxygen::exampleSDict);
        PageDef *pd=pdi.toFirst();
        if (pd)
        {
	  writeInclude(t, pd->getOutputFileBase());
        }
        for (++pdi;(pd=pdi.current());++pdi)
        {
	  writeInclude(t, pd->getOutputFileBase());
        }
      }
      t << "</chapter>\n";
      break;
    case isPageDocumentation:
      break;
    case isPageDocumentation2:
      break;
    case isEndIndex:
      break;
  }
}
void AsciidocGenerator::writePageLink(const char *name, bool /*first*/)
{
AD_GEN_C
  PageSDict::Iterator pdi(*Doxygen::pageSDict);
  PageDef *pd = pdi.toFirst();
  for (pd = pdi.toFirst();(pd=pdi.current());++pdi)
  {
    if (!pd->getGroupDef() && !pd->isReference() && pd->name() == stripPath(name))
    {
      if (!pd->title().isEmpty())
      {
        t << "== " << convertToAsciidoc(pd->title()) << endl;
      }
      else
      {
        t << "== " << convertToAsciidoc(pd->name()) << endl;
      }
      t << endl;
      writeInclude(t, pd->getOutputFileBase());
    }
  }
}

void AsciidocGenerator::startProjectNumber()
{
AD_GEN_C
}

void AsciidocGenerator::endProjectNumber()
{
AD_GEN_C
}

void AsciidocGenerator::writeStyleInfo(int)
{
AD_GEN_C
}

void AsciidocGenerator::writeDoc(DocNode *n,Definition *ctx,MemberDef *)
{
AD_GEN_C
  AsciidocDocVisitor *visitor = new AsciidocDocVisitor(t,*this,ctx);
  n->accept(visitor);
  delete visitor;
}

void AsciidocGenerator::startParagraph(const char *)
{
AD_GEN_C
}

void AsciidocGenerator::endParagraph()
{
AD_GEN_C
  t << endl;
  t << endl;
}
void AsciidocGenerator::writeString(const char *text)
{
AD_GEN_C
  t << text;
}
void AsciidocGenerator::startMemberHeader(const char *name,int)
{
AD_GEN_C
  t << "=== ";
  m_inSimpleSect[m_levelListItem] = TRUE;
}

void AsciidocGenerator::endMemberHeader()
{
AD_GEN_C
  t << endl;
}
void AsciidocGenerator::startMemberSubtitle()
{
AD_GEN_C
  t << endl;
}
void AsciidocGenerator::endMemberSubtitle()
{
AD_GEN_C
  t << endl;
}

void AsciidocGenerator::docify(const char *str)
{
AD_GEN_C
  if (m_pageref) return;
  t << convertToAsciidoc(str);
}
void AsciidocGenerator::writeObjectLink(const char *ref, const char *file,
					const char *anchor, const char *text)
{
AD_GEN_C
  if (anchor)
  {
    t << "<<" << anchor;
    if (file) t << stripPath(file) << "1";
  }
  else
  {
    t << "<<" << stripPath(file);
  }
  t << ',';
  docify(text);
  t << ">>";
}

void AsciidocGenerator::startHtmlLink(const char *)
{
AD_GEN_C
}

void AsciidocGenerator::endHtmlLink()
{
AD_GEN_C
}

void AsciidocGenerator::startMemberList()
{
AD_GEN_C
  m_levelListItem++;
}
void AsciidocGenerator::endMemberList()
{
AD_GEN_C
  m_inListItem[m_levelListItem] = FALSE;
  m_levelListItem = (m_levelListItem> 0 ?  m_levelListItem - 1 : 0);
  m_inSimpleSect[m_levelListItem] = FALSE;
  t << endl;
}

void AsciidocGenerator::startInlineHeader()
{
AD_GEN_C
}
void AsciidocGenerator::endInlineHeader()
{
AD_GEN_C
}
void AsciidocGenerator::startAnonTypeScope(int)
{
AD_GEN_C
}
void AsciidocGenerator::endAnonTypeScope(int)
{
AD_GEN_C
}

void AsciidocGenerator::startMemberItem(const char *,int,const char *)
{
AD_GEN_C
  if (m_inListItem[m_levelListItem]) t << endl;
  m_inListItem[m_levelListItem] = TRUE;
}
void AsciidocGenerator::endMemberItem()
{
AD_GEN_C
    t << " +" << endl;
}
void AsciidocGenerator::startBold(void)
{
AD_GEN_C
  t << "**";
}
void AsciidocGenerator::endBold(void)
{
AD_GEN_C
  t << "**";
}
void AsciidocGenerator::startGroupHeader(int extraIndentLevel)
{
AD_GEN_C2("m_inLevel " << m_inLevel)
AD_GEN_C2("extraIndentLevel " << extraIndentLevel)
  m_firstMember = TRUE; 
  m_inSimpleSect[m_levelListItem] = FALSE;
  if (m_inLevel != -1) m_inGroup = TRUE;
  m_inLevel = extraIndentLevel;
  t << "=== ";
}
void AsciidocGenerator::writeRuler(void)
{
AD_GEN_C2("m_inLevel " << m_inLevel)
AD_GEN_C2("m_inGroup " << m_inGroup)
  if (m_inGroup) t << endl;
  m_inGroup = FALSE;
  // {hruler}
}

void AsciidocGenerator::startDescription()
{
AD_GEN_C;
}
void AsciidocGenerator::endDescription()
{
AD_GEN_C;
}
void AsciidocGenerator::startDescItem()
{
AD_GEN_C;
}
void AsciidocGenerator::startDescForItem()
{
AD_GEN_C;
}
void AsciidocGenerator::endDescForItem()
{
AD_GEN_C;
}
void AsciidocGenerator::endDescItem()
{
AD_GEN_C;
}

void AsciidocGenerator::startCenter()
{
AD_GEN_C;
}
void AsciidocGenerator::endCenter()
{
AD_GEN_C;
}

void AsciidocGenerator::startSmall()
{
AD_GEN_C;
t << "[.small]#";
}
void AsciidocGenerator::endSmall()
{
AD_GEN_C;
t << "#";
}

void AsciidocGenerator::endGroupHeader(int)
{
AD_GEN_C
  t << endl;
  t << endl;
}

void AsciidocGenerator::startMemberSections()
{
AD_GEN_C;
}
void AsciidocGenerator::endMemberSections()
{
AD_GEN_C;
}
void AsciidocGenerator::startHeaderSection()
{
AD_GEN_C;
}
void AsciidocGenerator::endHeaderSection()
{
AD_GEN_C;
}

void AsciidocGenerator::startParameterList(bool openBracket)
{
AD_GEN_C
//    if (openBracket) t << "(";
//  t << "``";
}
void AsciidocGenerator::endParameterList()
{
AD_GEN_C
}
void AsciidocGenerator::writeNonBreakableSpace(int n)
{
AD_GEN_C
  for (int i=0;i<n;i++) t << "{nbsp}";
}
void AsciidocGenerator::lineBreak(const char *)
{
AD_GEN_C
  if (m_denseText) t << " +" << endl;
  else
  {
    t << endl
      << endl;
  }
}

void AsciidocGenerator::startTypewriter()
{
AD_GEN_C
  if (!m_denseText) t << "``";
}
void AsciidocGenerator::endTypewriter()
{
AD_GEN_C
  if (!m_denseText) t << "``";
}

void AsciidocGenerator::startEmphasis()
{
AD_GEN_C
  if (!m_denseText) t << "__";
}
void AsciidocGenerator::endEmphasis()
{
AD_GEN_C
  if (!m_denseText) t << "__";
}

void AsciidocGenerator::startTextBlock(bool dense)
{
AD_GEN_C
  if (dense)
  {
    m_denseText = TRUE;
    t << "``";
  }
}
void AsciidocGenerator::endTextBlock(bool dense)
{
AD_GEN_C
  if (m_denseText)
  {
    m_denseText = FALSE;
    t << "``" << endl
      << endl;
  }
}

void AsciidocGenerator::lastIndexPage()
{
AD_GEN_C
}

void AsciidocGenerator::startMemberDoc(const char *clname, const char *memname,
				       const char *anchor, const char *title,
				       int memCount, int memTotal,
				       bool showInline)
{
AD_GEN_C2("m_inLevel " << m_inLevel)
  t << "==== " << convertToAsciidoc(title) << endl
    << endl;
  if (memTotal>1)
  {
    t << "[.small]#[" << memCount << "/" << memTotal << "]#" << endl;
  }
  if (memname && memname[0]!='@')
  {
    addIndexTerm(t,memname,clname);
    addIndexTerm(t,clname,memname);
  }
  t << "``";
}
void AsciidocGenerator::endMemberDoc(bool)
{
AD_GEN_C
  t << "``" << endl;
}
void AsciidocGenerator::startTitleHead(const char *)
{
AD_GEN_C
  t << "== ";
}
void AsciidocGenerator::endTitleHead(const char *fileName,const char *name)
{
AD_GEN_C
  t << endl;
  if (name) addIndexTerm(t, name);
}

void AsciidocGenerator::startIndexListItem()
{
AD_GEN_C
}

void AsciidocGenerator::endIndexListItem()
{
AD_GEN_C
  t << endl;
}

void AsciidocGenerator::startIndexList()
{
AD_GEN_C
}

void AsciidocGenerator::endIndexList()
{
AD_GEN_C
}

void AsciidocGenerator::startIndexKey()
{
AD_GEN_C
}

void AsciidocGenerator::endIndexKey()
{
AD_GEN_C
  t << ":: ";
}

void AsciidocGenerator::startIndexValue(bool have)
{
AD_GEN_C
  if (!have) t << "[.small]#N/A#" << endl;
}

void AsciidocGenerator::endIndexValue(const char *,bool)
{
AD_GEN_C
  t << endl;
}

void AsciidocGenerator::startItemList()
{
AD_GEN_C
}

void AsciidocGenerator::endItemList()
{
AD_GEN_C
}

void AsciidocGenerator::startIndexItem(const char *,const char *)
{
AD_GEN_C
}

void AsciidocGenerator::endIndexItem(const char *,const char *)
{
AD_GEN_C
}

void AsciidocGenerator::startItemListItem()
{
AD_GEN_C
}

void AsciidocGenerator::endItemListItem()
{
AD_GEN_C
}

void AsciidocGenerator::startDoxyAnchor(const char *fName,const char *manName,
                                 const char *anchor,const char *name,
                                 const char *args)
{
AD_GEN_C
  if (!m_inListItem[m_levelListItem] && !m_descTable)
  {
    m_firstMember = FALSE;
  }
  if (anchor)
  {
      t << "[[" << anchor << stripPath(fName) << "1" << "]]" << endl;
  }
}
void AsciidocGenerator::endDoxyAnchor(const char *fileName,const char *anchor)
{
AD_GEN_C
    t << endl; // Needed
}

void AsciidocGenerator::writeStartAnnoItem(const char *,const char *,
					   const char *,const char *)
{
AD_GEN_C
}
void AsciidocGenerator::writeEndAnnoItem(const char *name)
{
AD_GEN_C
}

void AsciidocGenerator::startMemberDescription(const char *,const char *, bool)
{
AD_GEN_C
}
void AsciidocGenerator::endMemberDescription()
{
AD_GEN_C
}
void AsciidocGenerator::writeInheritedSectionTitle(const char *id,
						   const char *ref,
						   const char *file,
						   const char *anchor,
						   const char *title,
						   const char *name)
{
AD_GEN_C
}
void AsciidocGenerator::startMemberDeclaration()
{
AD_GEN_C
}
void AsciidocGenerator::endMemberDeclaration(const char *,const char *)
{
AD_GEN_C
}


void AsciidocGenerator::startMemberDocName(bool)
{
AD_GEN_C
}
void AsciidocGenerator::endMemberDocName()
{
AD_GEN_C
}

void AsciidocGenerator::startParameterType(bool,const char *)
{
AD_GEN_C
}
void AsciidocGenerator::endParameterType()
{
AD_GEN_C
}


void AsciidocGenerator::startMemberGroupHeader(bool hasHeader)
{
AD_GEN_C
  t << "==== ";
}
void AsciidocGenerator::endMemberGroupHeader()
{
AD_GEN_C
}
void AsciidocGenerator::startMemberGroupDocs()
{
AD_GEN_C
}
void AsciidocGenerator::endMemberGroupDocs()
{
AD_GEN_C
}
void AsciidocGenerator::startMemberGroup()
{
AD_GEN_C
}
void AsciidocGenerator::endMemberGroup(bool)
{
AD_GEN_C
  t << endl;
}
void AsciidocGenerator::insertMemberAlign(bool)
{
AD_GEN_C
}
void AsciidocGenerator::insertMemberAlignLeft(int, bool)
{
AD_GEN_C
}

void AsciidocGenerator::startIndent()
{
AD_GEN_C
}
void AsciidocGenerator::endIndent()
{
AD_GEN_C
}
void AsciidocGenerator::writeSynopsis()
{
AD_GEN_C
}
void AsciidocGenerator::startClassDiagram()
{
AD_GEN_C
}

void AsciidocGenerator::endClassDiagram(const ClassDiagram &d, const char *fileName,const char *)
{
AD_GEN_C
  visitADPreStart(t, FALSE, relPath + fileName + ".png", NULL, NULL);
  d.writeImage(t,dir,relPath,fileName,FALSE);
  visitADPostEnd(t, FALSE);
  t << endl;
}
void  AsciidocGenerator::startLabels()
{
AD_GEN_C
}

void  AsciidocGenerator::writeLabel(const char *l,bool isLast)
{
AD_GEN_C
  t << " [.small]#" << l << "#";
  if (!isLast) t << ",";
}

void  AsciidocGenerator::endLabels()
{
AD_GEN_C
}
void AsciidocGenerator::setCurrentDoc(Definition *,const char *,bool)
{
AD_GEN_C
}
void AsciidocGenerator::addWord(const char *,bool)
{
AD_GEN_C
}

void AsciidocGenerator::startExamples()
{
AD_GEN_C
  t << "=== ";
  docify(theTranslator->trExamples());
}

void AsciidocGenerator::endExamples()
{
AD_GEN_C
  t << endl;
}

void AsciidocGenerator::startParamList(BaseOutputDocInterface::ParamListTypes,
				       const char *)
{
AD_GEN_C
}
void AsciidocGenerator::endParamList()
{
AD_GEN_C
}
void AsciidocGenerator::startTitle()
{
AD_GEN_C
}
void AsciidocGenerator::endTitle()
{
AD_GEN_C
}

void AsciidocGenerator::startSubsection(void)
{
AD_GEN_C
  t << "== ";
}
void AsciidocGenerator::endSubsection(void)
{
AD_GEN_C
  t << endl;
}
void AsciidocGenerator::writeAnchor(const char *,const char *)
{
AD_GEN_C
  t << endl;
}

void AsciidocGenerator::startSubsubsection(void)
{
AD_GEN_C
  t << "=== ";
}
void AsciidocGenerator::endSubsubsection(void)
{
AD_GEN_C
  t << endl;
}

void AsciidocGenerator::writeChar(char c)
{
AD_GEN_C
  char cs[2];
  cs[0]=c;
  cs[1]=0;
  docify(cs);
}
void AsciidocGenerator::startMemberDocPrefixItem()
{
AD_GEN_C
}
void AsciidocGenerator::endMemberDocPrefixItem()
{
AD_GEN_C
  t << endl;
}
void AsciidocGenerator::exceptionEntry(const char* prefix,bool closeBracket)
{
AD_GEN_C
  if (prefix)
    t << " " << prefix << "(";
  else if (closeBracket)
    t << ")";
  t << " ";
}
void AsciidocGenerator::startParameterName(bool)
{
AD_GEN_C
}
void AsciidocGenerator::endParameterName(bool last,bool /*emptyList*/,bool closeBracket)
{
AD_GEN_C
  if (last)
  {
//    if (closeBracket) t << ")";
  }
}
void AsciidocGenerator::startCodeFragment(SrcLangExt lang)
{
AD_GEN_C
  static const char *langnames[] = {
    "", "idl", "java", "c#", "d", "php", "objc", "c++", "js", "python",
    "fortran", "vhdl", "xml", "tcl", "markdown", "sql"
  };

  t << lang << endl;
  t << endl << "[source,c++,linenums,+macros]" << endl;
  t << "----" << endl;
}
void AsciidocGenerator::endCodeFragment()
{
AD_GEN_C
    t << "----" << endl;
}
void AsciidocGenerator::startMemberTemplateParams()
{
AD_GEN_C
}

void AsciidocGenerator::endMemberTemplateParams(const char *,const char *)
{
AD_GEN_C
  t << endl;
}
void AsciidocGenerator::startSection(const char *lab,const char *,SectionInfo::SectionType type)
{
AD_GEN_C
  t << "[[" << stripPath(lab) << "]] ";
}
void AsciidocGenerator::endSection(const char *lab,SectionInfo::SectionType)
{
AD_GEN_C
}
void AsciidocGenerator::addIndexItem(const char *prim,const char *sec)
{
AD_GEN_C
  addIndexTerm(t, prim, sec);
}

void AsciidocGenerator::startDescTable(const char *title)
{
AD_GEN_C
  // if (title) t << "." << convertToAsciidoc(title) << endl;
  t << "[cols=\"1,5\"]" << endl;
  t << "|===" << endl;
  m_descTable = TRUE;
}

void AsciidocGenerator::endDescTable()
{
AD_GEN_C
  t << "|===" << endl;
  m_descTable = FALSE;
}

void AsciidocGenerator::startDescTableRow()
{
AD_GEN_C
  t << "|";
}

void AsciidocGenerator::endDescTableRow()
{
AD_GEN_C
}

void AsciidocGenerator::startDescTableTitle()
{
AD_GEN_C
}

void AsciidocGenerator::endDescTableTitle()
{
AD_GEN_C
  t << endl;
}

void AsciidocGenerator::startDescTableData()
{
AD_GEN_C
  t << "| ";
}
void AsciidocGenerator::endDescTableData()
{
AD_GEN_C
  t << endl; // Needed
}

void AsciidocGenerator::startTextLink(const char *,const char *)
{
AD_GEN_C
}
void AsciidocGenerator::endTextLink()
{
AD_GEN_C
}


void AsciidocGenerator::startPageRef()
{
AD_GEN_C
    m_pageref = TRUE;
}

void AsciidocGenerator::endPageRef(const char *,const char *)
{
AD_GEN_C
    m_pageref = FALSE;
}

void AsciidocGenerator::startGroupCollaboration()
{
AD_GEN_C
}
void AsciidocGenerator::endGroupCollaboration(const DotGroupCollaboration &g)
{
AD_GEN_C
  t << endl;
  g.writeGraph(t,GOF_BITMAP,EOF_Asciidoc,Config_getString(ASCIIDOC_OUTPUT),fileName,relPath,FALSE);
  t << endl;
}
void AsciidocGenerator::startDotGraph()
{
AD_GEN_C
}
void AsciidocGenerator::endDotGraph(const DotClassGraph &g)
{
AD_GEN_C
  t << endl;
  g.writeGraph(t,GOF_BITMAP,EOF_Asciidoc,Config_getString(ASCIIDOC_OUTPUT),fileName,relPath,TRUE,FALSE);
  t << endl;
}
void AsciidocGenerator::startInclDepGraph()
{
AD_GEN_C
}
void AsciidocGenerator::endInclDepGraph(const DotInclDepGraph &g)
{
AD_GEN_C
  t << endl;
  g.writeGraph(t,GOF_BITMAP,EOF_Asciidoc,Config_getString(ASCIIDOC_OUTPUT), fileName,relPath,FALSE);
  t << endl;
}
void AsciidocGenerator::startCallGraph()
{
AD_GEN_C
  t << endl;
}
void AsciidocGenerator::endCallGraph(const DotCallGraph &g)
{
AD_GEN_C
  t << endl;
  g.writeGraph(t,GOF_BITMAP,EOF_Asciidoc,Config_getString(ASCIIDOC_OUTPUT), fileName,relPath,FALSE);
  t << endl;
}
void AsciidocGenerator::startDirDepGraph()
{
AD_GEN_C
}
void AsciidocGenerator::endDirDepGraph(const DotDirDeps &g)
{
AD_GEN_C
  t << endl;
  g.writeGraph(t,GOF_BITMAP,EOF_Asciidoc,Config_getString(ASCIIDOC_OUTPUT), fileName,relPath,FALSE);
  t << endl;
}
void AsciidocGenerator::writeGraphicalHierarchy(const DotGfxHierarchyTable &)
{
AD_GEN_C
}

void AsciidocGenerator::startQuickIndices()
{
AD_GEN_C
}
void AsciidocGenerator::endQuickIndices()
{
AD_GEN_C
}

void AsciidocGenerator::writeSplitBar(const char *)
{
AD_GEN_C
  //t << "{hruler}" << endl;
}
void AsciidocGenerator::writeNavigationPath(const char *)
{
AD_GEN_C
}
void AsciidocGenerator::writeLogo()
{
AD_GEN_C
}
void AsciidocGenerator::writeQuickLinks(bool,HighlightedItem,const char *)
{
AD_GEN_C
}
void AsciidocGenerator::writeSummaryLink(const char *,const char *,
					 const char *,bool)
{
AD_GEN_C
}
void AsciidocGenerator::startContents()
{
AD_GEN_C
}
void AsciidocGenerator::endContents()
{
AD_GEN_C
}
void AsciidocGenerator::startPageDoc(const char *)
{
AD_GEN_C
}
void AsciidocGenerator::endPageDoc()
{
AD_GEN_C
}

void AsciidocGenerator::startMemberDocList()
{
AD_GEN_C
}
void AsciidocGenerator::endMemberDocList()
{
AD_GEN_C
  m_inGroup = TRUE;
}
void AsciidocGenerator::startConstraintList(const char *header)
{
AD_GEN_C
  t << "==== ";
  docify(header);
  t << endl;
  t << endl;
}
void AsciidocGenerator::startConstraintParam()
{
AD_GEN_C
  t << "NOTE: ";
}
void AsciidocGenerator::endConstraintParam()
{
AD_GEN_C
}
void AsciidocGenerator::startConstraintType()
{
AD_GEN_C
  t << ":";
}
void AsciidocGenerator::endConstraintType()
{
AD_GEN_C
  t << "__" << endl;
}
void AsciidocGenerator::startConstraintDocs()
{
AD_GEN_C
}
void AsciidocGenerator::endConstraintDocs()
{
AD_GEN_C
}
void AsciidocGenerator::endConstraintList()
{
AD_GEN_C
}

void AsciidocGenerator::startMemberDocSimple(bool)
{
AD_GEN_C
}
void AsciidocGenerator::endMemberDocSimple(bool)
{
AD_GEN_C
}
void AsciidocGenerator::startInlineMemberType()
{
AD_GEN_C
}
void AsciidocGenerator::endInlineMemberType()
{
AD_GEN_C
}
void AsciidocGenerator::startInlineMemberName()
{
AD_GEN_C
}
void AsciidocGenerator::endInlineMemberName()
{
AD_GEN_C
}
void AsciidocGenerator::startInlineMemberDoc()
{
AD_GEN_C
}
void AsciidocGenerator::endInlineMemberDoc()
{
AD_GEN_C
}
