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
*/

#ifndef ASCIIDOCGEN_H
#define ASCIIDOCGEN_H

#include "outputgen.h"

class AsciidocCodeGenerator : public CodeOutputInterface
{
  public:
    AsciidocCodeGenerator(FTextStream &t);
    AsciidocCodeGenerator();
    virtual ~AsciidocCodeGenerator();
    void setTextStream(FTextStream &t)
    {
      m_streamSet = t.device()!=0;
      m_t.setDevice(t.device());
    }
    void setRelativePath(const QCString &path) { m_relPath = path; }
    void setSourceFileName(const QCString &sourceFileName) { m_sourceFileName = sourceFileName; }
    QCString sourceFileName() { return m_sourceFileName; }

    void codify(const char *text);
    void writeCodeLink(const char *ref,const char *file,
        const char *anchor,const char *name,
        const char *tooltip);
    void writeCodeLinkLine(const char *ref,const char *file,
        const char *anchor,const char *name,
        const char *tooltip);
    void writeTooltip(const char *, const DocLinkInfo &, const char *,
                      const char *, const SourceLinkInfo &, const SourceLinkInfo &
                     );
    void startCodeLine(bool);
    void endCodeLine();
    void startFontClass(const char *colorClass);
    void endFontClass();
    void writeCodeAnchor(const char *);
    void writeLineNumber(const char *extRef,const char *compId,
        const char *anchorId,int l);
    void setCurrentDoc(Definition *,const char *,bool);
    void addWord(const char *,bool);
    void finish();
    void startCodeFragment(SrcLangExt lang);
    void endCodeFragment();

  private:
    FTextStream m_t;
    bool m_streamSet;
    QCString m_refId;
    QCString m_external;
    int m_lineNumber;
    int m_col;
    bool m_insideCodeLine;
    bool m_insideSpecialHL;
    QCString m_relPath;
    QCString m_sourceFileName;
    bool m_prettyCode;
};


#if 1
// define for cases that have been implemented with an empty body
#define AD_GEN_EMPTY  t << "<!-- DBG_GEN_head_check " << __LINE__ << " -->\n";
#else
#define AD_GEN_EMPTY
#endif

#if 1
// Generic debug statements
#define AD_GEN_H AD_GEN_H1(t)
#define AD_GEN_H1(x) x << "<!-- DBG_GEN_head " << __LINE__ << " -->\n";
#define AD_GEN_H2(y) AD_GEN_H2a(t,y)
#define AD_GEN_H2a(x,y) x << "<!-- DBG_GEN_head " << __LINE__ << " " << y << " -->\n";
// define for cases that have NOT yet been implemented / considered
#define AD_GEN_NEW fprintf(stderr,"DBG_GEN_head %d\n",__LINE__); AD_GEN_H
#else
#define AD_GEN_H
#define AD_GEN_H1(x)
#define AD_GEN_H2(y)
#define AD_GEN_H2a(x,y)
#define AD_GEN_NEW
#endif

class AsciidocGenerator : public OutputGenerator
{
  public:
    AsciidocGenerator();
    ~AsciidocGenerator();
    static void init();

    ///////////////////////////////////////////////////////////////
    // generic generator methods
    ///////////////////////////////////////////////////////////////
    void enable() 
    { if (genStack->top()) active=*genStack->top(); else active=TRUE; }
    void disable() { active=FALSE; }
    void enableIf(OutputType o)  { if (o==Asciidoc) enable();  }
    void disableIf(OutputType o) { if (o==Asciidoc) disable(); }
    void disableIfNot(OutputType o) { if (o!=Asciidoc) disable(); }
    bool isEnabled(OutputType o) { return (o==Asciidoc && active); } 
    OutputGenerator *get(OutputType o) { return (o==Asciidoc) ? this : 0; }

    // --- CodeOutputInterface
    void codify(const char *text)
    { m_codeGen.codify(text); }
    void writeCodeLink(const char *ref, const char *file,
                       const char *anchor,const char *name,
                       const char *tooltip)
    { m_codeGen.writeCodeLink(ref,file,anchor,name,tooltip); }
    void writeLineNumber(const char *ref,const char *file,const char *anchor,int lineNumber)
    { m_codeGen.writeLineNumber(ref,file,anchor,lineNumber); }
    void writeTooltip(const char *id, const DocLinkInfo &docInfo, const char *decl,
                      const char *desc, const SourceLinkInfo &defInfo, const SourceLinkInfo &declInfo
                     )
    { m_codeGen.writeTooltip(id,docInfo,decl,desc,defInfo,declInfo); }
    void startCodeLine(bool hasLineNumbers)
    { m_codeGen.startCodeLine(hasLineNumbers); }
    void endCodeLine()
    { m_codeGen.endCodeLine(); }
    void startFontClass(const char *s)
    { m_codeGen.startFontClass(s); }
    void endFontClass()
    { m_codeGen.endFontClass(); }
    void writeCodeAnchor(const char *anchor)
    { m_codeGen.writeCodeAnchor(anchor); }
    // ---------------------------

    void writeDoc(DocNode *,Definition *ctx,MemberDef *md);

    ///////////////////////////////////////////////////////////////
    // structural output interface
    ///////////////////////////////////////////////////////////////
    void startFile(const char *name,const char *manName,
                           const char *title);
    void writeSearchInfo();
    void writeFooter(const char *navPath);
    void endFile();
    void startIndexSection(IndexSections);
    void endIndexSection(IndexSections);
    void writePageLink(const char *,bool);
    void startProjectNumber();
    void endProjectNumber();
    void writeStyleInfo(int part);
    void startTitleHead(const char *);
    void endTitleHead(const char *fileName,const char *name);
    void startIndexListItem();
    void endIndexListItem();
    void startIndexList();
    void endIndexList();
    void startIndexKey();
    void endIndexKey();
    void startIndexValue(bool);
    void endIndexValue(const char *,bool);
    void startItemList();
    void endItemList();

    void startIndexItem(const char *ref,const char *file);
    void endIndexItem(const char *ref,const char *file);
    void startItemListItem();
    void endItemListItem();

    void docify(const char *text);
    void writeChar(char);
    void writeString(const char *);
    void startParagraph(const char *);
    void endParagraph(void);
    void writeObjectLink(const char *,const char *,const char *,const char *);
    void startHtmlLink(const char *);
    void endHtmlLink(void);
    void startBold(void);
    void endBold(void);
    void startTypewriter(void);
    void endTypewriter(void);
    void startEmphasis(void);
    void endEmphasis(void);
    void startCodeFragment(SrcLangExt lang);
    void endCodeFragment(void);
    void writeRuler(void);
    void startDescription(void);
    void endDescription(void);
    void startDescItem(void);
    void startDescForItem(void);
    void endDescForItem(void);
    void endDescItem(void);
    void startCenter(void);
    void endCenter(void);
    void startSmall(void);
    void endSmall(void);
    void startExamples(void);
    void endExamples(void);
    void startParamList(BaseOutputDocInterface::ParamListTypes,const char *);
    void endParamList(void);
    void startTitle(void);
    void endTitle(void);
    void writeAnchor(const char *,const char *);
    void startSection(const char *,const char *,SectionInfo::SectionType);
    void endSection(const char *,SectionInfo::SectionType);
    void lineBreak(const char *);
    void addIndexItem(const char *,const char *);
    void writeNonBreakableSpace(int);
    void startDescTable(const char *);
    void endDescTable(void);
    void startDescTableRow(void);
    void endDescTableRow(void);
    void startDescTableTitle(void);
    void endDescTableTitle(void);
    void startDescTableData(void);
    void endDescTableData(void);
    void startTextLink(const char *,const char *);
    void endTextLink(void);
    void startPageRef(void);
    void endPageRef(const char *,const char *);
    void startSubsection(void);
    void endSubsection(void);
    void startSubsubsection(void);
    void endSubsubsection(void);

    void startGroupHeader(int);
    void endGroupHeader(int);
    void startMemberSections();
    void endMemberSections();
    void startHeaderSection();
    void endHeaderSection();
    void startMemberHeader(const char *anchor, int typ);
    void endMemberHeader();
    void startMemberSubtitle();
    void endMemberSubtitle();
    void startMemberDocList();
    void endMemberDocList();
    void startMemberList();
    void endMemberList();
    void startInlineHeader();
    void endInlineHeader();
    void startAnonTypeScope(int);
    void endAnonTypeScope(int);
    void startMemberItem(const char *,int,const char *);
    void endMemberItem();
    void startMemberTemplateParams();
    void endMemberTemplateParams(const char *,const char *);
    void startMemberGroupHeader(bool);
    void endMemberGroupHeader();
    void startMemberGroupDocs();
    void endMemberGroupDocs();
    void startMemberGroup();
    void endMemberGroup(bool);
    void insertMemberAlign(bool);
    void insertMemberAlignLeft(int,bool);
    void startMemberDoc(const char *,const char *,
                                const char *,const char *,int,int,bool);
    void endMemberDoc(bool);
    void startDoxyAnchor(const char *fName,const char *manName,
                                 const char *anchor,const char *name,
                                 const char *args);
    void endDoxyAnchor(const char *fileName,const char *anchor);
    void writeLatexSpacing(){AD_GEN_EMPTY}
    void writeStartAnnoItem(const char *type,const char *file,
			    const char *path,const char *name);
    void writeEndAnnoItem(const char *name);
    void startMemberDescription(const char *anchor,const char *inheritId,
				bool typ);
    void endMemberDescription();
    void startMemberDeclaration();
    void endMemberDeclaration(const char *anchor,const char *inheritId);
    void writeInheritedSectionTitle(const char *id,const char *ref,
				    const char *file,const char *anchor,
				    const char *title,const char *name);
    void startIndent();
    void endIndent();
    void writeSynopsis();
    void startClassDiagram();
    void endClassDiagram(const ClassDiagram &,const char *,const char *);
    void startDotGraph();
    void endDotGraph(const DotClassGraph &g);
    void startInclDepGraph();
    void endInclDepGraph(const DotInclDepGraph &g);
    void startGroupCollaboration();
    void endGroupCollaboration(const DotGroupCollaboration &g);
    void startCallGraph();
    void endCallGraph(const DotCallGraph &g);
    void startDirDepGraph();
    void endDirDepGraph(const DotDirDeps &g);
    void writeGraphicalHierarchy(const DotGfxHierarchyTable &g);
    void startQuickIndices();
    void endQuickIndices();
    void writeSplitBar(const char *);
    void writeNavigationPath(const char *);
    void writeLogo();
    void writeQuickLinks(bool compact,HighlightedItem hli,const char *file);
    void writeSummaryLink(const char *file,const char *anchor,const char *title,bool first);
    void startContents();
    void endContents();
    void startPageDoc(const char *pageTitle);
    void endPageDoc();
    void startTextBlock(bool);
    void endTextBlock(bool);
    void lastIndexPage();
    void startMemberDocPrefixItem();
    void endMemberDocPrefixItem();
    void startMemberDocName(bool);
    void endMemberDocName();
    void startParameterType(bool,const char *key);
    void endParameterType();
    void startParameterName(bool);
    void endParameterName(bool,bool,bool);
    void startParameterList(bool);
    void endParameterList();
    void exceptionEntry(const char*,bool);

    void startConstraintList(const char *);
    void startConstraintParam();
    void endConstraintParam();
    void startConstraintType();
    void endConstraintType();
    void startConstraintDocs();
    void endConstraintDocs();
    void endConstraintList();

    void startMemberDocSimple(bool);
    void endMemberDocSimple(bool);
    void startInlineMemberType();
    void endInlineMemberType();
    void startInlineMemberName();
    void endInlineMemberName();
    void startInlineMemberDoc();
    void endInlineMemberDoc();

    void startLabels();
    void writeLabel(const char *,bool);
    void endLabels();

    void setCurrentDoc(Definition *,const char *,bool);
    void addWord(const char *,bool);

private:
    AsciidocGenerator(const AsciidocGenerator &o);
    AsciidocGenerator &operator=(const AsciidocGenerator &o);
 
    QCString relPath;
    AsciidocCodeGenerator m_codeGen;
    bool m_prettyCode;
    bool m_denseText;
    bool m_inGroup;
    bool m_inDetail;
    int  m_levelListItem;
    bool m_inListItem[20];
    bool m_inSimpleSect[20];
    bool m_descTable;
    int m_inLevel;
    bool m_firstMember;
    bool m_pageref;
};

#endif
