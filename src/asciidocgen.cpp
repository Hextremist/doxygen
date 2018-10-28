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
#define DB_GEN_C DB_GEN_C1(t)
#define DB_GEN_C1(x) x << "<!-- DB_GEN_C " << __LINE__ << " -->\n";
#define DB_GEN_C2(y) DB_GEN_C2a(t,y)
#define DB_GEN_C2a(x,y) x << "<!-- DB_GEN_C " << __LINE__ << " " << y << " -->\n";
#else
#define DB_GEN_C
#define DB_GEN_C1(x)
#define DB_GEN_C2(y)
#define DB_GEN_C2a(x,y)
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
  t << "<<1" << stripPath(compoundId);
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
  m_t << "[source1]" << endl;
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
DB_GEN_C
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
DB_GEN_C
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
DB_GEN_C
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

  if (!pageName.isEmpty()) t << "12[[_" <<  stripPath(pageName) << "]]" << endl;
}

void AsciidocGenerator::endFile()
{
DB_GEN_C
  m_inDetail = FALSE;
  while (m_inLevel != -1)
  {
    m_inLevel--;
  }
  m_inGroup = FALSE;

  QCString fileType="section";
  QCString fileName= m_codeGen.sourceFileName();
  if (fileName == "index.xml")
  {
    fileType="book";
  }
  else if (fileName == "mainpage.xml")
  {
    fileType="chapter";
  }
  endPlainFile();
  m_codeGen.setSourceFileName("");
}

void AsciidocGenerator::startIndexSection(IndexSections is)
{
DB_GEN_C2("IndexSections " << is)
  switch (is)
  {
    case isTitlePageStart:
      {
        QCString dbk_projectName = Config_getString(PROJECT_NAME);
        t << "<info>" << endl;
        t << "== 37" << convertToAsciidoc(dbk_projectName) << endl;
	t << endl;
        t << "    </info>" << endl;
      }
      break;
    case isTitlePageAuthor:
      break;
    case isMainPage:
      t << "== 38";
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
      t << "== 39";
      break;
    case isDirDocumentation:
      t << "== 40";
      break;
    case isNamespaceDocumentation:
      t << "== 41";
      break;
    case isClassDocumentation:
      t << "== 42";
      break;
    case isFileDocumentation:
      t << "== 43";
      break;
    case isExampleDocumentation:
      t << "== 44";
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
DB_GEN_C2("IndexSections " << is)
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
DB_GEN_C
  PageSDict::Iterator pdi(*Doxygen::pageSDict);
  PageDef *pd = pdi.toFirst();
  for (pd = pdi.toFirst();(pd=pdi.current());++pdi)
  {
    if (!pd->getGroupDef() && !pd->isReference() && pd->name() == stripPath(name))
    {
      if (!pd->title().isEmpty())
      {
        t << "== 45" << convertToAsciidoc(pd->title()) << endl;
      }
      else
      {
        t << "== 46" << convertToAsciidoc(pd->name()) << endl;
      }
      t << endl;
      t << "<xi:include href=\"" << pd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
    }
  }
}
void AsciidocGenerator::writeDoc(DocNode *n,Definition *ctx,MemberDef *)
{
DB_GEN_C
  AsciidocDocVisitor *visitor =
    new AsciidocDocVisitor(t,*this);
  n->accept(visitor);
  delete visitor;
}

void AsciidocGenerator::startParagraph(const char *)
{
DB_GEN_C
  t << endl;
}

void AsciidocGenerator::endParagraph()
{
DB_GEN_C
  t << endl;
}
void AsciidocGenerator::writeString(const char *text)
{
DB_GEN_C
  t << text;
}
void AsciidocGenerator::startMemberHeader(const char *name,int)
{
DB_GEN_C
  t << "=== 48";
  m_inSimpleSect[m_levelListItem] = TRUE;
}

void AsciidocGenerator::endMemberHeader()
{
DB_GEN_C
  t << endl;
  t << endl;
}
void AsciidocGenerator::docify(const char *str)
{
DB_GEN_C
  t << convertToAsciidoc(str);
}
void AsciidocGenerator::writeObjectLink(const char *ref, const char *f,
					const char *anchor, const char *text)
{
DB_GEN_C
  if (anchor)
  {
    t << "<<2_" << anchor;
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
DB_GEN_C
  m_levelListItem++;
}
void AsciidocGenerator::endMemberList()
{
DB_GEN_C
  m_inListItem[m_levelListItem] = FALSE;
  m_levelListItem = (m_levelListItem> 0 ?  m_levelListItem - 1 : 0);
  m_inSimpleSect[m_levelListItem] = FALSE;
  t << endl;
}
void AsciidocGenerator::startMemberItem(const char *,int,const char *)
{
DB_GEN_C
  if (m_inListItem[m_levelListItem]) t << endl;
  m_inListItem[m_levelListItem] = TRUE;
}
void AsciidocGenerator::endMemberItem()
{
DB_GEN_C
  t << endl;
}
void AsciidocGenerator::startBold(void)
{
DB_GEN_C
  t << "**";
}
void AsciidocGenerator::endBold(void)
{
DB_GEN_C
  t << "**";
}
void AsciidocGenerator::startGroupHeader(int extraIndentLevel)
{
DB_GEN_C2("m_inLevel " << m_inLevel)
DB_GEN_C2("extraIndentLevel " << extraIndentLevel)
  m_firstMember = TRUE; 
  m_inSimpleSect[m_levelListItem] = FALSE;
  if (m_inLevel != -1) m_inGroup = TRUE;
  m_inLevel = extraIndentLevel;
  t << "=== 49";
}
void AsciidocGenerator::writeRuler(void)
{
DB_GEN_C2("m_inLevel " << m_inLevel)
DB_GEN_C2("m_inGroup " << m_inGroup)
  if (m_inGroup) t << endl;
  m_inGroup = FALSE;
}

void AsciidocGenerator::endGroupHeader(int)
{
DB_GEN_C
  t << endl;
  t << endl;
}

void AsciidocGenerator::startParameterList(bool openBracket)
{
DB_GEN_C
  if (openBracket) t << "(";
}
void AsciidocGenerator::endParameterList()
{
DB_GEN_C
}
void AsciidocGenerator::writeNonBreakableSpace(int n)
{
DB_GEN_C
  for (int i=0;i<n;i++) t << " ";
}
void AsciidocGenerator::lineBreak(const char *)
{
DB_GEN_C
  t << endl;
}
void AsciidocGenerator::startTypewriter()
{
DB_GEN_C
  if (!m_denseText) t << "``";
}
void AsciidocGenerator::endTypewriter()
{
DB_GEN_C
  if (!m_denseText) t << "``" << endl;
}
void AsciidocGenerator::startTextBlock(bool dense)
{
DB_GEN_C
  if (dense)
  {
    m_denseText = TRUE;
    t << "<programlisting>";
  }
}
void AsciidocGenerator::endTextBlock(bool dense)
{
DB_GEN_C
  if (m_denseText)
  {
    m_denseText = FALSE;
    t << "</programlisting>";
  }
}
void AsciidocGenerator::startMemberDoc(const char *clname, const char *memname, const char *anchor, const char *title,
                                      int memCount, int memTotal, bool showInline)
{
DB_GEN_C2("m_inLevel " << m_inLevel)
  t << "==== 50" << convertToAsciidoc(title) << endl;
  if (memTotal>1)
  {
    t << "[source2]" << endl;
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
DB_GEN_C
  t << endl;
}
void AsciidocGenerator::startTitleHead(const char *)
{
DB_GEN_C
  t << "== 51";
}
void AsciidocGenerator::endTitleHead(const char *fileName,const char *name)
{
DB_GEN_C
  t << endl;
  if (name) addIndexTerm(t, name);
}
void AsciidocGenerator::startDoxyAnchor(const char *fName,const char *manName,
                                 const char *anchor,const char *name,
                                 const char *args)
{
DB_GEN_C
  if (!m_inListItem[m_levelListItem] && !m_descTable)
  {
    m_firstMember = FALSE;
  }
  if (anchor)
  {
      t << "13[[_" << stripPath(fName) << "_1" << anchor << "]]" << endl;
  }
}
void AsciidocGenerator::endDoxyAnchor(const char *fileName,const char *anchor)
{
DB_GEN_C
}
void AsciidocGenerator::startMemberDocName(bool)
{
DB_GEN_C
  t << "[source3]" << endl;
  t << "----" << endl;
}
void AsciidocGenerator::endMemberDocName()
{
DB_GEN_C
  t << "----" << endl;
}
void AsciidocGenerator::startMemberGroupHeader(bool hasHeader)
{
DB_GEN_C
}
void AsciidocGenerator::endMemberGroupHeader()
{
DB_GEN_C
}
void AsciidocGenerator::startMemberGroup()
{
DB_GEN_C
}
void AsciidocGenerator::endMemberGroup(bool)
{
DB_GEN_C
  t << "</simplesect>" << endl;
}
void AsciidocGenerator::startClassDiagram()
{
DB_GEN_C
}

void AsciidocGenerator::endClassDiagram(const ClassDiagram &d, const char *fileName,const char *)
{
DB_GEN_C
  visitADPreStart(t, FALSE, relPath + fileName + ".png", NULL, NULL);
  d.writeImage(t,dir,relPath,fileName,FALSE);
  visitADPostEnd(t, FALSE);
  t << endl;
  t << endl;
}
void  AsciidocGenerator::startLabels()
{
DB_GEN_C
}

void  AsciidocGenerator::writeLabel(const char *l,bool isLast)
{
DB_GEN_C
  t << "[source4]" << endl;
  t << "----" << endl;
  t << l << "]" << endl;
  if (!isLast) t << ", ";
}

void  AsciidocGenerator::endLabels()
{
DB_GEN_C
  t << "----" << endl;
}
void AsciidocGenerator::startExamples()
{
DB_GEN_C
  t << "=== 52";
  docify(theTranslator->trExamples());
}

void AsciidocGenerator::endExamples()
{
DB_GEN_C
  t << endl;
}
void AsciidocGenerator::startSubsubsection(void)
{
DB_GEN_C
  t << "=== 53";
}
void AsciidocGenerator::endSubsubsection(void)
{
DB_GEN_C
  t << endl;
}
void AsciidocGenerator::writeChar(char c)
{
DB_GEN_C
  char cs[2];
  cs[0]=c;
  cs[1]=0;
  docify(cs);
}
void AsciidocGenerator::startMemberDocPrefixItem()
{
DB_GEN_C
  t << "[source5]" << endl;
}
void AsciidocGenerator::endMemberDocPrefixItem()
{
DB_GEN_C
  t << endl;
}
void AsciidocGenerator::exceptionEntry(const char* prefix,bool closeBracket)
{
DB_GEN_C
  if (prefix)
    t << " " << prefix << "(";
  else if (closeBracket)
    t << ")";
  t << " ";
}
void AsciidocGenerator::startParameterName(bool)
{
DB_GEN_C
  t << " ";
}
void AsciidocGenerator::endParameterName(bool last,bool /*emptyList*/,bool closeBracket)
{
DB_GEN_C
  if (last)
  {
    if (closeBracket) t << ")";
  }
}
void AsciidocGenerator::startCodeFragment()
{
DB_GEN_C
  t << "[source9]" << endl;
  t << "----" << endl;
}
void AsciidocGenerator::endCodeFragment()
{
DB_GEN_C
    t << "----" << endl;
}
void AsciidocGenerator::startMemberTemplateParams()
{
DB_GEN_C
}

void AsciidocGenerator::endMemberTemplateParams(const char *,const char *)
{
DB_GEN_C
  t << endl;
}
void AsciidocGenerator::startSection(const char *lab,const char *,SectionInfo::SectionType type)
{
DB_GEN_C
  t << "14[[_" << stripPath(lab) << "]] ";
}
void AsciidocGenerator::endSection(const char *lab,SectionInfo::SectionType)
{
DB_GEN_C
}
void AsciidocGenerator::addIndexItem(const char *prim,const char *sec)
{
DB_GEN_C
  addIndexTerm(t, prim, sec);
}

void AsciidocGenerator::startDescTable(const char *title)
{
DB_GEN_C
  int ncols = 2;
  t << "[cols=\"";
  for (int i = 0; i < ncols; i++)
  {
    t << "<colspec colname='c" << i+1 << "'/>\n";
  }
  t << "\"]" << endl;
  //if (title)t << "=== " << convertToAsciidoc(title) << endl;
  t << "|===6" << endl;
  m_descTable = TRUE;
}

void AsciidocGenerator::endDescTable()
{
DB_GEN_C
  t << "|===7" << endl;
  m_descTable = FALSE;
}

void AsciidocGenerator::startDescTableRow()
{
DB_GEN_C
  t << "|";
}

void AsciidocGenerator::endDescTableRow()
{
DB_GEN_C
}

void AsciidocGenerator::startDescTableTitle()
{
DB_GEN_C
}

void AsciidocGenerator::endDescTableTitle()
{
DB_GEN_C
  t << endl;
}

void AsciidocGenerator::startDescTableData()
{
DB_GEN_C
  t << "</entry><entry>";
}

void AsciidocGenerator::endDescTableData()
{
DB_GEN_C
  t << "</entry>";
}
void AsciidocGenerator::startGroupCollaboration()
{
DB_GEN_C
}
void AsciidocGenerator::endGroupCollaboration(const DotGroupCollaboration &g)
{
DB_GEN_C
  g.writeGraph(t,GOF_BITMAP,EOF_Asciidoc,Config_getString(ASCIIDOC_OUTPUT),fileName,relPath,FALSE);
}
void AsciidocGenerator::startDotGraph()
{
DB_GEN_C
}
void AsciidocGenerator::endDotGraph(const DotClassGraph &g)
{
DB_GEN_C
  g.writeGraph(t,GOF_BITMAP,EOF_Asciidoc,Config_getString(ASCIIDOC_OUTPUT),fileName,relPath,TRUE,FALSE);
}
void AsciidocGenerator::startInclDepGraph()
{
DB_GEN_C
}
void AsciidocGenerator::endInclDepGraph(const DotInclDepGraph &g)
{
DB_GEN_C
  QCString fn = g.writeGraph(t,GOF_BITMAP,EOF_Asciidoc,Config_getString(ASCIIDOC_OUTPUT), fileName,relPath,FALSE);
}
void AsciidocGenerator::startCallGraph()
{
DB_GEN_C
}
void AsciidocGenerator::endCallGraph(const DotCallGraph &g)
{
DB_GEN_C
  QCString fn = g.writeGraph(t,GOF_BITMAP,EOF_Asciidoc,Config_getString(ASCIIDOC_OUTPUT), fileName,relPath,FALSE);
}
void AsciidocGenerator::startDirDepGraph()
{
DB_GEN_C
}
void AsciidocGenerator::endDirDepGraph(const DotDirDeps &g)
{
DB_GEN_C
  QCString fn = g.writeGraph(t,GOF_BITMAP,EOF_Asciidoc,Config_getString(ASCIIDOC_OUTPUT), fileName,relPath,FALSE);
}
void AsciidocGenerator::startMemberDocList()
{
DB_GEN_C
}
void AsciidocGenerator::endMemberDocList()
{
DB_GEN_C
  m_inGroup = TRUE;
}
void AsciidocGenerator::startConstraintList(const char *header)
{
DB_GEN_C
  t << "==== 54";
  docify(header);
  t << endl;
  t << endl;
}
void AsciidocGenerator::startConstraintParam()
{
DB_GEN_C
  t << "NOTE: ";
}
void AsciidocGenerator::endConstraintParam()
{
DB_GEN_C
}
void AsciidocGenerator::startConstraintType()
{
DB_GEN_C
  t << ":";
}
void AsciidocGenerator::endConstraintType()
{
DB_GEN_C
  t << "__" << endl;
}
void AsciidocGenerator::startConstraintDocs()
{
DB_GEN_C
}
void AsciidocGenerator::endConstraintDocs()
{
DB_GEN_C
}
void AsciidocGenerator::endConstraintList()
{
DB_GEN_C
}
