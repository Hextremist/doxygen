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

static AsciidocSectionMapper g_asciidocSectionMapper;


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
      case ' ':  t << "&#32;"; col++;  break;
      case '<':  t << "&lt;"; col++;   break;
      case '>':  t << "&gt;"; col++;   break;
      case '&':  t << "&amp;"; col++;  break;
      case '\'': t << "&apos;"; col++; break;
      case '"':  t << "&quot;"; col++; break;
      case '\007':  t << "^G"; col++; break; // bell
      case '\014':  t << "^L"; col++; break; // form feed
      default:   t << c; col++;        break;
    }
  }
}

static void writeAsciidocHeaderMainpage(FTextStream &t, QCString &pageName)
{
  if (!pageName.isEmpty()) t << " xml:id=\"_" << pageName << "\"";
  t << ">" << endl;
}

static void writeAsciidocHeader_ID(FTextStream &t, QCString id)
{
}

static void addIndexTerm(FTextStream &t, QCString prim, QCString sec = "")
{
  t << "<indexterm><primary>";
  t << convertToAsciidoc(prim);
  t << "</primary>";
  if (!sec.isEmpty())
  {
    t << "<secondary>";
    t << convertToAsciidoc(sec);
    t << "</secondary>";
  }
  t << "</indexterm>" << endl;
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
  m_t << "<literallayout><computeroutput>" << endl;
}
void AsciidocCodeGenerator::endCodeFragment()
{
  m_t << "</computeroutput></literallayout>" << endl;
}

static void writeTemplateArgumentList(ArgumentList *al,
    FTextStream &t,
    Definition *scope,
    FileDef *fileScope,
    int indent)
{
  QCString indentStr;
  indentStr.fill(' ',indent);
  if (al)
  {
    t << indentStr << "<templateparamlist>" << endl;
    ArgumentListIterator ali(*al);
    Argument *a;
    for (ali.toFirst();(a=ali.current());++ali)
    {
      t << indentStr << "  <param>" << endl;
      if (!a->type.isEmpty())
      {
        t << indentStr <<  "    <type>";
        linkifyText(TextGeneratorAsciidocImpl(t),scope,fileScope,0,a->type);
        t << "</type>" << endl;
      }
      if (!a->name.isEmpty())
      {
        t << indentStr <<  "    <declname>" << a->name << "</declname>" << endl;
        t << indentStr <<  "    <defname>" << a->name << "</defname>" << endl;
      }
      if (!a->defval.isEmpty())
      {
        t << indentStr << "    <defval>";
        linkifyText(TextGeneratorAsciidocImpl(t),scope,fileScope,0,a->defval);
        t << "</defval>" << endl;
      }
      t << indentStr << "  </param>" << endl;
    }
    t << indentStr << "</templateparamlist>" << endl;
  }
}

static void writeTemplateList(ClassDef *cd,FTextStream &t)
{
  writeTemplateArgumentList(cd->templateArguments(),t,cd,0,4);
}

static void writeAsciidocDocBlock(FTextStream &t,
    const QCString &fileName,
    int lineNr,
    Definition *scope,
    MemberDef * md,
    const QCString &text)
{
  QCString stext = text.stripWhiteSpace();
  if (stext.isEmpty()) return;
  // convert the documentation string into an abstract syntax tree
  DocNode *root = validatingParseDoc(fileName,lineNr,scope,md,text,FALSE,FALSE);
  // create a code generator
  AsciidocCodeGenerator *asciidocCodeGen = new AsciidocCodeGenerator(t);
  // create a parse tree visitor for Asciidoc
  AsciidocDocVisitor *visitor = new AsciidocDocVisitor(t,*asciidocCodeGen);
  // visit all nodes
  root->accept(visitor);
  // clean up
  delete visitor;
  delete asciidocCodeGen;
  delete root;
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

static QCString classOutputFileBase(ClassDef *cd)
{
  //static bool inlineGroupedClasses = Config_getBool(INLINE_GROUPED_CLASSES);
  //if (inlineGroupedClasses && cd->partOfGroups()!=0)
  return cd->getOutputFileBase();
  //else
  //  return cd->getOutputFileBase();
}

static QCString memberOutputFileBase(MemberDef *md)
{
  //static bool inlineGroupedClasses = Config_getBool(INLINE_GROUPED_CLASSES);
  //if (inlineGroupedClasses && md->getClassDef() && md->getClassDef()->partOfGroups()!=0)
  //  return md->getClassDef()->getAsciidocOutputFileBase();
  //else
  //  return md->getOutputFileBase();
  return md->getOutputFileBase();
}

static void generateTOC(FTextStream &t, PageDef *pd)
{
  if (pd->localToc().isAsciidocEnabled())
  {
    t << "    <toc>" << endl;
    t << "    <title>" << theTranslator->trRTFTableOfContents() << "</title>" << endl;
    SectionDict *sectionDict = pd->getSectionDict();
    SDict<SectionInfo>::Iterator li(*sectionDict);
    SectionInfo *si;
    int level=1,l;
    bool inLi[5]={ FALSE, FALSE, FALSE, FALSE, FALSE };
    int maxLevel = pd->localToc().asciidocLevel();
    for (li.toFirst();(si=li.current());++li)
    {
      if (si->type==SectionInfo::Section       ||
          si->type==SectionInfo::Subsection    ||
          si->type==SectionInfo::Subsubsection ||
          si->type==SectionInfo::Paragraph)
      {
        //printf("  level=%d title=%s\n",level,si->title.data());
        int nextLevel = (int)si->type;
        if (nextLevel>level)
        {
          for (l=level;l<nextLevel;l++)
          {
            if (l < maxLevel) t << "    <tocdiv>" << endl;
          }
        }
        else if (nextLevel<level)
        {
          for (l=level;l>nextLevel;l--)
          {
            inLi[l]=FALSE;
            if (l <= maxLevel) t << "    </tocdiv>" << endl;
          }
        }
        if (nextLevel <= maxLevel)
        {
          QCString titleDoc = convertToAsciidoc(si->title);
          t << "      <tocentry>" << (si->title.isEmpty()?si->label:titleDoc) << "</tocentry>" << endl;
        }
        inLi[nextLevel]=TRUE;
        level = nextLevel;
      }
    }
    t << "    </toc>" << endl;
  }
}

static void generateSourceRefList(FTextStream &t,const char *scopeName, const QCString &text,MemberSDict *members, Definition *def)
{
  static bool sourceBrowser   = Config_getBool(SOURCE_BROWSER);
  static bool refLinkSource   = Config_getBool(REFERENCES_LINK_SOURCE);

  if (members)
  {
    members->sort();

    t << "<formalpara><title>" << convertToAsciidoc(text) << "</title><para>";

    QCString ldefLine=theTranslator->trWriteList(members->count());

    QRegExp marker("@[0-9]+");
    int index=0,newIndex,matchLen;
    // now replace all markers in inheritLine with links to the classes
    while ((newIndex=marker.match(ldefLine,index,&matchLen))!=-1)
    {
      bool ok;
      t << convertToAsciidoc(ldefLine.mid(index,newIndex-index));
      uint entryIndex = ldefLine.mid(newIndex+1,matchLen-1).toUInt(&ok);
      MemberDef *md=members->at(entryIndex);
      if (ok && md)
      {
        QCString scope=md->getScopeString();
        QCString name=md->name();
        //printf("class=%p scope=%s scopeName=%s\n",md->getClassDef(),scope.data(),scopeName);
        if (!scope.isEmpty() && scope!=scopeName)
        {
          name.prepend(scope+getLanguageSpecificSeparator(def->getLanguage()));
        }
        if (!md->isObjCMethod() &&
            (md->isFunction() || md->isSlot() ||
             md->isPrototype() || md->isSignal()
            )
           )
        {
          name+="()";
        }
        //Definition *d = md->getOutputFileBase();
        //if (d==Doxygen::globalScope) d=md->getBodyDef();
        if (sourceBrowser &&
            !(md->isLinkable() && !refLinkSource) &&
            md->getStartBodyLine()!=-1 &&
            md->getBodyDef()
           )
        {
          //printf("md->getBodyDef()=%p global=%p\n",md->getBodyDef(),Doxygen::globalScope);

          const int maxLineNrStr = 10;
          char anchorStr[maxLineNrStr];
          qsnprintf(anchorStr,maxLineNrStr,"l%05d",md->getStartBodyLine());
          //printf("Write object link to %s\n",md->getBodyDef()->getSourceFileBase().data());
          t << convertToAsciidoc(name);
          // or
          // ol.writeObjectLink(0,md->getBodyDef()->getSourceFileBase(),anchorStr,name);
        }
        else if (md->isLinkable() /*&& d && d->isLinkable()*/)
        {
          t << convertToAsciidoc(name);
          // or
          // ol.writeObjectLink(md->getReference(), md->getOutputFileBase(), md->anchor(),name);
        }
        else
        {
          t << convertToAsciidoc(name);
        }
      }
      index=newIndex+matchLen;
    }
    t << ldefLine.right(ldefLine.length()-index);
    t<< ".";
    t << "</para></formalpara>";
  }
}
static void generateInlineCode(FTextStream &t,const char *scopeName, Definition *def)
{
  static bool inlineSources = Config_getBool(INLINE_SOURCES);
  //printf("Source Fragment %s: %d-%d bodyDef=%p\n",name().data(),
  //        m_startBodyLine,m_endBodyLine,m_bodyDef);
  if (inlineSources && def->hasSources())
  {
    QCString codeFragment;
    int actualStart=def->getStartBodyLine(),actualEnd=def->getEndBodyLine();
    if (readCodeFragment(def->getBodyDef()->absFilePath(),
          actualStart,actualEnd,codeFragment)
       )
    {
      //printf("Adding code fragment '%s' ext='%s'\n",
      //    codeFragment.data(),m_impl->defFileExt.data());
      ParserInterface *pIntf = Doxygen::parserManager->getParser(def->getDefFileExtension());
      pIntf->resetCodeParserState();
      //printf("Read:\n`%s'\n\n",codeFragment.data());
      MemberDef *thisMd = 0;
      if (def->definitionType()==Definition::TypeMember) thisMd = (MemberDef *)def;

      AsciidocCodeGenerator *asciidocGen = new AsciidocCodeGenerator(t);
      asciidocGen->startCodeFragment();
      pIntf->parseCode(*asciidocGen,      // codeOutIntf
                       scopeName,        // scope
                       codeFragment,     // input
                       def->getLanguage(),     // lang
                       FALSE,            // isExample
                       0,                // exampleName
                       def->getBodyDef(),  // fileDef
                       actualStart,      // startLine
                       actualEnd,        // endLine
                       TRUE,             // inlineFragment
                       thisMd,           // memberDef
                       TRUE              // show line numbers
                     );
      asciidocGen->finish();
      asciidocGen->endCodeFragment();
      delete asciidocGen;
    }
  }
}

static void definedAtLine(int line, QCString fileName, FTextStream &t)
{
  QCString refText = theTranslator->trDefinedAtLineInSourceFile();
  int lineMarkerPos = refText.find("@0");
  int fileMarkerPos = refText.find("@1");
  if (lineMarkerPos!=-1 && fileMarkerPos!=-1) // should always pass this.
  {
    if (lineMarkerPos<fileMarkerPos) // line marker before file marker
    {
      t << refText.left(lineMarkerPos)
        << line
        << refText.mid(lineMarkerPos+2, fileMarkerPos-lineMarkerPos-2)
        << fileName
        << refText.right(refText.length()-fileMarkerPos-2);
    }
    else // file marker before line marker
    {
      t << refText.left(fileMarkerPos)
        << fileName
        << refText.mid(fileMarkerPos+2, lineMarkerPos-fileMarkerPos-2)
        << line
        << refText.right(refText.length()-lineMarkerPos-2);
    }
  }
  else
  {
    err("translation error: invalid markers in trDefinedAtLineInSourceFile()\n");
  }
}

static void generateAsciidocForMember(MemberDef *md,FTextStream &t,Definition *def, bool detailed=0)
{
  // + declaration/definition arg lists
  // + reimplements
  // + reimplementedBy
  // + exceptions
  // + const/volatile specifiers
  // - examples
  // + source definition
  // + source references
  // + source referenced by
  // - body code
  // + template arguments
  //     (templateArguments(), definitionTemplateParameterLists())
  // - call graph

  // enum values are written as part of the enum
  if (md->memberType()==MemberType_EnumValue) return;
  if (md->isHidden()) return;
  //if (md->name().at(0)=='@') return; // anonymous member

  // group members are only visible in their group
  //if (def->definitionType()!=Definition::TypeGroup && md->getGroupDef()) return;
  QCString memType;
  switch (md->memberType())
  {
    case MemberType_Define:      memType="define";    break;
    case MemberType_Function:    memType="function";  break;
    case MemberType_Variable:    memType="variable";  break;
    case MemberType_Typedef:     memType="typedef";   break;
    case MemberType_Enumeration: memType="enum";      break;
    case MemberType_EnumValue:   ASSERT(0);           break;
    case MemberType_Signal:      memType="signal";    break;
    case MemberType_Slot:        memType="slot";      break;
    case MemberType_Friend:      memType="friend";    break;
    case MemberType_DCOP:        memType="dcop";      break;
    case MemberType_Property:    memType="property";  break;
    case MemberType_Event:       memType="event";     break;
    case MemberType_Interface:   memType="interface"; break;
    case MemberType_Service:     memType="service";   break;
  }
  QCString scopeName;
  if (md->getClassDef())
  {
    scopeName=md->getClassDef()->name();
  }
  else if (md->getNamespaceDef())
  {
    scopeName=md->getNamespaceDef()->name();
  }
  if (detailed==0)
  {
    t << "            <para>" << endl;
    t << "                <itemizedlist>" << endl;
    t << "                    <listitem>" << endl;
    //enum
    bool closePara=TRUE;
    if (md->memberType()==MemberType_Enumeration)
    {
      bool inLi[5]={ FALSE, FALSE, FALSE, FALSE, FALSE };
      MemberList *enumFields = md->enumFieldList();
      t << "                            <para><literallayout>" << memType << " <link linkend=\"_";
      if (md->getGroupDef() && def->definitionType()==Definition::TypeGroup)
      {
        t << md->getGroupDef()->getOutputFileBase();
      }
      else
      {
        t << memberOutputFileBase(md);
      }
      t << "_1" << md->anchor() << "\">" << convertToAsciidoc(md->name()) << "</link>";
      if (enumFields!=0)
      {
        MemberListIterator emli(*enumFields);
        MemberDef *emd;
        t << " {" << endl;
        int cnt=0;
        for (emli.toFirst();(emd=emli.current());++emli)
        {
          if (cnt!=0)
          {
            t << "," << endl;
          }
          t << "<link linkend=\"_" << memberOutputFileBase(emd) << "_1" << emd->anchor() << "\">";
          writeAsciidocString(t,emd->name());
          t << "</link>";
          if (!emd->initializer().isEmpty())
          {
            writeAsciidocString(t,emd->initializer());
          }
          cnt++;
        }
        t << endl << "}";
      }
      t << "</literallayout>" << endl;
      if (md->briefDescription())
      {
          t << "<para><emphasis>";
          writeAsciidocString(t,md->briefDescription());
          t << "</emphasis></para>" << endl;
      }
    }
    else if (md->memberType()==MemberType_Define)
    {
      t << "                            <para>" << "#" << memType << " <link linkend=\"_";
      if (md->getGroupDef() && def->definitionType()==Definition::TypeGroup)
      {
        t << md->getGroupDef()->getOutputFileBase();
      }
      else
      {
        t << memberOutputFileBase(md);
      }
      t << "_1" << md->anchor() << "\">" << convertToAsciidoc(md->name()) << "</link>";
      if (!md->initializer().isEmpty() && md->initializer().length()<2000)
      {
        t << " ";
        linkifyText(TextGeneratorAsciidocImpl(t),def,md->getBodyDef(),md,md->initializer());
      }
      if (md->briefDescription())
      {
        t << "<para><emphasis>";
        writeAsciidocString(t,md->briefDescription());
        t << "</emphasis></para>" << endl;
      }
    }
    else if (md->memberType()==MemberType_Variable)
    {
      if (md->getClassDef())
      {
        t << "                        <para>" << convertToAsciidoc(md->declaration());
        if (md->briefDescription())
        {
          t << "<para><emphasis>";
          writeAsciidocString(t,md->briefDescription());
          t << "</emphasis></para>";
        }
      }
      else
      {
        t << "                        <para>";
        linkifyText(TextGeneratorAsciidocImpl(t),def,md->getBodyDef(),md,md->typeString());
        t << " <link linkend=\"_";
        if (md->getGroupDef() && def->definitionType()==Definition::TypeGroup)
        {
          t << md->getGroupDef()->getOutputFileBase();
        }
        else
        {
          t << memberOutputFileBase(md);
        }
        t << "_1" << md->anchor() << "\">" << convertToAsciidoc(md->name()) << "</link>";
        if (md->briefDescription())
        {
            t << "<para><emphasis>";
            writeAsciidocString(t,md->briefDescription());
            t << "</emphasis></para>" << endl;
        }
      }
    }
    else if (md->memberType()==MemberType_Typedef)
    {
      t << "                            <para>" << memType;
      t << " ";
      linkifyText(TextGeneratorAsciidocImpl(t),def,md->getBodyDef(),md,md->typeString());
      t << " ";
      t << " <link linkend=\"_";
      if (md->getGroupDef() && def->definitionType()==Definition::TypeGroup)
      {
        t << md->getGroupDef()->getOutputFileBase();
      }
      else
      {
        t << memberOutputFileBase(md);
      }
      t << "_1" << md->anchor() << "\">" << convertToAsciidoc(md->name()) << "</link>";
      if (md->briefDescription())
      {
          t << "<para><emphasis>";
          writeAsciidocString(t,md->briefDescription());
          t << "</emphasis></para>" << endl;
      }
    }
    else if (md->memberType()==MemberType_Function)
    {
      t << "                        <para>";
      linkifyText(TextGeneratorAsciidocImpl(t),def,md->getBodyDef(),md,md->typeString());
      t << " <link linkend=\"_";
      if (md->getGroupDef() && def->definitionType()==Definition::TypeGroup)
      {
        t << md->getGroupDef()->getOutputFileBase();
      }
      else
      {
        t << memberOutputFileBase(md);
      }
      t << "_1" << md->anchor() << "\">" << convertToAsciidoc(md->name()) << "</link>";
      t << " (" << endl;
      ArgumentList *declAl = md->declArgumentList();
      if (declAl && declAl->count()>0)
      {
        ArgumentListIterator declAli(*declAl);
        Argument *a;
        int cnt=0;
        for (declAli.toFirst();(a=declAli.current());++declAli)
        {
          if (cnt!=0)
          {
            t << ", ";
          }
          if (!a->type.isEmpty())
          {
            linkifyText(TextGeneratorAsciidocImpl(t),def,md->getBodyDef(),md,a->type);
          }
          t << " ";
          if (!a->name.isEmpty())
          {
            writeAsciidocString(t,a->name);
          }
          cnt++;
        }
      }
      t << ")";
      if (md->briefDescription())
      {
          t << "<para><emphasis>";
          writeAsciidocString(t,md->briefDescription());
          t << "</emphasis></para>" << endl;
      }
    }
    else
    {
      closePara = FALSE;
    }
    if (closePara) t << "</para>" << endl;
    t << "                    </listitem>" << endl;
    t << "                </itemizedlist>" << endl;
    t << "            </para>" << endl;
  }
  else
  {
    if (md->memberType()==MemberType_Enumeration)
    {
      MemberList *enumFields = md->enumFieldList();
      t << "            <section xml:id=\"_";
      if (md->getGroupDef() && def->definitionType()==Definition::TypeGroup)
      {
        t << md->getGroupDef()->getOutputFileBase();
      }
      else
      {
        t << memberOutputFileBase(md);
      }
      t << "_1" << md->anchor() << "\">" << endl;
      t << "               <title>" << memType << " " << convertToAsciidoc(md->name()) << " " << "</title>" << endl;
      t << "               ";
      writeAsciidocDocBlock(t,md->docFile(),md->docLine(),md->getOuterScope(),md,md->documentation());
      t << endl;
      if (enumFields!=0)
      {
        MemberListIterator emli(*enumFields);
        MemberDef *emd;
        t << "               <formalpara>" << endl;
        t << "                    <title>" << theTranslator->trEnumerationValues() << ":</title>" << endl;
        t << "                    <variablelist>" << endl;
        for (emli.toFirst();(emd=emli.current());++emli)
        {
          t << "                        <varlistentry xml:id=\"_";
          t << memberOutputFileBase(emd) << "_1" << emd->anchor() << "\">" << endl;
          t << "                            <term>";
          writeAsciidocString(t,emd->name());
          t << "</term>" << endl;
          t << "                            <listitem>" << endl;
          if(Config_getBool(REPEAT_BRIEF))
          {
              t << "                                <para>";
              writeAsciidocString(t,emd->briefDescription());
              t << "</para>" << endl;
          }
          t << "                            </listitem>" << endl;
          t << "                        </varlistentry>" << endl;
        }
        t << "                     </variablelist>" << endl;
        t << "                </formalpara>" << endl;
        t << "                <para>";
        t << "                <para>";
        definedAtLine(md->getDefLine(),stripPath(md->getDefFileName()),t);
        t << "</para>" << endl;

        t << "                    <literallayout><computeroutput>" << endl;
        t << "{" << endl;
        for (emli.toFirst();(emd=emli.current());++emli)
        {
          writeAsciidocString(t,emd->name());
          if (!emd->initializer().isEmpty())
          {
            writeAsciidocString(t,emd->initializer());
          }
          t << ", " << endl;
        }
        t << "}" << convertToAsciidoc(md->name()) << ";" << endl;
        t << "                    </computeroutput></literallayout>" << endl;
        t << "                </para>" << endl;
      }
      t << "            </section>" << endl;
    }
    else if (md->memberType()==MemberType_Typedef)
    {
      t << "            <section xml:id=\"_";
      if (md->getGroupDef() && def->definitionType()==Definition::TypeGroup)
      {
        t << md->getGroupDef()->getOutputFileBase();
      }
      else
      {
        t << memberOutputFileBase(md);
      }
      t << "_1" << md->anchor() << "\">" << endl;
      t << "                <title>" << convertToAsciidoc(md->definition()) << "</title>";
      if(Config_getBool(REPEAT_BRIEF))
      {
          t << " <emphasis>";
          writeAsciidocString(t,md->briefDescription());
          t << "</emphasis>" << endl;
      }
      t << "                ";
      writeAsciidocDocBlock(t,md->docFile(),md->docLine(),md->getOuterScope(),md,md->documentation());
      t << endl;
      t << "            </section>" << endl;
    }
    else if (md->memberType()==MemberType_Function)
    {
      t << "            <section xml:id=\"_";
      if (md->getGroupDef() && def->definitionType()==Definition::TypeGroup)
      {
        t << md->getGroupDef()->getOutputFileBase();
      }
      else
      {
        t << memberOutputFileBase(md);
      }
      t << "_1" << md->anchor() << "\">" << endl;
      t << "                <title>" << convertToAsciidoc(md->definition()) << " " << convertToAsciidoc(md->argsString()) << "</title>";
      addIndexTerm(t,md->name(),def->name());
      addIndexTerm(t,def->name(),md->name());
      if(Config_getBool(REPEAT_BRIEF))
      {
        if (!md->briefDescription().isEmpty())
        {
          t << " <emphasis>";
          writeAsciidocString(t,md->briefDescription());
          t << "</emphasis>" << endl;
        }
      }
      t << "                ";
      writeAsciidocDocBlock(t,md->docFile(),md->docLine(),md->getOuterScope(),md,md->documentation());
      t << endl;

      if (Config_getBool(REFERENCED_BY_RELATION))
      {
        generateSourceRefList(t,md->name(),theTranslator->trReferencedBy(),md->getReferencedByMembers(),md);
      }
      if (Config_getBool(REFERENCES_RELATION))
      {
        generateSourceRefList(t,md->name(),theTranslator->trReferences(),md->getReferencesMembers(),md);
      }
      generateInlineCode(t,md->name(),md);
      t << "            </section>" << endl;
    }
    else if (md->memberType()==MemberType_Define)
    {
      if (md->documentation())
      {
        t << "            <section xml:id=\"_";
        if (md->getGroupDef() && def->definitionType()==Definition::TypeGroup)
        {
          t << md->getGroupDef()->getOutputFileBase();
        }
        else
        {
          t << memberOutputFileBase(md);
        }
        t << "_1" << md->anchor() << "\">" << endl;
        t << "                <title>" << convertToAsciidoc(md->definition()) << "</title>";
        t << "                ";
        writeAsciidocDocBlock(t,md->docFile(),md->docLine(),md->getOuterScope(),md,md->documentation());
        t << endl;
        t << "            </section>" << endl;
      }
    }
    else if (md->memberType()==MemberType_Variable)
    {
      if (md->getClassDef())
      {
        if (md->documentation())
        {
          t << "            <section xml:id=\"_";
          if (md->getGroupDef() && def->definitionType()==Definition::TypeGroup)
          {
            t << md->getGroupDef()->getOutputFileBase();
          }
          else
          {
            t << memberOutputFileBase(md);
          }
          t << "_1" << md->anchor() << "\">" << endl;
          t << "                <title>" << convertToAsciidoc(md->definition()) << "</title>";
          addIndexTerm(t,md->name(),def->name());
          addIndexTerm(t,def->name(),md->name());
          t << "                ";
          writeAsciidocDocBlock(t,md->docFile(),md->docLine(),md->getOuterScope(),md,md->documentation());
          t << endl;
          t << "            </section>" << endl;
        }
      }
      else
      {
        t << "            <section xml:id=\"_";
        if (md->getGroupDef() && def->definitionType()==Definition::TypeGroup)
        {
          t << md->getGroupDef()->getOutputFileBase();
        }
        else
        {
          t << memberOutputFileBase(md);
        }
        t << "_1" << md->anchor() << "\">" << endl;
        t << "                <title>" << convertToAsciidoc(md->definition()) << "</title>";
        addIndexTerm(t,md->name(),def->name());
        addIndexTerm(t,def->name(),md->name());
        if(Config_getBool(REPEAT_BRIEF))
        {
            t << " <emphasis>";
            writeAsciidocString(t,md->briefDescription());
            t << "</emphasis>" << endl;
        }
        t << "                ";
        writeAsciidocDocBlock(t,md->docFile(),md->docLine(),md->getOuterScope(),md,md->documentation());
        t << endl;
        t << "            </section>" << endl;
      }
    }
  }
}

static void generateAsciidocSection(Definition *d,FTextStream &t,MemberList *ml,const char *,
    bool detailed=0, const char *header=0,const char *documentation=0)
{
  if (ml==0) return;
  MemberListIterator mli(*ml);
  MemberDef *md;
  int count=0;
  int doc_count=0;
  QCString title, desctitle, subtitle;

  for (mli.toFirst();(md=mli.current());++mli)
  {
    // namespace members are also inserted in the file scope, but
    // to prevent this duplication in the Asciidoc output, we filter those here.
    if (d->definitionType()!=Definition::TypeFile || md->getNamespaceDef()==0)
    {
      count++;
    }
  }

  if (count==0) return; // empty list

  subtitle = "";
  switch (ml->listType())
  {
    case MemberListType_pubMethods:        d->getLanguage()==SrcLangExt_ObjC ? title = theTranslator->trInstanceMethods() : title = theTranslator->trPublicMembers();
      switch (d->getLanguage())
      {
        case SrcLangExt_ObjC:    desctitle = theTranslator->trMethodDocumentation(); break;
        case SrcLangExt_Fortran: desctitle = theTranslator->trMemberFunctionDocumentationFortran(); break;
        default:                 desctitle = theTranslator->trMemberFunctionDocumentation(); break;
      };
      break;
    case MemberListType_priMethods:        title=theTranslator->trPrivateMembers();      desctitle="";   break;

    case MemberListType_decTypedefMembers: title=theTranslator->trTypedefs();            desctitle=theTranslator->trTypedefDocumentation();           break;
    case MemberListType_decEnumMembers:    title=theTranslator->trEnumerations();        desctitle=theTranslator->trEnumerationTypeDocumentation();   break;
    case MemberListType_decFuncMembers:    title=theTranslator->trFunctions();           desctitle=theTranslator->trFunctionDocumentation();          break;
    case MemberListType_decVarMembers:     title=theTranslator->trVariables();           desctitle=theTranslator->trVariableDocumentation();          break;
    case MemberListType_pubAttribs:        title=theTranslator->trPublicAttribs();       desctitle=theTranslator->trMemberDataDocumentation();        break;
    case MemberListType_priAttribs:        title=theTranslator->trPrivateAttribs();      desctitle=theTranslator->trMemberDataDocumentation();        break;
    case MemberListType_proAttribs:        title=theTranslator->trProtectedAttribs();    desctitle=theTranslator->trMemberDataDocumentation();        break;
    case MemberListType_decDefineMembers:  title=theTranslator->trDefines();             desctitle=theTranslator->trDefineDocumentation();        break;
    case MemberListType_related:           title=theTranslator->trRelatedFunctions();    desctitle=theTranslator->trRelatedFunctionDocumentation();
                                           subtitle=theTranslator->trRelatedSubscript(); break;
    default:                               title="";                                     desctitle="";
  }

  if (detailed)
  {
    for (mli.toFirst();(md=mli.current());++mli)
    {
        if (md->documentation().isEmpty() && !Config_getBool(REPEAT_BRIEF))
        {
            continue;
        }
        doc_count = 1;
        break;
    }

    if(doc_count == 0) return;

    if (!QCString(header).isEmpty())
    {
      t << "        <section>" << endl;
      t << "            <title>" << convertToAsciidoc(header) << "</title>" << endl;
    }
    else if (desctitle)
    {
      t << "        <section>" << endl;
      t << "            <title>" << desctitle << "</title>" << endl;
    }
  }
  else
  {
    t << "        <section>" << endl;
    if (!QCString(header).isEmpty())
    {
      t << "            <title>" << convertToAsciidoc(header) << "</title>" << endl;
    }
    else
    {
      t << "            <title>" << title << "</title>" << endl;
    }
    if (!subtitle.isEmpty())
      t << "            <para>" << subtitle << "</para>" << endl;
  }

  if (documentation)
  {
    t << "      <description>";
    writeAsciidocDocBlock(t,d->docFile(),d->docLine(),d,0,documentation);
    t << "</description>" << endl;
  }
  for (mli.toFirst();(md=mli.current());++mli)
  {
    // namespace members are also inserted in the file scope, but
    // to prevent this duplication in the Asciidoc output, we filter those here.
    if (d->definitionType()!=Definition::TypeFile || md->getNamespaceDef()==0)
    {
        if (detailed && md->documentation().isEmpty() && !Config_getBool(REPEAT_BRIEF))
        {
            continue;
        }

        generateAsciidocForMember(md,t,d,detailed);
    }
  }
  if (detailed)
  {
    if (!QCString(header).isEmpty())
    {
      t << "        </section>" << endl;
    }
    else if (desctitle)
    {
      t << "        </section>" << endl;
    }
  }
  else
  {
    t << "        </section>" << endl;
  }
}

static void writeInnerClasses(const ClassSDict *cl,FTextStream &t)
{
  if (cl)
  {
    ClassSDict::Iterator cli(*cl);
    ClassDef *cd;
    QCString title = theTranslator->trClasses();

    if (cli.toFirst())
    {
      t << "        <section>" << endl;
      t << "            <title> " << title << " </title>" << endl;
    }
    for (cli.toFirst();(cd=cli.current());++cli)
    {
      if (!cd->isHidden() && cd->name().find('@')==-1)
      {
        t << "            <para>" << endl;
        t << "                <itemizedlist>" << endl;
        t << "                    <listitem>" << endl;
        t << "                        <para>" << "struct <link linkend=\"_" << classOutputFileBase(cd) << "\">" << convertToAsciidoc(cd->name()) << "</link>";
        t << "</para>" << endl;
        if (cd->briefDescription())
        {
            t << "<para><emphasis>";
            writeAsciidocString(t,cd->briefDescription());
            t << "</emphasis></para>" << endl;
        }
        t << "                    </listitem>" << endl;
        t << "                </itemizedlist>" << endl;
        t << "            </para>" << endl;
      }
    }
    if (cli.toFirst())
    {
      t << "        </section>" << endl;
    }
  }
}

static void writeInnerNamespaces(const NamespaceSDict *nl,FTextStream &t)
{
  if (nl)
  {
    NamespaceSDict::Iterator nli(*nl);
    NamespaceDef *nd;
    QCString title = theTranslator->trNamespaces();

    if (nli.toFirst())
    {
      t << "        <simplesect>" << endl;
      t << "            <title> " << title << " </title>" << endl;
    }
    for (nli.toFirst();(nd=nli.current());++nli)
    {
      if (!nd->isHidden() && nd->name().find('@')==-1) // skip anonymous scopes
      {
        t << "            <para>" << endl;
        t << "                <itemizedlist>" << endl;
        t << "                    <listitem>" << endl;
        t << "                        <para>" << "struct <link linkend=\"_" << nd->getOutputFileBase() << "\">" << convertToAsciidoc(nd->name()) << "</link>";
        t << "</para>" << endl;
        t << "                    </listitem>" << endl;
        t << "                </itemizedlist>" << endl;
        t << "            </para>" << endl;
      }
    }
    if (nli.toFirst())
    {
      t << "        </simplesect>" << endl;
    }
  }
}

static void writeInnerFiles(const FileList *fl,FTextStream &t)
{
  if (fl)
  {
    QListIterator<FileDef> fli(*fl);
    FileDef *fd;
    QCString title =  theTranslator->trFile(TRUE,TRUE);

    if (fli.toFirst())
    {
      t << "        <simplesect>" << endl;
      t << "            <title> " << title << " </title>" << endl;
    }
    for (fli.toFirst();(fd=fli.current());++fli)
    {
      t << "            <para>" << endl;
      t << "                <itemizedlist>" << endl;
      t << "                    <listitem>" << endl;
      t << "                        <para>" << "file <link linkend=\"_" << fd->getOutputFileBase() << "\">" << convertToAsciidoc(fd->name()) << "</link>";
      t << "</para>" << endl;
      t << "                    </listitem>" << endl;
      t << "                </itemizedlist>" << endl;
      t << "            </para>" << endl;
    }
    if (fli.toFirst())
    {
      t << "        </simplesect>" << endl;
    }
  }
}

static void writeInnerPages(const PageSDict *pl,FTextStream &t)
{
  if (pl)
  {
    PageSDict::Iterator pli(*pl);
    PageDef *pd;

    for (pli.toFirst();(pd=pli.current());++pli)
    {
      t << "<xi:include href=\"" << pd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>"  << endl;
    }
  }
}

static void writeInnerGroups(const GroupList *gl,FTextStream &t)
{
  if (gl)
  {
    GroupListIterator gli(*gl);
    GroupDef *sgd;

    //Asciidoc header tags for inner groups
    if (gli.toFirst())
    {
      t << "    <simplesect>" << endl;
      t << "        <title>" << theTranslator->trModules() << "</title>" << endl;
      t << "    </simplesect>" << endl;
      t << "    <para>" << endl;
      t << "        <itemizedlist>" << endl;
    }

    for (gli.toFirst();(sgd=gli.current());++gli)
    {
      t << "            <listitem><para><link linkend=\"_" << sgd->getOutputFileBase() << "\">" << convertToAsciidoc(sgd->groupTitle()) << "</link></para></listitem>" << endl;
    }

    //Asciidoc footer tags for inner groups
    if (gli.toFirst())
    {
      t << "        </itemizedlist>" << endl;
      t << "    </para>" << endl;
    }

  }
}

static void writeInnerDirs(const DirList *dl,FTextStream &t)
{
  if (dl)
  {
    QListIterator<DirDef> subdirs(*dl);
    DirDef *subdir;
    QCString title = theTranslator->trDirectories();
    if (subdirs.toFirst())
    {
      t << "        <simplesect>" << endl;
      t << "            <title> " << title << " </title>" << endl;
    }
    for (subdirs.toFirst();(subdir=subdirs.current());++subdirs)
    {
      t << "            <para>" << endl;
      t << "                <itemizedlist>" << endl;
      t << "                    <listitem>" << endl;
      t << "                        <para>" << "dir <link linkend=\"_" << subdir->getOutputFileBase() << "\">" << convertToAsciidoc(subdir->displayName()) << "</link>";
      t << "</para>" << endl;
      t << "                    </listitem>" << endl;
      t << "                </itemizedlist>" << endl;
      t << "            </para>" << endl;
    }
    if (subdirs.toFirst())
    {
      t << "        </simplesect>" << endl;
    }
  }
}

static void writeInnerGroupFiles(const GroupList *gl,FTextStream &t)
{
  if (gl)
  {
    GroupListIterator gli(*gl);
    GroupDef *sgd;

    for (gli.toFirst();(sgd=gli.current());++gli)
    {
      t << "<xi:include href=\"" << sgd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>"  << endl;
    }
  }
}

static void generateAsciidocForClass(ClassDef *cd,FTextStream &ti)
{
  // + brief description
  // + detailed description
  // + template argument list(s)
  // - include file
  // + member groups
  // + inheritance diagram
  // + list of direct super classes
  // + list of direct sub classes
  // + list of inner classes
  // + collaboration diagram
  // + list of all members
  // + user defined member sections
  // + standard member sections
  // + detailed member documentation
  // - examples using the class

  if (cd->isReference())        return; // skip external references.
  if (cd->isHidden())           return; // skip hidden classes.
  if (cd->name().find('@')!=-1) return; // skip anonymous compounds.
  if (cd->templateMaster()!=0)  return; // skip generated template instances.

  msg("Generating Asciidoc output for class %s\n",cd->name().data());

  QCString fileAsciidoc=cd->getOutputFileBase()+".xml";
  //Add the file Documentation info to index file
  ti << "        <xi:include href=\"" << fileAsciidoc << "\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;

  QCString outputDirectory = Config_getString(ASCIIDOC_OUTPUT);
  QCString fileName=outputDirectory+"/"+ classOutputFileBase(cd)+".xml";
  QCString relPath = relativePathToRoot(fileName);
  QFile f(fileName);
  if (!f.open(IO_WriteOnly))
  {
    err("Cannot open file %s for writing!\n",fileName.data());
    return;
  }
  FTextStream t(&f);
  //t.setEncoding(FTextStream::UnicodeUTF8);

  writeAsciidocHeader_ID(t, classOutputFileBase(cd));
  t << "<title>";
  writeAsciidocString(t,cd->name());
  addIndexTerm(t,cd->name());
  t << " " << cd->compoundTypeString() << " Reference";
  t << "</title>" << endl;
  if (cd->briefDescription())
  {
    t << "    <para>" << endl;
    writeAsciidocDocBlock(t,cd->briefFile(),cd->briefLine(),cd,0,cd->briefDescription());
    t << "    </para>" << endl;
  }

  IncludeInfo *ii=cd->includeInfo();
  if (ii)
  {
    QCString nm = ii->includeName;
    if (nm.isEmpty() && ii->fileDef) nm = ii->fileDef->docName();
    if (!nm.isEmpty())
    {
      t << "<para>" << endl;
      t << "    <programlisting>#include ";
      if (ii->fileDef && !ii->fileDef->isReference()) // TODO: support external references
      {
        t << "<link linkend=\"_" << ii->fileDef->getOutputFileBase() << "\">";
      }
      if (ii->local)
      {
        t << "&quot;";
      }
      else
      {
        t << "&lt;";
      }
      t << convertToAsciidoc(nm);
      if (ii->local)
      {
        t << "&quot;";
      }
      else
      {
        t << "&gt;";
      }
      if (ii->fileDef && !ii->fileDef->isReference())
      {
        t << "</link>";
      }
      t << "</programlisting>" << endl;
      t << "</para>" << endl;
    }
  }

  if (Config_getBool(HAVE_DOT) && (Config_getBool(CLASS_DIAGRAMS) || Config_getBool(CLASS_GRAPH)))
  {
    t << "<para>Inheritance diagram for " << convertToAsciidoc(cd->name()) << "</para>" << endl;
    DotClassGraph inheritanceGraph(cd,DotNode::Inheritance);
    inheritanceGraph.writeGraph(t,GOF_BITMAP,EOF_Asciidoc,Config_getString(ASCIIDOC_OUTPUT),fileName,relPath,TRUE,FALSE);
  }

  if (Config_getBool(HAVE_DOT) && Config_getBool(COLLABORATION_GRAPH))
  {
    t << "<para>Collaboration diagram for " << convertToAsciidoc(cd->name()) << "</para>" << endl;
    DotClassGraph collaborationGraph(cd,DotNode::Collaboration);
    collaborationGraph.writeGraph(t,GOF_BITMAP,EOF_Asciidoc,Config_getString(ASCIIDOC_OUTPUT),fileName,relPath,TRUE,FALSE);
  }

  writeInnerClasses(cd->getClassSDict(),t);

  writeTemplateList(cd,t);
  if (cd->getMemberGroupSDict())
  {
    MemberGroupSDict::Iterator mgli(*cd->getMemberGroupSDict());
    MemberGroup *mg;
    for (;(mg=mgli.current());++mgli)
    {
      generateAsciidocSection(cd,t,mg->members(),"user-defined",0,mg->header(),
          mg->documentation());
    }
  }

  QListIterator<MemberList> mli(cd->getMemberLists());
  MemberList *ml;
  for (mli.toFirst();(ml=mli.current());++mli)
  {
    if ((ml->listType()&MemberListType_detailedLists)==0)
    {
      generateAsciidocSection(cd,t,ml,g_asciidocSectionMapper.find(ml->listType()));
    }
  }

  if ((Config_getBool(REPEAT_BRIEF) && cd->briefDescription()) || cd->documentation())
  {
    t << "        <simplesect>" << endl;
    t << "            <title>" << theTranslator->trDetailedDescription() << "</title>" << endl;

    if(Config_getBool(REPEAT_BRIEF))
    {
      if (cd->briefDescription())
      {
          t << "    <para>" << endl;
          // A title as 'Brief Description' may not be necessary.
          //t << "        <title>" << theTranslator->trBriefDescription() << "</title>" << endl;
          writeAsciidocDocBlock(t,cd->briefFile(),cd->briefLine(),cd,0,cd->briefDescription());
          t << "    </para>" << endl;
      }
    }

    if (cd->documentation())
    {
      writeAsciidocDocBlock(t,cd->docFile(),cd->docLine(),cd,0,cd->documentation());
    }
    t << "        </simplesect>" << endl;
  }

  if (cd->getMemberGroupSDict())
  {
    MemberGroupSDict::Iterator mgli(*cd->getMemberGroupSDict());
    MemberGroup *mg;
    for (;(mg=mgli.current());++mgli)
    {
      generateAsciidocSection(cd,t,mg->members(),"user-defined",1,mg->header(),
          mg->documentation());
    }
  }

  for (mli.toFirst();(ml=mli.current());++mli)
  {
    if ((ml->listType()&MemberListType_detailedLists)==0)
    {
      generateAsciidocSection(cd,t,ml,g_asciidocSectionMapper.find(ml->listType()),1);
    }
  }

  /*// TODO: Handling of Inheritance and Colloboration graph for Asciidoc to be implemented
    DotClassGraph inheritanceGraph(cd,DotNode::Inheritance);
    if (!inheritanceGraph.isTrivial())
    {
    t << "    <inheritancegraph>" << endl;
    inheritanceGraph.writeAsciidoc(t);
    t << "    </inheritancegraph>" << endl;
    }
    DotClassGraph collaborationGraph(cd,DotNode::Collaboration);
    if (!collaborationGraph.isTrivial())
    {
    t << "    <collaborationgraph>" << endl;
    collaborationGraph.writeAsciidoc(t);
    t << "    </collaborationgraph>" << endl;
    }
    t << "    <location file=\""
    << cd->getDefFileName() << "\" line=\""
    << cd->getDefLine() << "\"";
    if (cd->getStartBodyLine()!=-1)
    {
    FileDef *bodyDef = cd->getBodyDef();
    if (bodyDef)
    {
    t << " bodyfile=\"" << bodyDef->absFilePath() << "\"";
    }
    t << " bodystart=\"" << cd->getStartBodyLine() << "\" bodyend=\""
    << cd->getEndBodyLine() << "\"";
    }
    t << "/>" << endl;
    writeListOfAllMembers(cd,t);
   */

  t << "                <para>" << cd->generatedFromFiles() << "</para>" << endl;
  t << "                <para><itemizedlist><listitem><para>" << stripPath(cd->getDefFileName()) << "</para></listitem></itemizedlist></para>" << endl;
  t << "</section>" << endl;

}

static void generateAsciidocForNamespace(NamespaceDef *nd,FTextStream &ti)
{
  // + contained class definitions
  // + contained namespace definitions
  // + member groups
  // + normal members
  // + brief desc
  // + detailed desc
  // + location
  // - files containing (parts of) the namespace definition

  if (nd->isReference() || nd->isHidden()) return; // skip external references

  QCString fileAsciidoc=nd->getOutputFileBase()+".xml";
  //Add the file Documentation info to index file
  ti << "        <xi:include href=\"" << fileAsciidoc << "\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;

  QCString outputDirectory = Config_getString(ASCIIDOC_OUTPUT);
  QCString fileName=outputDirectory+"/"+nd->getOutputFileBase()+".xml";
  QFile f(fileName);
  if (!f.open(IO_WriteOnly))
  {
    err("Cannot open file %s for writing!\n",fileName.data());
    return;
  }
  FTextStream t(&f);
  //t.setEncoding(FTextStream::UnicodeUTF8);

  writeAsciidocHeader_ID(t, nd->getOutputFileBase());
  t << "<title>";
  addIndexTerm(t,nd->displayName());
  writeAsciidocString(t,nd->title());
  t << "</title>" << endl;

  if (nd->briefDescription())
  {
      t << "    <para>" << endl;
          //t << "        <title>" << theTranslator->trBriefDescription() << "</title>" << endl;
      t << "    </para>" << endl;
  }
  writeInnerClasses(nd->getClassSDict(),t);
  writeInnerNamespaces(nd->getNamespaceSDict(),t);

  if (nd->getMemberGroupSDict())
  {
    MemberGroupSDict::Iterator mgli(*nd->getMemberGroupSDict());
    MemberGroup *mg;
    for (;(mg=mgli.current());++mgli)
    {
      generateAsciidocSection(nd,t,mg->members(),"user-defined",0,mg->header(),
          mg->documentation());
    }
  }

  QListIterator<MemberList> mli(nd->getMemberLists());
  MemberList *ml;
  for (mli.toFirst();(ml=mli.current());++mli)
  {
    if ((ml->listType()&MemberListType_declarationLists)!=0)
    {
      generateAsciidocSection(nd,t,ml,g_asciidocSectionMapper.find(ml->listType()));
    }
  }

  if ((Config_getBool(REPEAT_BRIEF) && nd->briefDescription()) || nd->documentation())
  {
    t << "        <simplesect>" << endl;
    t << "            <title>" << theTranslator->trDetailedDescription() << "</title>" << endl;

    if(Config_getBool(REPEAT_BRIEF))
    {
      if (nd->briefDescription())
      {
          t << "    <para>" << endl;
          //t << "        <title>" << theTranslator->trBriefDescription() << "</title>" << endl;
          writeAsciidocDocBlock(t,nd->briefFile(),nd->briefLine(),nd,0,nd->briefDescription());
          t << "    </para>" << endl;
      }
    }

    if (nd->documentation())
    {
      writeAsciidocDocBlock(t,nd->docFile(),nd->docLine(),nd,0,nd->documentation());
    }
    t << "        </simplesect>" << endl;
  }

  if (nd->getMemberGroupSDict())
  {
    MemberGroupSDict::Iterator mgli(*nd->getMemberGroupSDict());
    MemberGroup *mg;
    for (;(mg=mgli.current());++mgli)
    {
      generateAsciidocSection(nd,t,mg->members(),"user-defined",1,mg->header(),
          mg->documentation());
    }
  }

  for (mli.toFirst();(ml=mli.current());++mli)
  {
    if ((ml->listType()&MemberListType_detailedLists)==0)
    {
      if (ml->listType() != MemberListType_allMembersList &&
          ml->listType() != MemberListType_docFuncMembers)
        generateAsciidocSection(nd,t,ml,g_asciidocSectionMapper.find(ml->listType()),1);
    }
  }
  // we actually need here "namespace"
  // t << "                <para>" << theTranslator->trGeneratedFromFiles(ClassDef::Struct, FALSE) << "</para>" << endl;
  // t << "                <para><itemizedlist><listitem><para>" << stripPath(nd->getDefFileName()) << "</para></listitem></itemizedlist></para>" << endl;
  t << "</section>" << endl;
}

static void generateAsciidocForFile(FileDef *fd,FTextStream &ti)
{
  // + includes files
  // + includedby files
  // + include graph
  // + included by graph
  // + contained class definitions
  // + contained namespace definitions
  // + member groups
  // + normal members
  // + brief desc
  // + detailed desc
  // + source code
  // + location
  // - number of lines

  if (fd->isReference()) return; // skip external references

  QCString fileAsciidoc=fd->getOutputFileBase()+".xml";
  //Add the file Documentation info to index file
  ti << "        <xi:include href=\"" << fileAsciidoc << "\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;

  QCString outputDirectory = Config_getString(ASCIIDOC_OUTPUT);
  QCString fileName=outputDirectory+"/"+fd->getOutputFileBase()+".xml";
  QCString relPath = relativePathToRoot(fileName);

  QFile f(fileName);
  if (!f.open(IO_WriteOnly))
  {
    err("Cannot open file %s for writing!\n",fileName.data());
    return;
  }
  FTextStream t(&f);
  //t.setEncoding(FTextStream::UnicodeUTF8);
  writeAsciidocHeader_ID(t, fd->getOutputFileBase());

  t << "    <title>";
  writeAsciidocString(t,fd->name());
  t << " File Reference";
  t << "</title>" << endl;

  if (fd->briefDescription())
  {
    t << "    <para>" << endl;
    writeAsciidocDocBlock(t,fd->briefFile(),fd->briefLine(),fd,0,fd->briefDescription());
    t << "    </para>" << endl;
  }

  IncludeInfo *inc;

  if (fd->includeFileList())
  {
    QListIterator<IncludeInfo> ili1(*fd->includeFileList());
    for (ili1.toFirst();(inc=ili1.current());++ili1)
    {
      t << "    <programlisting>#include ";
      if (inc->local)
      {
        t << "&quot;";
      }
      else
      {
        t << "&lt;";
      }
      t << convertToAsciidoc(inc->includeName);
      if (inc->local)
      {
        t << "&quot;";
      }
      else
      {
        t << "&gt;";
      }
      t << "</programlisting>" << endl;
    }
  }
  if (Config_getBool(HAVE_DOT))
  {
    if (Config_getBool(INCLUDE_GRAPH))
    {
      t << "<para>Include dependency diagram for " << convertToAsciidoc(fd->name()) << "</para>" << endl;
      DotInclDepGraph idepGraph(fd, FALSE);
      idepGraph.writeGraph(t,GOF_BITMAP,EOF_Asciidoc,Config_getString(ASCIIDOC_OUTPUT),fileName,relPath,FALSE);
    }
    if (Config_getBool(INCLUDED_BY_GRAPH))
    {
      t << "<para>Included by dependency diagram for " << convertToAsciidoc(fd->name()) << "</para>" << endl;
      DotInclDepGraph ibdepGraph(fd, TRUE);
      ibdepGraph.writeGraph(t,GOF_BITMAP,EOF_Asciidoc,Config_getString(ASCIIDOC_OUTPUT),fileName,relPath,FALSE);
    }
  }

  if (fd->getClassSDict())
  {
    writeInnerClasses(fd->getClassSDict(),t);
  }
  if (fd->getNamespaceSDict())
  {
    writeInnerNamespaces(fd->getNamespaceSDict(),t);
  }

  if (fd->getMemberGroupSDict())
  {
    MemberGroupSDict::Iterator mgli(*fd->getMemberGroupSDict());
    MemberGroup *mg;
    for (;(mg=mgli.current());++mgli)
    {
      generateAsciidocSection(fd,t,mg->members(),"user-defined",0,mg->header(),
          mg->documentation());
    }
  }

  QListIterator<MemberList> mli(fd->getMemberLists());
  MemberList *ml;
  for (mli.toFirst();(ml=mli.current());++mli)
  {
    if ((ml->listType()&MemberListType_declarationLists)!=0)
    {
      generateAsciidocSection(fd,t,ml,g_asciidocSectionMapper.find(ml->listType()));
    }
  }

  t << "    <simplesect>" << endl;
  t << "        <title>" << theTranslator->trDetailedDescription() << "</title>" << endl;
  if(Config_getBool(REPEAT_BRIEF))
  {
    if (fd->briefDescription())
    {
      writeAsciidocDocBlock(t,fd->briefFile(),fd->briefLine(),fd,0,fd->briefDescription());
    }
  }
  writeAsciidocDocBlock(t,fd->docFile(),fd->docLine(),fd,0,fd->documentation());
  if (Config_getBool(FULL_PATH_NAMES))
  {
    t << "    <para>Definition in file " << fd->getDefFileName() << "</para>" << endl;
  }
  else
  {
    t << "    <para>Definition in file " << stripPath(fd->getDefFileName()) << "</para>" << endl;
  }
  t << "    </simplesect>" << endl;

  for (mli.toFirst();(ml=mli.current());++mli)
  {
    if ((ml->listType()&MemberListType_declarationLists)!=0)
    {
      generateAsciidocSection(fd,t,ml,g_asciidocSectionMapper.find(ml->listType()),1);
    }
  }

  if (Config_getBool(ASCIIDOC_PROGRAMLISTING))
  {
    t << "    <literallayout><computeroutput>" << endl;;
    writeAsciidocCodeBlock(t,fd);
    t << "    </computeroutput></literallayout>" << endl;
  }

  t << "</section>" << endl;
}

static void generateAsciidocForGroup(GroupDef *gd,FTextStream &ti)
{
  // + members
  // + member groups
  // + files
  // + classes
  // + namespaces
  // - packages
  // + pages
  // + child groups
  // - examples
  // + brief description
  // + detailed description

  if (gd->isReference()) return; // skip external references

  if (!gd->isASubGroup())
  {
    QCString fileAsciidoc=gd->getOutputFileBase()+".xml";
    //Add the file Documentation info to index file
    ti << "        <xi:include href=\"" << fileAsciidoc << "\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
  }

  QCString outputDirectory = Config_getString(ASCIIDOC_OUTPUT);
  QCString fileName=outputDirectory+"/"+gd->getOutputFileBase()+".xml";
  QCString relPath = relativePathToRoot(fileName);

  QFile f(fileName);
  if (!f.open(IO_WriteOnly))
  {
    err("Cannot open file %s for writing!\n",fileName.data());
    return;
  }

  FTextStream t(&f);
  //t.setEncoding(FTextStream::UnicodeUTF8);
  writeAsciidocHeader_ID(t, gd->getOutputFileBase());

  t << "    <title>" << convertToAsciidoc(gd->groupTitle()) << "</title>" << endl;
  if (gd->briefDescription())
  {
      t << "    <para>" << endl;
      writeAsciidocDocBlock(t,gd->briefFile(),gd->briefLine(),gd,0,gd->briefDescription());
      t << "    </para>" << endl;
  }

  if (Config_getBool(GROUP_GRAPHS) && Config_getBool(HAVE_DOT))
  {
    t << "<para>Collaboration diagram for " << convertToAsciidoc(gd->groupTitle()) << "</para>" << endl;
    DotGroupCollaboration collaborationGraph(gd);
    collaborationGraph.writeGraph(t,GOF_BITMAP,EOF_Asciidoc,Config_getString(ASCIIDOC_OUTPUT),fileName,relPath,FALSE);
  }

  writeInnerFiles(gd->getFiles(),t);
  writeInnerClasses(gd->getClasses(),t);
  writeInnerNamespaces(gd->getNamespaces(),t);
  writeInnerPages(gd->getPages(),t);
  writeInnerGroups(gd->getSubGroups(),t);

  if (gd->getMemberGroupSDict())
  {
    MemberGroupSDict::Iterator mgli(*gd->getMemberGroupSDict());
    MemberGroup *mg;
    for (;(mg=mgli.current());++mgli)
    {
      generateAsciidocSection(gd,t,mg->members(),"user-defined",0,mg->header(),
          mg->documentation());
    }
  }

  QListIterator<MemberList> mli(gd->getMemberLists());
  MemberList *ml;
  for (mli.toFirst();(ml=mli.current());++mli)
  {
    if ((ml->listType()&MemberListType_declarationLists)!=0)
    {
      generateAsciidocSection(gd,t,ml,g_asciidocSectionMapper.find(ml->listType()));
    }
  }

  if(Config_getBool(REPEAT_BRIEF))
  {
    if (gd->briefDescription())
    {
      //t << "    <section>" << endl;
      //t << "        <title>" << theTranslator->trBriefDescription() << "</title>" << endl;
      writeAsciidocDocBlock(t,gd->briefFile(),gd->briefLine(),gd,0,gd->briefDescription());
      //t << "    </section>" << endl;
    }
  }

  if (gd->documentation())
  {
    t << "        <simplesect>" << endl;
    t << "            <title>" << theTranslator->trDetailedDescription() << "</title>" << endl;
    writeAsciidocDocBlock(t,gd->docFile(),gd->docLine(),gd,0,gd->documentation());
    t << "        </simplesect>" << endl;
  }

  for (mli.toFirst();(ml=mli.current());++mli)
  {
    if ((ml->listType()&MemberListType_declarationLists)!=0)
    {
      generateAsciidocSection(gd,t,ml,g_asciidocSectionMapper.find(ml->listType()),1);
    }
  }

  writeInnerGroupFiles(gd->getSubGroups(),t);

  t << "</section>" << endl;

}

static void generateAsciidocForDir(DirDef *dd,FTextStream &ti)
{
  if (dd->isReference()) return; // skip external references

  QCString fileAsciidoc=dd->getOutputFileBase()+".xml";
  //Add the file Documentation info to index file
  ti << "        <xi:include href=\"" << fileAsciidoc << "\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;

  QCString outputDirectory = Config_getString(ASCIIDOC_OUTPUT);
  QCString fileName=outputDirectory+"/"+dd->getOutputFileBase()+".xml";
  QFile f(fileName);
  QCString relPath = relativePathToRoot(fileName);

  if (!f.open(IO_WriteOnly))
  {
    err("Cannot open file %s for writing!\n",fileName.data());
    return;
  }

  FTextStream t(&f);
  //t.setEncoding(FTextStream::UnicodeUTF8);
  writeAsciidocHeader_ID(t, dd->getOutputFileBase());

  t << "    <title>";
  t << theTranslator->trDirReference(dd->displayName());
  t << "</title>" << endl;
  if (dd->briefDescription())
  {
    writeAsciidocDocBlock(t,dd->briefFile(),dd->briefLine(),dd,0,dd->briefDescription());
  }
  if (Config_getBool(DIRECTORY_GRAPH) && Config_getBool(HAVE_DOT))
  {
    t << "<para>Directory dependency diagram for " << convertToAsciidoc(dd->displayName()) << "</para>" << endl;
    DotDirDeps dirdepGraph(dd);
    dirdepGraph.writeGraph(t,GOF_BITMAP,EOF_Asciidoc,Config_getString(ASCIIDOC_OUTPUT),fileName,relPath,FALSE);
  }

  writeInnerDirs(&dd->subDirs(),t);
  writeInnerFiles(dd->getFiles(),t);

  t << "    <simplesect>" << endl;
  t << "        <title>" << theTranslator->trDetailedDescription() << "</title>" << endl;
  if (dd->briefDescription())
  {
    t << "    <para>" << endl;
    writeAsciidocDocBlock(t,dd->briefFile(),dd->briefLine(),dd,0,dd->briefDescription());
    t << "    </para>" << endl;
  }
  writeAsciidocDocBlock(t,dd->docFile(),dd->docLine(),dd,0,dd->documentation());
  t << "    <para>Directory location is " << dd->name() << "</para>" << endl;
  t << "    </simplesect>" << endl;

  t << "</section>" << endl;
}

static void generateAsciidocForPage(PageDef *pd,FTextStream &ti,bool isExample)
{
  // + name
  // + title
  // + documentation

  if (pd->isReference()) return;

  QCString pageName = pd->getOutputFileBase();
  if (pd->getGroupDef())
  {
    pageName+=(QCString)"_"+pd->name();
  }
  if (pageName=="index")
  {
    pageName="mainpage"; // to prevent overwriting the generated index page.
  }

  QCString outputDirectory = Config_getString(ASCIIDOC_OUTPUT);
  QCString fileName=outputDirectory+"/"+pageName+".xml";
  QFile f(fileName);
  if (!f.open(IO_WriteOnly))
  {
    err("Cannot open file %s for writing!\n",fileName.data());
    return;
  }

  FTextStream t(&f);
  //t.setEncoding(FTextStream::UnicodeUTF8);

  if(isExample)
  {
    QCString fileAsciidoc=pageName+".xml";
    ti << "        <xi:include href=\"" << fileAsciidoc << "\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
  }

  if (!pd->hasParentPage() && !isExample)
  {
    QCString fileAsciidoc=pageName+".xml";
    //Add the file Documentation info to index file
    ti << "        <xi:include href=\"" << fileAsciidoc << "\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
    writeAsciidocHeaderMainpage(t,pageName);
  }
  else
  {
    QCString pid;
    if(isExample)
    {
      pid = pageName;
    }
    else
    {
      pid = pageName+"_1"+pageName;
    }
    writeAsciidocHeader_ID(t, pid);
  }

  SectionInfo *si = Doxygen::sectionDict->find(pd->name());
  if (si)
  {
    if ( pageName == "mainpage")
      t << "    <title>" << convertToAsciidoc(theTranslator->trMainPage()) << "</title>" << endl;
    else
      t << "    <title>" << convertToAsciidoc(si->title) << "</title>" << endl;
  }
  else
  {
    t << "    <title>" << convertToAsciidoc(pd->name()) << "</title>" << endl;
  }

  generateTOC(t, pd);
  if (isExample)
  {
    writeAsciidocDocBlock(t,pd->docFile(),pd->docLine(),pd,0,
        pd->documentation()+"\n\\include "+pd->name());
  }
  else
  {
    writeAsciidocDocBlock(t,pd->docFile(),pd->docLine(),pd,0,
        pd->documentation());
  }
  writeInnerPages(pd->getSubPages(),t);

  if (!pd->hasParentPage() && !isExample)
  {
    t << endl << "</chapter>" << endl;
  }
  else
  {
    t << endl << "</section>" << endl;
  }
}
void generateAsciidoc_v1()
{

  // + classes
  // + namespaces
  // + files
  // + groups
  // + related pages
  // - examples

  QCString outputDirectory = Config_getString(ASCIIDOC_OUTPUT);
  if (outputDirectory.isEmpty())
  {
    outputDirectory=QDir::currentDirPath().utf8();
  }
  else
  {
    QDir dir(outputDirectory);
    if (!dir.exists())
    {
      dir.setPath(QDir::currentDirPath());
      if (!dir.mkdir(outputDirectory))
      {
        err("tag ASCIIDOC_OUTPUT: Output directory `%s' does not "
            "exist and cannot be created\n",outputDirectory.data());
        exit(1);
      }
      else
      {
        msg("Notice: Output directory `%s' does not exist. "
            "I have created it for you.\n", outputDirectory.data());
      }
      dir.cd(outputDirectory);
    }
    outputDirectory=dir.absPath().utf8();
  }

  QDir dir(outputDirectory);
  if (!dir.exists())
  {
    dir.setPath(QDir::currentDirPath());
    if (!dir.mkdir(outputDirectory))
    {
      err("Cannot create directory %s\n",outputDirectory.data());
      return;
    }
  }
  QDir asciidocDir(outputDirectory);
  createSubDirs(asciidocDir);

  QCString fileName=outputDirectory+"/index.xml";
  QCString dbk_projectName = Config_getString(PROJECT_NAME);
  QFile f(fileName);

  f.setName(fileName);
  if (!f.open(IO_WriteOnly))
  {
    err("Cannot open file %s for writing!\n",fileName.data());
    return;
  }
  FTextStream t(&f);
  //t.setEncoding(FTextStream::UnicodeUTF8);

  // write index header for Asciidoc which calls the structure file
  t << "    <info>" << endl;
  t << "    <title>" << convertToAsciidoc(dbk_projectName) << "</title>" << endl;
  t << "    </info>" << endl;

  // NAMESPACE DOCUMENTATION
  NamespaceSDict::Iterator nli(*Doxygen::namespaceSDict);
  NamespaceDef *nd;

  //Namespace Documentation index header
  if (nli.toFirst())
  {
    t << "    <chapter>" << endl;
    t << "        <title>Namespace Documentation</title>" << endl;
  }

  for (nli.toFirst();(nd=nli.current());++nli)
  {
    msg("Generating Asciidoc output for namespace %s\n",nd->name().data());
    generateAsciidocForNamespace(nd,t);
  }

  //Namespace Documentation index footer
  if (nli.toFirst())
  {
    t << "    </chapter>" << endl;
  }

  /** MAINPAGE DOCUMENTATION **/

  if (Doxygen::mainPage)
  {
    msg("Generating Asciidoc output for the main page\n");
    generateAsciidocForPage(Doxygen::mainPage,t,FALSE);
  }

  // PAGE DOCUMENTATION
  {
    PageSDict::Iterator pdi(*Doxygen::pageSDict);
    PageDef *pd=0;

    for (pdi.toFirst();(pd=pdi.current());++pdi)
    {
      msg("Generating Asciidoc output for page %s\n",pd->name().data());
      generateAsciidocForPage(pd,t,FALSE);
    }
  }

  /** MODULE GROUP DOCUMENTATION **/

  GroupSDict::Iterator gli(*Doxygen::groupSDict);
  GroupDef *gd;

  //Module group Documentation index header
  if (gli.toFirst())
  {
    t << "    <chapter>" << endl;
    t << "        <title>" << theTranslator->trModuleDocumentation() << "</title>" << endl;
  }

  for (;(gd=gli.current());++gli)
  {
    msg("Generating Asciidoc output for group %s\n",gd->name().data());
    generateAsciidocForGroup(gd,t);
  }

  //Module group Documentation index footer
  if (gli.toFirst())
  {
    t << "    </chapter>" << endl;
  }

  //CLASS DOCUMENTATION

  {
    ClassSDict::Iterator cli(*Doxygen::classSDict);
    ClassDef *cd;

    //Class Documentation index header
    if (cli.toFirst())
    {
      t << "    <chapter>" << endl;
      t << "        <title>" << theTranslator->trClassDocumentation() << "</title>" << endl;
    }

    for (cli.toFirst();(cd=cli.current());++cli)
    {
      generateAsciidocForClass(cd,t);
    }

    //Class Documentation index footer
    if (cli.toFirst())
    {
      t << "    </chapter>" << endl;
    }
  }

  // FILE DOCUMENTATION

  static bool showFiles = Config_getBool(SHOW_FILES);
  if (showFiles)
  {
    FileNameListIterator fnli(*Doxygen::inputNameList);
    FileName *fn;

    //File Documentation index header
    if (fnli.toFirst())
    {
      t << "    <chapter>" << endl;
      t << "        <title>" << theTranslator->trFileDocumentation() << "</title>" << endl;
    }

    for (;(fn=fnli.current());++fnli)
    {
      FileNameIterator fni(*fn);
      FileDef *fd;
      for (;(fd=fni.current());++fni)
      {
        msg("Generating Asciidoc output for file %s\n",fd->name().data());
        generateAsciidocForFile(fd,t);
      }
    }

    //File Documentation index footer
    if (fnli.toFirst())
    {
      t << "    </chapter>" << endl;
    }
  }

  // DIRECTORY DOCUMENTATION
  if (Config_getBool(DIRECTORY_GRAPH) && Config_getBool(HAVE_DOT))
  {
    DirDef *dir;
    DirSDict::Iterator sdi(*Doxygen::directories);

    //Directory Documentation index header
    if (sdi.toFirst())
    {
      t << "    <chapter>" << endl;
      t << "        <title>" << theTranslator->trDirDocumentation() << "</title>" << endl;
    }

    for (sdi.toFirst();(dir=sdi.current());++sdi)
    {
      msg("Generate Asciidoc output for dir %s\n",dir->name().data());
      generateAsciidocForDir(dir,t);
    }

    //Module group Documentation index footer
    if (sdi.toFirst())
    {
      t << "    </chapter>" << endl;
    }
  }

  // EXAMPLE PAGE DOCUMENTATION

  {
    PageSDict::Iterator pdi(*Doxygen::exampleSDict);
    PageDef *pd=0;

    //Example Page Documentation index header
    if (pdi.toFirst())
    {
      t << "    <chapter>" << endl;
      t << "        <title>" << theTranslator->trExampleDocumentation() << "</title>" << endl;
    }

    for (pdi.toFirst();(pd=pdi.current());++pdi)
    {
      msg("Generating Asciidoc output for example %s\n",pd->name().data());
      generateAsciidocForPage(pd,t,TRUE);
    }

    //Example Page Documentation index footer
    if (pdi.toFirst())
    {
      t << "    </chapter>" << endl;
    }
  }

  t << "<index/>" << endl;
  t << "</book>" << endl;

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
  if (fileName.right(4)!=".xml") fileName+=".xml";
  startPlainFile(fileName);
  m_codeGen.setTextStream(t);
  m_codeGen.setRelativePath(relPath);
  m_codeGen.setSourceFileName(stripPath(fileName));

  if (!pageName.isEmpty()) t << " xml:id=\"_" <<  stripPath(pageName) << "\"";
  t << ">" << endl;
}

void AsciidocGenerator::endFile()
{
DB_GEN_C
  if (m_inDetail) t << "</section>" << endl;
  m_inDetail = FALSE;
  while (m_inLevel != -1)
  {
    t << "</section>" << endl;
    m_inLevel--;
  }
  if (m_inGroup) t << "</section>" << endl;
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
  t << "</" << fileType << ">" << endl;
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
        t << "    <info>" << endl;
        t << "    <title>" << convertToAsciidoc(dbk_projectName) << "</title>" << endl;
        t << "    </info>" << endl;
      }
      break;
    case isTitlePageAuthor:
      break;
    case isMainPage:
      t << "<chapter>" << endl;
      t << "    <title>";
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
      t << "<chapter>\n";
      t << "    <title>";
      break;
    case isDirDocumentation:
      t << "<chapter>\n";
      t << "    <title>";
      break;
    case isNamespaceDocumentation:
      t << "<chapter>\n";
      t << "    <title>";
      break;
    case isClassDocumentation:
      t << "<chapter>\n";
      t << "    <title>";
      break;
    case isFileDocumentation:
      t << "<chapter>\n";
      t << "    <title>";
      break;
    case isExampleDocumentation:
      t << "<chapter>\n";
      t << "    <title>";
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
      t << "</title>" << endl;
      t << "    <xi:include href=\"mainpage.xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
      t << "</chapter>" << endl;
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
        t << "</title>" << endl;
        GroupSDict::Iterator gli(*Doxygen::groupSDict);
        GroupDef *gd;
        bool found=FALSE;
        for (gli.toFirst();(gd=gli.current()) && !found;++gli)
        {
          if (!gd->isReference())
          {
            t << "    <xi:include href=\"" << gd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
            found=TRUE;
          }
        }
        for (;(gd=gli.current());++gli)
        {
          if (!gd->isReference())
          {
            t << "    <xi:include href=\"" << gd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
          }
        }
      }
      t << "</chapter>\n";
      break;
    case isDirDocumentation:
      {
        t << "</title>" << endl;
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
            t << "    <xi:include href=\"" << dd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
          }
        }
      }
      t << "</chapter>\n";
      break;
    case isNamespaceDocumentation:
      {
        t << "</title>" << endl;
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
      t << "</chapter>\n";
      break;
    case isClassDocumentation:
      {
        t << "</title>" << endl;
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
            t << "    <xi:include href=\"" << cd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
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
            t << "    <xi:include href=\"" << cd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
          } 
        }
      }
      t << "</chapter>\n";
      break;
    case isFileDocumentation:
      {
        t << "</title>" << endl;
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
                t << "    <xi:include href=\"" << fd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
                if (sourceBrowser && m_prettyCode && fd->generateSourceFile())
                {
                  t << "    <xi:include href=\"" << fd->getSourceFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
                }
                isFirst=FALSE;
              }
              else
              {
                t << "    <xi:include href=\"" << fd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
                if (sourceBrowser && m_prettyCode && fd->generateSourceFile())
                {
                  t << "    <xi:include href=\"" << fd->getSourceFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
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
        t << "</title>" << endl;
        PageSDict::Iterator pdi(*Doxygen::exampleSDict);
        PageDef *pd=pdi.toFirst();
        if (pd)
        {
          t << "    <xi:include href=\"" << pd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
        }
        for (++pdi;(pd=pdi.current());++pdi)
        {
          t << "    <xi:include href=\"" << pd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
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
      t << "<chapter>\n";
      if (!pd->title().isEmpty())
      {
        t << "    <title>" << convertToAsciidoc(pd->title()) << "</title>" << endl;
      }
      else
      {
        t << "    <title>" << convertToAsciidoc(pd->name()) << "</title>" << endl;
      }
      t << "    <xi:include href=\"" << pd->getOutputFileBase() << ".xml\" xmlns:xi=\"http://www.w3.org/2001/XInclude\"/>" << endl;
      t << "</chapter>\n";
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
  t << "<para>" << endl;
}

void AsciidocGenerator::endParagraph()
{
DB_GEN_C
  t << "</para>" << endl;
}
void AsciidocGenerator::writeString(const char *text)
{
DB_GEN_C
  t << text;
}
void AsciidocGenerator::startMemberHeader(const char *name,int)
{
DB_GEN_C
  t << "<simplesect>" << endl;
  m_inSimpleSect[m_levelListItem] = TRUE;
  t << "    <title>";
}

void AsciidocGenerator::endMemberHeader()
{
DB_GEN_C
  t << "    </title>" << endl;
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
    t << " <<_" << anchor;
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
  t << "        <itemizedlist>" << endl;
  m_levelListItem++;
}
void AsciidocGenerator::endMemberList()
{
DB_GEN_C
  if (m_inListItem[m_levelListItem]) t << "</listitem>" << endl;
  m_inListItem[m_levelListItem] = FALSE;
  t << "        </itemizedlist>" << endl;
  m_levelListItem = (m_levelListItem> 0 ?  m_levelListItem - 1 : 0);
  if (m_inSimpleSect[m_levelListItem]) t << "</simplesect>" << endl;
  m_inSimpleSect[m_levelListItem] = FALSE;
}
void AsciidocGenerator::startMemberItem(const char *,int,const char *)
{
DB_GEN_C
  if (m_inListItem[m_levelListItem]) t << "</listitem>" << endl;
  t << "            <listitem><para>";
  m_inListItem[m_levelListItem] = TRUE;
}
void AsciidocGenerator::endMemberItem()
{
DB_GEN_C
  t << "</para>" << endl;
}
void AsciidocGenerator::startBold(void)
{
DB_GEN_C
  t << "<emphasis role=\"strong\">";
}
void AsciidocGenerator::endBold(void)
{
DB_GEN_C
  t << "</emphasis>";
}
void AsciidocGenerator::startGroupHeader(int extraIndentLevel)
{
DB_GEN_C2("m_inLevel " << m_inLevel)
DB_GEN_C2("extraIndentLevel " << extraIndentLevel)
  m_firstMember = TRUE; 
  if (m_inSimpleSect[m_levelListItem]) t << "</simplesect>" << endl;
  m_inSimpleSect[m_levelListItem] = FALSE;
  if (m_inLevel != -1) m_inGroup = TRUE;
  if (m_inLevel == extraIndentLevel) t << "</section>" << endl;
  m_inLevel = extraIndentLevel;
  t << "<section>" << endl;
  t << "<title>";
}
void AsciidocGenerator::writeRuler(void)
{
DB_GEN_C2("m_inLevel " << m_inLevel)
DB_GEN_C2("m_inGroup " << m_inGroup)
  if (m_inGroup) t << "</section>" << endl;
  m_inGroup = FALSE;
}

void AsciidocGenerator::endGroupHeader(int)
{
DB_GEN_C
  t << "</title>" << endl;
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
  if (!m_denseText) t << "<computeroutput>";
}
void AsciidocGenerator::endTypewriter()
{
DB_GEN_C
  if (!m_denseText) t << "</computeroutput>" << endl;
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
  t << "    <section>" << endl;
  t << "    <title>" << convertToAsciidoc(title);
  if (memTotal>1)
  {
    t << "<computeroutput>[" << memCount << "/" << memTotal << "]</computeroutput>";
  }
  t << "</title>" << endl;
  if (memname && memname[0]!='@')
  {
    addIndexTerm(t,memname,clname);
    addIndexTerm(t,clname,memname);
  }
}
void AsciidocGenerator::endMemberDoc(bool)
{
DB_GEN_C
  t << "</computeroutput></para>";
}
void AsciidocGenerator::startTitleHead(const char *)
{
DB_GEN_C
  t << "<title>";
}
void AsciidocGenerator::endTitleHead(const char *fileName,const char *name)
{
DB_GEN_C
  t << "</title>" << endl;
  if (name) addIndexTerm(t, name);
}
void AsciidocGenerator::startDoxyAnchor(const char *fName,const char *manName,
                                 const char *anchor,const char *name,
                                 const char *args)
{
DB_GEN_C
  if (!m_inListItem[m_levelListItem] && !m_descTable)
  {
    if (!m_firstMember) t << "    </section>";
    m_firstMember = FALSE;
  }
  if (anchor)
  {
    t << "<anchor xml:id=\"_" << stripPath(fName) << "_1" << anchor << "\"/>";
  }
}
void AsciidocGenerator::endDoxyAnchor(const char *fileName,const char *anchor)
{
DB_GEN_C
}
void AsciidocGenerator::startMemberDocName(bool)
{
DB_GEN_C
  t << "<para><computeroutput>";
}
void AsciidocGenerator::endMemberDocName()
{
DB_GEN_C
}
void AsciidocGenerator::startMemberGroupHeader(bool hasHeader)
{
DB_GEN_C
  t << "<simplesect><title>";
}
void AsciidocGenerator::endMemberGroupHeader()
{
DB_GEN_C
  t << "</title>" << endl;
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
  t << "<para>";
}

void AsciidocGenerator::endClassDiagram(const ClassDiagram &d, const char *fileName,const char *)
{
DB_GEN_C
  visitADPreStart(t, FALSE, relPath + fileName + ".png", NULL, NULL);
  d.writeImage(t,dir,relPath,fileName,FALSE);
  visitADPostEnd(t, FALSE);
  t << "</para>" << endl;
}
void  AsciidocGenerator::startLabels()
{
DB_GEN_C
}

void  AsciidocGenerator::writeLabel(const char *l,bool isLast)
{
DB_GEN_C
  t << "<computeroutput>[" << l << "]</computeroutput>";
  if (!isLast) t << ", ";
}

void  AsciidocGenerator::endLabels()
{
DB_GEN_C
}
void AsciidocGenerator::startExamples()
{
DB_GEN_C
  t << "<simplesect><title>";
  docify(theTranslator->trExamples());
  t << "</title>";
}

void AsciidocGenerator::endExamples()
{
DB_GEN_C
  t << "</simplesect>" << endl;
}
void AsciidocGenerator::startSubsubsection(void)
{
DB_GEN_C
  t << "<simplesect><title>";
}
void AsciidocGenerator::endSubsubsection(void)
{
DB_GEN_C
  t << "</title></simplesect>" << endl;
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
  t << "<computeroutput>";
}
void AsciidocGenerator::endMemberDocPrefixItem()
{
DB_GEN_C
  t << "</computeroutput>";
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
    t << "<programlisting>";
}
void AsciidocGenerator::endCodeFragment()
{
DB_GEN_C
    t << "</programlisting>";
}
void AsciidocGenerator::startMemberTemplateParams()
{
DB_GEN_C
}

void AsciidocGenerator::endMemberTemplateParams(const char *,const char *)
{
DB_GEN_C
  t << "</para>";
  t << "<para>";
}
void AsciidocGenerator::startSection(const char *lab,const char *,SectionInfo::SectionType type)
{
DB_GEN_C
  t << "    <section xml:id=\"_" << stripPath(lab) << "\">";
  t << "<title>";
}
void AsciidocGenerator::endSection(const char *lab,SectionInfo::SectionType)
{
DB_GEN_C
  t << "</title>";
  t << "    </section>";
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
  t << "<informaltable frame=\"all\">" << endl;
  if (title)t << "<title>" << convertToAsciidoc(title) << "</title>" << endl;
  t << "    <tgroup cols=\"" << ncols << "\" align=\"left\" colsep=\"1\" rowsep=\"1\">" << endl;
  for (int i = 0; i < ncols; i++)
  {
    t << "      <colspec colname='c" << i+1 << "'/>\n";
  }
  t << "<tbody>\n";
  m_descTable = TRUE;
}

void AsciidocGenerator::endDescTable()
{
DB_GEN_C
  t << "    </tbody>" << endl;
  t << "    </tgroup>" << endl;
  t << "</informaltable>" << endl;
  m_descTable = FALSE;
}

void AsciidocGenerator::startDescTableRow()
{
DB_GEN_C
  t << "<row>";
  t << "<entry>";
}

void AsciidocGenerator::endDescTableRow()
{
DB_GEN_C
  t << "</row>";
}

void AsciidocGenerator::startDescTableTitle()
{
DB_GEN_C
}

void AsciidocGenerator::endDescTableTitle()
{
DB_GEN_C
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
  t << "<simplesect><title>";
  docify(header);
  t << "</title>" << endl;
}
void AsciidocGenerator::startConstraintParam()
{
DB_GEN_C
  t << "<para><emphasis role=\"strong\">";
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
  t << "</emphasis></para>" << endl;
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
  t << "</simplesect>" << endl;
}
