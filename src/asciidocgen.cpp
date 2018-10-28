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

// no debug info
#define Asciidoc_DB(x) do {} while(0)
// debug to stdout
//#define Asciidoc_DB(x) printf x
// debug inside output
//#define Asciidoc_DB(x) QCString __t;__t.sprintf x;m_t << __t

#if 0
#define AD_GEN_C AD_GEN_C1(t)
#define AD_GEN_C1(x) x << "# AD_GEN_C " << __LINE__ << "\n";
#define AD_GEN_C2(y) AD_GEN_C2a(t,y)
#define AD_GEN_C2a(x,y) x << "# AD_GEN_C " << __LINE__ << " " << y << "\n";
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

inline void writeAsciidocCodeString(FTextStream &t,const char *s, int &col)
{
  char c;
  while ((c=*s++))
  {
    switch(c)
    {
      case '\t':
        {
          static int tabSize = Config_getInt(TAB_SIZE);
          int spacesToNextTabStop = tabSize - (col%tabSize);
          col+=spacesToNextTabStop;
          while (spacesToNextTabStop--) t << "&#32;";
          break;
        }
      case '\007':  t << "^G"; col++; break; // bell
      case '\014':  t << "^L"; col++; break; // form feed
      default:   t << c; col++;        break;
    }
  }
}

static void addIndexTerm(FTextStream &t, QCString prim, QCString sec = "")
{
  // No indexterm support in asciidoc yet
}

void writeAsciidocLink(FTextStream &t,const char * /*extRef*/,const char *compoundId,
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
  Asciidoc_DB(("(codify \"%s\")\n",text));
  writeAsciidocCodeString(m_t,text,m_col);
}
void AsciidocCodeGenerator::writeCodeLink(const char *ref,const char *file,
    const char *anchor,const char *name,
    const char *tooltip)
{
  Asciidoc_DB(("(writeCodeLink)\n"));
  writeAsciidocLink(m_t,ref,file,anchor,name,tooltip);
  m_col+=strlen(name);
}
void AsciidocCodeGenerator::writeCodeLinkLine(const char *ref,const char *file,
    const char *anchor,const char *name,
    const char *tooltip)
{
  Asciidoc_DB(("(writeCodeLinkLine)\n"));
  m_t << "<anchor xml:id=\"_" << stripExtensionGeneral(stripPath(file),".xml");
  m_t << "_1l";
  writeAsciidocString(m_t,name);
  m_t << "\"/>";
  m_col+=strlen(name);
}
void AsciidocCodeGenerator::writeTooltip(const char *, const DocLinkInfo &, const char *,
                  const char *, const SourceLinkInfo &, const SourceLinkInfo &
                 )
{
  Asciidoc_DB(("(writeToolTip)\n"));
}
void AsciidocCodeGenerator::startCodeLine(bool)
{
  Asciidoc_DB(("(startCodeLine)\n"));
  m_insideCodeLine=TRUE;
  m_col=0;
}
void AsciidocCodeGenerator::endCodeLine()
{
  m_t << endl;
  Asciidoc_DB(("(endCodeLine)\n"));
  m_lineNumber = -1;
  m_refId.resize(0);
  m_external.resize(0);
  m_insideCodeLine=FALSE;
}
void AsciidocCodeGenerator::startFontClass(const char *colorClass)
{
  Asciidoc_DB(("(startFontClass)\n"));
  m_t << "<emphasis role=\"" << colorClass << "\">";
  m_insideSpecialHL=TRUE;
}
void AsciidocCodeGenerator::endFontClass()
{
  Asciidoc_DB(("(endFontClass)\n"));
  m_t << "</emphasis>"; // non Asciidoc
  m_insideSpecialHL=FALSE;
}
void AsciidocCodeGenerator::writeCodeAnchor(const char *)
{
  Asciidoc_DB(("(writeCodeAnchor)\n"));
}
void AsciidocCodeGenerator::writeLineNumber(const char *ref,const char *fileName,
    const char *anchor,int l)
{
  Asciidoc_DB(("(writeLineNumber)\n"));
  m_insideCodeLine = TRUE;
  if (m_prettyCode)
  {
    QCString lineNumber;
    lineNumber.sprintf("%05d",l);

    if (fileName && !m_sourceFileName.isEmpty())
    {
      writeCodeLinkLine(ref,m_sourceFileName,anchor,lineNumber,0);
      writeCodeLink(ref,fileName,anchor,lineNumber,0);
    }
    else
    {
      codify(lineNumber);
    }
    m_t << " ";
  }
  else
  {
    m_t << l << " ";
  }

}
void AsciidocCodeGenerator::setCurrentDoc(Definition *,const char *,bool)
{
}
void AsciidocCodeGenerator::addWord(const char *,bool)
{
}
void AsciidocCodeGenerator::finish()
{
  if (m_insideCodeLine) endCodeLine();
}
void AsciidocCodeGenerator::startCodeFragment()
{
  m_t << "[source]" << endl;
  m_t << "----" << endl;
}
void AsciidocCodeGenerator::endCodeFragment()
{
  m_t << "----" << endl;
  m_t << endl;
}

void writeAsciidocCodeBlock(FTextStream &t,FileDef *fd)
{
  ParserInterface *pIntf=Doxygen::parserManager->getParser(fd->getDefFileExtension());
  SrcLangExt langExt = getLanguageFromFileName(fd->getDefFileExtension());
  pIntf->resetCodeParserState();
  AsciidocCodeGenerator *asciidocGen = new AsciidocCodeGenerator(t);
  pIntf->parseCode(*asciidocGen,  // codeOutIntf
      0,           // scopeName
      fileToString(fd->absFilePath(),Config_getBool(FILTER_SOURCE_FILES)),
      langExt,     // lang
      FALSE,       // isExampleBlock
      0,           // exampleName
      fd,          // fileDef
      -1,          // startLine
      -1,          // endLine
      FALSE,       // inlineFragement
      0,           // memberDef
      TRUE         // showLineNumbers
      );
  asciidocGen->finish();
  delete asciidocGen;
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

  if (!pageName.isEmpty()) t << "[[_" <<  stripPath(pageName) << "]]" << endl;
}

void AsciidocGenerator::endFile()
{
AD_GEN_C
  endPlainFile();
}

void AsciidocGenerator::startIndexSection(IndexSections is)
{
AD_GEN_C2("IndexSections " << is)
  switch (is)
  {
    case isTitlePageStart:
      {
        QCString dbk_projectName = Config_getString(PROJECT_NAME);
        t << "<info>" << endl;
        t << "== " << convertToAsciidoc(dbk_projectName) << endl;
	t << endl;
        t << "    </info>" << endl;
      }
      break;
    case isTitlePageAuthor:
      break;
    case isMainPage:
      t << "== ";
      break;
    case isModuleIndex:
      //Module Index}\n"
      break;
    case isDirIndex:
      //Directory Index}\n"
      break;
    case isNamespaceIndex:
      //Namespace Index}\n"
      break;
    case isClassHierarchyIndex:
      //Hierarchical Index}\n"
      break;
    case isCompoundIndex:
      //t << "{"; //Class Index}\n"
      break;
    case isFileIndex:
      //Annotated File Index}\n"
      break;
    case isPageIndex:
      //Annotated Page Index}\n"
      break;
    case isModuleDocumentation:
      t << "== ";
      break;
    case isDirDocumentation:
      t << "== ";
      break;
    case isNamespaceDocumentation:
      t << "== ";
      break;
    case isClassDocumentation:
      t << "== ";
      break;
    case isFileDocumentation:
      t << "== ";
      break;
    case isExampleDocumentation:
      t << "== ";
      break;
    case isPageDocumentation:
      break;
    case isPageDocumentation2:
      break;
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
      //t << "<xi:include href=\"dirs.xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>";
      //t << "</chapter>" << endl;
      break;
    case isNamespaceIndex:
      //t << "<xi:include href=\"namespaces.xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>";
      //t << "</chapter>" << endl;
      break;
    case isClassHierarchyIndex:
      //t << "<xi:include href=\"hierarchy.xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>";
      //t << "</chapter>" << endl;
      break;
    case isCompoundIndex:
      //t << "</chapter>" << endl;
      break;
    case isFileIndex:
      //t << "<xi:include href=\"files.xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>";
      //t << "</chapter>" << endl;
      break;
    case isPageIndex:
      //t << "<xi:include href=\"pages.xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>";
      //t << "</chapter>" << endl;
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
            t << "<xi:include href=\"" << gd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
            found=TRUE;
          }
        }
        for (;(gd=gli.current());++gli)
        {
          if (!gd->isReference())
          {
            t << "<xi:include href=\"" << gd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
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
            t << "<    xi:include href=\"" << dd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
            found=TRUE;
          }
        }
        for (;(dd=dli.current());++dli)
        {
          if (dd->isLinkableInProject())
          {
            t << "<xi:include href=\"" << dd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
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
            t << "<xi:include href=\"" << nd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
            found=TRUE;
          }
        }
        while ((nd=nli.current()))
        {
          if (nd->isLinkableInProject())
          {
            t << "<xi:include href=\"" << nd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
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
            t << "<xi:include href=\"" << cd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
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
            t << "<xi:include href=\"" << cd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
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
                t << "<xi:include href=\"" << fd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
                if (sourceBrowser && m_prettyCode && fd->generateSourceFile())
                {
                  t << "<xi:include href=\"" << fd->getSourceFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
                }
                isFirst=FALSE;
              }
              else
              {
                t << "<xi:include href=\"" << fd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
                if (sourceBrowser && m_prettyCode && fd->generateSourceFile())
                {
                  t << "<xi:include href=\"" << fd->getSourceFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
                }
              }
            }
          }
        }
      }
      t << "</chapter>\n";
      break;
    case isExampleDocumentation:
      {
        PageSDict::Iterator pdi(*Doxygen::exampleSDict);
        PageDef *pd=pdi.toFirst();
        if (pd)
        {
          t << "<xi:include href=\"" << pd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
        }
        for (++pdi;(pd=pdi.current());++pdi)
        {
          t << "<xi:include href=\"" << pd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
        }
      }
      t << "</chapter>\n";
      break;
    case isPageDocumentation:
      break;
    case isPageDocumentation2:
      break;
    case isEndIndex:
      t << "<index/>" << endl;
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
      t << "<xi:include href=\"" << pd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
    }
  }
}
void AsciidocGenerator::writeDoc(DocNode *n,Definition *ctx,MemberDef *)
{
AD_GEN_C
  AsciidocDocVisitor *visitor =
    new AsciidocDocVisitor(t,*this);
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
void AsciidocGenerator::docify(const char *str)
{
AD_GEN_C
  t << convertToAsciidoc(str);
}
void AsciidocGenerator::writeObjectLink(const char *ref, const char *f,
					const char *anchor, const char *text)
{
AD_GEN_C
  if (anchor)
  {
    t << "<<_" << anchor;
    if (f) t << stripPath(f) << "_1";
  }
  else
      t << " <<_" << stripPath(f);
  t << ',';
  docify(text);
  t << ">>";
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
void AsciidocGenerator::startMemberItem(const char *,int,const char *)
{
AD_GEN_C
  if (m_inListItem[m_levelListItem]) t << endl;
  m_inListItem[m_levelListItem] = TRUE;
}
void AsciidocGenerator::endMemberItem()
{
AD_GEN_C
  t << endl;
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
}

void AsciidocGenerator::endGroupHeader(int)
{
AD_GEN_C
  t << endl;
  t << endl;
}

void AsciidocGenerator::startParameterList(bool openBracket)
{
AD_GEN_C
  if (openBracket) t << "(";
}
void AsciidocGenerator::endParameterList()
{
AD_GEN_C
  t << "``";
}
void AsciidocGenerator::writeNonBreakableSpace(int n)
{
AD_GEN_C
  for (int i=0;i<n;i++) t << "{nbsp}";
}
void AsciidocGenerator::lineBreak(const char *)
{
AD_GEN_C
  t << endl;
}
void AsciidocGenerator::startTypewriter()
{
AD_GEN_C
  if (!m_denseText) t << "``";
}
void AsciidocGenerator::endTypewriter()
{
AD_GEN_C
  if (!m_denseText) t << "``" << endl;
}
void AsciidocGenerator::startTextBlock(bool dense)
{
AD_GEN_C
  if (dense)
  {
    m_denseText = TRUE;
    t << "<programlisting>";
  }
}
void AsciidocGenerator::endTextBlock(bool dense)
{
AD_GEN_C
  if (m_denseText)
  {
    m_denseText = FALSE;
    t << "</programlisting>";
  }
}
void AsciidocGenerator::startMemberDoc(const char *clname, const char *memname, const char *anchor, const char *title,
                                      int memCount, int memTotal, bool showInline)
{
AD_GEN_C2("m_inLevel " << m_inLevel)
  t << "==== " << convertToAsciidoc(title) << endl;
  if (memTotal>1)
  {
    t << "[source]" << endl;
    t << "----" << endl;
    t << memCount << "/" << memTotal << "]";
    t << "----" << endl;
  }
  t << endl;
  if (memname && memname[0]!='@')
  {
    addIndexTerm(t,memname,clname);
    addIndexTerm(t,clname,memname);
  }
}
void AsciidocGenerator::endMemberDoc(bool)
{
AD_GEN_C
  t << endl;
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
      t << "[[_" << stripPath(fName) << "_1" << anchor << "]]" << endl;
  }
}
void AsciidocGenerator::endDoxyAnchor(const char *fileName,const char *anchor)
{
AD_GEN_C
}
void AsciidocGenerator::startMemberDocName(bool)
{
AD_GEN_C
  t << "``";
}
void AsciidocGenerator::endMemberDocName()
{
AD_GEN_C
}
void AsciidocGenerator::startMemberGroupHeader(bool hasHeader)
{
AD_GEN_C
}
void AsciidocGenerator::endMemberGroupHeader()
{
AD_GEN_C
}
void AsciidocGenerator::startMemberGroup()
{
AD_GEN_C
    t << "STARTMEMBER\n";
}
void AsciidocGenerator::endMemberGroup(bool)
{
AD_GEN_C
  t << "ENDMEMBER";
  t << endl;
  t << endl;
}
void AsciidocGenerator::startClassDiagram()
{
AD_GEN_C
  t << "STARTCLASSDIAGRAM" << endl;
}

void AsciidocGenerator::endClassDiagram(const ClassDiagram &d, const char *fileName,const char *)
{
AD_GEN_C
    t << "ENDCLASSDIAGRAM\n";
  visitADPreStart(t, FALSE, relPath + fileName + ".png", NULL, NULL);
  d.writeImage(t,dir,relPath,fileName,FALSE);
  visitADPostEnd(t, FALSE);
  t << endl;
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
  if (!isLast) t << ", ";
}

void  AsciidocGenerator::endLabels()
{
AD_GEN_C
  t << endl;
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
  t << "[source]" << endl;
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
  t << " ";
}
void AsciidocGenerator::endParameterName(bool last,bool /*emptyList*/,bool closeBracket)
{
AD_GEN_C
  if (last)
  {
    if (closeBracket) t << ")";
  }
}
void AsciidocGenerator::startCodeFragment()
{
AD_GEN_C
  t << "[source]" << endl;
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
  t << "[[_" << stripPath(lab) << "]] ";
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
  int ncols = 2;
  t << "[cols=\"";
  for (int i = 0; i < ncols; i++)
  {
    t << "<colspec colname='c" << i+1 << "'/>\n";
  }
  t << "\"]" << endl;
  //if (title)t << "=== " << convertToAsciidoc(title) << endl;
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
  t << "</entry><entry>";
}

void AsciidocGenerator::endDescTableData()
{
AD_GEN_C
  t << "</entry>";
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
